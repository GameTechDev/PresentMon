// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NanoCefBrowserClient.h"
#include "NanoCefProcessHandler.h"
#include "util/ServiceBooter.h"
#include "../resource.h"
#include <Core/source/infra/log/Logging.h>
#include <Core/source/infra/svc/Services.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <Core/source/infra/opt/Options.h>
#include <dwmapi.h>

#pragma comment(lib, "Dwmapi.lib")

using namespace p2c;
namespace ccef = client::cef;
using infra::svc::Services;
namespace opt = infra::opt;

// globals
constexpr const char* BrowserWindowClassName = "BrowserWindowClass";
constexpr const char* MessageWindowClassName = "MessageWindowClass";
constexpr const int quitCefCode = 0xABAD1DEA;
CefRefPtr<client::cef::NanoCefBrowserClient> pBrowserClient;
HWND hwndAppMsg = nullptr;

LRESULT CALLBACK BrowserWindowWndProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_CLOSE:
        if (pBrowserClient) {
            if (auto lResult = pBrowserClient->HandleCloseMessage()) {
                return *lResult;
            }
        }
        break;
    case WM_CREATE:
    {
        pBrowserClient = new ccef::NanoCefBrowserClient{};

        RECT rect = { 0 };
        GetClientRect(window_handle, &rect);
        CefRect cefRect;
        cefRect.x = rect.left;
        cefRect.y = rect.top;
        cefRect.width = rect.right - rect.left;
        cefRect.height = rect.bottom - rect.top;

        CefWindowInfo info;
        info.SetAsChild(window_handle, cefRect);

        CefBrowserSettings settings;
        // this special url (domain+schema) triggers load-from-disk behavior
        std::string url = "https://app/index.html";
        if (opt::get().url) {
            url = *opt::get().url;
        }
        CefBrowserHost::CreateBrowser(
            info, pBrowserClient.get(), url,
            settings, {}, {}
        );
        break;
    }
    case WM_SIZE:
        // from the cefclient example, do not allow the window to be resized to 0x0 or the layout will break;
        // also be aware that if the size gets too small, GPU acceleration disables
        if ((w_param != SIZE_MINIMIZED)
            && (pBrowserClient.get())
            && (pBrowserClient->GetBrowser()))
        {
            CefWindowHandle hwnd(pBrowserClient->GetBrowser()->GetHost()->GetWindowHandle());
            if (hwnd)
            {
                RECT rect = { 0 };
                GetClientRect(window_handle, &rect);
                HDWP hdwp = BeginDeferWindowPos(1);
                hdwp = DeferWindowPos(hdwp, hwnd, NULL, rect.left,
                    rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
                EndDeferWindowPos(hdwp);
            }
        }
        break;
    case WM_ERASEBKGND:
        if (pBrowserClient.get()
            && pBrowserClient->GetBrowser()
            && pBrowserClient->GetBrowser()->GetHost()->GetWindowHandle() != nullptr)
        {
            return 1;
        }
        break;
    case WM_ENTERMENULOOP:
        if (!w_param)
        {
            CefSetOSModalLoop(true);
        }
        break;
    case WM_EXITMENULOOP:
        if (!w_param)
        {
            CefSetOSModalLoop(false);
        }
        break;
    }
    return DefWindowProc(window_handle, message, w_param, l_param);
}

LRESULT CALLBACK MessageWindowWndProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_COMMAND:
        if (w_param == quitCefCode)
        {
            PostQuitMessage(0);
        }
        break;
    }
    return DefWindowProc(window_handle, message, w_param, l_param);
}

#define PROGRAM_NAME "Intel PresentMon"

HWND CreateBrowserWindow(HINSTANCE instance_handle, int show_minimize_or_maximize)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = BrowserWindowWndProc;
    wcex.hInstance = instance_handle;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.lpszClassName = BrowserWindowClassName;
    wcex.hIcon = static_cast<HICON>(LoadImage(
        instance_handle, MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON, 32, 32, 0
    ));
    RegisterClassEx(&wcex);

    HWND hwnd = CreateWindow(
        BrowserWindowClassName, PROGRAM_NAME,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 200, 20,
        1360, 1020, nullptr, nullptr, instance_handle, nullptr
    );

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));

    ShowWindow(hwnd, show_minimize_or_maximize);
    UpdateWindow(hwnd);

    return hwnd;
}

HWND CreateMessageWindow(HINSTANCE instance_handle)
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(wcex);
    wcex.lpfnWndProc = MessageWindowWndProc;
    wcex.hInstance = instance_handle;
    wcex.lpszClassName = MessageWindowClassName;
    RegisterClassEx(&wcex);
    return CreateWindow(MessageWindowClassName, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, instance_handle, 0);
}

void AppQuitMessageLoop()
{
    if (hwndAppMsg != nullptr)
    {
        PostMessage(hwndAppMsg, WM_COMMAND, quitCefCode, 0);
    }
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    using namespace client;
    try {
#ifdef NDEBUG
        std::wstring _path = std::filesystem::current_path();
        if (_path.find(L"\\Program Files\\", 0) == -1LL) // -1 means no offset was found
        {
            wchar_t msg[512]{};
            (void)_snwprintf_s(msg, _countof(msg),
                               L"Unable to start " PROGRAM_NAME " due to path not being in privileged location.\n"
                               "Path: \"%s\"\n"
                               "You must copy installation files to privileged location such as \"C:\\Program Files\".", _path.c_str());
            MessageBoxW(nullptr, msg, L"" PROGRAM_NAME " Startup Error", MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);
            return -1;
        }
#endif
#undef PROGRAM_NAME
        opt::init();
        util::BootServices();

        CefMainArgs main_args{ hInstance };
        CefRefPtr<ccef::NanoCefProcessHandler> app = new ccef::NanoCefProcessHandler{};

        if (const auto code = CefExecuteProcess(main_args, app.get(), nullptr); code >= 0)
        {
            return (int)code;
        }

        {
#ifdef NDEBUG
            constexpr bool is_debug = false;
#else
            constexpr bool is_debug = true;
#endif
            const auto pFolderResolver = Services::Resolve<infra::util::FolderResolver>();
            CefSettings settings;
            settings.multi_threaded_message_loop = true;
            settings.remote_debugging_port = is_debug ? 9009 : 0;
            settings.background_color = { 0x000000 };
            CefString(&settings.cache_path).FromWString(pFolderResolver->Resolve(infra::util::FolderResolver::Folder::App, L"cef-cache"));
            CefString(&settings.log_file).FromWString(pFolderResolver->Resolve(infra::util::FolderResolver::Folder::App, L"logs\\cef-debug.log"));
            settings.log_severity = is_debug ? cef_log_severity_t::LOGSEVERITY_DEFAULT : cef_log_severity_t::LOGSEVERITY_ERROR;
            CefInitialize(main_args, settings, app.get(), nullptr);
        }
        auto hwndBrowser = CreateBrowserWindow(hInstance, nCmdShow);
        hwndAppMsg = CreateMessageWindow(hInstance);

        p2clog.info(L"== hello from client process ==").pid().commit();

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DestroyWindow(hwndAppMsg);
        CefShutdown();
        DestroyWindow(hwndBrowser);

        UnregisterClass(BrowserWindowClassName, hInstance);
        UnregisterClass(MessageWindowClassName, hInstance);

        return (int)msg.wParam;
    }
    catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);
        return -1;
    }
    catch (...) {
        MessageBoxA(nullptr, "Unidentified exception was thrown", "Fatal Error",
            MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);
        return -1;
    }
}