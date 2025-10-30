#include "InjectorComplex.h"
#include <filesystem>
#include <boost/process/v2/windows/creation_flags.hpp>
#include "../../../FlashInjectorLibrary/act/Common.h"
#include "../../../FlashInjectorLibrary/act/PushConfig.h"

// TODO: mutex lock warnings, try enabling if msvc fixes broken static analysis here
#pragma warning (disable : 26110 26117)

namespace p2c::kern
{
	using namespace std::literals;
	namespace as = boost::asio;
	
	void InjectorComplex::SetActive(bool active)
	{
		std::lock_guard lk{ mtx_ };
		if (!IsActive(false) && active) {
			pInjector32_ = std::make_unique<InjectorModule_>(true);
			pInjector64_ = std::make_unique<InjectorModule_>(false);
		}
		else if (!active) {
			pInjector32_.reset();
			pInjector64_.reset();
		}
	}
	bool InjectorComplex::IsActive(bool lock) const
	{
		std::unique_lock lk{ mtx_, std::defer_lock };
		if (lock) {
			lk.lock();
		}
		assert(bool(pInjector32_) == bool(pInjector64_));
		return (bool)pInjector32_;
	}
	void InjectorComplex::UpdateConfig(const GfxLayer::Extension::OverlayConfig& cfg)
	{
		std::lock_guard lk{ mtx_ };
		if (IsActive(false)) {
			pInjector32_->UpdateConfig(cfg);
			pInjector64_->UpdateConfig(cfg);
		}
	}
	void InjectorComplex::ChangeTarget(std::optional<std::string> targetModuleName)
	{
		std::lock_guard lk{ mtx_ };
		if (IsActive(false) && targetModuleName_ != targetModuleName) {
			pmlog_dbg("Writing new target name to injectors").pmwatch(targetModuleName.value_or(""s));
			targetModuleName_ = targetModuleName;
			pInjector32_->ChangeTarget(targetModuleName);
			pInjector64_->ChangeTarget(targetModuleName);
		}
	}
	InjectorComplex::InjectorModule_::InjectorModule_(bool is32Bit)
		:
		pipeOut_{ ioctx_ },
		pipeIn_{ ioctx_ },
		pipeErr_{ ioctx_ },
		is32Bit_{ is32Bit },
		injectorProcess_{ ioctx_ }
	{
		// Determine the correct injector executable
		auto exe = is32Bit
			? "FlashInjector-Win32.exe"
			: "FlashInjector-x64.exe";

		// Spawn the child with Asio pipes for stdin/stdout/stderr
		injectorProcess_ = bp2::process{
			ioctx_,
			exe, // using relative path to injector here due to issue with boost.process (following up)
			/* no args = */ std::vector<std::string>{},
			bp2::windows::process_creation_flags<CREATE_NO_WINDOW>(),
			bp2::process_stdio{ pipeIn_, pipeOut_, pipeErr_ }
		};

		// if logging at debug level, pump stderr into pmlog
		namespace log = ::pmon::util::log;
		if (log::GlobalPolicy::Get().GetLogLevel() >= log::Level::Debug) {
			errListenerThread_ = std::jthread{ [this](std::stop_token st) {
				// when someone does listenerThread_.request_stop(),
				// this callback will fire and break ioctx_.run()
				std::stop_callback cb{ st, [this] { ioctx_.stop(); } };
				// kick off the first async read
				SpawnReadErrTask_();
				// enter Asio’s event loop
				ioctx_.run();
			} };
		}

		// Start the listener thread; on stop request we'll call ioctx_.stop()
		listenerThread_ = std::jthread{ [this](std::stop_token st) {
			// when someone does listenerThread_.request_stop(),
			// this callback will fire and break ioctx_.run()
			std::stop_callback cb{ st, [this] { ioctx_.stop(); } };
			// kick off the first async read
			SpawnReadPidTask_();
			// enter Asio’s event loop
			ioctx_.run();
		} };
	}
	void InjectorComplex::InjectorModule_::UpdateConfig(const GfxLayer::Extension::OverlayConfig& cfg)
	{
		config_ = cfg;
		PushConfig_();
	}
	void InjectorComplex::InjectorModule_::ChangeTarget(std::optional<std::string> targetModuleName)
	{
		// Drop any prior action client
		{
			std::lock_guard lk{ actionClientMutex_ };
			injectionPointClient_.reset();
		}

		// Build one message per call; child expects a newline for getline()
		auto msg = targetModuleName.value_or(std::string{}) + "\n";

		// Always hop to the io_context thread to ensure serialization and buffer (string) lifetime
		as::post(ioctx_, [this, m = std::move(msg)] {
			if (!pipeIn_.is_open()) {
				pmlog_warn("Injector stdin is closed; cannot send target");
				return;
			}

			boost::system::error_code ec;
			const auto n = as::write(pipeIn_, as::buffer(m), ec);
			if (ec) {
				// Common Windows pipe errors:
				// 109 = ERROR_BROKEN_PIPE, 232 = ERROR_NO_DATA, 233 = ERROR_PIPE_NOT_CONNECTED
				pmlog_error("Sync write to injector failed").pmwatch(ec.value());
			}
			else if (n != m.size()) {
				pmlog_error("Short write to injector").pmwatch(n).pmwatch(m.size());
			}
		});
	}
	void InjectorComplex::InjectorModule_::SpawnReadPidTask_()
	{
		as::async_read_until(pipeOut_, readBuffer_, '\n',
			[this](boost::system::error_code ec, std::size_t /*bytes*/) {
			if (!ec) {
				// read a line out from the asio buffer
				std::istream is(&readBuffer_);
				std::string line;
				std::getline(is, line);

				try {
					int pid = std::stoi(line);
					const auto pipeName = inj::act::MakePipeName(pid);
					::pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 2'000, 250);
					{
						std::lock_guard lk{ actionClientMutex_ };
						injectionPointClient_.emplace(pipeName);
						pmlog_info("Connected to injection point").pmwatch(pid);
					}
					PushConfig_();
				}
				catch (...) {
					pmlog_error("Failed to read attachment from injector");
				}
			}

			// queue the next read (unless we're shutting down)
			if (pipeOut_.is_open()) {
				SpawnReadPidTask_();
			}
		});
	}
	void InjectorComplex::InjectorModule_::SpawnReadErrTask_()
	{
		as::async_read_until(pipeErr_, errBuffer_, '\n',
			[this](boost::system::error_code ec, std::size_t /*bytes*/) {
			if (!ec) {
				// read a line out from the asio buffer
				std::istream is(&errBuffer_);
				std::string stderrLine;
				std::getline(is, stderrLine);

				try {
					pmlog_dbg("Stderr from injector").pmwatch(is32Bit_).pmwatch(stderrLine);
				}
				catch (...) {
					pmlog_error("Failed to read stderr from injector");
				}
			}

			// queue the next read (unless we're shutting down)
			if (pipeErr_.is_open()) {
				SpawnReadErrTask_();
			}
		});
	}
	void InjectorComplex::InjectorModule_::PushConfig_()
	{
		std::lock_guard lk{ actionClientMutex_ };
		if (injectionPointClient_) {
			if (injectionPointClient_->IsRunning()) {
				injectionPointClient_->DispatchSync(inj::act::PushConfig::Params{ config_ });
			}
			else {
				pmlog_dbg("Disconnection detected, destroying client");
				injectionPointClient_.reset();
			}
		}
	}
}