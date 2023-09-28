// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "DataBindAccessor.h"
#include <Core/source/kernel/Kernel.h>
#include <Core/source/infra/log/Logging.h>
#include <include/cef_task.h> 
#include <include/base/cef_callback.h> 
#include <include/wrapper/cef_closure_task.h> 
#include "util/CefValues.h"


namespace p2c::client::cef
{
    class DBAKernelHandler : public kern::KernelHandler
    {
    public:
        DBAKernelHandler(util::SignalManager* pSignals_) : pSignals{ pSignals_ } {}
        void OnTargetLost(uint32_t pid_) override
        {
            CefPostTask(TID_RENDERER, base::BindOnce(&DBAKernelHandler::TargetLostTask_, base::Unretained(this), pid_));
        }
        void OnOverlayDied() override
        {
            CefPostTask(TID_RENDERER, base::BindOnce(&DBAKernelHandler::OverlayDiedTask_, base::Unretained(this)));
        }
        void OnPresentmonInitFailed() override
        {
            CefPostTask(TID_RENDERER, base::BindOnce(&DBAKernelHandler::PresentmonInitFailedTask_, base::Unretained(this)));
        }
        void OnStalePidSelected() override
        {
            CefPostTask(TID_RENDERER, base::BindOnce(&DBAKernelHandler::StalePidTask_, base::Unretained(this)));
        }
    private:
        // functions needed for CefPostTask (which cannot handle lambdas)
        void TargetLostTask_(uint32_t pid_)
        {
            pSignals->SignalTargetLost(pid_);
        }
        void OverlayDiedTask_()
        {
            pSignals->SignalOverlayDied();
        }
        void PresentmonInitFailedTask_()
        {
            pSignals->SignalPresentmonInitFailed();
        }
        void StalePidTask_()
        {
            pSignals->SignalStalePid();
        }
        // data
        util::SignalManager* pSignals;
    };

    DataBindAccessor::DataBindAccessor(CefRefPtr<CefBrowser> pBrowser, util::KernelWrapper* pKernelWrapper_)
        :
        pBrowser{ std::move(pBrowser) },
        pKernelWrapper{ pKernelWrapper_ }
    {
        pKernelWrapper->pKernelHandler = std::make_unique<DBAKernelHandler>(&pKernelWrapper_->signals);
        pKernelWrapper->pHotkeys = std::make_unique<util::Hotkeys>();
        pKernelWrapper->pHotkeys->SetHandler([pWrap = pKernelWrapper](Action act) { pWrap->signals.SignalHotkeyFired((uint32_t)act); });
    }

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
                    p2clog.warn(L"registerSignalHandler called with incorrect parameter signature").commit();
                    exception = "registerSignalHandler called with incorrect parameter signature";
                }
                return true;
            }

            // only allow launchKernel async endpoint if we have not yet launched the kernel
            if (!pKernelWrapper->pKernel && arguments[0]->GetStringValue() != "launchKernel")
            {
                p2clog.warn(std::format(L"js endpoint called without kernel: {}", std::wstring(name))).commit();
                exception = "core kernel not launched";
                return true;
            }

            // async endpoint entry function
            if (name == "invokeEndpoint") {
                if (arguments.size() == 4 && arguments[0]->IsString() && arguments[1]->IsObject() &&
                    arguments[2]->IsFunction() && arguments[3]->IsFunction())
                {
                    pKernelWrapper->asyncEndpoints.DispatchInvocation(
                        arguments[0]->GetStringValue(),
                        { arguments[2], arguments[3], CefV8Context::GetCurrentContext() },
                        arguments[1],
                        *pBrowser,
                        *this,
                        * pKernelWrapper->pKernel
                    );
                }
                else
                {
                    p2clog.warn(L"invokeEndpoint called with incorrect parameter signature").commit();
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

    void DataBindAccessor::BindHotkey(CefValue& pArgObj, std::function<void(bool)> resultCallback)
    {
        // {action:int, combination: {modifiers:[], key:int}}

        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper)
        {
            const auto payload = pArgObj.GetDictionary();
            const auto comboJs = payload->GetDictionary("combination");
            const auto modsJs = comboJs->GetList("modifiers");

            auto mods = win::Mod::Null;
            for (int i = 0; i < modsJs->GetSize(); i++)
            {
                const auto modCode = modsJs->GetValue(i)->GetInt();
                mods = mods | *win::ModSet::SingleModFromCode(modCode);
            }

            pKernelWrapper->pHotkeys->BindAction(
                (Action)payload->GetInt("action"),
                win::Key{ (win::Key::Code)comboJs->GetInt("key") },
                mods,
                std::move(resultCallback)
            );
        }
    }

    void DataBindAccessor::ClearHotkey(CefValue& pArgObj, std::function<void(bool)> resultCallback)
    {
        // {action:int}

        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper)
        {
            pKernelWrapper->pHotkeys->ClearAction(
                (Action)pArgObj.GetDictionary()->GetInt("action"),
                std::move(resultCallback)
            );
        }
    }

    void DataBindAccessor::LaunchKernel()
    {
        std::shared_lock lk{ kernelMtx };
        if (pKernelWrapper)
        {
            if (pKernelWrapper->pKernel)
            {
                p2clog.warn(L"launchKernel called but kernel already exists").commit();
                return;
            }

            pKernelWrapper->pKernel = std::make_unique<kern::Kernel>(pKernelWrapper->pKernelHandler.get());
        }
    }

    void DataBindAccessor::ClearKernelWrapper()
    {
        std::lock_guard lk{ kernelMtx };
        pKernelWrapper = nullptr;
    }
}