// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;
namespace as = boost::asio;
using namespace std::literals;

namespace LoggingTests
{
	namespace
	{
		enum class CrashKind_
		{
			CppException = 0,
			Seh = 1,
			Terminate = 2,
		};

		std::vector<std::string> BuildArgs_(CrashKind_ kind)
		{
			return {
				"--mode"s, "LoggingCrashTest"s,
				"--submode"s, std::to_string(static_cast<int>(kind)),
				"--log-folder"s, logFolder_,
				"--log-name-pid"s,
				"--log-level"s, "debug"s,
			};
		}

		std::string ReadAllText_(const fs::path& path)
		{
			std::ifstream file{ path, std::ios::binary };
			if (!file) {
				return {};
			}
			return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
		}

		bool WaitForLogContains_(const fs::path& path, const std::string& needle, std::chrono::milliseconds timeout)
		{
			const auto deadline = std::chrono::steady_clock::now() + timeout;
			while (std::chrono::steady_clock::now() < deadline) {
				const auto text = ReadAllText_(path);
				if (text.find(needle) != std::string::npos) {
					return true;
				}
				std::this_thread::sleep_for(25ms);
			}
			return false;
		}

		void ResetLogFolder_()
		{
			const fs::path logFolder{ logFolder_ };
			std::error_code ec;
			fs::remove_all(logFolder, ec);
			fs::create_directories(logFolder, ec);
		}

		class CrashClientProcess_ : public TestProcess
		{
		public:
			CrashClientProcess_(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& args)
				:
				TestProcess{ ioctx, jm, "SampleClient.exe"s, args }
			{
			}

			DWORD GetExitCode()
			{
				DWORD exitCode = STILL_ACTIVE;
				GetExitCodeProcess(process_.native_handle(), &exitCode);
				return exitCode;
			}
		};
	}

	TEST_CLASS(CrashFlushIntegrationTests)
	{
		as::io_context ioctx_;
		JobManager jobMan_;
		std::optional<pmon::test::LogChannelManager> logManager_;

		void RunCase_(CrashKind_ kind, const char* scenarioToken)
		{
			ResetLogFolder_();
			if (!logManager_) {
				logManager_.emplace();
			}
			pmon::test::SetupTestLogging(logFolder_, "debug", {});

			CrashClientProcess_ client{ ioctx_, jobMan_, BuildArgs_(kind) };
			const auto clientPid = client.GetId();
			const auto logPath = fs::path{ logFolder_ } / std::format("sample-client-{}.txt", clientPid);

			if (!client.WaitForExit(5s)) {
				client.Murder();
				Assert::Fail(L"SampleClient did not exit after crash scenario");
			}

			Logger::WriteMessage(std::format("Crash test log path: {}\n", logPath.string()).c_str());
			const auto logReady = WaitForLogContains_(logPath, "CRASH_TEST:BEGIN", 3s);
			Assert::IsTrue(logReady, L"Expected crash prelude entry not found in log output");

			const auto scenarioNeedle = std::format("CRASH_TEST:SCENARIO={}", scenarioToken);
			const auto scenarioReady = WaitForLogContains_(logPath, scenarioNeedle, 3s);
			Assert::IsTrue(scenarioReady, L"Expected crash scenario breadcrumb not found in log output");

			const auto exitCode = client.GetExitCode();
			Assert::IsTrue(exitCode != 0, L"Crash mode should terminate abnormally");
		}

	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
			logManager_.reset();
		}

		TEST_METHOD(CppExceptionCrashFlush)
		{
			RunCase_(CrashKind_::CppException, "CPP_EXCEPTION");
		}

		TEST_METHOD(SehCrashFlush)
		{
			RunCase_(CrashKind_::Seh, "SEH");
		}

		TEST_METHOD(TerminateCrashFlush)
		{
			RunCase_(CrashKind_::Terminate, "TERMINATE");
		}
	};
}
