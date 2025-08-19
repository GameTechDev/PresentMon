// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/EventHookHandler.h>
#include <CommonUtilities/win/Process.h>

namespace p2c::kern
{
    class Overlay;

    class WindowActivateHandler : public win::EventHookHandler
    {
    public:
        WindowActivateHandler(::pmon::util::win::Process proc, Overlay* pOverlay);
        win::EventHookHandler::Filter GetFilter() const override;
    protected:
        void Handle(
            HWINEVENTHOOK hook, DWORD event, HWND hWnd,
            LONG idObject, LONG idChild,
            DWORD dwEventThread, DWORD dwmsEventTime) override;
    private:
        ::pmon::util::win::Process proc;
        Overlay* pOverlay;
    };
}