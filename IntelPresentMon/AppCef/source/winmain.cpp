// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NanoCefBrowserClient.h"
#include "NanoCefProcessHandler.h"
#include "../resource.h"
#include "util/Logging.h"
#include "util/LogSetup.h"
#include <Core/source/infra/util/FolderResolver.h>
#include "util/CliOptions.h"
#include <CommonUtilities/log/IdentificationTable.h>
#include <Versioning/BuildId.h>
#include <CommonUtilities/win/Utilities.h>
#include <dwmapi.h>
#include <boost/process.hpp>
#include <Shobjidl.h>
#include <include/cef_version.h>


#pragma comment(lib, "Dwmapi.lib")

using namespace p2c;
using namespace ::pmon::util;
using namespace ::pmon::bid;
using p2c::client::util::cli::Options;
namespace ccef = client::cef;
using namespace std::chrono_literals;

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
        if (Options::Get().url) {
            url = *Options::Get().url;
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
        BrowserWindowClassName, "Intel PresentMon",
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
    if (hwndAppMsg != nullptr) {
        PostMessage(hwndAppMsg, WM_COMMAND, quitCefCode, 0);
    }
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef NDEBUG
    constexpr bool is_debug = false;
#else
    constexpr bool is_debug = true;
#endif
    // parse the command line arguments and make them globally available
    if (auto err = Options::Init(__argc, __argv, false)) {
        return *err;
    }
    const auto& opt = Options::Get();
    if (opt.filesWorking) {
        infra::util::FolderResolver::SetDevMode();
    }

    // create logging system and ensure cleanup before main ext
    client::util::LogChannelManager zLogMan_;

    // wait for debugger connection
    if ((opt.cefType && *opt.cefType == "renderer" && opt.debugWaitRender) ||
        (!opt.cefType && opt.debugWaitClient)) {
        while (!IsDebuggerPresent()) {
            std::this_thread::sleep_for(20ms);
        }
        DebugBreak();
    }

    // name this process / thread
    log::IdentificationTable::AddThisProcess(opt.cefType.AsOptional().value_or("main-client"));
    log::IdentificationTable::AddThisThread("main");

    // initialize the logging system
    client::util::ConfigureLogging();

    // set the app id so that windows get grouped
    SetCurrentProcessExplicitAppUserModelID(L"Intel.PresentMon");

    try {
        using namespace client;
        // cef process constellation fork control
        CefMainArgs main_args{ hInstance };
        CefRefPtr<ccef::NanoCefProcessHandler> app = new ccef::NanoCefProcessHandler{};

        if (const auto code = CefExecuteProcess(main_args, app.get(), nullptr); code >= 0) {
            return (int)code;
        }

        // code from here on is only executed by the root process (browser window process)

        pmlog_info(std::format("== client section starting build#{} clean:{} CEF:{} ==",
            BuildIdShortHash(), !BuildIdDirtyFlag(), CEF_VERSION));

        {
            auto& folderResolver = infra::util::FolderResolver::Get();
            CefSettings settings;
            settings.multi_threaded_message_loop = true;
            settings.remote_debugging_port = is_debug || opt.enableChromiumDebug ? 9009 : 0;
            settings.background_color = { 0x000000 };
            CefString(&settings.cache_path).FromWString(folderResolver.Resolve(infra::util::FolderResolver::Folder::App, L"cef-cache"));
            if (opt.logFolder) {
                CefString(&settings.log_file).FromString(*opt.logFolder + "\\cef-debug.log");
            }
            else {
                CefString(&settings.log_file).FromWString(folderResolver.Resolve(infra::util::FolderResolver::Folder::App, L"logs\\cef-debug.log"));
            }
            settings.log_severity = is_debug ? cef_log_severity_t::LOGSEVERITY_DEFAULT : cef_log_severity_t::LOGSEVERITY_ERROR;
            CefInitialize(main_args, settings, app.get(), nullptr);
        }
        auto hwndBrowser = CreateBrowserWindow(hInstance, nCmdShow);
        hwndAppMsg = CreateMessageWindow(hInstance);


        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DestroyWindow(hwndAppMsg);
        CefShutdown();
        DestroyWindow(hwndBrowser);

        UnregisterClass(BrowserWindowClassName, hInstance);
        UnregisterClass(MessageWindowClassName, hInstance);

        pmlog_info("== UI client root process exiting ==");

        return (int)msg.wParam;
    }
    catch (...) {
        pmlog_error(ReportException("Fatal Error"));
        return -1;
    }
}