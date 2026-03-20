#include "CrashFlushHooks.h"
#include "Log.h"
#include "PanicLogger.h"
#include "../win/WinAPI.h"
#include <atomic>
#include <exception>
#include <mutex>
#include <cstdlib>

namespace pmon::util::log
{
	namespace
	{
		struct HookState_
		{
			std::mutex mtx;
			CrashFlushConfig config;
			std::terminate_handler pPrevTerminate = nullptr;
			LPTOP_LEVEL_EXCEPTION_FILTER pPrevUnhandled = nullptr;
			bool terminateInstalled = false;
			bool unhandledInstalled = false;
			std::atomic_flag inFlush = ATOMIC_FLAG_INIT;
		};

		HookState_& GetState_() noexcept
		{
			static HookState_ state;
			return state;
		}

		bool RunCrashFlush_() noexcept
		{
			auto& state = GetState_();
			if (state.inFlush.test_and_set()) {
				return false;
			}

			struct Guard_
			{
				std::atomic_flag& flag_;
				~Guard_() { flag_.clear(); }
			} guard{ state.inFlush };

			try {
				if (auto pChannel = GetDefaultChannel()) {
					return pChannel->TryFlushFor(state.config.flushTimeout);
				}
				return true;
			}
			catch (...) {
				pmlog_panic_("Exception thrown during crash flush attempt");
			}
			return false;
		}

		void TerminateHandler_() noexcept
		{
			RunCrashFlush_();

			auto pPrev = GetState_().pPrevTerminate;
			if (pPrev && pPrev != &TerminateHandler_) {
				pPrev();
			}

			abort();
		}

		LONG WINAPI UnhandledExceptionFilter_(EXCEPTION_POINTERS* pException) noexcept
		{
			RunCrashFlush_();

			auto pPrev = GetState_().pPrevUnhandled;
			if (pPrev && pPrev != &UnhandledExceptionFilter_) {
				return pPrev(pException);
			}

			return EXCEPTION_CONTINUE_SEARCH;
		}
	}

	void InstallCrashFlushHooks(const CrashFlushConfig& config) noexcept
	{
		auto& state = GetState_();
		std::lock_guard lk{ state.mtx };
		state.config = config;

		if (state.terminateInstalled) {
			std::set_terminate(state.pPrevTerminate);
			state.terminateInstalled = false;
		}
		if (state.unhandledInstalled) {
			SetUnhandledExceptionFilter(state.pPrevUnhandled);
			state.unhandledInstalled = false;
		}

		if (config.installTerminateHandler) {
			state.pPrevTerminate = std::set_terminate(TerminateHandler_);
			state.terminateInstalled = true;
		}
		if (config.installUnhandledExceptionFilter) {
			state.pPrevUnhandled = SetUnhandledExceptionFilter(UnhandledExceptionFilter_);
			state.unhandledInstalled = true;
		}
	}

	void UninstallCrashFlushHooks() noexcept
	{
		auto& state = GetState_();
		std::lock_guard lk{ state.mtx };

		if (state.terminateInstalled) {
			std::set_terminate(state.pPrevTerminate);
			state.terminateInstalled = false;
		}
		if (state.unhandledInstalled) {
			SetUnhandledExceptionFilter(state.pPrevUnhandled);
			state.unhandledInstalled = false;
		}
	}

	bool ExecuteCrashFlushNowForTesting() noexcept
	{
		return RunCrashFlush_();
	}

	long ExecuteUnhandledExceptionFilterForTesting(_EXCEPTION_POINTERS* pException) noexcept
	{
		return UnhandledExceptionFilter_(reinterpret_cast<EXCEPTION_POINTERS*>(pException));
	}
}
