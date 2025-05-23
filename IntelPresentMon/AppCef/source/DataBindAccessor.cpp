// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "DataBindAccessor.h"
#include "util/Logging.h"
#include <include/cef_task.h> 
#include <include/base/cef_callback.h> 
#include <include/wrapper/cef_closure_task.h> 
#include "util/CefValues.h"
#include <Interprocess/source/act/SymmetricActionServer.h>
#include "util/cact/TargetLostAction.h"
#include "util/cact/OverlayDiedAction.h"
#include "util/cact/PresentmonInitFailedAction.h"
#include "util/cact/StalePidAction.h"
#include "util/SignalManager.h"


namespace p2c::client::cef
{
    DataBindAccessor::DataBindAccessor(CefRefPtr<CefBrowser> pBrowser, util::KernelWrapper* pKernelWrapper_)
        :
        pBrowser{ std::move(pBrowser) },
        pKernelWrapper{ pKernelWrapper_ }
    {}

    bool DataBindAccessor::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
    {
        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper)
        {
            // this cannot be made an async endpoint object because it deals with V8 function objects
            // signals are registered before kernel and do not require its existence
            if (name == "registerSignalHandler") {
                if (arguments.size() == 2 && arguments[0]->IsString() && arguments[1]->IsFunction()) {
                    pKernelWrapper->signals.RegisterCallback(arguments[0]->GetStringValue(), { arguments[1], CefV8Context::GetCurrentContext() });
                }
                else {
                    pmlog_error("registerSignalHandler called with incorrect parameter signature");
                    exception = "registerSignalHandler called with incorrect parameter signature";
                }
                return true;
            }

            // async endpoint entry function
            if (name == "invokeEndpoint") {
                if (arguments.size() == 4 && arguments[0]->IsString() && arguments[1]->IsObject() &&
                    arguments[2]->IsFunction() && arguments[3]->IsFunction())
                {
                    const auto& key = arguments[0]->GetStringValue();
                    if (pKernelWrapper->pInvocationManager->HasHandler(key)) {
                        pKernelWrapper->pInvocationManager->DispatchInvocation(key,
                            { arguments[2], arguments[3], CefV8Context::GetCurrentContext() },
                            arguments[1]
                        );
                    }
                    else {
                        pKernelWrapper->asyncEndpoints.DispatchInvocation(key,
                            { arguments[2], arguments[3], CefV8Context::GetCurrentContext() },
                            arguments[1],
                            *pBrowser,
                            *this
                        );
                    }
                }
                else
                {
                    pmlog_error("invokeEndpoint called with incorrect parameter signature");
                    exception = "invokeEndpoint called with incorrect parameter signature";
                }
                return true;
            }
        }
        return false;
    }

    void DataBindAccessor::ResolveAsyncEndpoint(uint64_t uid, bool success, CefRefPtr<CefValue> pArgs)
    {
        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper)
        {
            pKernelWrapper->asyncEndpoints.ResolveInvocation(uid, success, std::move(pArgs));
        }
    }

    void DataBindAccessor::ClearKernelWrapper()
    {
        std::lock_guard lk{ kernelMtx };
        pKernelWrapper = nullptr;
    }
}