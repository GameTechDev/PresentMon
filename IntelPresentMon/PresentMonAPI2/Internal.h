#pragma once
#include "PresentMonAPI.h"
#include "PresentMonDiagnostics.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/LineTable.h"
#include "../CommonUtilities/log/GlobalPolicy.h"
#include "../CommonUtilities/log/IdentificationTable.h"
#include <memory>
#include <functional>
#include <cstddef>


// testing support functions
PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_();

// log configuration support functions
struct LoggingSingletons
{
	using GlobalPolicyRefFn = pmon::util::log::GlobalPolicy & (*)();
	using LineTableRefFn = pmon::util::log::LineTable & (*)();
	GlobalPolicyRefFn getGlobalPolicy = nullptr;
	LineTableRefFn getLineTable = nullptr;
	operator bool() const noexcept
	{
		return getGlobalPolicy || getLineTable;
	}
};
// Connect (subordinate) the dll logging system to the host module (copy link).
// The link passes C++ objects across the module boundary and is therefore only valid
// when host and middleware come from identical builds; use it only for explicit
// middleware log copy (e.g. --log-middleware-copy). Configure middleware logging via
// pmSetMiddlewareLog*_ below, which do not require or involve the link.
// Prefer pmLinkLoggingPtrs_ for all in-repo callers: do not pass shared_ptr or
// std::function across module boundaries (/MT heap). pmLinkLogging_ remains exported
// for older loader/middleware ABI (v2.5.1 mangled name); avoid new call sites.
// DEPRECATED for configuration: the returned getters for config singletons in the dll;
// use pmSetMiddlewareLogPolicy_ / pmSetMiddlewareLogFilter_ instead.
using PmLogEntryForwardFn = void(*)(pmon::util::log::IChannel*,
	const pmon::util::log::Entry&) noexcept;
using PmIdAddThreadFn = void(*)(pmon::util::log::IdentificationTable* pTable, uint32_t tid,
	uint32_t pid, const char* name, size_t nameLen) noexcept;
using PmIdAddProcessFn = void(*)(pmon::util::log::IdentificationTable* pTable, uint32_t pid,
	const char* name, size_t nameLen) noexcept;
PRESENTMON_API2_EXPORT LoggingSingletons pmLinkLoggingPtrs_(
	pmon::util::log::IChannel* pChannel,
	pmon::util::log::IdentificationTable* pExeTable,
	PmLogEntryForwardFn forwardLogEntryFn,
	PmIdAddThreadFn forwardAddThreadFn,
	PmIdAddProcessFn forwardAddProcessFn) noexcept;
PRESENTMON_API2_EXPORT LoggingSingletons pmLinkLogging_(
	std::shared_ptr<pmon::util::log::IChannel> pChannel,
	std::function<pmon::util::log::IdentificationTable&()> getIdTable);
// disconnect any logging bridge created by pmLinkLogging_
PRESENTMON_API2_EXPORT void pmUnlinkLogging_() noexcept;
// function to flush the dll's log channel worker queue when before exiting
PRESENTMON_API2_EXPORT void pmFlushEntryPoint_() noexcept;
// DEPRECATED: use pmSetMiddlewareLogPolicy_ + pmSetMiddlewareLogOutput_.
// Retained for loader/middleware ABI compatibility; do not add new call sites.
// set middleware to log using OutputDebugString (replaces the middleware log channel)
PRESENTMON_API2_EXPORT void pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL logLevel,
	PM_DIAGNOSTIC_LEVEL stackTraceLevel, bool exceptionTrace);

// DEPRECATED: use pmSetMiddlewareLogPolicy_ + pmSetMiddlewareLogOutput_.
// Retained for loader/middleware ABI compatibility; do not add new call sites.
// configure the middleware to log to a file (replaces the middleware log channel)
// filename: path of the logfile to write
// logLevel: only log entries at or above this severity
// stackTraceLevel: only add stacktraces to entries at or above this level
// exceptionTrace: capture additional stacktrace at point of throwing of exceptions
PRESENTMON_API2_EXPORT PM_STATUS pmSetupFileLogging_(const char* filename, PM_DIAGNOSTIC_LEVEL logLevel,
	PM_DIAGNOSTIC_LEVEL stackTraceLevel, bool exceptionTrace);

