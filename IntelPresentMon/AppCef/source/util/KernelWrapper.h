#pragma once
#include <memory>
#include "SignalManager.h"
#include "AsyncEndpointManager.h"
#include "IpcInvocationManager.h"
#include <string>


namespace p2c::client::util
{
	struct KernelWrapper
	{
        util::SignalManager signals;
        util::AsyncEndpointManager asyncEndpoints;
        std::unique_ptr<IpcInvocationManager> pInvocationManager;
        // versions the kernel reported to the browser process, received with browser creation
        struct KernelVersions
        {
            std::string serviceBuildId = "unknown";
            std::string serviceBuildTime = "unknown";
            std::string serviceVersion = "unknown";
            std::string middlewareApiVersion = "unknown";
        } kernelVersions;
	};
}
