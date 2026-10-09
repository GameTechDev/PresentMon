#pragma once
#include "MiddlewareExecutionContext.h"
#include "../Interprocess/source/act/SymmetricActionConnector.h"
#include "../CommonUtilities/win/ServiceProcess.h"
#include <string>

namespace pmon::mid
{
	void EnsureSharedPresentMonServiceAvailableBeforePipeConnect(const std::string& controlPipeBaseName);

	void VerifySharedServiceControlPipeServerIfNeeded(
		const std::string& controlPipeBaseName,
		ipc::act::SymmetricActionConnector<MiddlewareExecutionContext>& conn);

	bool IsDefaultSharedServiceControlPipe(const std::string& controlPipeBaseName) noexcept;

	void ValidateSharedServiceScmRecord(const util::win::WindowsServiceVerificationInfo& scmInfo);

	void ValidateSharedServicePipeServerProcessId(
		uint32_t pipeServerPid,
		const util::win::WindowsServiceVerificationInfo& scmInfo);
}
