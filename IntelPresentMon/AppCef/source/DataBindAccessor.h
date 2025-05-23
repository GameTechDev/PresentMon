// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_v8.h>
#include "util/KernelWrapper.h"
#include <shared_mutex>


namespace p2c::client::cef
{
    class DataBindAccessor : public CefV8Handler
    {
    public:
        DataBindAccessor(CefRefPtr<CefBrowser> pBrowser, util::KernelWrapper* pKernelWrapper_);
        bool Execute(const CefString& name,
            CefRefPtr<CefV8Value> object,
            const CefV8ValueList& arguments,
            CefRefPtr<CefV8Value>& retval,
            CefString& exception) override;
        void ResolveAsyncEndpoint(uint64_t uid, bool success, CefRefPtr<CefValue> pArgs);
        void ClearKernelWrapper();
    private:
        // data
        CefRefPtr<CefBrowser> pBrowser;
        std::shared_mutex kernelMtx;
        util::KernelWrapper* pKernelWrapper;

        IMPLEMENT_REFCOUNTING(DataBindAccessor);
        friend class DBAKernelHandler;
    };
}