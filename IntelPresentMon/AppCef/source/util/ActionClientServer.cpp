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
        serviceBuildConfig_ = std::move(res.serviceBuildConfig);
        middlewareBuildId_ = std::move(res.middlewareBuildId);
        middlewareBuildTime_ = std::move(res.middlewareBuildTime);
        middlewareVersion_ = std::move(res.middlewareVersion);
        middlewareBuildConfig_ = std::move(res.middlewareBuildConfig);
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

    const std::string& CefClient::GetServiceBuildConfig() const
    {
        return serviceBuildConfig_;
    }

    const std::string& CefClient::GetMiddlewareBuildId() const
    {
        return middlewareBuildId_;
    }

    const std::string& CefClient::GetMiddlewareBuildTime() const
    {
        return middlewareBuildTime_;
    }

    const std::string& CefClient::GetMiddlewareVersion() const
    {
        return middlewareVersion_;
    }

    const std::string& CefClient::GetMiddlewareBuildConfig() const
    {
        return middlewareBuildConfig_;
    }

    const std::string& CefClient::GetMiddlewareApiVersion() const
    {
        return middlewareApiVersion_;
    }
}
