// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "Folders.h"
#include "TestProcess.h"
#include "../CommonUtilities/test/MachineExpectations.h"
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace CaptureCliWaitTests
{
	namespace machine = pmon::util::test;

	class TestFixture : public CommonTestFixture
	{
	protected:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-capture-cli-wait-ctrl)",
				.shmNamePrefix = "pm_capture_cli_wait",
				.logLevel = "info",
				.logFolder = logFolder_,
				.suppressService = true,
			};
			return args;
		}
	};

	static bool FileContains_(const std::filesystem::path& path, const std::string& text)
	{
		std::ifstream file{ path };
		const std::string contents{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
		return contents.find(text) != std::string::npos;
	}

	static bool ClientLogContains_(const std::filesystem::path& folder, const std::string& text)
	{
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator{ folder, ec }) {
			const auto name = entry.path().filename().string();
			if (name.starts_with("pmlog-client-") && name.ends_with(".txt") &&
				FileContains_(entry.path(), text)) {
				return true;
			}
		}
		return false;
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

	static std::vector<std::string> MakeCaptureArgs_(std::vector<std::string> extras = {})
	{
		// Parent-level flags must precede the capture subcommand for CLI11.
		std::vector<std::string> args{
			"--log-synchronous"s,
			"capture"s,
		};
		args.append_range(std::move(extras));
		return args;
	}

	TEST_CLASS(CaptureCliWaitTests)
	{
	public:
		TEST_METHOD_INITIALIZE(SetUp)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(TearDown)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(FailFastWhenTargetMissing)
		{
			auto kernel = fixture_.LaunchKernel(MakeCaptureArgs_({
				"--target-name"s, "NoSuchProcess_CaptureCliWait.exe"s,
			}));
			Assert::IsTrue(kernel.WaitForExit(machine::ScaleWait(5s)),
				L"capture should exit quickly when target is missing");
			Assert::AreEqual(-1, kernel.GetExitCode());
			Assert::IsTrue(ClientLogContains_(logFolder_,
				"Failed to find any processes matching supplied main module name"),
				L"missing-target failure should be logged");
		}

		TEST_METHOD(WaitTimesOutWhenTargetMissing)
		{
			const auto started = std::chrono::steady_clock::now();
			auto kernel = fixture_.LaunchKernel(MakeCaptureArgs_({
				"--target-name"s, "NoSuchProcess_CaptureCliWait.exe"s,
				"--wait-for-target"s, "1"s,
			}));
			Assert::IsTrue(kernel.WaitForExit(machine::ScaleWait(5s)),
				L"timed wait should exit after the timeout");
			const auto elapsed = std::chrono::steady_clock::now() - started;
			Assert::AreEqual(-1, kernel.GetExitCode());
			Assert::IsTrue(elapsed >= 900ms, L"timed wait returned before the requested timeout");
			Assert::IsTrue(ClientLogContains_(logFolder_,
				"Timed out waiting for process matching supplied main module name"),
				L"wait timeout should be logged");
		}

		TEST_METHOD(WaitRequiresTargetName)
		{
			auto kernel = fixture_.LaunchKernel(MakeCaptureArgs_({
				"--wait-for-target"s,
			}));
			Assert::IsTrue(kernel.WaitForExit(machine::ScaleWait(5s)));
			Assert::AreNotEqual(0, kernel.GetExitCode());
		}

		TEST_METHOD(WaitRejectsNonPositiveTimeout)
		{
			auto kernel = fixture_.LaunchKernel(MakeCaptureArgs_({
				"--target-name"s, "NoSuchProcess_CaptureCliWait.exe"s,
				"--wait-for-target"s, "0"s,
			}));
			Assert::IsTrue(kernel.WaitForExit(machine::ScaleWait(5s)));
			Assert::AreEqual(-1, kernel.GetExitCode());
		}

		TEST_METHOD(WaitUnblocksWhenTargetAppears)
		{
			auto kernel = fixture_.LaunchKernel(MakeCaptureArgs_({
				"--target-name"s, "PresentBench2.exe"s,
				"--wait-for-target"s, "5"s,
			}));
			Assert::IsTrue(kernel.IsRunning(), L"capture should remain armed while waiting");
			Assert::IsFalse(ClientLogContains_(logFolder_, "kernel process starting"),
				L"kernel startup should not begin before the target appears");

			auto presenter = fixture_.LaunchPresenter();
			Assert::IsTrue(WaitFor_(machine::ScaleWait(5s), [&] {
				return ClientLogContains_(logFolder_, "kernel process starting");
			}), L"wait mode should proceed once PresentBench2.exe appears");

			kernel.Murder();
			presenter.Murder();
		}

	private:
		TestFixture fixture_;
	};
}
