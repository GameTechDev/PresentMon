// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <Core/source/win/WinAPI.h>
#include <CppUnitTest.h>

#include <Core/source/win/HotkeyListener.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace HotkeyTests
{
	using p2c::win::Hotkeys;
	using p2c::win::Key;
	using p2c::win::ModSet;

	// Each test creates one Hotkeys at a time: it registers a fixed window class name,
	// so two live instances in one process cannot both start.
	TEST_CLASS(HotkeyBindingTests)
	{
	public:
		// The kernel clears every binding when its UI session ends.
		TEST_METHOD(ClearAllRemovesEveryBinding)
		{
			Hotkeys hotkeys;
			hotkeys.BindAction(1, Key{ Key::Code::F23 }, ModSet::FromCodes({}));
			hotkeys.BindAction(2, Key{ Key::Code::F24 }, ModSet::FromCodes({}));
			Assert::AreEqual((size_t)2, hotkeys.ClearAll(), L"ClearAll did not remove both bindings");
			Assert::AreEqual((size_t)0, hotkeys.ClearAll(), L"Bindings survived ClearAll");
		}
		// Binding an action again replaces its old combination rather than adding a second one,
		// which a reloaded UI page relies on when it binds its hotkeys again.
		TEST_METHOD(RebindingAnActionReplacesItsCombination)
		{
			Hotkeys hotkeys;
			hotkeys.BindAction(1, Key{ Key::Code::F23 }, ModSet::FromCodes({}));
			hotkeys.BindAction(1, Key{ Key::Code::F24 }, ModSet::FromCodes({}));
			Assert::AreEqual((size_t)1, hotkeys.ClearAll(), L"Rebinding an action left its old combination bound");
		}
	};
}
