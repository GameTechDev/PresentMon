// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WindowActivateHandler.h"
#include "Overlay.h"
#include <Core/source/win/Window.h>
#include <CommonUtilities/str/String.h>

namespace p2c::kern
{
    using ::pmon::util::log::GlobalPolicy;
    using ::pmon::util::log::Level;

    WindowActivateHandler::WindowActivateHandler(::pmon::util::win::Process proc, Overlay* pOverlay) : proc{ std::move(proc) }, pOverlay{ pOverlay }
    {
        pmlog_verb(v::procwatch)(std::format("win activate handler ctor | pid:{:5x} hwd:{:8x}",
            this->proc.pid,
            reinterpret_cast<uintptr_t>(this->proc.hWnd)
        ));
    }

    win::EventHookHandler::Filter WindowActivateHandler::GetFilter() const
    {
        return {
            .minEvent = EVENT_SYSTEM_FOREGROUND,
            .maxEvent = EVENT_SYSTEM_FOREGROUND,
        };
    }

    void WindowActivateHandler::Handle(
        HWINEVENTHOOK hook, DWORD event, HWND hWnd,
        LONG idObject, LONG idChild,
        DWORD dwEventThread, DWORD dwmsEventTime)
    {
        const auto isTarget = hWnd == proc.hWnd;

        if (GlobalPolicy::VCheck(v::procwatch)) {
            DWORD pid = 0;
            RECT r{};
            GetWindowThreadProcessId(hWnd, &pid);
            GetWindowRect(hWnd, &r);
            pmlog_(Level::Verbose).note(std::format("win-activate-event | pid:{:5} hwd:{:8x} tgt:{} own:{:8x} vis:{} l:{} r:{} t:{} b:{} siz:{} nam:{}",
                pid,
                reinterpret_cast<uintptr_t>(hWnd),
                isTarget,
                reinterpret_cast<uintptr_t>(GetWindow(hWnd, GW_OWNER)),
                IsWindowVisible(hWnd),
                r.left,
                r.right,
                r.top,
                r.bottom,
                win::RectToDims(r).GetArea(),
                ::pmon::util::str::ToNarrow(win::GetWindowTitle(hWnd))
            ));
        }

        pOverlay->UpdateTargetOrder(isTarget);
    }
}
