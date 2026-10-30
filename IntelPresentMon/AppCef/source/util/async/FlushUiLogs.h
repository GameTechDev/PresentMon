// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../AsyncEndpointManager.h"
#include "../CefValues.h"
#include <CommonUtilities/log/Log.h>
#include <chrono>

namespace p2c::client::util::async
{
	class FlushUiLogs : public AsyncEndpoint
	{
	public:
		static constexpr std::string GetKey() { return "flushUiLogs"; }
		FlushUiLogs() : AsyncEndpoint{ AsyncEndpoint::Environment::BrowserProcess } {}

		void BeforeBrowserDispatch(CefRefPtr<CefValue>) const override
		{
			FlushBounded_();
		}

		void ExecuteOnBrowser(uint64_t uid, CefRefPtr<CefValue>, CefRefPtr<CefBrowser> pBrowser) const override
		{
			FlushBounded_();

			auto msg = CefProcessMessage::Create(AsyncEndpointManager::GetResolveMessageName());
			const auto args = msg->GetArgumentList();
			args->SetInt(0, (int)uid);
			args->SetBool(1, true);
			args->SetValue(2, CefValueNull());
			pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(msg));
		}

	private:
		// called on the renderer and browser UI threads: bound the wait so a stalled
		// log pipe to the kernel cannot freeze the UI
		static void FlushBounded_() noexcept
		{
			if (auto pChannel = ::pmon::util::log::GetDefaultChannel()) {
				if (!pChannel->TryFlushFor(std::chrono::milliseconds{ 1000 })) {
					pmlog_warn("Timed out flushing UI log channel");
				}
			}
		}
	};
}
