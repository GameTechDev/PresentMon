// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <cstdint>
#include <map>
#include <thread>
#include <semaphore>
#include <functional>
#include <mutex>
#include <bitset>
#include <Core/source/win/Key.h>
#include <Core/source/win/ModSet.h>
#include <CommonUtilities/mt/Thread.h>

namespace p2c::win
{
	class Hotkeys
	{
	public:
		Hotkeys();
		Hotkeys(const Hotkeys&) = delete;
		Hotkeys& operator=(const Hotkeys&) = delete;
		~Hotkeys();
		bool BindAction(int action, win::Key key, win::ModSet mods);
		bool ClearAction(int action);
		void SetHandler(std::function<void(int)> handler);
	private:
		// types
		struct Hotkey
		{
		public:
			Hotkey(win::Key key, win::ModSet mods);
			bool operator<(const Hotkey& rhs) const;
			bool operator==(const Hotkey& rhs) const;
		private:
			win::Key key;
			win::ModSet mods;
		};
		// functions
		void Kernel_() noexcept;
		void DispatchHotkey_(int action) const;
		win::ModSet GatherModifiers_() const;
		// data
		std::binary_semaphore startupSemaphore_{ 0 };
		::pmon::util::mt::Thread thread_;
		unsigned long threadId_{};
		void* messageWindowHandle_ = nullptr;
		std::bitset<win::Key::virtualKeyTableSize> pressedKeys_;
		// control access to concurrent memory for key map / handler
		mutable std::mutex mtx_;
		std::function<void(int)> Handler_;
		std::map<Hotkey, int> registeredHotkeys_;
	};
}