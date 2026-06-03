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

	static std::vector<std::string> MakeKernelArgs_(const std::string& mutexSuffix)
	{
		return {
			"--ui-mutex-name"s, mutexSuffix,
			"--ui-flag"s, "no-net-fail"s,
		};
	}

	static std::vector<std::string> MakeDuplicateKernelArgs_(const std::string& mutexSuffix, const std::string& response)
	{
		return {
			"--ui-mutex-name"s, mutexSuffix,
			"--duplicate-ui-response"s, response,
			"--ui-flag"s, "no-net-fail"s,
		};
	}

	static std::vector<std::unique_ptr<KernelProcess>> LaunchSimultaneousDuplicateKernels_(
		TestFixture& fixture,
		const std::string& mutexSuffix,
		const std::string& response)
	{
		std::promise<void> launchGate;
		auto launchSignal = launchGate.get_future().share();
		std::vector<std::future<std::unique_ptr<KernelProcess>>> duplicateFutures;
		for (int i = 0; i < 5; ++i) {
			duplicateFutures.push_back(std::async(std::launch::async,
				[&fixture, mutexSuffix, response, launchSignal] {
					launchSignal.wait();
					return fixture.LaunchKernelAsPtr(MakeDuplicateKernelArgs_(mutexSuffix, response));
				}));
		}

		launchGate.set_value();

		std::vector<std::unique_ptr<KernelProcess>> duplicates;
		for (auto& future : duplicateFutures) {
			duplicates.push_back(future.get());
		}
		return duplicates;
	}

	static size_t CountRunningProcesses_(std::vector<std::unique_ptr<KernelProcess>>& processes)
	{
		size_t count = 0;
		for (auto& process : processes) {
			if (process->IsRunning()) {
				++count;
			}
		}
		return count;
	}

	static void TerminateUiInstanceAndWait_(const std::string& mutexSuffix, TestProcess& process)
	{
		Assert::IsTrue(ui::TerminateUiInstanceProcessTree(mutexSuffix), L"UI process tree was not terminated");
		Assert::IsTrue(process.WaitForExit(5s), L"Kernel process did not exit");
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

		TEST_METHOD(ApplicationLaunchBringsExistingUiToForeground)
		{
			const auto mutexSuffix = MakeMutexSuffix_("Foreground");
			auto first = fixture_.LaunchKernel(MakeKernelArgs_(mutexSuffix));
			const auto hWnd = WaitForUiWindow_(mutexSuffix);

			ShowWindow(hWnd, SW_MINIMIZE);
			Assert::IsTrue(WaitFor_(5s, [hWnd] {
				return IsIconic(hWnd) != FALSE;
			}), L"Timed out waiting for UI window to minimize");

			auto second = fixture_.LaunchKernel({
				"--ui-mutex-name"s, mutexSuffix,
				"--duplicate-ui-response"s, "yes"s,
			});
			AssertUiAlreadyRunningExit_(second);
			Assert::IsTrue(first.IsRunning(), L"Original kernel process should remain active");

			Assert::IsTrue(WaitFor_(5s, [hWnd] {
				const auto hForeground = GetForegroundWindow();
				const auto hRootForeground = hForeground ? GetAncestor(hForeground, GA_ROOT) : nullptr;
				return IsIconic(hWnd) == FALSE && hRootForeground == hWnd;
			}), L"Existing UI window was not brought to the foreground");

			TerminateUiInstanceAndWait_(mutexSuffix, first);
		}

		TEST_METHOD(ApplicationLaunchReplacesExistingUi)
		{
			const auto mutexSuffix = MakeMutexSuffix_("Replace");
			auto first = fixture_.LaunchKernel(MakeKernelArgs_(mutexSuffix));
			WaitForUiWindow_(mutexSuffix);

			auto second = fixture_.LaunchKernel(MakeDuplicateKernelArgs_(mutexSuffix, "no"s));

			Assert::IsTrue(first.WaitForExit(5s), L"Original kernel process was not terminated");
			WaitForUiWindow_(mutexSuffix);
			Assert::IsTrue(second.IsRunning(), L"Replacement kernel process should remain active");

			TerminateUiInstanceAndWait_(mutexSuffix, second);
		}

		TEST_METHOD(SimultaneousApplicationLaunchesBringExistingUiToForeground)
		{
			const auto mutexSuffix = MakeMutexSuffix_("SimultaneousForeground");
			auto first = fixture_.LaunchKernel(MakeKernelArgs_(mutexSuffix));
			const auto hWnd = WaitForUiWindow_(mutexSuffix);

			ShowWindow(hWnd, SW_MINIMIZE);
			Assert::IsTrue(WaitFor_(5s, [hWnd] {
				return IsIconic(hWnd) != FALSE;
			}), L"Timed out waiting for UI window to minimize");

			auto duplicates = LaunchSimultaneousDuplicateKernels_(fixture_, mutexSuffix, "yes"s);
			for (auto& duplicate : duplicates) {
				AssertUiAlreadyRunningExit_(*duplicate);
			}
			Assert::IsTrue(first.IsRunning(), L"Original kernel process should remain active");

			Assert::IsTrue(WaitFor_(5s, [hWnd] {
				const auto hForeground = GetForegroundWindow();
				const auto hRootForeground = hForeground ? GetAncestor(hForeground, GA_ROOT) : nullptr;
				return IsIconic(hWnd) == FALSE && hRootForeground == hWnd;
			}), L"Existing UI window was not brought to the foreground");

			TerminateUiInstanceAndWait_(mutexSuffix, first);
		}

		TEST_METHOD(SimultaneousApplicationLaunchesReplaceExistingUi)
		{
			const auto mutexSuffix = MakeMutexSuffix_("SimultaneousReplace");
			auto first = fixture_.LaunchKernel(MakeKernelArgs_(mutexSuffix));
			WaitForUiWindow_(mutexSuffix);

			auto duplicates = LaunchSimultaneousDuplicateKernels_(fixture_, mutexSuffix, "no"s);

			Assert::IsTrue(first.WaitForExit(5s), L"Original kernel process was not terminated");
			Assert::IsTrue(WaitFor_(15s, [&] {
				return ui::FindUiBrowserWindow(mutexSuffix) != nullptr &&
					CountRunningProcesses_(duplicates) == 1;
			}), L"Replacement launches did not settle to one active UI instance");

			Assert::IsTrue(ui::TerminateUiInstanceProcessTree(mutexSuffix), L"Replacement UI process tree was not terminated");
			for (auto& duplicate : duplicates) {
				Assert::IsTrue(duplicate->WaitForExit(5s), L"Replacement kernel process did not exit");
			}
		}
	};
}
