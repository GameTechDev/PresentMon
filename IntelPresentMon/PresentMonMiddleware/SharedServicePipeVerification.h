#pragma once
#include "MiddlewareExecutionContext.h"
#include <string>

namespace pmon::mid
{
	void EnsureSharedPresentMonServiceAvailableBeforePipeConnect(const std::string& controlPipeBaseName);

	void VerifySharedServiceControlPipeServerIfNeeded(
		const std::string& controlPipeBaseName,
		ipc::act::SymmetricActionConnector<MiddlewareExecutionContext>& conn);

	bool IsDefaultSharedServiceControlPipe(const std::string& controlPipeBaseName) noexcept;
}
