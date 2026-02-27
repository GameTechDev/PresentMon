#include <memory>
#include <crtdbg.h>
#include <unordered_map>
#include "../PresentMonMiddleware/Middleware.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../CommonUtilities/log/Verbose.h"
#include "Internal.h"
#include "PresentMonAPI.h"
#include "PresentMonDiagnostics.h"
#include "../PresentMonMiddleware/LogSetup.h"
#include "../Versioning/PresentMonAPIVersion.h"
#include <ranges>
#include <string>
#include <type_traits>


using namespace pmon;
using namespace pmon::mid;
namespace rn = std::ranges;
using v = pmon::util::log::V;

// global state
bool useCrtHeapDebug_ = false;
// map handles (session, query, introspection) to middleware instances
std::unordered_map<const void*, std::shared_ptr<Middleware>> handleMap_;
const auto DescribePointerArg_ = []<typename T>(const T* p, bool dereferenceScalar = true) -> std::string
{
	if (!p) {
		return "null";
	}
	using U = std::remove_cv_t<T>;
	constexpr bool isCharLike = std::is_same_v<U, char> || std::is_same_v<U, signed char> || std::is_same_v<U, unsigned char>;
	if constexpr ((std::is_integral_v<U> && !isCharLike) || std::is_floating_point_v<U>) {
		if (dereferenceScalar) {
			if constexpr (std::is_integral_v<U>) {
				if constexpr (std::is_signed_v<U>) {
					return std::to_string((long long)*p);
				}
				else {
					return std::to_string((unsigned long long)*p);
				}
			}
			else {
				return std::to_string((double)*p);
			}
		}
	}
	return "set";
};


// private implementation functions
Middleware& LookupMiddleware_(const void* handle)
{
	try {
		return *handleMap_.at(handle);
	}
	catch (...) {
		pmlog_error("session handle not found during lookup").diag();
		throw util::Except<ipc::PmStatusError>(PM_STATUS_SESSION_NOT_OPEN);
	}
}
Middleware& LookupMiddlewareCheckDropped_(const void* handle)
{
	auto& mid = LookupMiddleware_(handle);
	if (!mid.ServiceConnected()) {
		pmlog_error("Service dropped; proactive abort")
			.code(PM_STATUS_SESSION_NOT_OPEN)
			.raise<ipc::PmStatusError>();
	}
	return mid;
}

void DestroyMiddleware_(PM_SESSION_HANDLE handle)
{
	if (!handleMap_.erase(handle)) {
		pmlog_error("session handle not found during destruction").diag();
		throw util::Except<ipc::PmStatusError>(PM_STATUS_SESSION_NOT_OPEN);
	}
}

void AddHandleMapping_(PM_SESSION_HANDLE sessionHandle, const void* dependentHandle)
{
	handleMap_[dependentHandle] = handleMap_[sessionHandle];
}

void RemoveHandleMapping_(const void* dependentHandle)
{
	if (!handleMap_.erase(dependentHandle)) {
		pmlog_error("handle not found").diag();
		throw util::Except<ipc::PmStatusError>(PM_STATUS_BAD_HANDLE);
	}
}

PRESENTMON_API2_EXPORT _CrtMemState pmCreateHeapCheckpoint_()
{
	_CrtMemState s;
	_CrtMemCheckpoint(&s);
	return s;
}

PRESENTMON_API2_EXPORT LoggingSingletons pmLinkLogging_(
	std::shared_ptr<pmon::util::log::IChannel> pChannel,
	std::function<pmon::util::log::IdentificationTable&()> getIdTable)
{
	using namespace util::log;
	// set api dll default logging channel to copy to exe logging channel
	SetupCopyChannel(std::move(pChannel));
	// connecting id tables (dll => exe)
	if (getIdTable) {
		// hooking exe table up so that it receives updates
		class Sink : public IIdentificationSink
		{
		public:
			Sink(std::function<IdentificationTable& ()> getIdTable)
				:
				getIdTable_{ std::move(getIdTable) }
			{}
			void AddThread(uint32_t tid, uint32_t pid, std::string name) override
			{
				getIdTable_().AddThread_(tid, pid, name);
			}
			void AddProcess(uint32_t pid, std::string name) override
			{
				getIdTable_().AddProcess_(pid, name);
			}
		private:
			std::function<IdentificationTable& ()> getIdTable_;
		};
		IdentificationTable::RegisterSink(std::make_shared<Sink>(getIdTable));
		// copying current contents of table to exe
		const auto bulk = IdentificationTable::GetBulk();
		for (auto& t : bulk.threads) {
			getIdTable().AddThread_(t.tid, t.pid, t.name);
		}
		for (auto& p : bulk.processes) {
			getIdTable().AddProcess_(p.pid, p.name);
		}
	}
	// return functions to access the global settings objects
	return {
		.getGlobalPolicy = []() -> GlobalPolicy& { return GlobalPolicy::Get(); },
		.getLineTable = []() -> LineTable& { return LineTable::Get_(); },
	};
}

