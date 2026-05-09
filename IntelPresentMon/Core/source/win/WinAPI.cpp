// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WinAPI.h"
#include <Core/source/infra/Logging.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/win/HrError.h>


namespace p2c::win
{
    using ::pmon::util::Except;
    namespace cwin = ::pmon::util::win;

    std::optional<gfx::RectI> GetWindowRectIOpt(HWND hWnd)
    {
        try {
            return GetWindowRectI(hWnd);
        }
        catch (...){
        }
        return {};
    }

    gfx::RectI GetWindowRectI(HWND hWnd)
    {
        RECT wr{};
        if (!::GetWindowRect(hWnd, &wr)) {
            const auto error = GetLastError();
            pmlog_error("failed to get window rect").hr(error);
            throw Except<cwin::HrError>((HRESULT)error, "failed to get window rect");
        }
        return gfx::RectI{ wr.left, wr.top, wr.right, wr.bottom };
    }

    std::optional<gfx::RectI> GetWindowClientRectIOpt(HWND hWnd)
    {
        try  {
            return GetWindowClientRectI(hWnd);
        }
        catch (...) {
        }
        return {};
    }

    gfx::RectI GetWindowClientRectI(HWND hWnd)
    {
        RECT cr{};
        if (!GetClientRect(hWnd, &cr))  {
            const auto error = GetLastError();
            pmlog_error("failed to get window client rect").hr(error);
            throw Except<cwin::HrError>((HRESULT)error, "failed to get window client rect");
        }

        POINT tl{ 0, 0 };
        if (!ClientToScreen(hWnd, &tl))  {
            const auto error = GetLastError();
            pmlog_error("failed to convert window client rect to screen coordinates").hr(error);
            throw Except<cwin::HrError>((HRESULT)error, "failed to convert window client rect to screen coordinates");
        }
        return gfx::RectI{ tl.x, tl.y, tl.x + cr.right, tl.y + cr.bottom };
    }

    std::wstring GetWindowTitle(HWND hWnd)
    {
        wchar_t buffer[512];
        GetWindowTextW(hWnd, buffer, (int)std::size(buffer));
        return buffer;
    }
}
