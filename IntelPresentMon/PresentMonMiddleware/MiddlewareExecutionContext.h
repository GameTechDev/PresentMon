#pragma once
#include "../Interprocess/source/act/SymmetricActionConnector.h"
#include <cstdint>
#include <memory>
#include <optional>

namespace pmon::mid
{
    struct MiddlewareExecutionContext;

    struct MiddlewareSessionContext
    {
        std::unique_ptr<ipc::act::SymmetricActionConnector<MiddlewareExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
    };

    struct MiddlewareExecutionContext
    {
        using SessionContextType = MiddlewareSessionContext;
        std::optional<uint32_t> responseWriteTimeoutMs;
    };
}
