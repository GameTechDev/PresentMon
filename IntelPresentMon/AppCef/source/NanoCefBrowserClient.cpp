// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NanoCefBrowserClient.h"
#include <include/wrapper/cef_helpers.h>
#include "util/Logging.h"
#include <format>
#include <fstream>
#include <streambuf>
#include "util/AsyncEndpointManager.h"
#include "NanoCefProcessHandler.h"
#include "util/KernelRelay.h"

// implemented in winmain.cpp
void AppQuitMessageLoop();

using namespace pmon::util;

namespace p2c::client::cef
{
    NanoCefBrowserClient::NanoCefBrowserClient()
    {
        class NullMenuHandler : public CefContextMenuHandler {
        public:
            NullMenuHandler() = default;
            void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefContextMenuParams> params,
                CefRefPtr<CefMenuModel> model) override {
                // Clear the context menu to disable the default right-click behavior.
                model->Clear();
            }

            IMPLEMENT_REFCOUNTING(NullMenuHandler);
            DISALLOW_COPY_AND_ASSIGN(NullMenuHandler);
        };
        pContextMenuHandler = new NullMenuHandler{};
    }

    void NanoCefBrowserClient::SetKernelCallRelay(util::kernel_relay::CallRelay* pRelay)
    {
        pKernelCallRelay = pRelay;
    }

    CefRefPtr<CefBrowser> NanoCefBrowserClient::GetBrowser()
    {
        return pBrowser;
    }

    CefRefPtr<CefLifeSpanHandler> NanoCefBrowserClient::GetLifeSpanHandler()
    {
        return this;
    }

    CefRefPtr<CefDisplayHandler> NanoCefBrowserClient::GetDisplayHandler()
    {
        return this;
    }

    void NanoCefBrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser_)
    {
        pBrowser = browser_;

        CefLifeSpanHandler::OnAfterCreated(std::move(browser_));
    }

    void NanoCefBrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser_)
    {
        CefLifeSpanHandler::OnBeforeClose(browser_);
        pBrowser.reset();
        AppQuitMessageLoop();
    }

    bool NanoCefBrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
    {
        if (pKernelCallRelay && pKernelCallRelay->HandleMessage(*message, frame)) {
            return true;
        }
        if (message->GetName() == util::AsyncEndpointManager::GetDispatchMessageName())
        {
            if (auto pEndpoint = endpoints.Find(message->GetArgumentList()->GetString(0)))
            {
                pEndpoint->ExecuteOnBrowser(message->GetArgumentList()->GetInt(1), message->GetArgumentList()->GetValue(2), browser);
            }
            else
            {
                pmlog_warn(std::format("Endpoint async not found: {}", message->GetArgumentList()->GetString(0).ToString()));
            }
            return true;
        }
        return false;
    }

    CefRefPtr<CefContextMenuHandler> NanoCefBrowserClient::GetContextMenuHandler()
    {
        return pContextMenuHandler;
    }

#define xjs_pmlog_(lvl) ((PMLOG_BUILD_LEVEL_ < lvl) || (::pmon::util::log::GlobalPolicy::Get().GetLogLevel() < lvl)) \
	? (void)0 : (void)::pmon::util::log::EntryBuilder{ lvl, source.ToString(), {}, line } \
	.to(::pmon::util::log::GetDefaultChannel()).no_trace().note(message.ToString())

    bool NanoCefBrowserClient::OnConsoleMessage(
        CefRefPtr<CefBrowser> browser,
        cef_log_severity_t level,
        const CefString& message,
        const CefString& source,
        int line)
    {
        switch (level) {
        case cef_log_severity_t::LOGSEVERITY_FATAL:
            xjs_pmlog_(log::Level::Fatal); break;
        case cef_log_severity_t::LOGSEVERITY_ERROR:
            xjs_pmlog_(log::Level::Error); break;
        case cef_log_severity_t::LOGSEVERITY_WARNING:
            xjs_pmlog_(log::Level::Warning); break;
        case cef_log_severity_t::LOGSEVERITY_DEFAULT:
        case cef_log_severity_t::LOGSEVERITY_INFO:
            xjs_pmlog_(log::Level::Info); break;
        case cef_log_severity_t::LOGSEVERITY_DEBUG:
            xjs_pmlog_(log::Level::Debug); break;
        }
        return true;
    }
}