// ============================================================================
// Middleware log configuration (internal, not for external clients)
//
// ABI contract for all structs in this section:
// - caller sets size = sizeof(struct) as compiled and fills ALL fields; every
//   call is a full replacement of that struct's settings
// - callee applies every field that lies entirely within size; fields beyond
//   size (caller is older) take their documented default
// - bytes beyond the callee's own sizeof (caller is newer) are ignored
// - fields are only ever appended; existing fields are never removed,
//   reordered, retyped, or reinterpreted; reserved_ bytes are ignored
// - all-or-nothing: if any field fails validation, nothing is applied and
//   PM_STATUS_BAD_ARGUMENT is returned
// - none of these endpoints link, unlink, or depend on the host logging copy
//   link (pmLinkLoggingPtrs_); they are safe across mismatched builds
// ============================================================================
namespace pmon::api2
{
	// ---- policy: what gets logged and how much detail is captured ----------
	struct MiddlewareLogPolicy
	{
		uint32_t size = sizeof(MiddlewareLogPolicy);
		PM_DIAGNOSTIC_LEVEL logLevel = PM_DIAGNOSTIC_LEVEL_ERROR;
		PM_DIAGNOSTIC_LEVEL traceLevel = PM_DIAGNOSTIC_LEVEL_NONE;
		uint8_t exceptionTrace = 0;
		uint8_t sehTrace = 0;
		uint8_t resolveTraceInClientThread = 0;
		uint8_t reserved_ = 0;
		// bit index maps to util::log::V enum value
		uint64_t verboseModuleBitset = 0;
	};
	static_assert(sizeof(MiddlewareLogPolicy) == 24);
	static_assert(offsetof(MiddlewareLogPolicy, verboseModuleBitset) == 16);
	// frozen: size of the first published layout, used for minimum size check
	inline constexpr uint32_t kMiddlewareLogPolicySizeV1 = 24;

	// ---- output: where the middleware log entries go ----------------------
	// Outputs are attached to / detached from the middleware existing log
	// channel as named drivers; the channel is created (with error resolution
	// and line policy) if none exists. Setting outputs never replaces the
	// channel, so it does not sever a copy link or a diagnostic driver.
	struct MiddlewareLogOutput
	{
		uint32_t size = sizeof(MiddlewareLogOutput);
		// attach (1) or detach (0) OutputDebugString driver
		uint8_t debuggerOutput = 0;
		uint8_t reserved_[3] = {};
		// full path of log file; nullptr or empty detaches the file driver;
		// read only during the call; same path as current is a no-op
		const wchar_t* pFilePath = nullptr;
	};
#ifdef _WIN64
	static_assert(sizeof(MiddlewareLogOutput) == 16);
	inline constexpr uint32_t kMiddlewareLogOutputSizeV1 = 16;
#else
	static_assert(sizeof(MiddlewareLogOutput) == 12);
	inline constexpr uint32_t kMiddlewareLogOutputSizeV1 = 12;
#endif
	static_assert(offsetof(MiddlewareLogOutput, pFilePath) == 8);

	struct MiddlewareLogOutputState
	{
		uint32_t size = sizeof(MiddlewareLogOutputState);
		uint8_t debuggerOutput = 0;
		uint8_t reserved_[3] = {};
		// caller-provided buffer and capacity in wchar_t elements, including terminator
		uint32_t filePathCapacity = 0;
		// required path length in wchar_t elements, excluding terminator
		uint32_t filePathLength = 0;
		wchar_t* pFilePath = nullptr;
	};
#ifdef _WIN64
	static_assert(sizeof(MiddlewareLogOutputState) == 24);
	inline constexpr uint32_t kMiddlewareLogOutputStateSizeV1 = 24;
#else
	static_assert(sizeof(MiddlewareLogOutputState) == 20);
	inline constexpr uint32_t kMiddlewareLogOutputStateSizeV1 = 20;
#endif
	static_assert(offsetof(MiddlewareLogOutputState, pFilePath) == 16);

