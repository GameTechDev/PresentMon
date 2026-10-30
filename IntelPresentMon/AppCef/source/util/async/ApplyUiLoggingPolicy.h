// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../AsyncEndpointManager.h"
#include "../CefValues.h"
#include <CommonUtilities/log/GlobalPolicy.h>
#include <CommonUtilities/log/Level.h>

namespace p2c::client::util::async
{
	class ApplyUiLoggingPolicy : public AsyncEndpoint
	{
	public:
		static constexpr std::string GetKey() { return "applyUiLoggingPolicy"; }
		ApplyUiLoggingPolicy() : AsyncEndpoint{ AsyncEndpoint::Environment::BrowserProcess } {}

		void BeforeBrowserDispatch(CefRefPtr<CefValue> pArgObj) const override
		{
			ApplyPolicy_(pArgObj);
		}

		void ExecuteOnBrowser(uint64_t uid, CefRefPtr<CefValue> pArgObj, CefRefPtr<CefBrowser> pBrowser) const override
		{
			ApplyPolicy_(pArgObj);
			auto msg = CefProcessMessage::Create(AsyncEndpointManager::GetResolveMessageName());
			const auto args = msg->GetArgumentList();
			args->SetInt(0, (int)uid);
			args->SetBool(1, true);
			args->SetValue(2, CefValueNull());
			pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(msg));
		}

	private:
		static void ApplyPolicy_(CefRefPtr<CefValue> pArgObj)
		{
			using Level = ::pmon::util::log::Level;
			const auto dict = pArgObj->GetDictionary();
			auto& policy = ::pmon::util::log::GlobalPolicy::Get();
			policy.SetLogLevel((Level)dict->GetInt("logLevel"));
			policy.SetTraceLevel((Level)dict->GetInt("traceLevel"));
			policy.StoreVerboseModules((uint64_t)dict->GetInt("verboseModules"));
			const bool exceptionTrace = dict->GetBool("exceptionTrace");
			policy.SetExceptionTrace(exceptionTrace);
			policy.SetSehTracing(exceptionTrace);
		}
	};
}
