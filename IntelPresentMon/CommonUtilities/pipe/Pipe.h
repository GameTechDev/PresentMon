#pragma once
#include "../win/WinAPI.h"
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include "../win/Handle.h"
#include "../win/Event.h"
#include "../Exception.h"
#include "../log/Log.h"
#include "SecurityMode.h"
#include "CoroMutex.h"
#include <ranges>

namespace pmon::util::pipe
{
	namespace as = boost::asio;
	using namespace std::literals;

	PM_DEFINE_EX(PipeError);
	// A read that had a deadline and missed it. Distinct from a peer disconnect so
	// the session owner can report SessionEndReason::PeerStalled.
	PM_DEFINE_EX_FROM(PipeError, PipeReadTimeout);
	// pipe errors that are often part of acceptable program flow
	PM_DEFINE_EX_FROM(PipeError, BenignPipeError);
	// pipe was broken (closed by remote)
	PM_DEFINE_EX_FROM(BenignPipeError, PipeBroken);
	// pipe operation was canceled (e.g. by operator ||, ioctx.stop)
	PM_DEFINE_EX_FROM(BenignPipeError, PipeOperationCanceled);

	// Bounds imposed on the length prefix of an incoming packet. The declared body
	// length is remote input, so it is validated before the receive buffer is grown
	// to hold it. Owners with a known packet shape pass tighter limits at construction.
	struct PipeLimits
	{
		uint32_t minBodyBytes = 1;
		uint32_t maxBodyBytes = 1024 * 1024;
		// once a length has been declared the sender has committed, so a stall is abuse
		uint32_t bodyReadTimeoutMs = 2000;
	};

	// Written into the stream before the body is serialized, then overwritten in place.
	inline constexpr uint32_t kPacketBodyLengthPlaceholder = 0x54454D50u;

