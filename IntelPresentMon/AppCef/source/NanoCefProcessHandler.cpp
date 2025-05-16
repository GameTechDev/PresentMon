// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NanoCefBrowserClient.h"
#include <include/wrapper/cef_helpers.h>
#include <include/wrapper/cef_closure_task.h>
#include <include/cef_parser.h>
#include <include/cef_task.h>
#include <include/base/cef_callback.h>
#include "NanoCefProcessHandler.h"
#include "SchemeHandlerFactory.h"
#include "DataBindAccessor.h"
#include "util/Logging.h"
#include "util/LogSetup.h"
#include "util/AsyncEndpointManager.h"
#include "util/CefValues.h"
#include "util/CliOptions.h"

using namespace pmon::util;
using namespace std::chrono_literals;

namespace p2c::client::cef
{
    void NanoCefProcessHandler::OnContextInitialized()
    {
#ifdef NDEBUG
        constexpr bool is_debug = false;
#else
        constexpr bool is_debug = true;
#endif
        const auto& opt = util::cli::Options::Get();
        std::string host;
        std::string port;

        // parse cli-passed url
        if (opt.url) {
            CefURLParts url_parts;
            if (CefParseURL(*opt.url, url_parts)) {
                host = CefString(&url_parts.host);
                port = CefString(&url_parts.port);
            }
            else {
                pmlog_warn("Bad cli-passed url");
            }
        }
        // use url parts to determine the scheme mode
        using Mode = SchemeHandlerFactory::SchemeMode;
        Mode mode;
        if (host.empty()) {
            mode = Mode::File;
        }
        else if (host == "localhost") {
            mode = Mode::Local;
        }
        else {
            mode = Mode::Web;
        }

        const auto hardFail = is_debug && !opt.noNetFail;

        const auto webRootOverride = *opt.webRoot;

        CefRegisterSchemeHandlerFactory("https", "", new SchemeHandlerFactory{ mode, hardFail, host, port, webRootOverride });
        CefRegisterSchemeHandlerFactory("http", "", new SchemeHandlerFactory{ mode, hardFail, host, port, webRootOverride });
    }

    CefRefPtr<CefBrowserProcessHandler> NanoCefProcessHandler::GetBrowserProcessHandler()
    {
        return this;
    }

    CefRefPtr<CefRenderProcessHandler> NanoCefProcessHandler::GetRenderProcessHandler()
    {
        return this;
    }

    void NanoCefProcessHandler::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> pChildCommandLine)
    {
        auto& opt = util::cli::Options::Get();
        // propagate custom cli switches to children
        auto pCmdLine = CefCommandLine::GetGlobalCommandLine();
        for (auto&&[name, val] : opt.GetForwardedOptions()) {
            if (val.empty()) {
                pChildCommandLine->AppendSwitch(std::move(name));
            }
            else {
                pChildCommandLine->AppendSwitchWithValue(std::move(name), std::move(val));
            }
        }
        // initiate logging ipc connection for renderer children
        if (pChildCommandLine->GetSwitchValue("type") == "renderer") {
            // inject logging ipc pipe cli option to child
            static int count = 0;
            std::string pipePrefix = std::format("pm-uir-log-{}-{}", GetCurrentProcessId(), ++count);
            pChildCommandLine->AppendSwitchWithValue(opt.logPipeName.GetName(), pipePrefix);
            // launch connector thread
            util::ConnectToLoggingSourcePipe(pipePrefix);
        }
    }

    void NanoCefProcessHandler::AddFunctionToObject_(CefString name, CefRefPtr<CefV8Value>& pObj, CefRefPtr<DataBindAccessor>& pAccessor)
    {
        pObj->SetValue(name, CefV8Value::CreateFunction(name, pAccessor), V8_PROPERTY_ATTRIBUTE_NONE);
    }

    void NanoCefProcessHandler::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
    {
        std::this_thread::sleep_for(50ms);
        pKernelWrapper->pClient = std::make_unique<util::CefClient>(
            *util::cli::Options::Get().actName, util::cact::CefExecutionContext{.pSignalManager = &pKernelWrapper->signals}
        );
        pKernelWrapper->pInvocationManager = std::make_unique<util::IpcInvocationManager>(*pKernelWrapper->pClient);
        pAccessor = new DataBindAccessor{ pBrowser, pKernelWrapper.get() };

        auto core = CefV8Value::CreateObject(nullptr, nullptr);
        AddFunctionToObject_("invokeEndpoint", core, pAccessor);
        AddFunctionToObject_("registerSignalHandler", core, pAccessor);
        context->GetGlobal()->SetValue("core", std::move(core), V8_PROPERTY_ATTRIBUTE_NONE);
    }

    void NanoCefProcessHandler::OnBrowserCreated(CefRefPtr<CefBrowser> browser_, CefRefPtr<CefDictionaryValue> extra_info)
    {
        log::IdentificationTable::AddThisThread("cef-proc");

        pBrowser = browser_;

        CefRenderProcessHandler::OnBrowserCreated(std::move(browser_), std::move(extra_info));
    }

    bool NanoCefProcessHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
    {
        if (message->GetName() == util::AsyncEndpointManager::GetResolveMessageName())
        {
            pAccessor->ResolveAsyncEndpoint(
                message->GetArgumentList()->GetInt(0),
                message->GetArgumentList()->GetBool(1),
                message->GetArgumentList()->GetValue(2)
            );
            return true;
        }
        return false;
    }
}