	// ---- filter: per-line deny/allow list ----------------------------------
	enum class MiddlewareLogListMode : uint32_t
	{
		None = 0,
		Deny = 1,
		Allow = 2,
	};

	struct MiddlewareLogFilter
	{
		uint32_t size = sizeof(MiddlewareLogFilter);
		// None disables list filtering (pListPath must be null); Deny/Allow
		// replace the current list from pListPath (required) and enable that mode
		// (mode stays None when the list contains only trace override lines)
		MiddlewareLogListMode listMode = MiddlewareLogListMode::None;
		// list file to ingest (trace override markers are honored); read only
		// during the call
		const wchar_t* pListPath = nullptr;
	};
#ifdef _WIN64
	static_assert(sizeof(MiddlewareLogFilter) == 16);
	inline constexpr uint32_t kMiddlewareLogFilterSizeV1 = 16;
#else
	static_assert(sizeof(MiddlewareLogFilter) == 12);
	inline constexpr uint32_t kMiddlewareLogFilterSizeV1 = 12;
#endif
	static_assert(offsetof(MiddlewareLogFilter, pListPath) == 8);
}

extern "C"
{
	// apply full middleware log policy
	PRESENTMON_API2_EXPORT PM_STATUS pmSetMiddlewareLogPolicy_(
		const pmon::api2::MiddlewareLogPolicy* pPolicy) noexcept;
	// read current middleware log policy; fills fields within pPolicy->size, then sets
	// pPolicy->size to the number of bytes actually filled (min of caller size and
	// callee sizeof) so the caller can tell which fields this middleware knows
	PRESENTMON_API2_EXPORT PM_STATUS pmGetMiddlewareLogPolicy_(
		pmon::api2::MiddlewareLogPolicy* pPolicy) noexcept;
	// apply full middleware output configuration (attach/detach middleware-local drivers)
	PRESENTMON_API2_EXPORT PM_STATUS pmSetMiddlewareLogOutput_(
		const pmon::api2::MiddlewareLogOutput* pOutput) noexcept;
	// read active middleware-local outputs; sets size to bytes filled (min of caller size
	// and callee sizeof) and filePathLength to the active path length (0 when no file);
	// returns PM_STATUS_INSUFFICIENT_BUFFER only when a file is active and the capacity
	// (including terminator) is too small, so a zero-capacity call doubles as a size query
	PRESENTMON_API2_EXPORT PM_STATUS pmGetMiddlewareLogOutput_(
		pmon::api2::MiddlewareLogOutputState* pOutput) noexcept;
	// apply full middleware filter configuration
	PRESENTMON_API2_EXPORT PM_STATUS pmSetMiddlewareLogFilter_(
		const pmon::api2::MiddlewareLogFilter* pFilter) noexcept;
	// flush the middleware channel without applying entry-point exit semantics; waits up
	// to timeoutMs for completion (PM_STATUS_FAILURE on timeout), 0 waits indefinitely
	PRESENTMON_API2_EXPORT PM_STATUS pmFlushMiddlewareLog_(uint32_t timeoutMs) noexcept;
}

namespace pmon::api2
{
	inline constexpr uint32_t kComponentBuildInfoVersion = 1;

	struct ComponentBuildInfo
	{
		// Must remain first. Increment kComponentBuildInfoVersion whenever this
		// structure's layout or field semantics change.
		uint32_t version = kComponentBuildInfoVersion;
		char productVersion[32];
		char buildId[64];
		char buildTime[32];
		char buildConfig[4];
	};
}

PRESENTMON_API2_EXPORT PM_STATUS pmGetServiceBuildInfo_(
	PM_SESSION_HANDLE handle, pmon::api2::ComponentBuildInfo* pInfo);
PRESENTMON_API2_EXPORT PM_STATUS pmGetMiddlewareBuildInfo_(
	pmon::api2::ComponentBuildInfo* pInfo);

PRESENTMON_API2_EXPORT PM_STATUS pmStopPlayback_(PM_SESSION_HANDLE handle);
