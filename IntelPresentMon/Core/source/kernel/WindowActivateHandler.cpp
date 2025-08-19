// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WindowActivateHandler.h"
#include "Overlay.h"

namespace p2c::kern
{
    WindowActivateHandler::WindowActivateHandler(::pmon::util::win::Process proc, Overlay* pOverlay) : proc{ std::move(proc) }, pOverlay{ pOverlay } {}

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
        pOverlay->UpdateTargetOrder(hWnd == proc.hWnd);
    }
}