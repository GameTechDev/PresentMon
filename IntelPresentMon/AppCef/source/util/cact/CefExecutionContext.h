#pragma once
#include <optional>
#include <cstdint>

namespace p2c::client::util::kernel_relay
{
    class SignalRelay;
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
        // kernel events arrive in the browser process and are forwarded to the page through this
        const kernel_relay::SignalRelay* pSignalRelay = nullptr;
        std::optional<uint32_t> responseWriteTimeoutMs;
    };
}
