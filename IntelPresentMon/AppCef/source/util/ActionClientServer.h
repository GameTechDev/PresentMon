#pragma once
#include <Interprocess/source/act/SymmetricActionClient.h>
#include "cact/CefExecutionContext.h"

namespace p2c::client::util
{
    using namespace ::pmon::ipc;

    class CefClient : public act::SymmetricActionClient<cact::CefExecutionContext> {
    public:
        // connectedPipe: the client end of the pipe pair the kernel created for this UI
        CefClient(::pmon::util::win::Handle connectedPipe, cact::CefExecutionContext context);
        const std::string& GetServiceBuildId() const;
        const std::string& GetServiceBuildTime() const;
        const std::string& GetServiceVersion() const;
        const std::string& GetMiddlewareApiVersion() const;
    private:
        std::string serviceBuildId_;
        std::string serviceBuildTime_;
        std::string serviceVersion_;
        std::string middlewareApiVersion_;
    };
}
