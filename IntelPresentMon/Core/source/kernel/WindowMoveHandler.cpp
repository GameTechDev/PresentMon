// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WindowMoveHandler.h"
#include "Overlay.h"
#include <Core/source/win/Window.h>
#include <CommonUtilities/str/String.h>

namespace p2c::kern
{
    using ::pmon::util::log::GlobalPolicy;
    using ::pmon::util::log::Level;

    WindowMoveHandler::WindowMoveHandler(::pmon::util::win::Process proc, Overlay* pOverlay) : proc{ std::move(proc) }, pOverlay{ pOverlay }
    {
        pmlog_verb(v::procwatch)(std::format("win move handler ctor | pid:{:5x} hwd:{:8x}",
            this->proc.pid,
            reinterpret_cast<uintptr_t>(this->proc.hWnd)
        ));
    }
    
    win::EventHookHandler::Filter WindowMoveHandler::GetFilter() const
    {
        return {
            .minEvent = EVENT_OBJECT_LOCATIONCHANGE,
            .maxEvent = EVENT_OBJECT_LOCATIONCHANGE,
            .pid = proc.pid
        };
    }

    void WindowMoveHandler::Handle(
        HWINEVENTHOOK hook, DWORD event, HWND hWnd,
        LONG idObject, LONG idChild,
        DWORD dwEventThread, DWORD dwmsEventTime)
    {
        if (GlobalPolicy::VCheck(v::procwatch)) {
            RECT r{};
            GetWindowRect(hWnd, &r);
            pmlog_(Level::Verbose).note(std::format("win-move-event | pid:{:5} hwd:{:8x} tgt:{} own:{:8x} obj:{:5x} chl:{:5x} vis:{} siz:{} nam:{}",
                proc.pid,
                reinterpret_cast<uintptr_t>(hWnd),
                hWnd == proc.hWnd,
                reinterpret_cast<uintptr_t>(GetWindow(hWnd, GW_OWNER)),
                idObject,
                idChild,
                IsWindowVisible(hWnd),
                win::RectToDims(r).GetArea(),
                ::pmon::util::str::ToNarrow(win::GetWindowTitle(hWnd))
            ));
        }

        if (hWnd == proc.hWnd)
        {
            const auto tgtRect = win::GetWindowClientRect(hWnd);
            pOverlay->UpdateTargetRect(tgtRect);
            pOverlay->UpdateTargetFullscreenStatus();
        }
    }
}
