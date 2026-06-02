#pragma once
#include <Interprocess/source/act/SymmetricActionClient.h>
#include "cact/CefExecutionContext.h"

namespace p2c::client::util
{
    using namespace ::pmon::ipc;

    class CefClient : public act::SymmetricActionClient<cact::CefExecutionContext> {
    public:
        CefClient(std::string pipeName, cact::CefExecutionContext context = {});
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
