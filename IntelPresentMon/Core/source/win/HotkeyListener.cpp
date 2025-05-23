// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "HotkeyListener.h"
#include <Core/source/infra/Logging.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/ref/WrapReflect.h>
#include <ranges>

namespace rn = std::ranges;
namespace vi = rn::views;
using namespace pmon::util;

// TODO: general logging in this codebase (winapi calls like PostThreadMessage etc.)

namespace p2c::win
{
	Hotkeys::Hotkeys()
		:
		thread_{ "hotkey", &Hotkeys::Kernel_, this }
	{
		startupSemaphore_.acquire();
		pmlog_verb(v::hotkey2)("Hotkey process ctor complete");
	}
	Hotkeys::~Hotkeys()
	{
		pmlog_verb(v::hotkey2)("Destroying hotkey processor");
		PostThreadMessageA(threadId_, WM_QUIT, 0, 0);
	}
	void Hotkeys::Kernel_() noexcept
	{
		try {
			pmlog_verb(v::hotkey2)("Hotkey processor kernel start");

			// capture thread id
			threadId_ = GetCurrentThreadId();

			// create message window class
			const WNDCLASSEXA wx = {
				.cbSize = sizeof(WNDCLASSEX),
				.lpfnWndProc = DefWindowProc,
				.hInstance = GetModuleHandle(nullptr),
				.lpszClassName = "$PresentmonMessageWindowClass$",
			};
			const auto atom = RegisterClassExA(&wx);
			if (!atom) {
				pmlog_error().hr();
				return;
			}

			pmlog_verb(v::hotkey2)(std::format("Hotkey processor wndclass registered: {:X}", atom));

			// create message window
			messageWindowHandle_ = CreateWindowExW(
				0, MAKEINTATOM(atom), L"$PresentmonMessageWindow$",
				0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr
			);
			if (!messageWindowHandle_) {
				pmlog_error().hr();
				return;
			}

			pmlog_verb(v::hotkey2)(std::format("Hotkey processor wnd created: {:X}",
				reinterpret_cast<uintptr_t>(messageWindowHandle_)));

			// register to receive raw keyboard input
			const RAWINPUTDEVICE rid{
				.usUsagePage = 0x01,
				.usUsage = 0x06,
				.dwFlags = RIDEV_INPUTSINK,
				.hwndTarget = static_cast<HWND>(messageWindowHandle_),
			};
			if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
				pmlog_error().hr();
				return;
			}

			pmlog_verb(v::hotkey2)("Raw input registered");

			// signal that constuction is complete
			startupSemaphore_.release();

			MSG msg;
			while (GetMessageA(&msg, nullptr, 0, 0)) {
				if (msg.message == WM_INPUT) {
					pmlog_verb(v::hotkey2)("WM_INPUT received");

					auto& lParam = msg.lParam;
					// allocate memory for raw input data
					UINT dwSize{};
					if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER))) {
						pmlog_error().hr();
						return;
					}
					const auto inputBackingBuffer = std::make_unique<BYTE[]>(dwSize);

