// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_v8.h>
#include <include/wrapper/cef_helpers.h>
#include <string>
#include <unordered_map>
#include <functional>
#include "AsyncEndpointCollection.h"
#include "KernelRelay.h"
#include "V8Transfer.h"
#include <KernelProcess/kact/PushSpecification.h>


namespace p2c::client::util
{
	// Renderer: the page's calls to kernel actions. This class exists to:
	// A) bridge from key string to action
	// B) encode conversion from V8 to Params{}
	// C) encode conversion from Response{} to V8
	// The browser process owns the connection to the kernel, so each call is sent there as a
	// kernel_relay::kCallMessage and resolved when its kReplyMessage comes back. Everything
	// here runs on the renderer thread, so nothing is locked.
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
			dispatchBindings_[key](*this, std::move(ctx), std::move(pArgs));
		}
		bool HasHandler(const std::string& key) const
		{
			return dispatchBindings_.contains(key);
		}
		// Handles a kernel_relay::kReplyMessage. False for any other message.
		bool HandleReply(CefProcessMessage& message);
		template<class A>
		static void RegisterDispatchBinding()
		{
			using Params = typename A::Params;
			using Response = typename A::Response;
			// register handler function for the endpoint identifer (e.g. SetCapture) that takes in V8 context and V8 action args
			// this handler will be called when the DBA receives an Execute call on the invokeEndpoint channel
			dispatchBindings_[A::Identifier] = [](IpcInvocationManager& manager, CallbackContext ctx, CefRefPtr<CefV8Value> pObj) {
				// the call goes out from the frame that made it, so the reply comes back to it
				auto frame = ctx.pV8Context->GetFrame();
				manager.SendCall_(A::Identifier, kernel_relay::ToBinary(FromV8<Params>(*pObj)), std::move(frame),
					[ctx = std::move(ctx)](bool succeeded, CefRefPtr<CefBinaryValue> pResponse) {
						// a failed call reaches the page as a rejection
						if (succeeded) {
							try {
								ResolveInvocation_(ctx, kernel_relay::FromBinary<Response>(pResponse.get()), true);
								return;
							}
							catch (...) {
								pmlog_error(::pmon::util::ReportException("Malformed kernel call reply"));
							}
						}
						ResolveInvocation_(ctx, Response{}, false);
					});
			};
		}
	private:
		// types
		// succeeded, and the Response payload (null when it serialized to no bytes)
		using ReplyHandler = std::function<void(bool, CefRefPtr<CefBinaryValue>)>;
		// functions
		void SendCall_(const std::string& action, CefRefPtr<CefBinaryValue> pParams, CefRefPtr<CefFrame> frame,
			ReplyHandler onReply);
		template<class Response>
		static void ResolveInvocation_(const CallbackContext& ctx, const Response& res, bool succeeded)
		{
			CEF_REQUIRE_RENDERER_THREAD();

			// TODO: give error details in the failure path
			ctx.pV8Context->Enter();
			if (succeeded) {
				ctx.pV8ResolveFunction->ExecuteFunction(nullptr, { ToV8(res) });
			}
			else {
				ctx.pV8RejectFunction->ExecuteFunction(nullptr, { ToV8(res) });
			}
			ctx.pV8Context->Exit();
		}
		// data
		static std::unordered_map<std::string, std::function<void(IpcInvocationManager&, CallbackContext, CefRefPtr<CefV8Value>)>> dispatchBindings_;
		// unique across the renderer process, so a reply meant for a page that has since
		// reloaded cannot match a call made by the new one
		static int nextCallId_;
		std::unordered_map<int, ReplyHandler> pendingCalls_;
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