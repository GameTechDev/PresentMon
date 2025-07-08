// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include <fstream>
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include <boost/process.hpp>
#include "CsvHelper.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include <string>
#include <iostream>
#include <format>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EtlTests
{
	void RunTestCaseV2(std::unique_ptr<pmapi::Session>&& pSession,
		const uint32_t& processId, const std::string& processName, CsvParser& goldCsvFile,
		std::optional<std::ofstream>& debugCsvFile) {
		using namespace std::chrono_literals;
		pmapi::ProcessTracker processTracker;
		static constexpr uint32_t numberOfBlobs = 10000u;
		uint32_t totalFramesValidated = 0;

		PM_QUERY_ELEMENT queryElements[]{
			//{ PM_METRIC_APPLICATION, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_SWAP_CHAIN_ADDRESS, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_SYNC_INTERVAL, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_PRESENT_FLAGS, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_ALLOWS_TEARING, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_PRESENT_MODE, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_FRAME_TYPE, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_PRESENT_START_QPC, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_BETWEEN_SIMULATION_START, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_BETWEEN_PRESENTS, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_BETWEEN_DISPLAY_CHANGE, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_IN_PRESENT_API, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_RENDER_PRESENT_LATENCY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_UNTIL_DISPLAYED, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_PC_LATENCY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_CPU_START_QPC, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_BETWEEN_APP_START, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_CPU_BUSY, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_CPU_WAIT, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_GPU_LATENCY, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_GPU_TIME, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_GPU_BUSY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_GPU_WAIT, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_ANIMATION_ERROR, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_ANIMATION_TIME, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_FLIP_DELAY, PM_STAT_NONE, 0, 0 },
			{ PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_CLICK_TO_PHOTON_LATENCY, PM_STAT_NONE, 0, 0},
			{ PM_METRIC_INSTRUMENTED_LATENCY, PM_STAT_NONE, 0, 0 },
		};

		auto frameQuery = pSession->RegisterFrameQuery(queryElements);
		auto blobs = frameQuery.MakeBlobContainer(numberOfBlobs);

		processTracker = pSession->TrackProcess(processId);

		using Clock = std::chrono::high_resolution_clock;
		const auto start = Clock::now();

		int emptyPollCount = 0;
		while (1) {
			frameQuery.Consume(processTracker, blobs);
			if (blobs.GetNumBlobsPopulated() == 0) {
				// if we poll 10 times in a row and get no new frames, consider this ETL finished
				if (++emptyPollCount >= 10) {
					if (totalFramesValidated > 0) {
						// only finish if we have consumed at least one frame
						break;
					}
					else if (Clock::now() - start >= 1s) {
						// if it takes longer than 1 second to consume the first frame, throw failure
						Assert::Fail(L"Timeout waiting to consume first frame");
					}
				}
				std::this_thread::sleep_for(8ms);
			}
			else {
				emptyPollCount = 0;
				goldCsvFile.VerifyBlobAgainstCsv(processName, processId, queryElements, blobs, debugCsvFile);
				totalFramesValidated += blobs.GetNumBlobsPopulated();
			}
		}
	}

	TEST_CLASS(GoldEtlCsvTests)
	{
		std::optional<boost::process::child> oChild;
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
			if (oChild) {
				oChild->terminate();
				oChild->wait();
				oChild.reset();
			}
			// sleep after every test to ensure that named pipe is no longer available
			using namespace std::literals;
			std::this_thread::sleep_for(50ms);
		}

		TEST_METHOD(OpenCsvTest)
		{
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";
			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, 1268);
			goldCsvFile.Close();
		}

		TEST_METHOD(OpenServiceTest)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(500ms);

			Assert::IsTrue(oChild->running());
		}
		TEST_METHOD(OpenMockSessionTest)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");
			
			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}
		}

		TEST_METHOD(ConsumeBlobsTest)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10792;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";

			oChild.emplace("PresentMonService.exe"s,
				//"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			pmapi::ProcessTracker processTracker;
			uint32_t totalFramesValidated = 0;

			PM_QUERY_ELEMENT queryElements[]{
				{ PM_METRIC_BETWEEN_PRESENTS, PM_STAT_NONE, 0, 0},
			};

			auto frameQuery = pSession->RegisterFrameQuery(queryElements);
			auto blobs = frameQuery.MakeBlobContainer(8);

			processTracker = pSession->TrackProcess(processId);

			using Clock = std::chrono::high_resolution_clock;
			const auto start = Clock::now();

			while (1) {
				frameQuery.Consume(processTracker, blobs);
				if (blobs.GetNumBlobsPopulated() == 0) {
					if (Clock::now() - start >= 1s) {
						// if it takes longer than 1 second to consume the first frame, throw failure
						Assert::Fail(L"Timeout waiting to consume first frame");
					}
					std::this_thread::sleep_for(8ms);
				}
				else {
					break;
				}
			}
		}
		TEST_METHOD(Tc000v2Presenter10792)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10792;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				//"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2DWM1268)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1268;
			const std::string processName = "dwm.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter8320)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 8320;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter11648)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11648;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter3976)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 3976;
			const std::string processName = "Presenter.exe";

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input
			std::optional<std::ofstream> debugCsv; // Empty optional

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter11112)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11112;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter2032)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 2032;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter5988)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5988;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter12268)
		{
			// This test is a sporadic failure due to timing of when the ETL session is
			// finishedby the the mock presentmon session. If the ETL session finishes
			// and sets the process id to not active from the mock presentmon session
			// when the middleware is starting to process the NSM it will determine
			// the process is not active and exit. Need to add some type of synchronization
			// in mock presentmon session to not shutdown the session until notified
			// by close session call.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 12268;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc000v2Presenter11100)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11100;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}

		TEST_METHOD(Tc001v2Dwm1564)
		{
			Assert::AreEqual(true, false, L"*** Expected Failure. WIP.");
			// This test is an expected failure. The reason for the failure is the
			// mock presentmon session is writing a present to the nsm that is
			// earlier than the console application allows because of swap chain
			// initialization that is not implemented in the mock presentmon session.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1564;
			const std::string processName = "dwm.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc001v2Presenter24560)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 24560;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc001v2devenv24944)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 24944;
			const std::string processName = "devenv.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_1.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_1.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc002v2Dwm1300)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1300;
			const std::string processName = "dwm.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_2.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_2.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc002v2Presenter10016)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10016;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_2.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_2.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc002v2Presenter5348)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5348;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_2.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_2.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc002v2Presenter5220)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5220;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_2.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_2.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc003v2Dwm1252)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 1252;
			const std::string processName = "dwm.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_3.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_3.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc003v2Presenter5892)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5892;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_3.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_3.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc003v2Presenter10112)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10112;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_3.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_3.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc003v2Presenter12980)
		{
			// This test is a sporadic failure due to timing of when the ETL session is
			// finishedby the the mock presentmon session. If the ETL session finishes
			// and sets the process id to not active from the mock presentmon session
			// when the middleware is starting to process the NSM it will determine
			// the process is not active and exit. Need to add some type of synchronization
			// in mock presentmon session to not shutdown the session until notified
			// by close session call.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 12980;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_3.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_3.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc004v2Presenter5192)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5192;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_4.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_4.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc004v2Presenter5236)
		{
			Assert::AreEqual(true, false, L"*** Expected Failure. WIP.");
			// Expected failure due to incorrect swap chain handling by middleware.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 5236;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_4.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_4.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc004v2Presenter8536)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 8536;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_4.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_4.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc004v2Presenter9620)
		{
			// This test is a sporadic failure due to timing of when the ETL session is
			// finishedby the the mock presentmon session. If the ETL session finishes
			// and sets the process id to not active from the mock presentmon session
			// when the middleware is starting to process the NSM it will determine
			// the process is not active and exit. Need to add some type of synchronization
			// in mock presentmon session to not shutdown the session until notified
			// by close session call.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 9620;
			const std::string processName = "Presenter.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_4.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_4.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc004v2Dwm10376)
		{
			Assert::AreEqual(true, false, L"*** Expected Failure. WIP.");
			// Expected failure due to incorrect swap chain handling by middleware.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10376;
			const std::string processName = "dwm.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_4.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_4.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc005v2PresentBench24892)
		{
			Assert::AreEqual(true, false, L"*** Expected Failure. WIP.");
			// Expected failure due to incorrect swap chain handling by middleware.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 24892;
			const std::string processName = "PresentBench.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "..\\..\\tests\\gold\\test_case_5.etl";
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_5.csv";

			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, processId);

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc006v2CPXellOn10796Ext)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10796;
			const std::string processName = "cpLauncher.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_6.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_6.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc007v2CPXellOnFgOn11320Ext)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 11320;
			const std::string processName = "cpLauncher.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_7.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_7.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc008v2ACSXellOnFgOn6920Ext)
		{
			Assert::AreEqual(true, false, L"*** Expected Failure. WIP.");
			// This test is an expected failure. The reason for the failure is the
			// mock presentmon session is writing a present to the nsm that is
			// earlier than the console application allows because of swap chain
			// initialization that is not implemented in the mock presentmon session.
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 6920;
			const std::string processName = "scimitar_engine_win64_vs2022_llvm_fusion_dx12_px.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_8.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_8.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
		}
		TEST_METHOD(Tc009v2F124XellOnFgOn10340Ext)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 10340;
			const std::string processName = "F1_24.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_9.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_9.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}
			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
			if (debugCsv.has_value()) {
				debugCsv->close();
			}
		}
		TEST_METHOD(Tc010MarvelOnNvPcl1FgOnExt)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 42132;
			const std::string processName = "Marvel-Win64-Shipping.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_10.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_10.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
				L"Timeout waiting for service control pipe");

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
			if (debugCsv.has_value()) {
				debugCsv->close();
			}
		}
		TEST_METHOD(Tc011CP2077Pcl2FgOffRelexOffExt)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 12524;
			const std::string processName = "Cyberpunk2077.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_11.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_11.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}
	
			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
			if (debugCsv.has_value()) {
				debugCsv->close();
			}
		}
		TEST_METHOD(Tc012MarvelOnNvPcl3FgOnAutoReflexOnFrameDelayExt)
		{
			namespace bp = boost::process;
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			const uint32_t processId = 24412;
			const std::string processName = "Marvel-Win64-Shipping.exe";
			std::optional<std::ofstream> debugCsv; // Empty optional

			bp::ipstream out; // Stream for reading the process's output
			bp::opstream in;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etlName = "F:\\EtlTesting\\test_case_12.etl";
			const auto goldCsvName = L"F:\\EtlTesting\\test_case_12.csv";

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(goldCsvName, processId)) {
				return;
			}

            std::string folder = "F:\\EtlTesting\\ETLDebugging\\testcase12\\"s;
			std::string csvName = "debug.csv"s;
			debugCsv = CreateCsvFile(folder,csvName);

			oChild.emplace("PresentMonService.exe"s,
				//"--timed-stop"s, "60000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etl-test-file"s, etlName,
				bp::std_out > out, bp::std_in < in);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			RunTestCaseV2(std::move(pSession), processId, processName, goldCsvFile, debugCsv);
			goldCsvFile.Close();
			if (debugCsv.has_value()) {
				debugCsv->close();
			}
		}
	};
}