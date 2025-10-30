// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WindowSpawnHandler.h"
#include "Overlay.h"
#include "OverlayContainer.h"
#include <CommonUtilities/str/String.h>

namespace p2c::kern
{
    using ::pmon::util::log::GlobalPolicy;
    using ::pmon::util::log::Level;

    WindowSpawnHandler::WindowSpawnHandler(DWORD pid, OverlayContainer* pOverlay) : pid{ pid }, pOverlay{ pOverlay }
    {
        pmlog_verb(v::procwatch)(std::format("win spawn handler ctor | pid:{:5x}", pid));
    }

    win::EventHookHandler::Filter WindowSpawnHandler::GetFilter() const
    {
        return {
            .minEvent = EVENT_OBJECT_CREATE,
            .maxEvent = EVENT_OBJECT_CREATE,
            .pid = pid,
        };
    }

    void WindowSpawnHandler::Handle(
        HWINEVENTHOOK hook, DWORD event, HWND hWnd,
        LONG idObject, LONG idChild,
        DWORD dwEventThread, DWORD dwmsEventTime)
    {
        if (GlobalPolicy::VCheck(v::procwatch)) {
            RECT r{};
            GetWindowRect(hWnd, &r);
            pmlog_(Level::Verbose).note(std::format("win-spawn-event | pid:{:5} hwd:{:8x} own:{:8x} vis:{} siz:{} nam:{}",
                pid,
                reinterpret_cast<uintptr_t>(hWnd),
                reinterpret_cast<uintptr_t>(GetWindow(hWnd, GW_OWNER)),
                IsWindowVisible(hWnd),
                win::RectToDims(r).GetArea(),
                ::pmon::util::str::ToNarrow(win::GetWindowTitle(hWnd))
            ));
        }

        // filter to only windows without an owner window
        // window could have died between creation and when we get around to handling
        // so check handle validity first
        if (idObject == OBJID_WINDOW && IsWindow(hWnd) && GetWindow(hWnd, GW_OWNER) == nullptr) {
            if (RECT r;  GetWindowRect(hWnd, &r)) {
                // filter to window with dimensions
                if (r.right - r.left > 0) {
                    pOverlay->RegisterWindowSpawn(pid, hWnd, r);
                }
            }
        }
    }
}