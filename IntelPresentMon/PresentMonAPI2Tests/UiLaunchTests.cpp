// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "Folders.h"
#include "TestProcess.h"
#include "../AppCef/source/util/UiProcessGuard.h"
#include <chrono>
#include <format>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace UiLaunchTests
{
	namespace ui = p2c::client::util;

	class TestFixture : public CommonTestFixture
	{
	protected:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-ui-launch-test-ctrl)",
				.shmNamePrefix = "pm_ui_launch_test",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "MultiClient",
				.suppressService = true,
			};
			return args;
		}
	};

	static std::string MakeMutexSuffix_(const char* testName)
	{
		return std::format("UiLaunchTests-{}-{}-{}",
			testName, GetCurrentProcessId(), GetTickCount64());
	}

	template<typename F>
	static bool WaitFor_(std::chrono::milliseconds timeout, F&& predicate)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		do {
			if (predicate()) {
				return true;
			}
			std::this_thread::sleep_for(25ms);
		} while (std::chrono::steady_clock::now() < deadline);
		return predicate();
	}

	static HWND WaitForUiWindow_(const std::string& mutexSuffix)
	{
		HWND hWnd = nullptr;
		Assert::IsTrue(WaitFor_(15s, [&] {
			hWnd = ui::FindUiBrowserWindow(mutexSuffix);
			return hWnd != nullptr;
		}), L"Timed out waiting for UI browser window");
		return hWnd;
	}

	static void AssertUiAlreadyRunningExit_(TestProcess& process)
	{
		Assert::IsTrue(process.WaitForExit(5s), L"Duplicate launch did not exit promptly");
		Assert::AreEqual(ui::UiAlreadyRunningExitCode, process.GetExitCode());
	}

	TEST_CLASS(UiProcessGuardTests)
	{
		TestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}

		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(SecondUiLaunchExitsBeforeFullLaunch)
		{
			const auto mutexSuffix = MakeMutexSuffix_("Serial");
			auto first = fixture_.LaunchUi({ "--p2c-ui-mutex-name"s, mutexSuffix });
			WaitForUiWindow_(mutexSuffix);

			auto second = fixture_.LaunchUi({ "--p2c-ui-mutex-name"s, mutexSuffix });
			AssertUiAlreadyRunningExit_(second);

			Assert::IsTrue(first.IsRunning(), L"Original UI process should remain active");
			first.Murder();
		}

		TEST_METHOD(SimultaneousDuplicateUiLaunchesExitBeforeFullLaunch)
		{
			const auto mutexSuffix = MakeMutexSuffix_("Simultaneous");
			auto first = fixture_.LaunchUi({ "--p2c-ui-mutex-name"s, mutexSuffix });
			WaitForUiWindow_(mutexSuffix);

			std::promise<void> launchGate;
			auto launchSignal = launchGate.get_future().share();
			std::vector<std::future<std::unique_ptr<UiProcess>>> duplicateFutures;
			for (int i = 0; i < 5; ++i) {
				duplicateFutures.push_back(std::async(std::launch::async,
					[&fixture = fixture_, mutexSuffix, launchSignal] {
						launchSignal.wait();
						return fixture.LaunchUiAsPtr({ "--p2c-ui-mutex-name"s, mutexSuffix });
					}));
			}

			launchGate.set_value();

			std::vector<std::unique_ptr<UiProcess>> duplicates;
			for (auto& future : duplicateFutures) {
				duplicates.push_back(future.get());
			}
			for (auto& duplicate : duplicates) {
				AssertUiAlreadyRunningExit_(*duplicate);
			}

			Assert::IsTrue(first.IsRunning(), L"Original UI process should remain active");
			first.Murder();
		}

		TEST_METHOD(SecondApplicationLaunchBringsExistingUiToForeground)
		{
			const auto mutexSuffix = MakeMutexSuffix_("Foreground");
			auto first = fixture_.LaunchUi({ "--p2c-ui-mutex-name"s, mutexSuffix });
			const auto hWnd = WaitForUiWindow_(mutexSuffix);

			ShowWindow(hWnd, SW_MINIMIZE);
			Assert::IsTrue(WaitFor_(5s, [hWnd] {
				return IsIconic(hWnd) != FALSE;
			}), L"Timed out waiting for UI window to minimize");

			auto second = fixture_.LaunchKernel({ "--ui-mutex-name"s, mutexSuffix });
			AssertUiAlreadyRunningExit_(second);

			Assert::IsTrue(WaitFor_(5s, [hWnd] {
				const auto hForeground = GetForegroundWindow();
				const auto hRootForeground = hForeground ? GetAncestor(hForeground, GA_ROOT) : nullptr;
				return IsIconic(hWnd) == FALSE && hRootForeground == hWnd;
			}), L"Existing UI window was not brought to the foreground");

			first.Murder();
		}
	};
}
