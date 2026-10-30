// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_v8.h>
#include <string>
#include <functional>

namespace p2c::client::cef
{
	class DataBindAccessor;
}

namespace p2c::kern
{
	class Kernel;
}

namespace p2c::client::util
{
	class AsyncEndpointManager;

	class AsyncEndpoint
	{
	public:
		// types
		enum class Environment
		{
			BrowserProcess,
			RenderProcess,
		};
		struct Result
		{
			bool succeeded = false;
			CefRefPtr<CefValue> pArgs;
		};
		// functions
		AsyncEndpoint(Environment env);
		virtual void ExecuteOnBrowser(uint64_t uid, CefRefPtr<CefValue> pArgObj, CefRefPtr<CefBrowser> pBrowser) const;
		virtual Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor& accessor) const;
		virtual void BeforeBrowserDispatch(CefRefPtr<CefValue> pArgObj) const;
		Environment GetEnvironment() const;
		static Result MakeStringErrorResult(std::wstring errorString);
	private:
		// data
		Environment env;
	};
}