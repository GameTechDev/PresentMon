#pragma once
#include <optional>
#include <cstdint>
#include "../../Core/source/kernel/Kernel.h"
#include "../../Core/source/win/HotkeyListener.h"

namespace kproc::kact
{
    using namespace ::pmon;
    struct KernelSessionContext
    {
        // required by the transport, see TransportSessionContext
        uint32_t remotePid = 0;
    };

    struct KernelExecutionContext
    {
        // types
        using SessionContextType = KernelSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;

        p2c::kern::Kernel** ppKernel = nullptr;
        p2c::win::Hotkeys* pHotkeys = nullptr;
    };
}