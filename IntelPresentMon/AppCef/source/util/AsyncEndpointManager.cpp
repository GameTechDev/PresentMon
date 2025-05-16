// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AsyncEndpointManager.h"
#include "Logging.h"
#include <algorithm>
#include "CefValues.h"
#include <include/cef_task.h>
#include <include/base/cef_callback.h>
#include <include/wrapper/cef_closure_task.h>
#include <include/wrapper/cef_helpers.h>
#include "CefValues.h"
#include <CommonUtilities\str\String.h>


namespace p2c::client::util
{
	using ::pmon::util::str::ToWide;

	void AsyncEndpointManager::DispatchInvocation(const std::string& key, CallbackContext ctx, CefRefPtr<CefV8Value> pObj, CefBrowser& browser, cef::DataBindAccessor& accessor)
	{
		CEF_REQUIRE_RENDERER_THREAD();

		// execute endpoint
		if (auto pEndpoint = endpoints.Find(key)) {
			std::unique_lock lck{ mtx };
			const auto uid = nextUid++;
			auto&&[iInvoke, inserted] = invocations.emplace(std::piecewise_construct,
				std::forward_as_tuple(uid),
				std::forward_as_tuple(std::move(ctx), pEndpoint)
			);
			lck.unlock();

			if constexpr (v::v8async) {
				pmlog_verb(v::v8async)(std::format("JS async call {{{}}} from V8 to endpoint [{}] with payload:\n{}",
					uid, key, Traverse(V8ToCefValue(*pObj)).Dump()));
			}
			else {
				pmlog_dbg(std::format("JS async call {{{}}} from V8 to endpoint [{}]", uid, key));
			}

			switch (pEndpoint->GetEnvironment())
			{
			case AsyncEndpoint::Environment::BrowserProcess:
				{
					auto msg = CefProcessMessage::Create(GetDispatchMessageName());
					msg->GetArgumentList()->SetString(0, key);
					msg->GetArgumentList()->SetInt(1, (int)uid);
					msg->GetArgumentList()->SetDictionary(2, V8ToCefValue(*pObj)->GetDictionary());
					browser.GetMainFrame()->SendProcessMessage(PID_BROWSER, std::move(msg));
					// resolve is invoked in the handler for the return message from browser process
				}
				break;
			case AsyncEndpoint::Environment::RenderProcess:
				// resolve is invoked in ExecuteOnRenderAccessor
				iInvoke->second.taskFuture = std::async([&accessor, key, uid, pArgObj = V8ToCefValue(*pObj), pEndpoint, this]() mutable {
					AsyncEndpoint::Result result;
					try {
						// run the async command procudure on the std::async thread pool
						result = pEndpoint->ExecuteOnRenderer(uid, std::move(pArgObj), accessor);
					}
					catch (...) {
						const auto rep =
							ReportException("Error in async API endpoint dispatch (render process)");
						result = AsyncEndpoint::MakeStringErrorResult(ToWide(rep.first));
					}
					// pass results and run the resolve logic (V8 conversion etc.) on the renderer thread
					CefPostTask(TID_RENDERER, base::BindOnce(
						&AsyncEndpointManager::ResolveInvocation, base::Unretained(this),
						uid, result.succeeded, std::move(result.pArgs)
					));
				});
				break;
			default:
				pmlog_error("Unknown async endp env").pmwatch(int(pEndpoint->GetEnvironment()));
			}
		}
		else {
			pmlog_error(std::format("Key [{}] does not have an async endpoint registered", key));
		}
	}

	void AsyncEndpointManager::ResolveInvocation(uint64_t uid, bool success, CefRefPtr<CefValue> pArgs)
	{
		CEF_REQUIRE_RENDERER_THREAD();

		std::unique_lock lck{ mtx };
		if (auto i = invocations.find(uid); i != std::end(invocations))
		{
			auto invocation = std::move(i->second);

			if constexpr (v::v8async) {
				pmlog_verb(v::v8async)(std::format("Async call {{{}}} resolved with payload: \n{}",
					uid, Traverse(pArgs).Dump()));
			}
			else {
				pmlog_verb(v::v8async)(std::format("Async call {{{}}} resolved", uid));
			}

			invocations.erase(i);
			lck.unlock();

			auto& ctx = invocation.context;
			ctx.pV8Context->Enter();

			auto pArgsV8 = util::CefToV8Value(*pArgs);
			if (success)
			{
				ctx.pV8ResolveFunction->ExecuteFunction(nullptr, { std::move(pArgsV8) });
			}
			else
			{
				ctx.pV8RejectFunction->ExecuteFunction(nullptr, { std::move(pArgsV8) });
			}

			ctx.pV8Context->Exit();
		}
		else
		{
			lck.unlock();
			pmlog_error("Failed to resolve async invocation (not registered).").no_trace();
		}
	}
}