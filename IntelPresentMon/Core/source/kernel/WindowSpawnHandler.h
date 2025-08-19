// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/EventHookHandler.h>

namespace p2c::kern
{
    class OverlayContainer;

    class WindowSpawnHandler : public win::EventHookHandler
    {
    public:
        WindowSpawnHandler(DWORD pid, OverlayContainer* pOverlay);
        win::EventHookHandler::Filter GetFilter() const override;
    protected:
        void Handle(
            HWINEVENTHOOK hook, DWORD event, HWND hWnd,
            LONG idObject, LONG idChild,
            DWORD dwEventThread, DWORD dwmsEventTime) override;
    private:
        DWORD pid;
        OverlayContainer* pOverlay;
    };
}