#pragma once
#include "../../../Interprocess/source/act/SymmetricActionClient.h"
#include <cstdint>
#include <string>
#include <optional>
#include <memory>

namespace p2c::iact
{
    using namespace ::pmon;
    using namespace util;

    // define minimial context for client side connection
    struct CoreInjectionExecutionContext;
    struct CoreInjectionSessionContext
    {
        // common session context items
        std::unique_ptr<ipc::act::SymmetricActionConnector<CoreInjectionExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
    };
    struct CoreInjectionExecutionContext
    {
        // types
        using SessionContextType = CoreInjectionSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };

    using ClientBase = ipc::act::SymmetricActionClient<CoreInjectionExecutionContext>;
    class ActionClient : public ClientBase
    {
    public:
        ActionClient(const std::string& pipeName);
    };
}