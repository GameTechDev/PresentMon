// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_client.h>
#include <semaphore>
#include <memory>
#include <optional>
#include "util/AsyncEndpointCollection.h"

namespace p2c::client::util::kernel_relay
{
    class CallRelay;
}

namespace p2c::client::cef
{
    class NanoCefBrowserClient :
        public CefClient,
        public CefLifeSpanHandler,
        public CefDisplayHandler
    {
    public:
        NanoCefBrowserClient();
        // Kernel calls from the page are handed to this relay. Set before the browser is created,
        // since only the page sends them; null leaves them unhandled.
        void SetKernelCallRelay(util::kernel_relay::CallRelay* pRelay);
        CefRefPtr<CefBrowser> GetBrowser();
        CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
        CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
        void OnAfterCreated(CefRefPtr<CefBrowser> browser_) override;
        void OnBeforeClose(CefRefPtr<CefBrowser> browser_) override;
        bool OnProcessMessageReceived(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefProcessId source_process,
            CefRefPtr<CefProcessMessage> message) override;
        CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override;
        bool OnConsoleMessage(
            CefRefPtr<CefBrowser> browser,
            cef_log_severity_t level,
            const CefString& message,
            const CefString& source,
            int line) override;

    protected:
        CefRefPtr<CefContextMenuHandler> pContextMenuHandler;
        CefRefPtr<CefBrowser> pBrowser;
        util::AsyncEndpointCollection endpoints;
        util::kernel_relay::CallRelay* pKernelCallRelay = nullptr;

        // Include the default reference counting implementation.
        IMPLEMENT_REFCOUNTING(NanoCefBrowserClient);
    };
}