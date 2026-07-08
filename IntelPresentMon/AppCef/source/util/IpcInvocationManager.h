// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_v8.h>
#include <include/wrapper/cef_helpers.h>
#include <include/cef_task.h>
#include <include/base/cef_callback.h>
#include <include/wrapper/cef_closure_task.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <future>
#include <unordered_map>
#include "AsyncEndpointCollection.h"
#include "ActionClientServer.h"
#include "V8Transfer.h"
#include "../KernelProcess/kact/PushSpecification.h"


namespace p2c::client::util
{
	// this class exists to:
	// A) bridge from key string to action
	// B) encode conversion from V8 to Params{}
	// C) encode conversion from Response{} to V8
	class IpcInvocationManager
	{
	public:
		// types
		struct CallbackContext
		{
			CefRefPtr<CefV8Value> pV8ResolveFunction;
			CefRefPtr<CefV8Value> pV8RejectFunction;
			CefRefPtr<CefV8Context> pV8Context;
		};
		// functions
		void DispatchInvocation(const std::string& key, CallbackContext&& ctx, CefRefPtr<CefV8Value> pArgs)
		{
			// TODO: explicit lookup and handle missing error
			dispatchBindings_[key](std::move(ctx), std::move(pArgs), client_);
		}
		bool HasHandler(const std::string& key) const
		{
			return dispatchBindings_.contains(key);
		}
		template<class A>
		static void RegisterDispatchBinding()
		{
			using Params = typename A::Params;
			using Response = typename A::Response;
			// register handler function for the endpoint identifer (e.g. SetCapture) that takes in V8 context and V8 action args
			// this handler will be called when the DBA receives an Execute call on the invokeEndpoint channel
			dispatchBindings_[A::Identifier] = [](CallbackContext ctx, CefRefPtr<CefV8Value> pObj, CefClient& client) {
				// convert the V8 args to the Params struct associated with the action A
				// create a lambda which accepts the incoming result of this action and stores the V8 context
				// lamba will be invoked after the response is received from the server, causing the response
				// to be returned to the original V8 caller in js land
				client.DispatchWithContinuation(FromV8<Params>(*pObj), [ctx = std::move(ctx)](Response&& res, std::exception_ptr pex) {
					CefPostTask(TID_RENDERER, base::BindOnce(
						&IpcInvocationManager::ResolveInvocation_<Response>,
						std::move(ctx), std::move(res), std::move(pex)
					));
				});
			};
		}
		IpcInvocationManager(CefClient& client) : client_{ client } {}
	private:
		// function
		template<class Response>
		static void ResolveInvocation_(CallbackContext ctx, Response res, std::exception_ptr pex)
		{
			CEF_REQUIRE_RENDERER_THREAD();

			// TODO: give error details in the failure path
			ctx.pV8Context->Enter();
			if (!pex) {
				ctx.pV8ResolveFunction->ExecuteFunction(nullptr, { ToV8(res) });
			}
			else {
				ctx.pV8RejectFunction->ExecuteFunction(nullptr, { ToV8(res) });
			}
			ctx.pV8Context->Exit();
		}
		// data
		static std::unordered_map<std::string, std::function<void(CallbackContext, CefRefPtr<CefV8Value>, CefClient&)>> dispatchBindings_;
		CefClient& client_;
	};

	template<>
	struct CustomV8Conversion<kproc::kact::push_spec_impl::Widget>
	{
		static void FromV8(CefV8Value& v8, kproc::kact::push_spec_impl::Widget& out)
		{
			kern::WidgetType type;
			::p2c::client::util::FromV8(*v8.GetValue("widgetType"), type);
			if (type == kern::WidgetType::Graph) {
				using Xfer = kproc::kact::push_spec_impl::Graph;
				out.emplace<Xfer>();
				::p2c::client::util::FromV8(v8, std::get<Xfer>(out));
			}
			else if (type == kern::WidgetType::Readout) {
				using Xfer = kproc::kact::push_spec_impl::Readout;
				out.emplace<Xfer>();
				::p2c::client::util::FromV8(v8, std::get<Xfer>(out));
			}
			else {
				pmlog_error("Unknown widget type").pmwatch(int(type));
			}
		}
	};

	template<>
	struct CustomV8Conversion<gfx::Color>
	{
		static void FromV8(CefV8Value& v8, gfx::Color& out)
		{
			out.r = float(v8.GetValue("r")->GetIntValue()) / 255.f;
			out.g = float(v8.GetValue("g")->GetIntValue()) / 255.f;
			out.b = float(v8.GetValue("b")->GetIntValue()) / 255.f;
			out.a = float(v8.GetValue("a")->GetDoubleValue());
		}
	};
}

#undef KEY_LIST