// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "OverlayWindow.h" 
#include <Core/source/infra/Logging.h>
#include <CommonUtilities/Exception.h>

#ifdef _DEBUG
#define ZBAND_DISABLED
#endif
 
using namespace pmon::util;
using v = log::V;

namespace p2c::win 
{
    enum class ZBID
    {
        ZBID_DEFAULT = 0,
        ZBID_DESKTOP = 1,
        ZBID_UIACCESS = 2,
        ZBID_IMMERSIVE_IHM = 3,
        ZBID_IMMERSIVE_NOTIFICATION = 4,
        ZBID_IMMERSIVE_APPCHROME = 5,
        ZBID_IMMERSIVE_MOGO = 6,
        ZBID_IMMERSIVE_EDGY = 7,
        ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
        ZBID_IMMERSIVE_INACTIVEDOCK = 9,
        ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
        ZBID_IMMERSIVE_ACTIVEDOCK = 11,
        ZBID_IMMERSIVE_BACKGROUND = 12,
        ZBID_IMMERSIVE_SEARCH = 13,
        ZBID_GENUINE_WINDOWS = 14,
        ZBID_IMMERSIVE_RESTRICTED = 15,
        ZBID_SYSTEM_TOOLS = 16,
        ZBID_LOCK = 17,
        ZBID_ABOVELOCK_UX = 18,
    };

    OverlayWindow::OverlayWindow(bool fsMode, int x, int y, gfx::DimensionsI clientDimensions, std::wstring name)
        :
        KernelWindow{ std::move(name), (WS_POPUP & ~WS_SYSMENU) | WS_VISIBLE },
        fullscreenMode{ fsMode }
    {
        DWORD exStyles = WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_LAYERED;

        const auto windowArea = ComputeWindowDimensions(clientDimensions);

        bool useZband = fullscreenMode;
#ifdef ZBAND_DISABLED
        useZband = false;
#else
        if (fullscreenMode) {
            exStyles |= WS_EX_TOPMOST;
        }
#endif

        try {
            if (useZband) {
                typedef HWND(WINAPI* CreateWindowInBand)(
                    _In_ DWORD dwExStyle,
                    _In_opt_ ATOM atom,
                    _In_opt_ LPCWSTR lpWindowName,
                    _In_ DWORD dwStyle,
                    _In_ int X, _In_ int Y,
                    _In_ int nWidth, _In_ int nHeight,
                    _In_opt_ HWND hWndParent,
                    _In_opt_ HMENU hMenu,
                    _In_opt_ HINSTANCE hInstance,
                    _In_opt_ LPVOID lpParam,
                    DWORD band
                );
                const auto hmod = LoadLibraryExA("user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                if (hmod == nullptr) {
                    pmlog_error().hr();
                    throw Except<Exception>();
                }
                const auto pCreateWindowInBand = reinterpret_cast<CreateWindowInBand>(GetProcAddress(hmod, "CreateWindowInBand"));
                if (pCreateWindowInBand == nullptr) {
                    pmlog_error().hr();
                    throw Except<Exception>();
                }

                pmlog_verb(v::core_window)(std::format("OVR-Z-CREATE: x:{} y:{} dim:[{},{}] area:[{},{}] name:{}",
                    x, y, clientDimensions.width, clientDimensions.height,
                    windowArea.width, windowArea.height, str::ToNarrow(GetTitle())));
                if (!pCreateWindowInBand(
                    exStyles,
                    GetDefaultClass(),
                    GetTitle().c_str(),
                    styles,
                    x, y, windowArea.width, windowArea.height,
                    nullptr, nullptr, GetModuleHandle(nullptr),
                    this,
                    (DWORD)ZBID::ZBID_UIACCESS
                )) {
                    pmlog_error().hr();
                    throw Except<Exception>();
                }
            }
        }
        catch (...) {}

        // fallback in any case to normal window creation
        if (GetHandle() == nullptr) {
            pmlog_verb(v::core_window)(std::format("OVR-N-CREATE: x:{} y:{} dim:[{},{}] area:[{},{}] name:{}",
                x, y, clientDimensions.width, clientDimensions.height,
                windowArea.width, windowArea.height, str::ToNarrow(GetTitle())));
            CreateWindowExW(
                exStyles,
                MAKEINTATOM(GetDefaultClass()),
                GetTitle().c_str(),
                styles,
                x, y, windowArea.width, windowArea.height,
                nullptr, nullptr, GetModuleHandleW(nullptr),
                this
            );
        }

        // check for error
        if (GetHandle() == nullptr) {
            pmlog_error().hr();
            throw Except<Exception>();
        }
    }

    std::optional<LRESULT> OverlayWindow::CustomHandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_CLOSE) {
            PostQuitMessage(0);
            return 0;
        }
        return {};
    }

    bool OverlayWindow::Fullscreen() const
    {
        return fullscreenMode;
    }
    bool OverlayWindow::Standard() const
    {
        return false;
    }
} 
