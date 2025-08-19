#include "ActionClient.h"
#include "../../../CommonUtilities/win/WinAPI.h"
#include "../../../Interprocess/source/PmStatusError.h"
#include "../../../FlashInjectorLibrary/act/AllActions.h"
#include <format>

namespace p2c::iact
{
    ActionClient::ActionClient(const std::string& pipeName) : ClientBase{ pipeName }
    {
        auto res = DispatchSync(inj::act::OpenSession::Params{
            .kernelPid = GetCurrentProcessId()
        });
        pmlog_info(std::format("Opened session with injector lib, pid = [{}]", res.injectedLibPid));
        EstablishSession_(res.injectedLibPid);
    }
}