#pragma once
#include <chrono>

struct _EXCEPTION_POINTERS;

namespace pmon::util::log
{
	struct CrashFlushConfig
	{
		std::chrono::milliseconds flushTimeout = std::chrono::milliseconds{ 250 };
		bool installTerminateHandler = true;
		bool installUnhandledExceptionFilter = true;
	};

	// install process-level best-effort crash flush hooks
	void InstallCrashFlushHooks(const CrashFlushConfig& config = {}) noexcept;
	// remove hooks and restore previous handlers
	void UninstallCrashFlushHooks() noexcept;
	// test helper to invoke the crash-flush routine without terminating the process
	bool ExecuteCrashFlushNowForTesting() noexcept;
	// test helper to invoke the unhandled exception filter hook body without crashing the process
	long ExecuteUnhandledExceptionFilterForTesting(_EXCEPTION_POINTERS* pException) noexcept;
}
