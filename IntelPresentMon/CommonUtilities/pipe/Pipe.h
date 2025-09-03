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
	// pipe errors that are often part of acceptable program flow
	PM_DEFINE_EX_FROM(PipeError, BenignPipeError);
	// pipe was broken (closed by remote)
	PM_DEFINE_EX_FROM(BenignPipeError, PipeBroken);
	// pipe operation was canceled (e.g. by operator ||, ioctx.stop)
	PM_DEFINE_EX_FROM(BenignPipeError, PipeOperationCanceled);

	class DuplexPipe
	{
	public:
		DuplexPipe(const DuplexPipe&) = delete;
		DuplexPipe& operator=(const DuplexPipe&) = delete;
		DuplexPipe(DuplexPipe&& other) = delete;
		DuplexPipe& operator=(DuplexPipe&&) = delete;
		~DuplexPipe() = default;
		as::awaitable<void> Accept();
		static DuplexPipe Connect(const std::string& name, as::io_context& ioctx);
		static DuplexPipe Make(const std::string& name, as::io_context& ioctx, const std::string& security = {});
		static std::unique_ptr<DuplexPipe> ConnectAsPtr(const std::string& name, as::io_context& ioctx);
		static std::unique_ptr<DuplexPipe> MakeAsPtr(const std::string& name, as::io_context& ioctx, const std::string& security = {});
		template<class H, class P>
		as::awaitable<void> WritePacket(const H& header, const P& payload, std::optional<uint32_t> timeoutMs = {})
		{
			// lock while this coro is running to prevent other coros from causing an overlapped operation fault
			auto lk = co_await CoroLock(writeMtx_);
			// some sanity checks
			assert(writeBuf_.size() == 0);
			assert(asioPipeHandle_.is_open());
			// first we directly write bytes for the size of the body as a placeholder until we know how many are serialized
			const uint32_t placeholderSize = 'TEMP';
			writeStream_.write(reinterpret_cast<const char*>(&placeholderSize), sizeof(placeholderSize));
			// record how many bytes used for the serialization of the size
			const auto sizeSize = writeBuf_.size();
			// serialize the packet body
			writeArchive_(header, payload);
			// calculate size of body
			const auto payloadSize = uint32_t(writeBuf_.size() - sizeSize);
			// replace the placeholder with the actual body size
			const auto pSizeInPlace = const_cast<char*>(&*as::buffers_begin(writeBuf_.data()));
			auto replacement = std::string_view{ reinterpret_cast<const char*>(&payloadSize), sizeof(payloadSize) };
			std::ranges::copy(replacement, pSizeInPlace);
			// transmit the packet
			co_await Write_(timeoutMs);
		}
		template<class H>
		as::awaitable<H> ReadPacketConsumeHeader(std::optional<uint32_t> timeoutMs = {})
		{
			// lock while this coro is running to prevent other coros from interrupting stream sequence
			// and/or causing an overlapped operation fault
			auto lk = co_await CoroLock(readMtx_);
			// some sanity checks
			assert(readBuf_.size() == 0);
			assert(asioPipeHandle_.is_open());
			// read in request
			// first read the number of bytes in the request payload (always 4-byte read)
			uint32_t payloadSize;
			co_await Read_(sizeof(payloadSize), timeoutMs);
			readStream_.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
			// read the payload
			co_await Read_(payloadSize, timeoutMs);
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
			readArchive_(payload);
			if (const auto sz = readBuf_.size()) {
				assert("unexpected data when reading packet payload from buffer!!" && false);
				pmlog_warn(std::format("Buffer contained unexpected data of size", sz));
				readBuf_.consume(sz);
			}
			return payload;
		}
		size_t GetWriteBufferPending() const;
		void ClearWriteBuffer();
		static bool WaitForAvailability(const std::string& name, uint32_t timeoutMs, uint32_t pollPeriodMs = 10);
		uint32_t GetId() const;
		std::string GetName() const;
		static std::string GetSecurityString(SecurityMode mode);
	private:
		// functions
		DuplexPipe(as::io_context& ioctx, HANDLE pipeHandle, std::string name, bool asClient);
		static HANDLE Connect_(const std::string& name);
		static HANDLE Make_(const std::string& name, const std::string& security = {});
		// wrapper to convert EOF system_error to PipeBroken error, with optional timeout
		as::awaitable<void> Read_(size_t byteCount, std::optional<uint32_t> timeoutMs = {});
		// wrapper to convert EOF system_error to PipeBroken error, with optional timeout
		as::awaitable<void> Write_(std::optional<uint32_t> timeoutMs = {});
		as::awaitable<void> Timeout_(uint32_t ms);
		void TransformError_(const boost::system::error_code& ec);
		// data
		static std::atomic<uint32_t> nextUid_;
		std::string name_;
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