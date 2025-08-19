// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WindowMoveHandler.h"
#include "Overlay.h"

namespace p2c::kern
{
    WindowMoveHandler::WindowMoveHandler(::pmon::util::win::Process proc, Overlay* pOverlay) : proc{ std::move(proc) }, pOverlay{ pOverlay } {}
    
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
        if (hWnd == proc.hWnd)
        {
            const auto tgtRect = win::GetWindowClientRect(hWnd);
            pOverlay->UpdateTargetRect(tgtRect);
            pOverlay->UpdateTargetFullscreenStatus();
        }
    }
}