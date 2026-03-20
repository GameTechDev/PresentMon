#pragma once
#include "PresentMonAPI.h"
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

	// Debug Diagnostic Layer (EXPERIMENTAL)
	// this sub-library interface enables users of the PresentMon API
	// to get detail feedback about issues such as errors in API usage
	// or suboptimal usage patterns
	// It gives more information than can be communicated in error codes
	// and it gives feedback in situations where there are errors in API
	// usage but they are handled permissively rather than hard-failing

	enum PM_DIAGNOSTIC_LEVEL
	{
		PM_DIAGNOSTIC_LEVEL_NONE = 0,
		PM_DIAGNOSTIC_LEVEL_FATAL = 10,
		PM_DIAGNOSTIC_LEVEL_ERROR = 20,
		PM_DIAGNOSTIC_LEVEL_WARNING = 30,
		PM_DIAGNOSTIC_LEVEL_INFO = 40,
		PM_DIAGNOSTIC_LEVEL_PERFORMANCE = 50,
		PM_DIAGNOSTIC_LEVEL_DEBUG = 60,
		PM_DIAGNOSTIC_LEVEL_VERBOSE = 70,
	};

	enum PM_DIAGNOSTIC_SUBSYSTEM
	{
		PM_DIAGNOSTIC_SUBSYSTEM_NONE,
		PM_DIAGNOSTIC_SUBSYSTEM_MIDDLEWARE,
		PM_DIAGNOSTIC_SUBSYSTEM_SERVER,
		PM_DIAGNOSTIC_SUBSYSTEM_WRAPPER,
		PM_DIAGNOSTIC_SUBSYSTEM_INTEL_PRESENTMON,
		PM_DIAGNOSTIC_SUBSYSTEM_FLASH_INJECTOR,
		PM_DIAGNOSTIC_SUBSYSTEM_FLASH_INJECTION_DLL,
		PM_DIAGNOSTIC_SUBSYSTEM_USER = 0x8000,
	};

	enum PM_DIAGNOSTIC_WAKE_REASON
	{
		PM_DIAGNOSTIC_WAKE_REASON_NONE,
		PM_DIAGNOSTIC_WAKE_REASON_MESSAGE_AVAILABLE,
		PM_DIAGNOSTIC_WAKE_REASON_TIMEOUT,
		PM_DIAGNOSTIC_WAKE_REASON_SHUTDOWN,
		PM_DIAGNOSTIC_WAKE_REASON_MANUAL_UNBLOCK,
		PM_DIAGNOSTIC_WAKE_REASON_ERROR,
	};

	enum PM_DIAGNOSTIC_OUTPUT_FLAGS
	{
		PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER = 1,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_STDERR = 2,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_STDOUT = 4,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_QUEUE = 8,
	};

	struct PM_DIAGNOSTIC_MESSAGE
	{
		PM_DIAGNOSTIC_LEVEL level;
		PM_DIAGNOSTIC_SUBSYSTEM system;
		const char* pText;
		const char* pLocation;
		const char* pTrace;
		const char* pTimestamp;
		uint32_t pid;
		uint32_t tid;
	};

	struct PM_DIAGNOSTIC_CONFIGURATION
	{
		// ignore all messages greater than filterLevel
		PM_DIAGNOSTIC_LEVEL filterLevel;
		// bitmask of PM_DIAGNOSTIC_OUTPUT_FLAGS destinations to transmit diagnostics to
		int outputFlags;
		// array of subsystems, ignore all not in this array (set as nullptr to accept all)
		PM_DIAGNOSTIC_SUBSYSTEM* pSubsystems;
		// number of elements in above array of subsystems
		uint32_t nSubsystems;
		// capture timestamps as a string and add to all non-queue outputs
		bool enableTimestamp;
		// capture stack traces as a string when available (typically for error-level messages)
		bool enableTrace;
		// capture source file and line number as a string
		bool enableLocation;
		// do not filter out internal (non-diagnostic marked) logging entries
		bool disableDiagnosticFilter;
		// bitset of verbose logging modules to enable; bit index maps to util::log::V enum value
		uint64_t verboseModuleBitset;
		// use synchronous logging channel mode (submit waits for processing + driver flush per entry)
		bool enableSynchronousLogging;
		// install crash/terminate best-effort flush hooks while diagnostics are configured
		bool enableFlushOnCrash;
	};

	// NOTE: pmDiagnosticDequeueMessage and pmDiagnosticWaitForMessage must both be accessed
	// from the same single thread, never concurrently from multiple threads

	// initialize and configure the diagnostic system; passing in nullptr yield default config
	// calling this function disables any previous logging modes that were set
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetup(const PM_DIAGNOSTIC_CONFIGURATION* pConfig);
	// get number of messages in the queue
	PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetQueuedMessageCount();
	// get max capacity of the message queue
	PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetMaxQueuedMessages();
	// set max capacity of the message queue
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetMaxQueuedMessages(uint32_t);
	// get total number of messages discarded from queue due to insufficient capacity (does not reset)
	PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetDiscardedMessageCount();
	// retrieve a message from the queue; sets pointer to NULL when no message available
	// use pmDiagnosticFreeMessage to delete the message after processed to avoid memory leaks
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticDequeueMessage(PM_DIAGNOSTIC_MESSAGE** ppMessage);
	// inject a message into the queue; messages must use a user-range subsystem id
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticEnqueueMessage(const PM_DIAGNOSTIC_MESSAGE* pMessage);
	// free the resources associated with a retrieved message
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticFreeMessage(PM_DIAGNOSTIC_MESSAGE* pMessage);
	// block calling thread until message is available in queue or timeout exceeded
	// set timeoutMs to 0 to indicate indefinite wait time
	PRESENTMON_API2_EXPORT PM_DIAGNOSTIC_WAKE_REASON pmDiagnosticWaitForMessage(uint32_t timeoutMs);
	// unblock the thread which is blocked on pmDiagnosticWaitForMessage
	// useful during shutdown if you have a worker thread blocked waiting for a message, you can
	// wake it up so that it can exit gracefully
	PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticUnblockWaitingThread();

#ifdef __cplusplus
} // extern "C"
#endif