PRESENTMON_API2_EXPORT void pmFlushEntryPoint_() noexcept
{
	pmon::util::log::FlushEntryPoint();
}

PRESENTMON_API2_EXPORT void pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL logLevel,
	PM_DIAGNOSTIC_LEVEL stackTraceLevel, bool exceptionTrace)
{
	pmon::util::log::SetupODSChannel((pmon::util::log::Level)logLevel,
		(pmon::util::log::Level)stackTraceLevel, exceptionTrace);
}

PRESENTMON_API2_EXPORT PM_STATUS pmOpenSessionWithPipe(PM_SESSION_HANDLE* pHandle, const char* pipe)
{
	try {
		pmlog_dbg("pmOpenSessionWithPipe")
			.pmwatch(pipe ? pipe : "null");
		if (!pHandle) {
			pmlog_error("null session handle outptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		std::shared_ptr<Middleware> pMiddleware;
		pMiddleware = std::make_shared<Middleware>(pipe ? std::optional<std::string>{ pipe } : std::nullopt);
		*pHandle = reinterpret_cast<PM_SESSION_HANDLE>(pMiddleware.get());
		handleMap_[*pHandle] = std::move(pMiddleware);
		pmlog_dbg("pmOpenSessionWithPipe complete").pmwatch(*pHandle);
		pmlog_info("Middleware successfully opened session with service");
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

// public endpoints
PRESENTMON_API2_EXPORT PM_STATUS pmOpenSession(PM_SESSION_HANDLE* pHandle)
{
	pmlog_dbg("pmOpenSession");
	return pmOpenSessionWithPipe(pHandle, nullptr);
}

PRESENTMON_API2_EXPORT PM_STATUS pmCloseSession(PM_SESSION_HANDLE handle)
{
	try {
		pmlog_dbg("pmCloseSession").pmwatch(handle);
		DestroyMiddleware_(handle);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmStartTrackingProcess(PM_SESSION_HANDLE handle, uint32_t processId)
{
	try {
		pmlog_dbg("pmStartTrackingProcess").pmwatch(handle).pmwatch(processId);
		LookupMiddleware_(handle).StartTracking(processId);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmStartPlaybackTracking(PM_SESSION_HANDLE handle, uint32_t processId, uint32_t isBackpressured)
{
	try {
		pmlog_dbg("pmStartPlaybackTracking").pmwatch(handle).pmwatch(processId).pmwatch(isBackpressured);
		LookupMiddleware_(handle).StartPlaybackTracking(processId, isBackpressured != 0);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmStopTrackingProcess(PM_SESSION_HANDLE handle, uint32_t processId)
{
	try {
		pmlog_dbg("pmStopTrackingProcess").pmwatch(handle).pmwatch(processId);
		LookupMiddleware_(handle).StopTracking(processId);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmGetIntrospectionRoot(PM_SESSION_HANDLE handle, const PM_INTROSPECTION_ROOT** ppInterface)
{
	try {
		pmlog_dbg("pmGetIntrospectionRoot")
			.pmwatch(handle);
		if (!ppInterface) {
			pmlog_error("null outptr for introspection interface").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto pIntro = LookupMiddlewareCheckDropped_(handle).GetIntrospectionData();
		// we don't need the middleware to free introspection data
		// detaching like this (eliding handle mapping) will allow introspection data
		// to not obstruct cleanup of middleware
		// if the lifecycle of marshalled introspection data changes this might need to
		// change as well
		// AddHandleMapping_(handle, pIntro);
		*ppInterface = pIntro;
		pmlog_dbg("pmGetIntrospectionRoot complete").pmwatch(handle).watch("interface_out", DescribePointerArg_(*ppInterface, false));
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmFreeIntrospectionRoot(const PM_INTROSPECTION_ROOT* pInterface)
{
	try {
		pmlog_dbg("pmFreeIntrospectionRoot").watch("pInterface", DescribePointerArg_(pInterface, false));
		if (!pInterface) {
			// freeing nullptr is a no-op
			return PM_STATUS_SUCCESS;
		}
		// see note in pmGetIntrospectionRoot above
		// RemoveHandleMapping_(pInterface);
		// if we free directly here instead of using the middleware method
		// we can support freeing even after middleware has been destroyed
		free(const_cast<PM_INTROSPECTION_ROOT*>(pInterface));
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmSetTelemetryPollingPeriod(PM_SESSION_HANDLE handle, uint32_t deviceId, uint32_t timeMs)
{
	try {
		pmlog_dbg("pmSetTelemetryPollingPeriod").pmwatch(handle).pmwatch(deviceId).pmwatch(timeMs);
		LookupMiddleware_(handle).SetTelemetryPollingPeriod(deviceId, timeMs);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmSetEtwFlushPeriod(PM_SESSION_HANDLE handle, uint32_t periodMs)
{
	try {
		pmlog_dbg("pmSetEtwFlushPeriod").pmwatch(handle).pmwatch(periodMs);
		LookupMiddleware_(handle).SetEtwFlushPeriod(periodMs ? std::optional{ periodMs } : std::nullopt);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmFlushFrames(PM_SESSION_HANDLE handle, uint32_t processId)
{
	try {
		pmlog_dbg("pmFlushFrames").pmwatch(handle).pmwatch(processId);
		LookupMiddleware_(handle).FlushFrames(processId);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmRegisterDynamicQuery(PM_SESSION_HANDLE sessionHandle, PM_DYNAMIC_QUERY_HANDLE* pQueryHandle,
	PM_QUERY_ELEMENT* pElements, uint64_t numElements, double windowSizeMs, double metricOffsetMs)
{
	try {
		pmlog_dbg("pmRegisterDynamicQuery")
			.pmwatch(sessionHandle)
			.watch("pQueryHandle_out", DescribePointerArg_(pQueryHandle, false))
			.watch("pElements", DescribePointerArg_(pElements, false))
			.pmwatch(numElements)
			.pmwatch(windowSizeMs)
			.pmwatch(metricOffsetMs);
		if (!pElements) {
			pmlog_error("null pointer to query element array argument").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!numElements) {
			pmlog_error("zero length query element array").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto queryHandle = LookupMiddlewareCheckDropped_(sessionHandle).RegisterDynamicQuery(
			{pElements, numElements}, windowSizeMs, metricOffsetMs);
		AddHandleMapping_(sessionHandle, queryHandle);
		*pQueryHandle = queryHandle;
		pmlog_dbg("pmRegisterDynamicQuery complete").pmwatch(sessionHandle).pmwatch(queryHandle).pmwatch(*pQueryHandle);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmFreeDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle)
{
	try {
		pmlog_dbg("pmFreeDynamicQuery").pmwatch(handle);
		if (!handle) {
			// freeing nullptr is a no-op
			return PM_STATUS_SUCCESS;
		}
		auto& mid = LookupMiddleware_(handle);
		RemoveHandleMapping_(handle);
		mid.FreeDynamicQuery(handle);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmPollDynamicQuery(PM_DYNAMIC_QUERY_HANDLE handle, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains)
{
	try {
		const auto requestedSwapChains = numSwapChains ? *numSwapChains : 0u;
		pmlog_verb(v::middleware)("pmPollDynamicQuery")
			.pmwatch(handle)
			.pmwatch(processId)
			.watch("pBlob", DescribePointerArg_(pBlob, false))
			.watch("numSwapChains", DescribePointerArg_(numSwapChains))
			.pmwatch(requestedSwapChains);
		if (!pBlob) {
			pmlog_error("null blob ptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!numSwapChains) {
			pmlog_error("null swap chain inoutptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!*numSwapChains) {
			pmlog_error("swap chain in count is zero").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		LookupMiddlewareCheckDropped_(handle).PollDynamicQuery(handle, processId, pBlob, numSwapChains);
		pmlog_verb(v::middleware)("pmPollDynamicQuery complete").pmwatch(handle).pmwatch(processId).pmwatch(*numSwapChains);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmPollDynamicQueryWithTimestamp(PM_DYNAMIC_QUERY_HANDLE handle, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains, uint64_t nowTimestamp)
{
	try {
		const auto requestedSwapChains = numSwapChains ? *numSwapChains : 0u;
		pmlog_verb(v::middleware)("pmPollDynamicQueryWithTimestamp")
			.pmwatch(handle)
			.pmwatch(processId)
			.watch("pBlob", DescribePointerArg_(pBlob, false))
			.watch("numSwapChains", DescribePointerArg_(numSwapChains))
			.pmwatch(requestedSwapChains)
			.pmwatch(nowTimestamp);
		if (!pBlob) {
			pmlog_error("null blob ptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!numSwapChains) {
			pmlog_error("null swap chain inoutptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!*numSwapChains) {
			pmlog_error("swap chain in count is zero").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		LookupMiddleware_(handle).PollDynamicQuery(handle, processId, pBlob, numSwapChains, nowTimestamp);
		pmlog_verb(v::middleware)("pmPollDynamicQueryWithTimestamp complete").pmwatch(handle).pmwatch(processId).pmwatch(*numSwapChains);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmPollStaticQuery(PM_SESSION_HANDLE sessionHandle, const PM_QUERY_ELEMENT* pElement, uint32_t processId, uint8_t* pBlob)
{
	try {
		pmlog_dbg("pmPollStaticQuery")
			.pmwatch(sessionHandle)
			.pmwatch(processId)
			.watch("pElement", DescribePointerArg_(pElement, false))
			.watch("pBlob", DescribePointerArg_(pBlob, false))
			.pmwatch(pElement ? (int)pElement->metric : -1)
			.pmwatch(pElement ? (int)pElement->stat : -1)
			.pmwatch(pElement ? pElement->deviceId : 0u)
			.pmwatch(pElement ? pElement->arrayIndex : 0u);
		if (!pElement) {
			pmlog_error("null ptr to query element").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!pBlob) {
			pmlog_error("null ptr to blob").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		LookupMiddlewareCheckDropped_(sessionHandle).PollStaticQuery(*pElement, processId, pBlob);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmRegisterFrameQuery(PM_SESSION_HANDLE sessionHandle, PM_FRAME_QUERY_HANDLE* pQueryHandle, PM_QUERY_ELEMENT* pElements, uint64_t numElements, uint32_t* pBlobSize)
{
	try {
		const auto inputBlobSize = pBlobSize ? *pBlobSize : 0u;
		pmlog_dbg("pmRegisterFrameQuery")
			.pmwatch(sessionHandle)
			.watch("pQueryHandle_out", DescribePointerArg_(pQueryHandle, false))
			.watch("pElements", DescribePointerArg_(pElements, false))
			.pmwatch(numElements)
			.watch("pBlobSize_inout", DescribePointerArg_(pBlobSize))
			.pmwatch(inputBlobSize);
		if (pElements && numElements) {
			pmlog_dbg("pmRegisterFrameQuery first element")
				.pmwatch((int)pElements[0].metric)
				.pmwatch((int)pElements[0].stat)
				.pmwatch(pElements[0].deviceId)
				.pmwatch(pElements[0].arrayIndex);
		}
		if (!pQueryHandle) {
			pmlog_error("null query handle outptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!pElements) {
			pmlog_error("null ptr to blob").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!numElements) {
			pmlog_error("zero query elements").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!pBlobSize) {
			pmlog_error("zero blob size").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto queryHandle = LookupMiddlewareCheckDropped_(sessionHandle).RegisterFrameEventQuery({ pElements, numElements }, *pBlobSize);
		AddHandleMapping_(sessionHandle, queryHandle);
		*pQueryHandle = queryHandle;
		pmlog_dbg("pmRegisterFrameQuery complete").pmwatch(sessionHandle).pmwatch(queryHandle).pmwatch(*pQueryHandle).pmwatch(*pBlobSize);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmConsumeFrames(PM_FRAME_QUERY_HANDLE handle, uint32_t processId, uint8_t* pBlob, uint32_t* pNumFramesToRead)
{
	try {
		const auto requestedFrames = pNumFramesToRead ? *pNumFramesToRead : 0u;
		pmlog_verb(v::middleware)("pmConsumeFrames")
			.pmwatch(handle)
			.pmwatch(processId)
			.watch("pBlob", DescribePointerArg_(pBlob, false))
			.watch("pNumFramesToRead", DescribePointerArg_(pNumFramesToRead))
			.pmwatch(requestedFrames);
		if (!pBlob) {
			pmlog_error("null blob outptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		if (!pNumFramesToRead) {
			pmlog_error("null frame count in-out ptr").diag();
			return PM_STATUS_BAD_ARGUMENT;
		}
		LookupMiddlewareCheckDropped_(handle).ConsumeFrameEvents(handle, processId, pBlob, *pNumFramesToRead);
		pmlog_verb(v::middleware)("pmConsumeFrames complete").pmwatch(handle).pmwatch(processId).pmwatch(*pNumFramesToRead);
		return PM_STATUS_SUCCESS;
	}
	catch (...) {
		const auto code = util::GeneratePmStatus();
		if (code == PM_STATUS_INVALID_PID) {
			// invalid pid is an exception that happens at the end of a normal workflow, so don't flag as error
			pmlog_info(util::ReportException()).code(code).diag();
		}
		else {
			pmlog_error(util::ReportException()).code(code).diag();
		}
		return code;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmFreeFrameQuery(PM_FRAME_QUERY_HANDLE handle)
{
	try {
		pmlog_dbg("pmFreeFrameQuery").pmwatch(handle);
		auto& mid = LookupMiddleware_(handle);
		RemoveHandleMapping_(handle);
		mid.FreeFrameEventQuery(handle);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmGetApiVersion(PM_VERSION* pVersion)
{
	pmlog_dbg("pmGetApiVersion").watch("pVersion_out", DescribePointerArg_(pVersion, false));
	if (!pVersion) {
		pmlog_error("null outptr for api version get").diag();
		return PM_STATUS_BAD_ARGUMENT;
	}
	*pVersion = pmon::bid::GetApiVersion();
	pmlog_dbg("pmGetApiVersion complete").pmwatch(pVersion->major).pmwatch(pVersion->minor).pmwatch(pVersion->patch);
	return PM_STATUS_SUCCESS;
}

PRESENTMON_API2_EXPORT PM_STATUS pmStopPlayback_(PM_SESSION_HANDLE handle)
{
	try {
		auto& mid = LookupMiddlewareCheckDropped_(handle);
		mid.StopPlayback();
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmStartEtlLogging(PM_SESSION_HANDLE session, PM_ETL_HANDLE* pEtlHandle,
	uint64_t reserved1, uint64_t reserved2)
{
	try {
		pmlog_dbg("pmStartEtlLogging")
			.pmwatch(session)
			.watch("pEtlHandle_out", DescribePointerArg_(pEtlHandle, false))
			.pmwatch(reserved1)
			.pmwatch(reserved2);
		auto& mid = LookupMiddlewareCheckDropped_(session);
		*pEtlHandle = mid.StartEtlLogging();
		pmlog_dbg("pmStartEtlLogging complete").pmwatch(session).pmwatch(*pEtlHandle);
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}

PRESENTMON_API2_EXPORT PM_STATUS pmFinishEtlLogging(PM_SESSION_HANDLE session, PM_ETL_HANDLE etlHandle,
	char* pOutputFilePathBuffer, uint32_t bufferSize)
{
	try {
		pmlog_dbg("pmFinishEtlLogging")
			.pmwatch(session)
			.pmwatch(etlHandle)
			.watch("pOutputFilePathBuffer", DescribePointerArg_(pOutputFilePathBuffer, false))
			.pmwatch(bufferSize);
		auto& mid = LookupMiddlewareCheckDropped_(session);
		const auto path = mid.FinishEtlLogging(etlHandle);
		if (path.size() + 1 > bufferSize) {
			const auto code = PM_STATUS_INSUFFICIENT_BUFFER;
			pmlog_error().code(code);
			// best effort to remove hanging temp file, ignore any further error here
			std::error_code ec;
			std::filesystem::remove(path, ec);
			return code;
		}
		rn::copy(path, pOutputFilePathBuffer);
		pOutputFilePathBuffer[path.size()] = '\0';
		pmlog_dbg("pmFinishEtlLogging complete").pmwatch(session).pmwatch(etlHandle).pmwatch(path.size());
		return PM_STATUS_SUCCESS;
	}
	pmcatch_report_diag(true);
}
