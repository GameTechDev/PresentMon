// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include <filesystem>
#include <fstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace PresentBench2ControlTests
{
	static constexpr const char* logFolder_ = "TestLogs\\PresentBench2Control";

	class TestFixture : public CommonTestFixture
	{
	public:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-pb2-control-test-ctrl)",
				.shmNamePrefix = "pm_pb2_control_test",
				.logLevel = "info",
				.logFolder = logFolder_,
				.sampleClientMode = "NONE",
				.suppressService = true,
			};
			return args;
		}
	};

	TEST_CLASS(PresentBench2ControlTests)
	{
		TestFixture fixture_;
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup({ "--enable-test-control"s });
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(CapabilitiesStatusWaitForFrameAndQuit)
		{
			Assert::IsTrue(std::filesystem::exists("PresentBench2.exe"),
				L"PresentBench2.exe not found beside tests");
			auto bench = fixture_.LaunchPresentBench2({
				"--frame-count"s, "4"s,
			});
			const auto caps = bench.CapabilitiesJson();
			Assert::IsTrue(caps.find("xessFg") != std::string::npos);

			Assert::AreEqual("wait-for-frame-ok"s,
				bench.Command(R"(wait-for-frame {"appFrameId":3,"timeoutMs":5000})"));
			Assert::AreEqual("error: run ended before frame"s,
				bench.Command(R"(wait-for-frame {"appFrameId":100,"timeoutMs":5000})"));
			const auto status = bench.StatusJson();
			Assert::IsTrue(status.find("\"appFrames\"") != std::string::npos);
			Assert::IsTrue(status.find("DX12") != std::string::npos);
			bench.Quit();
			Assert::AreEqual(0, bench.GetExitCode());
		}

		TEST_METHOD(ResultJsonWithoutControl)
		{
			Assert::IsTrue(std::filesystem::exists("PresentBench2.exe"),
				L"PresentBench2.exe not found beside tests");
			const auto resultPath = std::filesystem::absolute("pb2-result-test.json");
			std::error_code ec;
			std::filesystem::remove(resultPath, ec);

			auto presenter = fixture_.LaunchPresenter({
				"--frame-count"s, "3"s,
				"--result-json"s, resultPath.string(),
			});
			Assert::IsTrue(presenter.WaitForExit(10s));
			Assert::AreEqual(0, presenter.GetExitCode());
			Assert::IsTrue(std::filesystem::exists(resultPath));
			std::ifstream in(resultPath);
			std::string json((std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
			Assert::IsTrue(json.find("\"ok\":true") != std::string::npos ||
				json.find("\"ok\": true") != std::string::npos);
			Assert::IsTrue(json.find("\"appFrames\":3") != std::string::npos ||
				json.find("\"appFrames\": 3") != std::string::npos);
			in.close();
			std::filesystem::remove(resultPath, ec);
		}

		TEST_METHOD(SetupFailureReportsFailed)
		{
			Assert::IsTrue(std::filesystem::exists("PresentBench2.exe"),
				L"PresentBench2.exe not found beside tests");
			auto bench = fixture_.LaunchPresentBench2({
				"--adapter"s, "64"s,
				"--frame-count"s, "1"s,
			});
			Assert::AreEqual("error: run ended before frame"s,
				bench.Command(R"(wait-for-frame {"appFrameId":1,"timeoutMs":5000})"));
			const auto status = bench.StatusJson();
			Assert::IsTrue(status.find("\"state\":\"Failed\"") != std::string::npos);
			bench.Quit();
			Assert::AreEqual(6, bench.GetExitCode());
		}

		TEST_METHOD(StdinCloseExitsAfterRun)
		{
			Assert::IsTrue(std::filesystem::exists("PresentBench2.exe"),
				L"PresentBench2.exe not found beside tests");
			auto bench = fixture_.LaunchPresentBench2({
				"--frame-count"s, "3"s,
			});
			Assert::AreEqual("wait-for-frame-ok"s,
				bench.Command(R"(wait-for-frame {"appFrameId":3,"timeoutMs":5000})"));
			bench.CloseInput();
			Assert::IsTrue(bench.WaitForExit(2s));
			Assert::AreEqual(0, bench.GetExitCode());
		}
	};
}
