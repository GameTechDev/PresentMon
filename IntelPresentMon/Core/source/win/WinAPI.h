// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
// remove annoying A/W macros as necessary
#undef FormatMessage
#include <Core/source/gfx/base/Geometry.h>
#include <optional>


namespace p2c::win
{
    std::optional<gfx::RectI> GetWindowRectIOpt(HWND hWnd);
    gfx::RectI GetWindowRectI(HWND hWnd);
    std::optional<gfx::RectI> GetWindowClientRectIOpt(HWND hWnd);
    gfx::RectI GetWindowClientRectI(HWND hWnd);
    std::wstring GetWindowTitle(HWND hWnd);
}
