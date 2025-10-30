// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "StandardWindow.h"
#include <Core/source/infra/Logging.h>
#include <Core/source/gfx/Graphics.h>
#include <CommonUtilities/str/String.h>

 
namespace p2c::win 
{
    using namespace p2c::gfx;
    using v = ::pmon::util::log::V;

    StandardWindow::StandardWindow(int x, int y, DimensionsI clientDimensions, std::wstring name, bool bringToFront)
        :
        KernelWindow{ std::move(name), WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU }
    {
        const DWORD exStyles = WS_EX_NOREDIRECTIONBITMAP;

        pmlog_verb(v::core_window)(std::format("x:{} y:{} dim:[{},{}] name:{}", x, y, clientDimensions.width, clientDimensions.height,
            ::pmon::util::str::ToNarrow(GetTitle())));

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
            pmlog_error().hr();
        }
        // bring to front
        if (bringToFront) {
            SetForegroundWindow(GetHandle());
        }
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
