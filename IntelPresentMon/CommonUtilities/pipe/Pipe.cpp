#include "Pipe.h"
#include "../win/Security.h"
#include <string_view>

namespace pmon::util::pipe
{
	using namespace as::experimental::awaitable_operators;

	std::atomic<uint32_t> DuplexPipe::nextUid_ = 0;

	namespace
	{
		// A client is free to connect and vanish before the accept completes. That costs
		// the listener its pipe instance, but it is ordinary traffic on a public pipe
		// rather than a failure, so it is reported as benign and the caller re-arms.
		[[noreturn]] void ThrowAcceptError_(DWORD error)
		{
			if (error == ERROR_NO_DATA /* pipe is being closed */ || error == ERROR_BROKEN_PIPE) {
				pmlog_dbg("Client vanished before the pipe connection was accepted").hr(error);
				throw Except<PipeBroken>("Client vanished before the pipe connection was accepted");
			}
			pmlog_error("Failure accepting pipe connection").hr(error);
			throw Except<PipeError>("Failure accepting pipe connection");
		}
	}

	as::awaitable<void> DuplexPipe::Accept()
	{
		assert(!asioPipeHandle_.is_open());
		pmlog_dbg(std::format("{}:{} awaiting to accept connection", name_, uid_));
		as::windows::object_handle connEvt{ co_await as::this_coro::executor, win::Event{}.Release() };
		OVERLAPPED over{ .hEvent = connEvt.native_handle() };
		const auto result = ConnectNamedPipe(rawPipeHandle_, &over);
		if (result) {
			// some error has occurred during connect initiation
			// (this is not expected for an overlapped connect operation)
			pmlog_error("Failure accepting pipe connection (unexpected path)").hr();
			throw Except<PipeError>("Failure accepting pipe connection (unexpected path)");
		}
		if (const auto error = GetLastError(); error == ERROR_IO_PENDING) {
			// async operation is in-flight and not yet complete, do async wait while not complete
			for (;;) {
				co_await connEvt.async_wait(as::use_awaitable);
				// after completion signal, get result to A) make sure not a spurious wake,
				// B) make sure there was no error, and C) conclude the overlapped operation cleanly
				DWORD dummyBytes = 0;
				if (GetOverlappedResult(rawPipeHandle_, &over, &dummyBytes, FALSE)) {
					break;
				}
				if (const auto waitError = GetLastError(); waitError != ERROR_IO_INCOMPLETE) {
					ThrowAcceptError_(waitError);
				}
			}
			// now we have connected, so transfer pipe ownership to asio
			asioPipeHandle_.assign(rawPipeHandle_.Release());
		}
		else if (error == ERROR_PIPE_CONNECTED) {
			// connected even before we could await event
			// we have connected, so transfer pipe ownership to asio
			asioPipeHandle_.assign(rawPipeHandle_.Release());
		}
		else {
			// some error has occurred during connection
			ThrowAcceptError_(error);
		}
		pmlog_dbg(std::format("{}:{} has received a connection", name_, uid_));
	}
	DuplexPipe DuplexPipe::Connect(const std::string& name, as::io_context& ioctx, PipeLimits limits)
	{
		return DuplexPipe{ ioctx, Connect_(name), name, true, limits };
	}
	std::unique_ptr<DuplexPipe> DuplexPipe::ConnectAsPtr(const std::string& name, as::io_context& ioctx, PipeLimits limits)
	{
		return std::unique_ptr<DuplexPipe>(new DuplexPipe{ ioctx, Connect_(name), name, true, limits });
	}
	std::unique_ptr<DuplexPipe> DuplexPipe::MakeAsPtr(const std::string& name, as::io_context& ioctx, const std::string& security, PipeLimits limits)
	{
		return std::unique_ptr<DuplexPipe>(new DuplexPipe{ ioctx, Make_(name, security), name, false, limits });
	}
	void DuplexPipe::DiscardPacketPayload()
	{
		readBuf_.consume(readBuf_.size());
	}
	void DuplexPipe::Cancel_()
	{
		if (!asioPipeHandle_.is_open()) {
			return;
		}
		boost::system::error_code ec;
		asioPipeHandle_.cancel(ec);
		if (ec) {
			pmlog_warn("Failure cancelling pipe operations").pmwatch(ec.message());
		}
	}
	void DuplexPipe::Close()
	{
		Cancel_();
		boost::system::error_code ec;
		if (asioPipeHandle_.is_open()) {
			asioPipeHandle_.close(ec);
			if (ec) {
				pmlog_warn("Failure closing pipe").pmwatch(ec.message());
			}
		}
		rawPipeHandle_.Clear();
		// After the handle is closed, wake CoroMutex waiters so they observe
		// the closed pipe and return instead of resuming into a write.
		writeMtx_.CancelWaiters();
		readMtx_.CancelWaiters();
	}
	size_t DuplexPipe::GetWriteBufferPending() const
	{
		return writeBuf_.size();
	}
	void DuplexPipe::ClearWriteBuffer()
	{
		return writeBuf_.consume(GetWriteBufferPending());
	}
	bool DuplexPipe::WaitForAvailability(const std::string& name, uint32_t timeoutMs, uint32_t pollPeriodMs)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		while (std::chrono::high_resolution_clock::now() - start < 1ms * timeoutMs) {
			if (WaitNamedPipeA(name.c_str(), 0)) {
				return true;
			}
			else {
				std::this_thread::sleep_for(1ms * pollPeriodMs);
			}
		}
		return false;
	}
	bool DuplexPipe::WaitForVacancy(const std::string& name, uint32_t timeoutMs, uint32_t pollPeriodMs)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		while (std::chrono::high_resolution_clock::now() - start < 1ms * timeoutMs) {
			if (!WaitNamedPipeA(name.c_str(), 0)) {
				const DWORD err = GetLastError();
				if (err == ERROR_FILE_NOT_FOUND) {
					return true; // Vacant: no pipe instances exist.
				}
			}
			std::this_thread::sleep_for(1ms * pollPeriodMs);
		}
		return false;
	}

	uint32_t DuplexPipe::GetId() const
	{
		return uid_;
	}
	std::string DuplexPipe::GetName() const
	{
		return name_;
	}
	std::string DuplexPipe::GetSecurityString(SecurityMode mode)
	{
		switch (mode) {
		default:case SecurityMode::None: return {};
		case SecurityMode::Service: return "D:P(A;;GA;;;AU)S:(ML;;NW;;;LW)"s;
		case SecurityMode::Child: return "D:(A;OICI;GA;;;WD)"s;
		}
	}

	DuplexPipe::DuplexPipe(as::io_context& ioctx, HANDLE pipeHandle, std::string name, bool asClient, PipeLimits limits)
		:
		name_{ std::move(name) },
		limits_{ limits },
		rawPipeHandle_{ pipeHandle },
		asioPipeHandle_{ ioctx },
		readStream_{ &readBuf_ },
		readArchive_{ readStream_ },
		readMtx_{ ioctx },
		writeStream_{ &writeBuf_ },
		writeArchive_{ writeStream_ },
		writeMtx_{ ioctx }
	{
		if (asClient) {
			// client is automatically connected upon creation, so immediatly transfer pipe to asio
			asioPipeHandle_.assign(rawPipeHandle_.Release());
		}
	}
	HANDLE DuplexPipe::Connect_(const std::string& name)
	{
		win::Handle handle(CreateFileA(
			name.c_str(),					// Pipe name 
			GENERIC_READ | GENERIC_WRITE,	// Desired access: Read/Write 
			0,								// No sharing 
			NULL,							// Default security attributes
			OPEN_EXISTING,					// Opens existing pipe 
			FILE_FLAG_OVERLAPPED,			// Use overlapped (asynchronous) mode
			NULL));							// No template file 
		if (!handle) {
			pmlog_error("Client failed to connect to named pipe instance").pmwatch(name).hr();
			throw Except<PipeError>("Client failed to connect to named pipe instance");
		}
		return handle.Release();
	}
	HANDLE DuplexPipe::Make_(const std::string& name, const std::string& security)
	{
		pmlog_dbg(std::format("Creating instance of [{}] with security [{}]", name, security));
		// structure required for creating named pipe, create with placeholder pointer for descriptor
		SECURITY_ATTRIBUTES securityAttributes{
			.nLength = sizeof(securityAttributes),
			.lpSecurityDescriptor = nullptr,
			.bInheritHandle = FALSE,
		};
		// if we have a security string, create the descriptor
		UniqueLocalPtr<void> pSecDesc;
		if (!security.empty()) {
			pSecDesc = win::MakeSecurityDescriptor(security);
			securityAttributes.lpSecurityDescriptor = pSecDesc.get();
		}
		// if we have a security string, call create pipe with above structure, else call with nullptr
		SECURITY_ATTRIBUTES* pSecurityAttributes = security.empty() ? nullptr : &securityAttributes;
		// create the named pipe and retain the handle in a wrapper object
		win::Handle handle(CreateNamedPipeA(
			name.c_str(),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,							// open mode
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_REJECT_REMOTE_CLIENTS,	// pipe mode
			PIPE_UNLIMITED_INSTANCES,											// max instances
			4096,					// out buffer
			4096,					// in buffer
			0,						// timeout
			pSecurityAttributes));	// security
		if (!handle) {
			pmlog_error("Server failed to create named pipe instance").hr();
			throw Except<PipeError>("Server failed to create named pipe instance");
		}
		// release the owned handle to be captured by some other owner
		return handle.Release();
	}
	as::awaitable<void> DuplexPipe::Read_(size_t byteCount, std::optional<uint32_t> timeoutMs)
	{
		if (timeoutMs) {
			const auto result = co_await(as::async_read(asioPipeHandle_, readBuf_, as::transfer_exactly(byteCount),
				as::as_tuple(as::use_awaitable)) || Timeout_(*timeoutMs));
			// 2nd index active means the timer finished first: expired, or cancelled from outside
			if (result.index() == 1) {
				if (std::get<1>(result)) {
					throw Except<PipeReadTimeout>("Timeout during read");
				}
				throw Except<PipeOperationCanceled>("Read canceled");
			}
			// otherwise 1st index active => extract error code and transform
			auto&& [ec, n] = std::get<0>(result);
			TransformError_(ec);
		}
		else {
			const auto [ec, n] = co_await as::async_read(asioPipeHandle_, readBuf_, as::transfer_exactly(byteCount),
				as::as_tuple(as::use_awaitable));
			TransformError_(ec);
		}
	}
	as::awaitable<void> DuplexPipe::Write_(std::optional<uint32_t> timeoutMs)
	{
		if (timeoutMs) {
			const auto result = co_await(as::async_write(asioPipeHandle_, writeBuf_, as::as_tuple(as::use_awaitable))
				|| Timeout_(*timeoutMs));
			// 2nd index active means the timer finished first: expired, or cancelled from outside
			if (result.index() == 1) {
				if (std::get<1>(result)) {
					throw Except<PipeError>("Timeout during write");
				}
				throw Except<PipeOperationCanceled>("Write canceled");
			}
			// otherwise 1st index active => extract error code and transform
			auto&& [ec, n] = std::get<0>(result);
			TransformError_(ec);
		}
		else {
			const auto [ec, n] = co_await as::async_write(asioPipeHandle_, writeBuf_, as::as_tuple(as::use_awaitable));
			TransformError_(ec);
		}
	}
	as::awaitable<bool> DuplexPipe::Timeout_(uint32_t ms)
	{
		as::deadline_timer timer{ co_await as::this_coro::executor };
		timer.expires_from_now(boost::posix_time::millisec{ ms });
		// Losing the race to the I/O cancels this wait. That is the normal case, so it
		// completes quietly instead of throwing; only a real expiry reports true.
		auto ec = boost::system::error_code{};
		co_await timer.async_wait(as::redirect_error(as::use_awaitable, ec));
		co_return !ec;
	}
	
	void DuplexPipe::TransformError_(const boost::system::error_code& ec)
	{
		if (ec) {
			if (ec == as::error::broken_pipe || ec.value() == 232/* Pipe is being closed */) {
				throw Except<PipeBroken>();
			}
			else if (ec.value() == 995) {
				throw Except<PipeOperationCanceled>();
			}
			else {
				throw Except<PipeError>(ec.what());
			}
		}
	}
}