					// read in raw input data
					if (const auto size = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, inputBackingBuffer.get(),
						&dwSize, sizeof(RAWINPUTHEADER)); size != dwSize) {
						pmlog_error().hr();
						return;
					}
					const auto& input = reinterpret_cast<const RAWINPUT&>(*inputBackingBuffer.get());

					// handle raw input data for keyboard device
					if (input.header.dwType == RIM_TYPEKEYBOARD)
					{
						const auto& keyboard = input.data.keyboard;

						pmlog_verb(v::hotkey2)(std::format("KBD| vk:{} msg:{}", keyboard.VKey, keyboard.Message));

						// check for keycode outside of range of our table
						if (keyboard.VKey >= win::Key::virtualKeyTableSize) {
							pmlog_verb(v::hotkey2)("KBD| vk out of range");
							continue;
						}
						// key presses
						if (keyboard.Message == WM_KEYDOWN || keyboard.Message == WM_SYSKEYDOWN) {
							pmlog_verb(v::hotkey2)("key press");
							// don't handle autorepeat presses
							if (!pressedKeys_[keyboard.VKey]) {
								// mark key as in down state
								pressedKeys_[keyboard.VKey] = true;
								// if key is not modifier
								if (const auto key = win::Key::FromPlatformCode(keyboard.VKey)) {
									if (!key->IsModifierKey()) {
										// gather currently pressed modifiers
										const auto mods = GatherModifiers_();
										// check for matching registered hotkey action
										std::lock_guard lk{ mtx_ };
										if (auto i = registeredHotkeys_.find({ *key, mods });
											i != registeredHotkeys_.cend()) {
											// if match found dispatch action on renderer thread
											pmlog_verb(v::hotkey2)("hotkey dispatching");
											DispatchHotkey_(i->second);
										}
									}
								}
							}
							else {
								pmlog_verb(v::hotkey2)("key repeat");
							}
						}
						// key releases
						else if (keyboard.Message == WM_KEYUP || keyboard.Message == WM_SYSKEYUP) {
							pmlog_verb(v::hotkey2)("Key up");
							pressedKeys_[keyboard.VKey] = false;
						}
					}
					else {
						pmlog_warn("Unknown raw input type");
					}
				}
			}

			if (!DestroyWindow(static_cast<HWND>(messageWindowHandle_))) {
				pmlog_warn("failed window destroy").hr();
			}
			if (!UnregisterClass(MAKEINTATOM(atom), GetModuleHandle(nullptr))) {
				pmlog_warn("failed unreg class").hr();
			}

			pmlog_verb(v::hotkey2)();
		}
		catch (...) {
			pmlog_error(ReportException());
		}
	}
	void Hotkeys::DispatchHotkey_(int action) const
	{
		if (Handler_) {
			pmlog_dbg("hotkey action dispatched to handler").pmwatch(action);
			Handler_(action);
		}
		else {
			pmlog_warn("Hotkey handler not set");
		}
	}
	void Hotkeys::SetHandler(std::function<void(int)> handler)
	{
		std::lock_guard lk{ mtx_ };
		pmlog_verb(v::hotkey2)("Hotkey handler set");
		Handler_ = std::move(handler);
	}
	bool Hotkeys::BindAction(int action, win::Key key, win::ModSet mods)
	{
		pmlog_verb(v::hotkey2)("Hotkey action binding");
		std::lock_guard lk{ mtx_ };
		// if action is already bound, remove it
		if (const auto i = rn::find_if(registeredHotkeys_, [action](const auto& i) {
			return i.second == action;
		}); i != registeredHotkeys_.cend()) {
			pmlog_verb(v::hotkey2)("Hotkey action binding remove existing action");
			registeredHotkeys_.erase(i);
		}
		// if hotkey combination is already bound, remove it
		registeredHotkeys_.erase({ key, mods });
		// bind action to new hotkey combination
		registeredHotkeys_.emplace(std::piecewise_construct,
			std::forward_as_tuple(key, mods),
			std::forward_as_tuple(action)
		);
		// signal success
		return true;
	}
	bool Hotkeys::ClearAction(int action)
	{
		pmlog_verb(v::hotkey2)("Hotkey action clearing");
		std::lock_guard lk{ mtx_ };
		// if action is bound, remove it
		if (const auto i = rn::find_if(registeredHotkeys_, [action](const auto& i) {
			return i.second == action;
		}); i != registeredHotkeys_.cend()) {
			registeredHotkeys_.erase(i);
		}
		else {
			pmlog_warn("Attempted to clear unregistered hotkey");
		}
		// signal success
		return true;
	}
	win::ModSet Hotkeys::GatherModifiers_() const
	{
		win::ModSet mods;
		if (pressedKeys_[VK_SHIFT]) {
			mods = mods | win::Mod::Shift;
		}
		if (pressedKeys_[VK_CONTROL]) {
			mods = mods | win::Mod::Ctrl;
		}
		if (pressedKeys_[VK_MENU]) {
			mods = mods | win::Mod::Alt;
		}
		if (pressedKeys_[VK_LWIN]) {
			mods = mods | win::Mod::Win;
		}
		return mods;
	}


	Hotkeys::Hotkey::Hotkey(win::Key key, win::ModSet mods)
		:
		key{ key },
		mods{ mods }
	{}
	bool Hotkeys::Hotkey::operator<(const Hotkey& rhs) const
	{
		return key == rhs.key ? (mods < rhs.mods) : (key < rhs.key);
	}
	bool Hotkeys::Hotkey::operator==(const Hotkey& rhs) const
	{
		return key == rhs.key && mods == rhs.mods;
	}
}
