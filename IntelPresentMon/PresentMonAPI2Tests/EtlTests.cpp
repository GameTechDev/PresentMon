// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include <fstream>
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include "CsvHelper.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../CommonUtilities/str/String.h"
#include <string>
#include <iostream>
#include <format>
#include <filesystem>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process::v1;
namespace fs = std::filesystem;

namespace EtlTests
{
	static constexpr const char* controlPipe_ = R"(\\.\pipe\pm-etlults-ctrl)";
	static constexpr const char* introNsm_ = "pm_etlults_test_intro";
	static constexpr const char* nsmPrefix_ = "pmon_nsm_utest_";

    // Necessary data for each test case
	struct TestCaseData {
		std::string testName;
		uint32_t processId;
		std::string processName;
		std::string etlFile;
		std::wstring goldCsvFile;
		int pollCount;
		int waitTimeSecs;
		bool isExpectedFailure;
		std::string failureReason;
	};

	static const TestCaseData GOLD_TEST_CASES[] {
		{"test_case_0_10792",	10792,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_1268",	1268,	"dwm.exe",												"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_8320",	8320,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_11648",	11648,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_3976",	3976,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_11112",	11112,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_2032",	2032,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_5988",	5988,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_12268",	12268,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_0_11100",	11100,	"Presenter.exe",										"test_case_0.etl", L"test_case_0.csv",		10, 1,	false,	""},
		{"test_case_1_1564",	1564,	"dwm.exe",												"test_case_1.etl", L"test_case_1.csv",		10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_1_24560",	24560,	"Presenter.exe",										"test_case_1.etl", L"test_case_1.csv",		10, 1,	false,	""},
		{"test_case_1_24944",	24944,	"devenv.exe",											"test_case_1.etl", L"test_case_1.csv",		10, 1,	false,	""},
		{"test_case_2_1300",	1300,	"dwm.exe",												"test_case_2.etl", L"test_case_2.csv",		10, 1,	false,	""},
		{"test_case_2_10016",	10016,	"Presenter.exe",										"test_case_2.etl", L"test_case_2.csv",		10, 1,	false,	""},
		{"test_case_2_5348",	5348,	"Presenter.exe",										"test_case_2.etl", L"test_case_2.csv",		10, 1,	false,	""},
		{"test_case_2_5220",	5220,	"Presenter.exe",										"test_case_2.etl", L"test_case_2.csv",		10, 1,	false,	""},
		{"test_case_3_1252",	1252,	"dwm.exe",												"test_case_3.etl", L"test_case_3.csv",		10, 1,	false,	""},
		{"test_case_3_5892",	5892,	"dwm.exe",												"test_case_3.etl", L"test_case_3.csv",		10, 1,	false,	""},
		{"test_case_3_10112",	10112,	"Presenter.exe",										"test_case_3.etl", L"test_case_3.csv",		10, 1,	false,	""},
		{"test_case_3_12980",	12980,	"Presenter.exe",										"test_case_3.etl", L"test_case_3.csv",		10, 1,	false,	""},
		{"test_case_4_5192",	5192,	"Presenter.exe",										"test_case_4.etl", L"test_case_4.csv",		10, 1,	false,	""},
		{"test_case_4_12980",	12980,	"Presenter.exe",										"test_case_4.etl", L"test_case_4.csv",		10, 1,	false,	""},
		{"test_case_4_5236",	5236,	"Presenter.exe",										"test_case_4.etl", L"test_case_4.csv",		10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_4_8536",	8536,	"Presenter.exe",										"test_case_4.etl", L"test_case_4.csv",		10, 1,	false,	""},
		{"test_case_4_9620",	9620,	"Presenter.exe",										"test_case_4.etl", L"test_case_4.csv",		10, 1,	false,	""},
		{"test_case_4_10376",	10376,	"dwm.exe",												"test_case_4.etl", L"test_case_4.csv",		10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_5_24892",	24892,	"PresentBench.exe",										"test_case_5.etl", L"test_case_5.csv",		10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_6_10796",	10796,	"cpLauncher.exe",										"test_case_6.etl", L"test_case_6.csv",		10, 1,	false,	""},
		{"test_case_7_11320",	11320,	"cpLauncher.exe",										"test_case_7.etl", L"test_case_7.csv",		10, 1,	false,	""},
		{"test_case_8_6920",	6920,	"scimitar_engine_win64_vs2022_llvm_fusion_dx12_px.exe",	"test_case_8.etl", L"test_case_8.csv",		10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_9_10340",	10340,	"F1_24.exe",											"test_case_9.etl", L"test_case_9.csv",		10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_10_9888",	9888,	"NarakaBladepoint.exe",      							"test_case_10.etl", L"test_case_10.csv",	10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_11_1524",	1524,	"NarakaBladepoint.exe",									"test_case_11.etl", L"test_case_11.csv",	10, 1,	true,	"Expected failure - Multiple SwapChain support needed"},
		{"test_case_12_10168",	10168,	"F1_24.exe",											"test_case_12.etl", L"test_case_12.csv",	20, 5,	false,	""},
		{"test_case_13_11780",	11780,	"Dingo.Main_Win64_retail.exe",							"test_case_13.etl", L"test_case_13.csv",	10, 1,	false,	""},
	};

	TestCaseData* FindTestCaseByName(const std::string& testName) {
		auto it = std::ranges::find_if(GOLD_TEST_CASES,
			[&testName](const TestCaseData& tc) { return tc.testName == testName; });
		return it != std::end(GOLD_TEST_CASES) ? const_cast<TestCaseData*>(&*it) : nullptr;
	}

	void RunTestCaseV2(std::unique_ptr<pmapi::Session>&& pSession,
		const uint32_t& processId, const std::string& processName, CsvParser& goldCsvFile,
		std::optional<std::ofstream>& debugCsvFile, int pollCount, int waitTimeSecs) {
		using namespace std::chrono_literals;
		pmapi::ProcessTracker processTracker;
		static constexpr uint32_t numberOfBlobs = 2000;
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
				if (++emptyPollCount >= pollCount) {
					if (totalFramesValidated > 0) {
						// only finish if we have consumed at least one frame
						break;
					}
					else if (Clock::now() - start >= std::chrono::seconds(waitTimeSecs)) {
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
		std::optional<boost::process::v1::child> oChild;
	private:
		std::optional<std::string> GetAdditionalTestLocation() {
			// Check for additional test directory from environment variable
			// This allows developers to specify their own test directories without
			// modifying the source code. Set PRESENTMON_ADDITIONAL_TEST_DIR environment
			// variable or use a .runsettings.user file (see template).
			std::wstring additionalTestDir;
			wchar_t* envTestDir = nullptr;
			size_t envTestDirLen = 0;
			if (_wdupenv_s(&envTestDir, &envTestDirLen, L"PRESENTMON_ADDITIONAL_TEST_DIR") == 0 && envTestDir != nullptr) {
				additionalTestDir = envTestDir;
				free(envTestDir);
			}
			return additionalTestDir.empty() ? std::nullopt : std::make_optional<std::string>(pmon::util::str::ToNarrow(additionalTestDir));
		}

		bool SetupTestEnvironment(const std::string& etlFile, const std::string& timedStop, std::unique_ptr<pmapi::Session>& outSession)
		{
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream out;
			bp::opstream in;

			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, timedStop,
				"--control-pipe"s, controlPipe_,
				"--nsm-prefix"s, nsmPrefix_,
				"--intro-nsm"s, introNsm_,
				"--etl-test-file"s, etlFile,
				bp::std_out > out, bp::std_in < in);

			if (!pmon::util::pipe::DuplexPipe::WaitForAvailability(std::string(controlPipe_) + "-in", 500)) {
				Assert::Fail(L"Timeout waiting for service control pipe");
				return false;
			}

			try {
				pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
				pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
				outSession = std::make_unique<pmapi::Session>(controlPipe_);
				return true;
			}
			catch (const std::exception& e) {
				std::cout << "Error: " << e.what() << std::endl;
				Assert::Fail(L"Failed to connect to service via named pipe");
				return false;
			}
		}
		void RunGoldCsvTest(const TestCaseData& testCase, const std::string& goldPath, std::optional<std::ofstream>& debugCsv)
		{
			using namespace std::string_literals;

            if (testCase.isExpectedFailure) {
				Assert::Fail(pmon::util::str::ToWide(testCase.failureReason).c_str());
				return;
            }

            fs::path etlFile = fs::path(goldPath) / testCase.etlFile;
			fs::path csvPath = fs::path(goldPath) / testCase.goldCsvFile;

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(csvPath.wstring(), testCase.processId)) {
				Assert::Fail(L"Failed to open gold CSV file");
				return;
			}

			std::unique_ptr<pmapi::Session> pSession;
			if (!SetupTestEnvironment(etlFile.string(), "10000"s, pSession)) {
				goldCsvFile.Close();
				return;
			}

			RunTestCaseV2(std::move(pSession), testCase.processId, testCase.processName,
				goldCsvFile, debugCsv, testCase.pollCount, testCase.waitTimeSecs);

			goldCsvFile.Close();
		}
		void RunTestFromCase(const std::string& caseName, bool useDefault = true, bool createDebugCsv = false)
		{
			auto testCase = FindTestCaseByName(caseName);
			if (!testCase) {
				Assert::Fail(L"Test case not found");
				return;
			}

			fs::path path = useDefault ? fs::path("..") / ".." / "tests" / "gold"
				: fs::path(GetAdditionalTestLocation().value_or(""));

			std::optional<std::ofstream> debugCsv;
			if (createDebugCsv) {
				auto outputDir = path.string();
                auto debugCsvName = testCase->processName + "-debug.csv";
                debugCsv = CreateCsvFile(outputDir, debugCsvName);
			}
			RunGoldCsvTest(*testCase, path.string(), debugCsv);
			if (debugCsv.has_value()) {
				debugCsv->close();
			}
		}
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
            auto testCase = FindTestCaseByName("test_case_0_10792");
            auto etlFile = "..\\..\\tests\\gold\\" + testCase->etlFile;
			std::unique_ptr<pmapi::Session> pSession;
			auto result = SetupTestEnvironment(etlFile, "10000", pSession);
            Assert::IsTrue(result, L"SetupTestEnvironment failed");
		}
		TEST_METHOD(Tc000v2Presenter10792)
		{
			RunTestFromCase("test_case_0_10792");
		}
		TEST_METHOD(Tc000v2DWM1268)
		{
			RunTestFromCase("test_case_0_1268");
		}

		TEST_METHOD(Tc000v2Presenter8320)
		{
			RunTestFromCase("test_case_0_8320");
		}
		TEST_METHOD(Tc000v2Presenter11648)
		{
			RunTestFromCase("test_case_0_11648");
		}
		TEST_METHOD(Tc000v2Presenter3976)
		{
			RunTestFromCase("test_case_0_3976");
		}
		TEST_METHOD(Tc000v2Presenter11112)
		{
			RunTestFromCase("test_case_0_11112");
		}
		TEST_METHOD(Tc000v2Presenter2032)
		{
			RunTestFromCase("test_case_0_2032");
		}
		TEST_METHOD(Tc000v2Presenter5988)
		{
			RunTestFromCase("test_case_0_5988");
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
			RunTestFromCase("test_case_0_12268");
		}
		TEST_METHOD(Tc000v2Presenter11100)
		{
			RunTestFromCase("test_case_0_11100");
		}

		TEST_METHOD(Tc001v2Dwm1564)
		{
			RunTestFromCase("test_case_1_1564");
		}
		TEST_METHOD(Tc001v2Presenter24560)
		{
			RunTestFromCase("test_case_1_24560");
		}
		TEST_METHOD(Tc001v2devenv24944)
		{
			RunTestFromCase("test_case_1_24944");
		}
		TEST_METHOD(Tc002v2Dwm1300)
		{
			RunTestFromCase("test_case_2_1300");
		}
		TEST_METHOD(Tc002v2Presenter10016)
		{
			RunTestFromCase("test_case_2_10016");
		}
		TEST_METHOD(Tc002v2Presenter5348)
		{
			RunTestFromCase("test_case_2_5348");
		}
		TEST_METHOD(Tc002v2Presenter5220)
		{
			RunTestFromCase("test_case_2_5220");
		}
		TEST_METHOD(Tc003v2Dwm1252)
		{
			RunTestFromCase("test_case_3_1252");
		}
		TEST_METHOD(Tc003v2Presenter5892)
		{
			RunTestFromCase("test_case_3_5892");
		}
		TEST_METHOD(Tc003v2Presenter10112)
		{
			RunTestFromCase("test_case_3_10112");
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
			RunTestFromCase("test_case_3_12980");
		}
		TEST_METHOD(Tc004v2Presenter5192)
		{
			RunTestFromCase("test_case_4_5192", true, true);
		}
		TEST_METHOD(Tc004v2Presenter5236)
		{
			RunTestFromCase("test_case_4_5236");
		}
		TEST_METHOD(Tc004v2Presenter8536)
		{
			RunTestFromCase("test_case_4_8536");
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
			RunTestFromCase("test_case_4_9620");
		}
		TEST_METHOD(Tc004v2Dwm10376)
		{
			RunTestFromCase("test_case_4_10376");
		}
		TEST_METHOD(Tc005v2PresentBench24892)
		{
			RunTestFromCase("test_case_5_24892");
		}
		TEST_METHOD(Tc006CP2077)
		{	
			RunTestFromCase("test_case_6_10796", false);
		}
		TEST_METHOD(Tc007CP2077)
		{
			RunTestFromCase("test_case_7_11320", false);
		}
		TEST_METHOD(Tc008ACShadows)
		{
            RunTestFromCase("test_case_8_6920", false);
		}
		TEST_METHOD(Tc009F124)
		{
			RunTestFromCase("test_case_9_10340", false);
		}
		TEST_METHOD(Tc010NarakaBladepoint)
		{
			RunTestFromCase("test_case_10_9888", false);
		}
		TEST_METHOD(Tc011NarakaBladepoint)
		{
			RunTestFromCase("test_case_11_1524", false);
		}
		TEST_METHOD(Tc012F124)
		{
			RunTestFromCase("test_case_12_10168", false);
		}
		TEST_METHOD(Tc013Dingo)
		{
			RunTestFromCase("test_case_13_11780", false);
		}
	};
}