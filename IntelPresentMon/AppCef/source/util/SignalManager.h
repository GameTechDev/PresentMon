// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/WinAPI.h>
#include <include/cef_v8.h>
#include <string>
#include <unordered_map>


#define KEY_LIST \
	X_(targetLost) \
	X_(hotkeyFired) \
	X_(presentmonInitFailed) \
	X_(overlayDied) \
	X_(stalePid)


namespace p2c::client::util
{
	class SignalManager
	{
	public:
		// lookups
#define X_(key) static constexpr std::string key() {return #key;}
		struct Keys
		{
			KEY_LIST
		};
#undef X_
#define X_(key) #key,
		inline static const std::string keyList[] = {
			KEY_LIST
		};
#undef X_
		// types
		struct CallbackContext
		{
			CefRefPtr<CefV8Value> pV8Function;
			CefRefPtr<CefV8Context> pV8Context;
		};
		// functions
		void RegisterCallback(const std::string& key, CallbackContext ctx);
		// -- Signal-firing (call from C++ to invoke a callback registered by JS side)
		void SignalTargetLost(uint32_t pid); // targetLost(pid:number)
		void SignalHotkeyFired(uint32_t action); // hotkeyFired(action:number)
		void SignalPresentmonInitFailed(); // presentmonInitFailed()
		void SignalOverlayDied(); // overlayDied()
		void SignalStalePid(); // stalePid()
	private:
		// functions
		template<typename... Ts>
		void ImplementSignal_(const std::string& key, Ts&&... args);
		// data
		std::unordered_map<std::string, CallbackContext> callbacks;
	};
}

#undef KEY_LIST