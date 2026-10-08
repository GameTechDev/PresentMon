#include "SharedServicePipeVerification.h"
#include "../PresentMonService/GlobalIdentifiers.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../Interprocess/source/act/SymmetricActionConnector.h"
#include "../CommonUtilities/win/ServiceProcess.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/str/String.h"
#include <filesystem>
#include <format>

namespace pmon::mid
{
	using namespace util;
	using namespace util::win;
	using namespace ipc;

	namespace
	{
		// Named pipes are case-insensitive. Fold before the default-pipe gate so a
		// differently cased well-known name cannot skip the shared-service checks.
		std::string NormalizeControlPipeBaseName_(std::string pipeName)
		{
			constexpr std::string_view prefix = R"(\\.\pipe\)";
			std::string lowered = str::ToLower(pipeName);
			if (lowered.starts_with(prefix)) {
				return lowered;
			}
			return std::string{ prefix } + lowered;
		}

		bool ExecutableBaseNameIsPresentMonServiceWide_(const std::filesystem::path& imagePath)
		{
			return str::ToLower(imagePath.filename().wstring()) == L"presentmonservice.exe";
		}

		WindowsServiceVerificationInfo RequireSharedServiceScmInfo_()
		{
			const auto scmInfo = TryGetWindowsServiceVerificationInfo(gid::sharedWindowsServiceName);
			if (!scmInfo) {
				pmlog_error("Shared PresentMon Windows service is not running or not registered").diag();
				throw Except<PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH,
					"PresentMonSharedService is not running");
			}
			return *scmInfo;
		}

	}

	void ValidateSharedServiceScmRecord(const WindowsServiceVerificationInfo& scmInfo)
	{
		if (scmInfo.binaryPath.empty() ||
			!ExecutableBaseNameIsPresentMonServiceWide_(std::filesystem::path{ scmInfo.binaryPath })) {
			pmlog_error("Shared service binary path is not PresentMonService.exe")
				.pmwatch(str::ToNarrow(scmInfo.binaryPath))
				.diag();
			throw Except<PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH,
				"Shared service binary path mismatch");
		}
		if (!ServiceStartNameIsLocalSystem(scmInfo.serviceStartName)) {
			pmlog_error("Shared service is not configured to run as LocalSystem")
				.pmwatch(str::ToNarrow(scmInfo.serviceStartName))
				.diag();
			throw Except<PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH,
				"Shared service account mismatch");
		}
	}

	void ValidateSharedServicePipeServerProcessId(
		uint32_t pipeServerPid,
		const WindowsServiceVerificationInfo& scmInfo)
	{
		if (pipeServerPid != scmInfo.processId) {
			pmlog_error("Control pipe server process does not match SCM service process")
				.pmwatch(pipeServerPid)
				.pmwatch(scmInfo.processId)
				.diag();
			throw Except<PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH,
				"Control pipe server is not PresentMonSharedService");
		}
	}

	bool IsDefaultSharedServiceControlPipe(const std::string& controlPipeBaseName) noexcept
	{
		try {
			return NormalizeControlPipeBaseName_(controlPipeBaseName) == gid::defaultControlPipeName;
		}
		catch (...) {
			return false;
		}
	}

	void EnsureSharedPresentMonServiceAvailableBeforePipeConnect(const std::string& controlPipeBaseName)
	{
		if (!IsDefaultSharedServiceControlPipe(controlPipeBaseName)) {
			return;
		}

		const auto scmInfo = RequireSharedServiceScmInfo_();
		ValidateSharedServiceScmRecord(scmInfo);
		pmlog_dbg("Shared PresentMon Windows service is running and configured before pipe connect")
			.pmwatch(scmInfo.processId)
			.pmwatch(str::ToNarrow(scmInfo.binaryPath));
	}

	void VerifySharedServiceControlPipeServerIfNeeded(
		const std::string& controlPipeBaseName,
		ipc::act::SymmetricActionConnector<MiddlewareExecutionContext>& conn)
	{
		if (!IsDefaultSharedServiceControlPipe(controlPipeBaseName)) {
			return;
		}

		const uint32_t pipeServerPid = conn.ResolveConnectedServerProcessId();
		const auto scmInfo = RequireSharedServiceScmInfo_();
		ValidateSharedServiceScmRecord(scmInfo);
		ValidateSharedServicePipeServerProcessId(pipeServerPid, scmInfo);

		pmlog_info("Verified shared service control pipe server identity")
			.pmwatch(pipeServerPid)
			.pmwatch(str::ToNarrow(scmInfo.binaryPath));
	}
}
