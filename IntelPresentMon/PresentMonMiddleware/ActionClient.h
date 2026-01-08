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
            shmPrefix_ = res.shmPrefix;
            shmSalt_ = res.shmSalt;
            pmlog_info(std::format("Opened session with server, pid = [{}]", res.servicePid));
            EstablishSession_(res.servicePid);
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