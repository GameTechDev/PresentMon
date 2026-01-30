#pragma once
#include "PresentMonAPI.h"
#include "PresentMonDiagnostics.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/LineTable.h"
#include "../CommonUtilities/log/GlobalPolicy.h"
#include "../CommonUtilities/log/IdentificationTable.h"
#include <memory>
#include <functional>


// testing support functions
PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_();

// log configuration support functions
struct LoggingSingletons
{
	std::function<pmon::util::log::GlobalPolicy& ()> getGlobalPolicy;
	std::function<pmon::util::log::LineTable& ()> getLineTable;
	operator bool() const noexcept
	{
		return getGlobalPolicy || getLineTable;
	}
};
// function to connect (subordinate) the dll logging system to the exe one
// replace default channel (nullptr) with a channel that copies entries to pChannel
// optionally hook up an id table to copy entries to as well
// return getters for config singletons in the dll to config from the exe
PRESENTMON_API2_EXPORT LoggingSingletons pmLinkLogging_(
	std::shared_ptr<pmon::util::log::IChannel> pChannel,
	std::function<pmon::util::log::IdentificationTable&()> getIdTable);
// function to flush the dll's log channel worker queue when before exiting
PRESENTMON_API2_EXPORT void pmFlushEntryPoint_() noexcept;
// set middleware to log using OutputDebugString
PRESENTMON_API2_EXPORT void pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL logLevel,
	PM_DIAGNOSTIC_LEVEL stackTraceLevel, bool exceptionTrace);

// configure the middleware to log to a file (independent of and exclusive to diagnostic system)
// path: path to the folder to where the logfile will be written (set NULL to disable file logging)
// logLevel: only log entries at or above this severity
// stackTraceLevel: only add stacktraces to entries at or above this level 
// exceptionTrace: capture additional stacktrace at point of throwing of exceptions
PRESENTMON_API2_EXPORT PM_STATUS pmSetupFileLogging_(const char* filename, PM_DIAGNOSTIC_LEVEL logLevel,
	PM_DIAGNOSTIC_LEVEL stackTraceLevel, bool exceptionTrace);

PRESENTMON_API2_EXPORT PM_STATUS pmStopPlayback_(PM_SESSION_HANDLE handle);
