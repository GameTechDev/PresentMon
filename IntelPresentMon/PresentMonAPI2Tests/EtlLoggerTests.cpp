// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "TestProcess.h"
#include <string>
#include <ranges>
#include "Folders.h"
#include "JobManager.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace vi = std::views;
using namespace std::literals;
using namespace pmon;


namespace EtlLoggerTests
{
	class TestFixture : public CommonTestFixture
	{
	protected:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-etllog-test-ctrl)",
				.introNsm = "pm_etllog_test_intro",
				.frameNsm = "pm_etllog_test_nsm",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "EtlLogger",
			};
			return args;
		}
	};

	TEST_CLASS(CommonFixtureTests)
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
		// verify service lifetime and status command functionality
		TEST_METHOD(ServiceStatusTest)
		{
			// verify initial status
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(0ull, status.nsmStreamedPids.size());
			Assert::AreEqual(16u, status.telemetryPeriodMs);
			Assert::IsTrue((bool)status.etwFlushPeriodMs);
			Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
		}
		// verify client lifetime
		TEST_METHOD(ClientLaunchTest)
		{
			auto client = fixture_.LaunchClient();
		}
	};

	TEST_CLASS(RoundTripLoggerTest)
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
		// verify etl can be captured and processed
		TEST_METHOD(RecordAndProcessEtl)
		{
			const auto etlFilePath = outFolder_ + "\\RecordAndProcessEtl.etl"s;
			const auto csvFilePath = outFolder_ + "\\RecordAndProcessEtl.csv"s;
			// launch target for tracking
			auto presenter = fixture_.LaunchPresenter();
			std::this_thread::sleep_for(150ms);
			// launch client
			auto client = fixture_.LaunchClient({
				"--run-time"s, "1.15"s,
				"--output-path"s, etlFilePath
			});
			// wait for completion
			client.Quit();
			// make sure .etl file was written
			Assert::IsTrue(std::filesystem::exists(etlFilePath));
			// process .etl file in opm
			fixture_.LaunchOpm({
				"--etl_file"s, etlFilePath,
				"--process_id"s, std::to_string(presenter.GetId()),
				"--output_file"s, csvFilePath,
			}).Wait();
			// verify that the csv has expected minimum size
			Logger::WriteMessage(std::format("Processed CSV size: {:.2f}kB\n",
				double(std::filesystem::file_size(csvFilePath)) / 1024.).c_str());
			Assert::IsTrue(std::filesystem::file_size(csvFilePath) > 10'000);
		}
	};
}