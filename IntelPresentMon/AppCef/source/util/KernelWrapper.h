#pragma once
#include <memory>
#include "SignalManager.h"
#include "AsyncEndpointManager.h"
#include "ActionClientServer.h"
#include "IpcInvocationManager.h"


namespace p2c::client::util
{
	struct KernelWrapper
	{
        util::SignalManager signals;
        util::AsyncEndpointManager asyncEndpoints;
        std::unique_ptr<CefClient> pClient;
        std::unique_ptr<IpcInvocationManager> pInvocationManager;
	};
}
