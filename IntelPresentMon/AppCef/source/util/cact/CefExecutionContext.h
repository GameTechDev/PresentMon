#pragma once
#include <optional>
#include <cstdint>

namespace p2c::client::util
{
    class SignalManager;
}

namespace p2c::client::util::cact
{
    struct CefSessionContext
    {
        // required by the transport, see TransportSessionContext
        uint32_t remotePid = 0;
    };

    struct CefExecutionContext
    {
        // types
        using SessionContextType = CefSessionContext;

        // data
        SignalManager* pSignalManager = nullptr;
        std::optional<uint32_t> responseWriteTimeoutMs;
    };
}
