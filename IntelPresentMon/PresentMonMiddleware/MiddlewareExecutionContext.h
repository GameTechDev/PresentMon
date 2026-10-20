#pragma once
#include <cstdint>
#include <optional>

namespace pmon::mid
{
    // define minimal context for client side connection
    struct MiddlewareSessionContext
    {
        // required by the transport, see TransportSessionContext
        uint32_t remotePid = 0;
    };

    struct MiddlewareExecutionContext
    {
        using SessionContextType = MiddlewareSessionContext;
        std::optional<uint32_t> responseWriteTimeoutMs;
    };
}
