// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AsyncEndpoint.h"
#include "Logging.h"

namespace p2c::client::util
{
	AsyncEndpoint::AsyncEndpoint(Environment env) : env{ env } {}

	void AsyncEndpoint::ExecuteOnBrowser(uint64_t uid, CefRefPtr<CefValue> pArgObj, CefRefPtr<CefBrowser> pBrowser) const
	{
		pmlog_warn("virtual not implemented");
	}

	AsyncEndpoint::Result AsyncEndpoint::ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor& accessor) const
	{
		pmlog_warn("virtual not implemented");
		return MakeStringErrorResult(L"virtual not implemented");
	}

	void AsyncEndpoint::BeforeBrowserDispatch(CefRefPtr<CefValue>) const
	{}

	AsyncEndpoint::Environment AsyncEndpoint::GetEnvironment() const { return env; }

	AsyncEndpoint::Result AsyncEndpoint::MakeStringErrorResult(std::wstring errorString)
	{
		auto str = CefValue::Create();
		str->SetString(errorString);
		return { false, std::move(str) };
	}
}