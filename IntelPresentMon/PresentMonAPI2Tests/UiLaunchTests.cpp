// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "Folders.h"
#include "TestProcess.h"
#include "../AppCef/source/util/UiProcessGuard.h"
#include "../CommonUtilities/win/com/ComPtr.h"
#include <UIAutomation.h>
#include <oleauto.h>
#include <chrono>
#include <format>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "UIAutomationCore.lib")

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

	// Copy shown by the app when PresentMon API init fails because the service is down.
	static constexpr const wchar_t* kServiceUnavailableErrorTitle = L"PresentMon Initialization Error";
	static constexpr const wchar_t* kServiceUnavailableErrorBody =
		L"Failed to initialize PresentMon API. Ensure that PresentMon Service is installed and running, and try again.";

	static bool BlobHasServiceUnavailableError_(const std::wstring& blob)
	{
		return blob.find(kServiceUnavailableErrorTitle) != std::wstring::npos
			&& blob.find(kServiceUnavailableErrorBody) != std::wstring::npos;
	}

	static void AppendWindowText_(HWND hWnd, std::wstring& blob)
	{
		const int length = GetWindowTextLengthW(hWnd);
		if (length <= 0) {
			return;
		}
		std::wstring text((size_t)length + 1, L'\0');
		const int copied = GetWindowTextW(hWnd, text.data(), length + 1);
		if (copied <= 0) {
			return;
		}
		text.resize((size_t)copied);
		blob.append(text);
		blob.push_back(L'\n');
	}

	struct WindowTextSearch_
	{
		HWND hApp = nullptr;
		DWORD uiPid = 0;
		DWORD kernelPid = 0;
		std::wstring blob;
	};

	static BOOL CALLBACK AppendChildWindowText_(HWND hWnd, LPARAM param)
	{
		auto& search = *reinterpret_cast<WindowTextSearch_*>(param);
		AppendWindowText_(hWnd, search.blob);
		return TRUE;
	}

	static BOOL CALLBACK AppendTopLevelWindowText_(HWND hWnd, LPARAM param)
	{
		auto& search = *reinterpret_cast<WindowTextSearch_*>(param);
		DWORD pid = 0;
		GetWindowThreadProcessId(hWnd, &pid);
		const auto owner = GetWindow(hWnd, GW_OWNER);
		if (pid == search.uiPid || pid == search.kernelPid || owner == search.hApp) {
			AppendWindowText_(hWnd, search.blob);
			EnumChildWindows(hWnd, AppendChildWindowText_, param);
		}
		return TRUE;
	}

	static void AppendVariantText_(VARIANT& value, std::wstring& blob)
	{
		if (value.vt == VT_BSTR && value.bstrVal != nullptr && value.bstrVal[0] != L'\0') {
			blob.append(value.bstrVal);
			blob.push_back(L'\n');
		}
		VariantClear(&value);
	}

	static bool CollectUiaText_(IUIAutomationTreeWalker* pWalker, IUIAutomationElement* pElement,
		std::wstring& blob, int depth, size_t& remaining)
	{
		if (pElement == nullptr || depth > 40 || remaining == 0) {
			return false;
		}
		--remaining;

		VARIANT name{};
		VariantInit(&name);
		if (SUCCEEDED(pElement->GetCurrentPropertyValue(UIA_NamePropertyId, &name))) {
			AppendVariantText_(name, blob);
		}
		if (BlobHasServiceUnavailableError_(blob)) {
			return true;
		}

		pmon::util::win::com::ComPtr<IUIAutomationElement> pChild;
		if (FAILED(pWalker->GetFirstChildElement(pElement, pChild.GetAddressOf()))) {
			return false;
		}
		while (pChild) {
			if (CollectUiaText_(pWalker, pChild.Get(), blob, depth + 1, remaining)) {
				return true;
			}
			pmon::util::win::com::ComPtr<IUIAutomationElement> pNext;
			if (FAILED(pWalker->GetNextSiblingElement(pChild.Get(), pNext.GetAddressOf()))) {
				break;
			}
			pChild = std::move(pNext);
		}
		return BlobHasServiceUnavailableError_(blob);
	}

	struct UiaWindowSearch_
	{
		IUIAutomation* pAutomation = nullptr;
		IUIAutomationTreeWalker* pWalker = nullptr;
		std::wstring* pBlob = nullptr;
		size_t* pRemaining = nullptr;
	};

	static BOOL CALLBACK CollectChildUiaText_(HWND hWnd, LPARAM param)
	{
		auto& search = *reinterpret_cast<UiaWindowSearch_*>(param);
		if (*search.pRemaining == 0 || BlobHasServiceUnavailableError_(*search.pBlob)) {
			return FALSE;
		}
		pmon::util::win::com::ComPtr<IUIAutomationElement> pElement;
		if (FAILED(search.pAutomation->ElementFromHandle(hWnd, pElement.GetAddressOf())) || !pElement) {
			return TRUE;
		}
		CollectUiaText_(search.pWalker, pElement.Get(), *search.pBlob, 0, *search.pRemaining);
		return BlobHasServiceUnavailableError_(*search.pBlob) ? FALSE : TRUE;
	}

	static bool UiaShowsServiceUnavailableError_(HWND hApp, std::wstring& blob)
	{
		const HRESULT initHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool uninit = initHr == S_OK || initHr == S_FALSE;
		struct CoUninit_
		{
			bool armed = false;
			~CoUninit_()
			{
				if (armed) {
					CoUninitialize();
				}
			}
		} coUninit{ uninit };

		pmon::util::win::com::ComPtr<IUIAutomation> pAutomation;
		if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER,
			__uuidof(IUIAutomation), reinterpret_cast<void**>(pAutomation.GetAddressOf()))) || !pAutomation) {
			return false;
		}

		pmon::util::win::com::ComPtr<IUIAutomationTreeWalker> pWalker;
		if (FAILED(pAutomation->get_RawViewWalker(pWalker.GetAddressOf())) || !pWalker) {
			return false;
		}

		size_t remaining = 8000;
		pmon::util::win::com::ComPtr<IUIAutomationElement> pRoot;
		if (SUCCEEDED(pAutomation->ElementFromHandle(hApp, pRoot.GetAddressOf())) && pRoot) {
			if (CollectUiaText_(pWalker.Get(), pRoot.Get(), blob, 0, remaining)) {
				return true;
			}
		}

		UiaWindowSearch_ search{
			.pAutomation = pAutomation.Get(),
			.pWalker = pWalker.Get(),
			.pBlob = &blob,
			.pRemaining = &remaining,
		};
		EnumChildWindows(hApp, CollectChildUiaText_, reinterpret_cast<LPARAM>(&search));
		return BlobHasServiceUnavailableError_(blob);
	}

	static bool UiShowsServiceUnavailableError_(HWND hApp, DWORD kernelPid)
	{
		WindowTextSearch_ search{
			.hApp = hApp,
			.kernelPid = kernelPid,
		};
		GetWindowThreadProcessId(hApp, &search.uiPid);
		EnumWindows(AppendTopLevelWindowText_, reinterpret_cast<LPARAM>(&search));
		if (BlobHasServiceUnavailableError_(search.blob)) {
			return true;
		}
		return UiaShowsServiceUnavailableError_(hApp, search.blob);
	}

	class ServiceUnavailableFixture : public CommonTestFixture
	{
	protected:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-ui-svc-unavailable-test-ctrl)",
				.shmNamePrefix = "pm_ui_svc_unavailable_test",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "MultiClient",
				.suppressService = true,
				.launchAppUi = true,
			};
			return args;
		}
	};

	class LaunchedUiGuard_
	{
	public:
		LaunchedUiGuard_(std::string mutexSuffix, TestProcess& process)
			:
			mutexSuffix_{ std::move(mutexSuffix) },
			process_{ process }
		{}
		~LaunchedUiGuard_()
		{
			try {
				if (ui::FindUiBrowserWindow(mutexSuffix_)) {
					ui::TerminateUiInstanceProcessTree(mutexSuffix_);
				}
				if (process_.IsRunning()) {
					process_.Murder();
				}
			}
			catch (...) {
			}
		}
	private:
		std::string mutexSuffix_;
		TestProcess& process_;
	};

	TEST_CLASS(ServiceUnavailableUiTests)
	{
		ServiceUnavailableFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}

		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(WindowShowsErrorModalWhenServiceUnavailable)
		{
			const auto mutexSuffix = MakeMutexSuffix_("ServiceUnavailable");
			auto kernel = fixture_.LaunchKernel(MakeKernelArgs_(mutexSuffix));
			LaunchedUiGuard_ guard{ mutexSuffix, kernel };

			HWND hWnd = nullptr;
			const auto settled = WaitFor_(15s, [&] {
				hWnd = ui::FindUiBrowserWindow(mutexSuffix);
				return hWnd != nullptr || !kernel.IsRunning();
			});
			if (!kernel.IsRunning()) {
				kernel.WaitForExit(1s);
				Assert::Fail(std::format(
					L"Client exited ({}) before creating a window when the service action pipe was unavailable",
					kernel.GetExitCode()).c_str());
			}
			Assert::IsTrue(settled && hWnd != nullptr,
				L"Timed out waiting for the UI window when the service action pipe was unavailable");
			Assert::IsTrue(WaitFor_(15s, [&] {
				return UiShowsServiceUnavailableError_(hWnd, kernel.GetId());
			}), L"UI window did not show the service unavailable error modal");
			Assert::IsTrue(kernel.IsRunning(),
				L"Client exited after the UI window appeared");

			TerminateUiInstanceAndWait_(mutexSuffix, kernel);
		}
	};
}
