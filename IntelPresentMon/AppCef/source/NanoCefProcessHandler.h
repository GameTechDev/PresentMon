// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_app.h>
#include <memory>
#include "DataBindAccessor.h"
#include "util/KernelWrapper.h"

namespace p2c::client::cef
{
    class NanoCefProcessHandler :
        public CefApp,
        public CefRenderProcessHandler,
        public CefBrowserProcessHandler
    {
    public:
        void OnBeforeCommandLineProcessing(const CefString& processType, CefRefPtr<CefCommandLine> pCommandLine) override;
        void OnContextInitialized() override;
        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;
        CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;
        void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine>) override;
        void OnContextCreated(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefV8Context>) override;
        void OnBrowserCreated(CefRefPtr<CefBrowser>, CefRefPtr<CefDictionaryValue>) override;
        bool OnProcessMessageReceived(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefProcessId, CefRefPtr<CefProcessMessage>) override;

    private:
        static void AddFunctionToObject_(CefString name, CefRefPtr<CefV8Value>& pObj, CefRefPtr<DataBindAccessor>& pAccessor);
        std::unique_ptr<util::KernelWrapper> pKernelWrapper = std::make_unique<util::KernelWrapper>();
        CefRefPtr<DataBindAccessor> pAccessor;
        CefRefPtr<CefBrowser> pBrowser;

        IMPLEMENT_REFCOUNTING(NanoCefProcessHandler);
    };
}