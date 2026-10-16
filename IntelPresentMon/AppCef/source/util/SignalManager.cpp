// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "SignalManager.h"
#include "CefValues.h"
#include "Logging.h"
#include <CommonUtilities/str/String.h>
#include <algorithm>
#include <include/wrapper/cef_helpers.h>


namespace p2c::client::util
{
	void SignalManager::RegisterCallback(const std::string& key, CallbackContext ctx)
	{
		if (auto i = std::find(std::begin(keyList), std::end(keyList), key); i == std::end(keyList))
		{
			pmlog_warn(std::format("Key [{}] not a valid signal key", key));
		}
		callbacks[key] = std::move(ctx);
	}

	template<typename ...Ts>
	void SignalManager::ImplementSignal_(const std::string& key, Ts&& ...args)
	{
		CEF_REQUIRE_RENDERER_THREAD();

		if (auto i = callbacks.find(key); i != callbacks.end())
		{
			auto&& ctx = i->second;
			ctx.pV8Function->ExecuteFunctionWithContext(ctx.pV8Context, nullptr, BuildV8ArgumentList(args...));
		}
		else
		{
			pmlog_warn(std::format("Signal fired for [{}] with no handler set", key));
		}
	}

	void SignalManager::SignalFromBrowser(const std::string& key, std::optional<uint32_t> arg)
	{
		if (std::find(std::begin(keyList), std::end(keyList), key) == std::end(keyList)) {
			pmlog_warn(std::format("Kernel event [{}] is not a valid signal key", key));
			return;
		}
		if (arg) {
			ImplementSignal_(key, *arg);
		}
		else {
			ImplementSignal_(key);
		}
	}
}