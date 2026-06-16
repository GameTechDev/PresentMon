#include "ActionClientServer.h"
#include "../KernelProcess/kact/OpenSession.h"

namespace p2c::client::util
{
    CefClient::CefClient(std::string pipeName, cact::CefExecutionContext context)
        :
        SymmetricActionClient{ std::move(pipeName), std::move(context) }
    {
        auto res = DispatchSync(kproc::kact::OpenSession::Params{ GetCurrentProcessId() });
        serviceBuildId_ = std::move(res.serviceBuildId);
        serviceBuildTime_ = std::move(res.serviceBuildTime);
        serviceVersion_ = std::move(res.serviceVersion);
        middlewareApiVersion_ = std::move(res.middlewareApiVersion);
        EstablishSession_(res.kernelPid);
    }

    const std::string& CefClient::GetServiceBuildId() const
    {
        return serviceBuildId_;
    }

    const std::string& CefClient::GetServiceBuildTime() const
    {
        return serviceBuildTime_;
    }

    const std::string& CefClient::GetServiceVersion() const
    {
        return serviceVersion_;
    }

    const std::string& CefClient::GetMiddlewareApiVersion() const
    {
        return middlewareApiVersion_;
    }
}
