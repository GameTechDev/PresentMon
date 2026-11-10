#include "PresentMonDiagnostics.h"
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/log/DiagnosticDriver.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../PresentMonMiddleware/LogSetup.h"
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cstring>
#include "Internal.h"

using namespace pmon;
using namespace pmon::util;

log::Level GetLogLevel_(PM_DIAGNOSTIC_LEVEL dl) noexcept
{
	return log::Level(dl);
}

namespace
{
	bool IsValidDiagnosticLevel_(PM_DIAGNOSTIC_LEVEL level) noexcept
	{
		const auto value = (int)level;
		return value >= (int)PM_DIAGNOSTIC_LEVEL_NONE &&
			value <= (int)PM_DIAGNOSTIC_LEVEL_VERBOSE2 && value % 10 == 0;
	}

	// copy the caller's sized struct over a default-initialized one: fields beyond the
	// caller's size keep their defaults, bytes beyond our sizeof are ignored
	template<typename T>
	T ReadSizedStruct_(const T& in) noexcept
	{
		T local{};
		std::memcpy(&local, &in, std::min<size_t>(in.size, sizeof(T)));
		local.size = sizeof(T);
		return local;
	}
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetup(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
{
	try {
		if (!pConfig) {
			throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "pConfig pointer is null.");
		}
		log::SetupDiagnosticChannel(pConfig);
		return PM_STATUS_SUCCESS;
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetQueuedMessageCount(void)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->GetQueuedMessageCount();
		}
	} pmcatch_report_diag(false);
	return 0;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetMaxQueuedMessages(void)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->GetMaxQueuedMessages();
		}
	} pmcatch_report_diag(false);
	return 0;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetMaxQueuedMessages(uint32_t max)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			pDiag->SetMaxQueuedMessages(max);
			return PM_STATUS_SUCCESS;
		}
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetDiscardedMessageCount(void)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->GetDiscardedMessageCount();
		}
	} pmcatch_report_diag(false);
	return 0;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticDequeueMessage(PM_DIAGNOSTIC_MESSAGE** ppMessage)
{
	try {
		if (ppMessage) {
			if (auto pDiag = log::GetDiagnostics()) {
				if (auto pMessage = pDiag->DequeueMessage()) {
					*ppMessage = pMessage.release();
				}
				else {
					*ppMessage = nullptr;
				}
				return PM_STATUS_SUCCESS;
			}
		}
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticEnqueueMessage(const PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	try {
		if (pMessage) {
			if (auto pDiag = log::GetDiagnostics()) {
				pDiag->EnqueueMessage(pMessage);
				return PM_STATUS_SUCCESS;
			}
		}
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticFreeMessage(PM_DIAGNOSTIC_MESSAGE* pMessage)
{
	try {
		if (pMessage) {
			delete pMessage;
			return PM_STATUS_SUCCESS;
		}
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_DIAGNOSTIC_WAKE_REASON pmDiagnosticWaitForMessage(uint32_t timeoutMs)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->WaitForMessage(timeoutMs);
		}
	} pmcatch_report_diag(false);
	return PM_DIAGNOSTIC_WAKE_REASON_ERROR;
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticUnblockWaitingThread(void)
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			pDiag->UnblockWaitingThread();
			return PM_STATUS_SUCCESS;
		}
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmSetMiddlewareLogPolicy_(
	const api2::MiddlewareLogPolicy* pPolicy) noexcept
{
	try {
		if (!pPolicy || pPolicy->size < api2::kMiddlewareLogPolicySizeV1) {
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto policy = ReadSizedStruct_(*pPolicy);
		if (!IsValidDiagnosticLevel_(policy.logLevel) || !IsValidDiagnosticLevel_(policy.traceLevel)) {
			return PM_STATUS_BAD_ARGUMENT;
		}
		auto& global = log::GlobalPolicy::Get();
		global.SetLogLevel(GetLogLevel_(policy.logLevel));
		global.SetTraceLevel(GetLogLevel_(policy.traceLevel));
		global.SetExceptionTrace(policy.exceptionTrace != 0);
		global.SetSehTracing(policy.sehTrace != 0);
		global.SetResolveTraceInClientThread(policy.resolveTraceInClientThread != 0);
		global.StoreVerboseModules(policy.verboseModuleBitset);
		return PM_STATUS_SUCCESS;
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmGetMiddlewareLogPolicy_(
	api2::MiddlewareLogPolicy* pPolicy) noexcept
{
	try {
		if (!pPolicy || pPolicy->size < api2::kMiddlewareLogPolicySizeV1) {
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto& global = log::GlobalPolicy::Get();
		api2::MiddlewareLogPolicy current{};
		current.logLevel = (PM_DIAGNOSTIC_LEVEL)global.GetLogLevel();
		current.traceLevel = (PM_DIAGNOSTIC_LEVEL)global.GetTraceLevel();
		current.exceptionTrace = (uint8_t)global.GetExceptionTrace();
		current.sehTrace = (uint8_t)global.GetSehTracing();
		current.resolveTraceInClientThread = (uint8_t)global.GetResolveTraceInClientThread();
		current.verboseModuleBitset = global.GetVerboseModules();
		const auto filled = std::min<size_t>(pPolicy->size, sizeof(current));
		std::memcpy(pPolicy, &current, filled);
		pPolicy->size = (uint32_t)filled;
		return PM_STATUS_SUCCESS;
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmSetMiddlewareLogOutput_(
	const api2::MiddlewareLogOutput* pOutput) noexcept
{
	try {
		if (!pOutput || pOutput->size < api2::kMiddlewareLogOutputSizeV1) {
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto output = ReadSizedStruct_(*pOutput);
		std::filesystem::path filePath;
		if (output.pFilePath) {
			filePath = output.pFilePath;
		}
		if (!filePath.empty()) {
			if (!filePath.has_filename()) {
				return PM_STATUS_BAD_ARGUMENT;
			}
			std::error_code ec;
			if (const auto folder = filePath.parent_path();
				!folder.empty() && !std::filesystem::is_directory(folder, ec)) {
				return PM_STATUS_NONEXISTENT_FILE_PATH;
			}
		}
		log::SetLocalOutputs(filePath, output.debuggerOutput != 0);
		return PM_STATUS_SUCCESS;
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmGetMiddlewareLogOutput_(
	api2::MiddlewareLogOutputState* pOutput) noexcept
{
	try {
		if (!pOutput || pOutput->size < api2::kMiddlewareLogOutputStateSizeV1 ||
			(pOutput->filePathCapacity != 0 && !pOutput->pFilePath)) {
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto outputs = log::GetLocalOutputs();
		const auto path = outputs.filePath.wstring();
		pOutput->size = (uint32_t)std::min<size_t>(pOutput->size, sizeof(*pOutput));
		pOutput->debuggerOutput = (uint8_t)outputs.debuggerOutput;
		pOutput->filePathLength = (uint32_t)path.size();
		if (path.empty()) {
			// no active file: nothing to copy, succeeds regardless of capacity
			if (pOutput->filePathCapacity != 0) {
				pOutput->pFilePath[0] = L'\0';
			}
			return PM_STATUS_SUCCESS;
		}
		if (path.size() + 1 > pOutput->filePathCapacity) {
			return PM_STATUS_INSUFFICIENT_BUFFER;
		}
		std::copy(path.begin(), path.end(), pOutput->pFilePath);
		pOutput->pFilePath[path.size()] = L'\0';
		return PM_STATUS_SUCCESS;
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmSetMiddlewareLogFilter_(
	const api2::MiddlewareLogFilter* pFilter) noexcept
{
	try {
		if (!pFilter || pFilter->size < api2::kMiddlewareLogFilterSizeV1) {
			return PM_STATUS_BAD_ARGUMENT;
		}
		const auto filter = ReadSizedStruct_(*pFilter);
		const bool hasPath = filter.pListPath && *filter.pListPath;
		switch (filter.listMode) {
		case api2::MiddlewareLogListMode::None:
			if (hasPath) {
				return PM_STATUS_BAD_ARGUMENT;
			}
			log::LineTable::ClearList();
			return PM_STATUS_SUCCESS;
		case api2::MiddlewareLogListMode::Deny:
		case api2::MiddlewareLogListMode::Allow:
		{
			if (!hasPath) {
				return PM_STATUS_BAD_ARGUMENT;
			}
			const std::filesystem::path listPath{ filter.pListPath };
			std::error_code ec;
			if (!std::filesystem::is_regular_file(listPath, ec)) {
				return PM_STATUS_NONEXISTENT_FILE_PATH;
			}
			log::LineTable::ReplaceList(listPath,
				filter.listMode == api2::MiddlewareLogListMode::Deny
				? log::LineTable::ListMode::Black
				: log::LineTable::ListMode::White);
			return PM_STATUS_SUCCESS;
		}
		default:
			return PM_STATUS_BAD_ARGUMENT;
		}
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmFlushMiddlewareLog_(uint32_t timeoutMs) noexcept
{
	try {
		if (auto pChannel = log::GetDefaultChannel()) {
			// 0 waits indefinitely, matching pmDiagnosticWaitForMessage
			if (timeoutMs == 0) {
				pChannel->Flush();
			}
			else if (!pChannel->TryFlushFor(std::chrono::milliseconds{ timeoutMs })) {
				return PM_STATUS_FAILURE;
			}
		}
		return PM_STATUS_SUCCESS;
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT PM_STATUS pmSetupFileLogging_(const char* filename, PM_DIAGNOSTIC_LEVEL logLevel,
	PM_DIAGNOSTIC_LEVEL stackTraceLevel, bool exceptionTrace)
{
	auto filePath = std::filesystem::path(filename);
	log::SetupFileChannel(std::move(filePath), GetLogLevel_(logLevel),
		GetLogLevel_(stackTraceLevel), exceptionTrace);
	return PM_STATUS_SUCCESS;
}
