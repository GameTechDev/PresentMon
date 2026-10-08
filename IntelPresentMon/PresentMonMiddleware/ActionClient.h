#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/ref/StaticReflection.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonService/AllActions.h"
#include "../Versioning/BuildId.h"
#include "../Interprocess/source/act/SymmetricActionClient.h"
#include "MiddlewareExecutionContext.h"
#include "SharedServicePipeVerification.h"

namespace pmon::mid
{
    using namespace util;
    using namespace svc;
    using namespace acts;

    using ClientBase = ipc::act::SymmetricActionClient<MiddlewareExecutionContext>;
    class ActionClient : public ClientBase
    {
    public:
        ActionClient(const std::string& pipeName) : ClientBase{ pipeName }
        {
            VerifySharedServiceControlPipeServerIfNeeded(GetControlPipeBaseName_(), GetActionConnector_());
            const uint32_t verifiedPipeServerPid = IsDefaultSharedServiceControlPipe(GetControlPipeBaseName_())
                ? GetActionConnector_().ResolveConnectedServerProcessId()
                : 0u;
            auto res = DispatchSync(OpenSession::Params{
                .clientPid = GetCurrentProcessId(),
                .clientBuildId = bid::BuildIdLongHash(),
                .clientBuildConfig = bid::BuildIdConfig(),
            });
            if (IsDefaultSharedServiceControlPipe(GetControlPipeBaseName_()) &&
                res.servicePid != verifiedPipeServerPid) {
                pmlog_error("OpenSession service pid does not match control pipe server pid")
                    .pmwatch(res.servicePid)
                    .pmwatch(verifiedPipeServerPid)
                    .diag();
                throw Except<ipc::PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH);
            }
            if (res.serviceBuildId != bid::BuildIdLongHash()) {
                pmlog_error("build id mismatch between middleware and service")
                    .pmwatch(res.serviceBuildId).pmwatch(bid::BuildIdLongHash()).diag();
                throw Except<ipc::PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH);
            }
            if (res.serviceBuildConfig != bid::BuildIdConfig()) {
                pmlog_error("build config mismatch between middleware and service")
                    .pmwatch(res.serviceBuildConfig).pmwatch(bid::BuildIdConfig()).diag();
                throw Except<ipc::PmStatusError>(PM_STATUS_MIDDLEWARE_SERVICE_MISMATCH);
            }
            shmPrefix_ = res.shmPrefix;
            shmSalt_ = res.shmSalt;
            pmlog_info(std::format("Opened session with server, pid = [{}]", res.servicePid));
            EstablishSession_(res.servicePid);
        }
        template<class Params>
        auto DispatchSync(Params&& params)
        {
            // convert action client ipc error into presentmon api error
            try {
                return ClientBase::DispatchSync(std::forward<Params>(params));
            }
            catch (const ipc::act::ServerDroppedError& e) {
                pmlog_error(e.GetNote()).code(PM_STATUS_SESSION_NOT_OPEN);
                throw util::Except<ipc::PmStatusError>(PM_STATUS_SESSION_NOT_OPEN, e.GetNote());
            }
        }
        template<class Params>
        void DispatchDetached(Params&& params)
        {
            // convert action client ipc error into presentmon api error
            try {
                ClientBase::DispatchDetached(std::forward<Params>(params));
            }
            catch (const ipc::act::ServerDroppedError& e) {
                pmlog_error(e.GetNote()).code(PM_STATUS_SESSION_NOT_OPEN);
                throw util::Except<ipc::PmStatusError>(PM_STATUS_SESSION_NOT_OPEN, e.GetNote());
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
                pmlog_error(e.GetNote()).code(PM_STATUS_SESSION_NOT_OPEN);
                throw util::Except<ipc::PmStatusError>(PM_STATUS_SESSION_NOT_OPEN, e.GetNote());
            }
        }
        const std::string& GetShmPrefix() const
        {
            return shmPrefix_;
        }
        const std::string& GetShmSalt() const
        {
            return shmSalt_;
        }
    private:
        std::string shmPrefix_;
        std::string shmSalt_;
    };
}
