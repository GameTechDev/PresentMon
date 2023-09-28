// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "StandardWindow.h"
#include <Core/source/infra/log/Logging.h>
#include <Core/source/gfx/Graphics.h>
#include <Core/source/infra/log/v/Window.h>

 
namespace p2c::win 
{
    namespace vvv = infra::log::v;
    using namespace p2c::gfx;

    StandardWindow::StandardWindow(int x, int y, DimensionsI clientDimensions, std::wstring name)
        :
        KernelWindow{ std::move(name), WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU }
    {
        const DWORD exStyles = WS_EX_NOREDIRECTIONBITMAP;

        p2cvlog(vvv::window).note(std::format(L"x:{} y:{} dim:[{},{}] name:{}", x, y, clientDimensions.width, clientDimensions.height, GetTitle())).commit();

        const auto windowArea = ComputeWindowDimensions(clientDimensions);
        CreateWindowExW(
            exStyles,
            MAKEINTATOM(GetDefaultClass()),
            GetTitle().c_str(),
            styles,
            x, y, windowArea.width, windowArea.height,
            nullptr, nullptr, GetModuleHandleW(nullptr),
            this
        );
        // check for error
        if (GetHandle() == nullptr)
        {
            p2clog.hr().commit();
        }
        // bring to front
        SetForegroundWindow(GetHandle());
    }

    StandardWindow::~StandardWindow()
    {}

    std::optional<LRESULT> StandardWindow::CustomHandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_CLOSE)
        {
            PostQuitMessage(0);
            return 0;
        }
        return {};
    }
    bool StandardWindow::Standard() const
    {
        return true;
    }
    bool StandardWindow::Fullscreen() const
    {
        return false;
    }
} 