	class DuplexPipe
	{
	public:
		DuplexPipe(const DuplexPipe&) = delete;
		DuplexPipe& operator=(const DuplexPipe&) = delete;
		DuplexPipe(DuplexPipe&& other) = delete;
		DuplexPipe& operator=(DuplexPipe&&) = delete;
		~DuplexPipe() = default;
		as::awaitable<void> Accept();
		static DuplexPipe Connect(const std::string& name, as::io_context& ioctx, PipeLimits limits = {});
		static std::unique_ptr<DuplexPipe> ConnectAsPtr(const std::string& name, as::io_context& ioctx, PipeLimits limits = {});
		static std::unique_ptr<DuplexPipe> MakeAsPtr(const std::string& name, as::io_context& ioctx, const std::string& security = {}, PipeLimits limits = {});
		template<class H, class P>
		as::awaitable<void> WritePacket(const H& header, const P& payload, std::optional<uint32_t> timeoutMs = {})
		{
			// lock while this coro is running to prevent other coros from causing an overlapped operation fault
			auto lk = co_await CoroLock(writeMtx_);
			// Close cancels a waiter. Do not write on the closed handle.
			if (!lk || !asioPipeHandle_.is_open()) {
				throw Except<PipeOperationCanceled>("Write on closed pipe");
			}
			assert(writeBuf_.size() == 0);
			// first we directly write bytes for the size of the body as a placeholder until we know how many are serialized
			const uint32_t placeholderSize = kPacketBodyLengthPlaceholder;
			writeStream_.write(reinterpret_cast<const char*>(&placeholderSize), sizeof(placeholderSize));
			// record how many bytes used for the serialization of the size
			const auto sizeSize = writeBuf_.size();
			// serialize the packet body
			writeArchive_(header, payload);
			// calculate size of body
			const auto payloadSize = uint32_t(writeBuf_.size() - sizeSize);
			if (payloadSize < limits_.minBodyBytes || payloadSize > limits_.maxBodyBytes) {
				pmlog_error("Packet body size out of range").pmwatch(payloadSize);
				writeBuf_.consume(writeBuf_.size());
				throw Except<PipeError>("Packet body size out of range");
			}
			// replace the placeholder with the actual body size
			const auto pSizeInPlace = const_cast<char*>(&*as::buffers_begin(writeBuf_.data()));
			auto replacement = std::string_view{ reinterpret_cast<const char*>(&payloadSize), sizeof(payloadSize) };
			std::ranges::copy(replacement, pSizeInPlace);
			// transmit the packet
			co_await Write_(timeoutMs);
		}
		// The wait for the length prefix has no limit, because a peer may stay idle between
		// packets for any length of time. Once a length has been declared, the body is
		// bounded by PipeLimits::bodyReadTimeoutMs.
		template<class H>
		as::awaitable<H> ReadPacketConsumeHeader()
		{
			// lock while this coro is running to prevent other coros from interrupting stream sequence
			// and/or causing an overlapped operation fault
			auto lk = co_await CoroLock(readMtx_);
			if (!lk || !asioPipeHandle_.is_open()) {
				throw Except<PipeOperationCanceled>("Read on closed pipe");
			}
			assert(readBuf_.size() == 0);
			// read in request
			// first read the number of bytes in the request payload (always 4-byte read)
			uint32_t payloadSize;
			co_await Read_(sizeof(payloadSize));
			readStream_.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
			// validate the declared length before growing the receive buffer to fit it
			if (payloadSize < limits_.minBodyBytes || payloadSize > limits_.maxBodyBytes) {
				pmlog_error("Packet body size out of range").pmwatch(payloadSize);
				throw Except<PipeError>("Packet body size out of range");
			}
			// read the payload
			co_await Read_(payloadSize, limits_.bodyReadTimeoutMs);
			// deserialize header portion of request payload
			H header;
			readArchive_(header);
			co_return header;
		}
		// NOTE: it might be necessary to pull the read lock up into the transfer layer to make sure nothing
		// interposes between header and payload consumption
		// alternatively
		template<class P>
		P ConsumePacketPayload()
		{
			P payload;
			try {
				readArchive_(payload);
			}
			catch (...) {
				DiscardPacketPayload();
				throw;
			}
			if (const auto sz = readBuf_.size()) {
				assert("unexpected data when reading packet payload from buffer!!" && false);
				pmlog_warn(std::format("Buffer contained unexpected data of size {}", sz));
				readBuf_.consume(sz);
			}
			return payload;
		}
		// drops a packet body without deserializing it, for when the concrete payload
		// type is unknown but the stream must be left positioned at the next packet
		void DiscardPacketPayload();
		// Aborts this side's in-flight reads and writes, then closes the handle so the
		// peer observes a disconnect. A coroutine suspended on the pipe resumes with
		// PipeOperationCanceled and unwinds through its own cleanup.
		void Close();
		size_t GetWriteBufferPending() const;
		void ClearWriteBuffer();
		static bool WaitForAvailability(const std::string& name, uint32_t timeoutMs, uint32_t pollPeriodMs = 5);
		static bool WaitForVacancy(const std::string& name, uint32_t timeoutMs, uint32_t pollPeriodMs = 5);
		uint32_t GetId() const;
		std::string GetName() const;
		// server side only, after Accept
		uint32_t GetClientProcessId() const;
		// Client-side only: pipe handle must be connected (post-CreateFile).
		bool TryGetConnectedServerProcessId(uint32_t& serverProcessId) noexcept;
		static std::string GetSecurityString(SecurityMode mode);
		static std::string GetServiceControlPipeSecurityString();
		static std::string GetPrivateControlPipeSecurityString(bool allowAuthenticatedClients);
		// Client connect mask; must not include FILE_CREATE_PIPE_INSTANCE or DACL change rights.
		static DWORD GetClientPipeConnectAccessMask() noexcept;
	private:
		// functions
		DuplexPipe(as::io_context& ioctx, HANDLE pipeHandle, std::string name, bool asClient, PipeLimits limits);
		static HANDLE Connect_(const std::string& name);
		static HANDLE Make_(const std::string& name, const std::string& security = {});
		// aborts any in-flight read or write on this pipe; used by Close
		void Cancel_();
		// wrapper to convert EOF system_error to PipeBroken error, with optional timeout
		as::awaitable<void> Read_(size_t byteCount, std::optional<uint32_t> timeoutMs = {});
		// wrapper to convert EOF system_error to PipeBroken error, with optional timeout
		as::awaitable<void> Write_(std::optional<uint32_t> timeoutMs = {});
		// true when the deadline expired, false when the wait was cancelled; never throws
		as::awaitable<bool> Timeout_(uint32_t ms);
		void TransformError_(const boost::system::error_code& ec);
		// data
		static std::atomic<uint32_t> nextUid_;
		std::string name_;
		PipeLimits limits_;
		uint32_t uid_ = nextUid_++;
		win::Handle rawPipeHandle_;
		as::windows::stream_handle asioPipeHandle_;
		as::streambuf readBuf_;
		std::istream readStream_;
		cereal::BinaryInputArchive readArchive_;
		CoroMutex readMtx_;
		as::streambuf writeBuf_;
		std::ostream writeStream_;
		cereal::BinaryOutputArchive writeArchive_;
		CoroMutex writeMtx_;
	};
}