// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "Folders.h"
#include "TestProcess.h"
#include "../CommonUtilities/test/MachineExpectations.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace CaptureCliWaitTests
{
	namespace machine = pmon::util::test;
	// Stop-gap isolation for this suite: ignore log bytes written before each launch.
	// Replace this when the test log and test-data retention system gains per-run and
	// per-case artifact ownership.
	using ClientLogSnapshot = std::unordered_map<std::filesystem::path, std::uintmax_t>;

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

	static bool IsClientLog_(const std::filesystem::path& path)
	{
		const auto name = path.filename().string();
		return name.starts_with("pmlog-client-") && name.ends_with(".txt");
	}

	static ClientLogSnapshot SnapshotClientLogs_(const std::filesystem::path& folder)
	{
		ClientLogSnapshot snapshot;
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator{ folder, ec }) {
			if (!IsClientLog_(entry.path())) {
				continue;
			}
			const auto size = entry.file_size(ec);
			if (!ec) {
				snapshot.emplace(entry.path(), size);
			}
			ec.clear();
		}
		return snapshot;
	}

	static bool FileContainsAfter_(const std::filesystem::path& path, std::uintmax_t offset,
		const std::string& text)
	{
		std::error_code ec;
		const auto size = std::filesystem::file_size(path, ec);
		if (ec) {
			return false;
		}
		if (size < offset) {
			offset = 0;
		}
		std::ifstream file{ path, std::ios::binary };
		file.seekg((std::streamoff)offset);
		const std::string contents{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
		return contents.find(text) != std::string::npos;
	}

	static bool ClientLogContainsAfter_(const std::filesystem::path& folder,
		const ClientLogSnapshot& snapshot, const std::string& text)
	{
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator{ folder, ec }) {
			if (!IsClientLog_(entry.path())) {
				continue;
			}
			const auto it = snapshot.find(entry.path());
			const auto offset = it == snapshot.end() ? 0 : it->second;
			if (FileContainsAfter_(entry.path(), offset, text)) {
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
			const auto logSnapshot = SnapshotClientLogs_(logFolder_);
			auto kernel = fixture_.LaunchKernel(MakeCaptureArgs_({
				"--target-name"s, "NoSuchProcess_CaptureCliWait.exe"s,
			}));
			Assert::IsTrue(kernel.WaitForExit(machine::ScaleWait(5s)),
				L"capture should exit quickly when target is missing");
			Assert::AreEqual(-1, kernel.GetExitCode());
			Assert::IsTrue(ClientLogContainsAfter_(logFolder_, logSnapshot,
				"Failed to find any processes matching supplied main module name"),
				L"missing-target failure should be logged");
		}

		TEST_METHOD(WaitTimesOutWhenTargetMissing)
		{
			const auto logSnapshot = SnapshotClientLogs_(logFolder_);
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
			Assert::IsTrue(ClientLogContainsAfter_(logFolder_, logSnapshot,
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
			const auto logSnapshot = SnapshotClientLogs_(logFolder_);
			auto kernel = fixture_.LaunchKernel(MakeCaptureArgs_({
				"--target-name"s, "PresentBench2.exe"s,
				"--wait-for-target"s, "5"s,
			}));
			Assert::IsTrue(kernel.IsRunning(), L"capture should remain armed while waiting");
			Assert::IsFalse(ClientLogContainsAfter_(logFolder_, logSnapshot, "kernel process starting"),
				L"kernel startup should not begin before the target appears");

			auto presenter = fixture_.LaunchPresenter();
			Assert::IsTrue(WaitFor_(machine::ScaleWait(5s), [&] {
				return ClientLogContainsAfter_(logFolder_, logSnapshot, "kernel process starting");
			}), L"wait mode should proceed once PresentBench2.exe appears");

			kernel.Murder();
			presenter.Murder();
		}

	private:
		TestFixture fixture_;
	};
}
