#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/ref/StaticReflection.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonService/AllActions.h"
#include "../Versioning/BuildId.h"
#include "../Interprocess/source/act/SymmetricActionClient.h"

namespace pmon::mid
{
    using namespace util;
    using namespace svc;
    using namespace acts;

    // define minimial context for client side connection
    struct MiddlewareExecutionContext;
    struct MiddlewareSessionContext
    {
        // common session context items
        std::unique_ptr<SymmetricActionConnector<MiddlewareExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
    };
    struct MiddlewareExecutionContext
    {
        // types
        using SessionContextType = MiddlewareSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };

    using ClientBase = ipc::act::SymmetricActionClient<MiddlewareExecutionContext>;
    class ActionClient : public ClientBase
    {
    public:
        ActionClient(const std::string& pipeName) : ClientBase{ pipeName }
        {
            auto res = DispatchSync(OpenSession::Params{
                .clientPid = GetCurrentProcessId(),
                .clientBuildId = bid::BuildIdShortHash(),
                .clientBuildConfig = bid::BuildIdConfig(),
            });
            if (res.serviceBuildId != bid::BuildIdShortHash()) {
                pmlog_error("build id mismatch between middleware and service")
                    .pmwatch(res.serviceBuildId).pmwatch(bid::BuildIdShortHash());
                throw Except<ipc::PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH);
            }
            if (res.serviceBuildConfig != bid::BuildIdConfig()) {
                pmlog_error("build config mismatch between middleware and service")
                    .pmwatch(res.serviceBuildConfig).pmwatch(bid::BuildIdConfig());
                throw Except<ipc::PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH);
            }
            pmlog_info(std::format("Opened session with server, pid = [{}]", res.servicePid));
            EstablishSession_(res.servicePid);
        }
#pragma warning(push)
#pragma warning(disable: 4715)
        template<class Params>
        auto DispatchSync(Params&& params)
        {
            // convert action client ipc error into presentmon api error
            try {
                return ClientBase::DispatchSync(std::forward<Params>(params));
            }
            catch (const ipc::act::ServerDroppedError& e) {
                pmlog_error(e.GetNote()).code(PM_STATUS_SESSION_NOT_OPEN).raise<ipc::PmStatusError>();
            }
        }
#pragma warning(pop)
        template<class Params>
        void DispatchDetached(Params&& params)
        {
            // convert action client ipc error into presentmon api error
            try {
                ClientBase::DispatchDetached(std::forward<Params>(params));
            }
            catch (const ipc::act::ServerDroppedError& e) {
                pmlog_error(e.GetNote()).code(PM_STATUS_SESSION_NOT_OPEN).raise<ipc::PmStatusError>();
            }
        }
        template<class Params>
        void DispatchWithContinuation(Params&& params, std::function<void(ResponseFromParams<Params>&&, std::exception_ptr)> cont)
        {
            // convert action client ipc error into presentmon api error
            try {
                ClientBase::DispatchWithContinuation(std::forward<Params>(params), std::move(cont));
            }
            catch (const ipc::act::ServerDroppedError& e) {
                pmlog_error(e.GetNote()).code(PM_STATUS_SESSION_NOT_OPEN).raise<ipc::PmStatusError>();
            }
        }
    };
}
