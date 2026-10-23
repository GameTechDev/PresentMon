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
            std::string serviceBuildId = "N/A";
            std::string serviceBuildTime = "N/A";
            std::string serviceVersion = "N/A";
            std::string serviceBuildConfig = "N/A";
            std::string middlewareBuildId = "N/A";
            std::string middlewareBuildTime = "N/A";
            std::string middlewareVersion = "N/A";
            std::string middlewareBuildConfig = "N/A";
            std::string middlewareApiVersion = "N/A";
        } kernelVersions;
	};
}
