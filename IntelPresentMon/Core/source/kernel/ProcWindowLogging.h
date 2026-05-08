// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/infra/Logging.h>
#include <Core/source/win/Window.h>
#include <CommonUtilities/str/String.h>
#include <format>
#include <string_view>

namespace p2c::kern
{
    inline void LogProcessHwnds(DWORD pid, HWND selectedHwnd, std::string_view reason)
    {
        using ::pmon::util::log::GlobalPolicy;
        using ::pmon::util::log::Level;

        if (!GlobalPolicy::VCheck(::pmon::util::log::V::procwatch)) {
            return;
        }

        struct EnumContext
        {
            DWORD pid = 0;
            HWND selectedHwnd = nullptr;
            std::string_view reason;
            size_t count = 0;

            static BOOL CALLBACK Callback(HWND hWnd, LPARAM lParam)
            {
                auto& ctx = *reinterpret_cast<EnumContext*>(lParam);

                DWORD hwndPid = 0;
                GetWindowThreadProcessId(hWnd, &hwndPid);
                if (hwndPid != ctx.pid) {
                    return TRUE;
                }

                RECT rect{};
                const auto gotRect = GetWindowRect(hWnd, &rect) != FALSE;
                const auto dims = gotRect ? win::RectToDims(rect) : gfx::DimensionsI{};
                ctx.count++;

                pmlog_(Level::Verbose).note(std::format(
                    "procwatch-hwnd | why:{} pid:{:5} hwd:{:8x} sel:{} l:{} r:{} t:{} b:{} w:{} h:{} nam:{}",
                    ctx.reason,
                    ctx.pid,
                    reinterpret_cast<uintptr_t>(hWnd),
                    hWnd == ctx.selectedHwnd,
                    rect.left,
                    rect.right,
                    rect.top,
                    rect.bottom,
                    dims.width,
                    dims.height,
                    ::pmon::util::str::ToNarrow(win::GetWindowTitle(hWnd))
                ));

                return TRUE;
            }
        };

        EnumContext ctx{
            .pid = pid,
            .selectedHwnd = selectedHwnd,
            .reason = reason,
        };
        EnumWindows(&EnumContext::Callback, reinterpret_cast<LPARAM>(&ctx));

        pmlog_(Level::Verbose).note(std::format(
            "procwatch-hwnd-end | why:{} pid:{:5} sel:{:8x} cnt:{}",
            reason,
            pid,
            reinterpret_cast<uintptr_t>(selectedHwnd),
            ctx.count
        ));
    }
}
