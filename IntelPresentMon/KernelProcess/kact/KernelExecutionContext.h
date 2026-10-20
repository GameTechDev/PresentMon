#pragma once
#include <optional>
#include <cstdint>
#include "../../Core/source/kernel/Kernel.h"
#include "../../Core/source/win/HotkeyListener.h"
#include "../../CommonUtilities/log/Log.h"
#include "../../Interprocess/source/act/SessionEndReason.h"
#include <format>

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

        // Called by the server once for every session that ends. Hotkey bindings belong to
        // the UI session that made them, so none outlive it.
        void Dispose(SessionContextType&, ipc::act::SessionDisposition disposition) const
        {
            if (pHotkeys) {
                const auto cleared = pHotkeys->ClearAll();
                pmlog_info(std::format("Kernel session ended, cleared {} hotkey bindings reason:{}",
                    cleared, (int)disposition.reason));
            }
        }
    };
}