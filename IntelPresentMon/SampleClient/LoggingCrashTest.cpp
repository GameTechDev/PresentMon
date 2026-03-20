#include "LoggingCrashTest.h"
#include "../CommonUtilities/log/CrashFlushHooks.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/win/WinAPI.h"
#include <chrono>
#include <exception>
#include <format>
#include <stdexcept>
#include <thread>
#include <cstdlib>

using namespace std::literals;

namespace
{
	enum class CrashKind_
	{
		CppException = 0,
		Seh = 1,
		Terminate = 2,
	};

	CrashKind_ ParseCrashKind_(int mode)
	{
		if (mode <= static_cast<int>(CrashKind_::CppException)) {
			return CrashKind_::CppException;
		}
		if (mode >= static_cast<int>(CrashKind_::Terminate)) {
			return CrashKind_::Terminate;
		}
		return static_cast<CrashKind_>(mode);
	}

	void LogCrashPrelude_(const char* scenario)
	{
		pmlog_error("CRASH_TEST:BEGIN");
		pmlog_error(std::format("CRASH_TEST:SCENARIO={}", scenario));
		pmlog_error(std::format("CRASH_TEST:PID={}", GetCurrentProcessId()));
	}

	[[noreturn]] void TriggerCppExceptionCrash_()
	{
		std::jthread crashThread{ [] {
			throw std::runtime_error{ "CRASH_TEST:CPP_EXCEPTION" };
		} };
		crashThread.join();
		std::abort();
	}

	[[noreturn]] void TriggerSehCrash_()
	{
		std::jthread crashThread{ [] {
			::RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
		} };
		crashThread.join();
		std::abort();
	}

	[[noreturn]] void TriggerTerminateCrash_()
	{
		std::terminate();
	}
}

int RunLoggingCrashTest(int mode)
{
	pmon::util::log::InstallCrashFlushHooks({
		.flushTimeout = 1500ms,
		.installTerminateHandler = true,
		.installUnhandledExceptionFilter = true,
		});

	const auto crashKind = ParseCrashKind_(mode);
	if (crashKind == CrashKind_::CppException) {
		LogCrashPrelude_("CPP_EXCEPTION");
		TriggerCppExceptionCrash_();
	}
	if (crashKind == CrashKind_::Seh) {
		LogCrashPrelude_("SEH");
		TriggerSehCrash_();
	}
	if (crashKind == CrashKind_::Terminate) {
		LogCrashPrelude_("TERMINATE");
		TriggerTerminateCrash_();
	}

	return -1;
}
