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
        pmlog_verb(v::procwatch)(std::format("win activate handler ctor | pid:{:5} hwd:{:8x}",
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
        if (idObject != OBJID_WINDOW) {
            return;
        }

        const auto currentProc = pOverlay->GetProcess();
        const auto isTarget = hWnd == currentProc.hWnd;
        const auto rect = win::GetWindowClientRectIOpt(hWnd);

        if (GlobalPolicy::VCheck(v::procwatch)) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hWnd, &pid);
            const auto r = rect.value_or(gfx::RectI{});
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
                r.GetDimensions().GetArea(),
                ::pmon::util::str::ToNarrow(win::GetWindowTitle(hWnd))
            ));
        }

        bool upgraded = false;
        if (!isTarget && rect) {
            upgraded = pOverlay->ConsiderTargetWindowCandidate(hWnd, *rect);
        }

        pOverlay->UpdateTargetOrder(isTarget || upgraded);
    }
}
