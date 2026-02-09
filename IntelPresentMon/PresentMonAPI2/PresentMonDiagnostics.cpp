#include "PresentMonDiagnostics.h"
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/log/DiagnosticDriver.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include "../PresentMonMiddleware/LogSetup.h"
#include <filesystem>
#include "Internal.h"

using namespace pmon;
using namespace pmon::util;

log::Level GetLogLevel_(PM_DIAGNOSTIC_LEVEL dl) noexcept
{
	return log::Level(dl);
}

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticSetup(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
{
	try {
		log::SetupDiagnosticChannel(pConfig);
		return PM_STATUS_SUCCESS;
	} pmcatch_report_diag(true);
	return PM_STATUS_FAILURE;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetQueuedMessageCount()
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			return pDiag->GetQueuedMessageCount();
		}
	} pmcatch_report_diag(false);
	return 0;
}

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetMaxQueuedMessages()
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

PRESENTMON_API2_EXPORT uint32_t pmDiagnosticGetDiscardedMessageCount()
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

PRESENTMON_API2_EXPORT PM_STATUS pmDiagnosticUnblockWaitingThread()
{
	try {
		if (auto pDiag = log::GetDiagnostics()) {
			pDiag->UnblockWaitingThread();
			return PM_STATUS_SUCCESS;
		}
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
