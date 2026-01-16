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

	// Test case data structure - loaded from CSV file
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
		bool useAdditionalTestLocation;  // Load from additional test directory (runsettings)
		bool produceDebugCsv;             // Generate debug CSV output
		bool runTest;                     // Whether to run this test (for selective debugging)
	};

	// Helper function to parse boolean from CSV string
	bool ParseBool(const std::string& value) {
		std::string lower = value;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		return lower == "true" || lower == "1" || lower == "yes" || lower == "y";
	}

	// Helper function to trim whitespace from string
	std::string Trim(const std::string& str) {
		const auto start = str.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) return "";
		const auto end = str.find_last_not_of(" \t\r\n");
		return str.substr(start, end - start + 1);
	}

	// Parse a CSV line handling quoted fields
	std::vector<std::string> ParseCsvLine(const std::string& line) {
		std::vector<std::string> fields;
		std::string field;
		bool inQuotes = false;
		
		for (size_t i = 0; i < line.length(); ++i) {
			char c = line[i];
			
			if (c == '"') {
				inQuotes = !inQuotes;
			}
			else if (c == ',' && !inQuotes) {
				fields.push_back(Trim(field));
				field.clear();
			}
			else {
				field += c;
			}
		}
		fields.push_back(Trim(field));
		return fields;
	}

	// Load test cases from CSV file
	// CSV Format: TestName,ProcessID,ProcessName,EtlFile,GoldCsvFile,PollCount,WaitTimeSecs,IsExpectedFailure,FailureReason,UseAdditionalTestLocation,ProduceDebugCsv,RunTest
	std::vector<TestCaseData> LoadTestCasesFromCsv(const std::string& csvFilePath) {
		std::vector<TestCaseData> testCases;
		
		// Convert to absolute path for better error reporting
		fs::path absolutePath = fs::absolute(csvFilePath);
		
		std::ifstream file(csvFilePath);
		if (!file.is_open()) {
			throw std::runtime_error(std::format(
				"Failed to open test cases CSV file:\n"
				"  Requested path: {}\n"
				"  Absolute path: {}\n"
				"  Current directory: {}",
				csvFilePath, 
				absolutePath.string(),
				fs::current_path().string()));
		}

		std::string line;
		bool isFirstLine = true;
		size_t lineNumber = 0;

		while (std::getline(file, line)) {
			lineNumber++;
			
			// Skip empty lines
			if (Trim(line).empty()) {
				continue;
			}

			// Skip header line
			if (isFirstLine) {
				isFirstLine = false;
				continue;
			}

			try {
				auto fields = ParseCsvLine(line);
				
				// Validate field count
				if (fields.size() < 12) {
					throw std::runtime_error(std::format(
						"Line {}: Expected at least 12 fields, got {}",
						lineNumber, fields.size()));
				}

				TestCaseData testCase;
				testCase.testName = fields[0];
				testCase.processId = std::stoul(fields[1]);
				testCase.processName = fields[2];
				testCase.etlFile = fields[3];
				testCase.goldCsvFile = pmon::util::str::ToWide(fields[4]);
				testCase.pollCount = std::stoi(fields[5]);
				testCase.waitTimeSecs = std::stoi(fields[6]);
				testCase.isExpectedFailure = ParseBool(fields[7]);
				testCase.failureReason = fields[8];
				testCase.useAdditionalTestLocation = ParseBool(fields[9]);
				testCase.produceDebugCsv = ParseBool(fields[10]);
				testCase.runTest = ParseBool(fields[11]);

				testCases.push_back(testCase);
			}
			catch (const std::exception& e) {
				throw std::runtime_error(std::format(
					"Error parsing line {}: {}", lineNumber, e.what()));
			}
		}

		if (testCases.empty()) {
			throw std::runtime_error("No test cases loaded from CSV file");
		}

		return testCases;
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
						// if it takes longer than alloted test time to consume the first frame, throw failure
						throw CsvException("Timeout waiting to consume first frame");
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

			if (!pmon::util::pipe::DuplexPipe::WaitForAvailability(std::string(controlPipe_), 500)) {
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
		// Returns true if test passed, false if test failed
		// When throwOnFailure=false, returns status instead of throwing
		// When throwOnFailure=true, throws Assert::Fail on failure
		bool RunGoldCsvTest(const TestCaseData& testCase, const std::string& goldPath, std::optional<std::ofstream>& debugCsv, bool throwOnFailure = true)
		{
			using namespace std::string_literals;

			fs::path etlFile = fs::path(goldPath) / testCase.etlFile;
			fs::path csvPath = fs::path(goldPath) / testCase.goldCsvFile;

			CsvParser goldCsvFile;
			if (!goldCsvFile.Open(csvPath.wstring(), testCase.processId)) {
				if (throwOnFailure) {
					Assert::Fail(L"Failed to open gold CSV file");
				}
				else {
					throw std::runtime_error("Failed to open gold CSV file");
				}
				return false;
			}

			std::unique_ptr<pmapi::Session> pSession;
			if (!SetupTestEnvironment(etlFile.string(), "10000"s, pSession)) {
				goldCsvFile.Close();
				if (throwOnFailure) {
					Assert::Fail(L"Failed to setup test environment");
				}
				else {
					throw std::runtime_error("Failed to setup test environment");
				}
				return false;
			}

			// Track if we encountered an assertion failure
			bool testPassed = true;
			std::string exceptionMessage;

			try {
				RunTestCaseV2(std::move(pSession), testCase.processId, testCase.processName,
					goldCsvFile, debugCsv, testCase.pollCount, testCase.waitTimeSecs);
			}
			catch (const CsvValidationException& e) {
				testPassed = false;
				exceptionMessage = std::format(
					"CSV Validation Error:\n"
					"  Column: {}\n"
					"  Line: {}\n"
					"  Details: {}",
					GetHeaderString(e.GetColumnId()),
					e.GetLine(),
					e.what());
				Logger::WriteMessage(std::format("[ERROR] {}\n", exceptionMessage).c_str());
			}
			catch (const CsvConversionException& e) {
				testPassed = false;
				exceptionMessage = std::format(
					"CSV Conversion Error:\n"
					"  Column: {}\n"
					"  Line: {}\n"
					"  Invalid Value: '{}'\n"
					"  Details: {}",
					GetHeaderString(e.GetColumnId()),
					e.GetLine(),
					e.GetValue(),
					e.what());
				Logger::WriteMessage(std::format("[ERROR] {}\n", exceptionMessage).c_str());
			}
			catch (const CsvFileException& e) {
				testPassed = false;
				exceptionMessage = std::format("CSV File Error: {}", e.what());
				Logger::WriteMessage(std::format("[ERROR] {}\n", exceptionMessage).c_str());
			}
			catch (const CsvException& e) {
				testPassed = false;
				exceptionMessage = std::format("CSV Error: {}", e.what());
				Logger::WriteMessage(std::format("[ERROR] {}\n", exceptionMessage).c_str());
			}
			catch (const std::exception& e) {
				testPassed = false;
				exceptionMessage = std::format("Unexpected Error: {}", e.what());
				Logger::WriteMessage(std::format("[ERROR] {}\n", exceptionMessage).c_str());
			}
			catch (...) {
				testPassed = false;
				exceptionMessage = "Unknown exception caught";
				Logger::WriteMessage(std::format("[ERROR] {}\n", exceptionMessage).c_str());
			}

			goldCsvFile.Close();

			// Now handle expected failure logic
			if (testCase.isExpectedFailure) {
				if (testPassed) {
					// Test passed but was expected to fail - this is noteworthy!
					if (throwOnFailure) {
						Logger::WriteMessage(std::format(
							"[PASS] UNEXPECTED PASS: Test '{}' passed but was marked as expected failure!\n"
							"  Expected failure reason: {}\n"
							"  ACTION: Update GOLD_TEST_CASES to set isExpectedFailure = false\n",
							testCase.testName, testCase.failureReason).c_str());
					}
					// Return true because test technically passed (even though unexpected)
					return true;
				}
				else {
					// Test failed as expected
					if (throwOnFailure) {
						// For individual test methods, log and assert
						Logger::WriteMessage(std::format(
							"[FAIL] Expected failure: {}\n",
							testCase.failureReason).c_str());
						Assert::Fail(std::format(L"[EXPECTED FAILURE] {}",
							pmon::util::str::ToWide(testCase.failureReason)).c_str());
					}
					// Return false to indicate test failed (even though expected)
					return false;
				}
			}
			else if (!testPassed) {
				// Unexpected failure
				if (throwOnFailure) {
					Assert::Fail(pmon::util::str::ToWide(exceptionMessage).c_str());
				}
				else {
					// For CSV runner, throw std::runtime_error so it can be caught and counted
					throw std::runtime_error(exceptionMessage);
				}
				return false;
			}
			// Test passed and wasn't expected to fail - all good!
			return true;
		}

		// Run all test cases from a CSV file
		void RunTestsFromCsv(const std::string& csvFilePath)
		{
			// Load test cases from CSV
			std::vector<TestCaseData> testCases;
			try {
				testCases = LoadTestCasesFromCsv(csvFilePath);
				Logger::WriteMessage(std::format("Loaded {} test cases from {}\n", 
					testCases.size(), csvFilePath).c_str());
			}
			catch (const std::exception& e) {
				Assert::Fail(pmon::util::str::ToWide(
					std::format("Failed to load test cases CSV: {}", e.what())).c_str());
				return;
			}

			// Statistics
			int totalTests = 0;
			int passedTests = 0;
			int failedTests = 0;
			int expectedFailures = 0;
			int skippedTests = 0;
			std::vector<std::string> failureDetails;

			// Run each test case
			for (const auto& testCase : testCases) {
				// Skip if RunTest is false
				if (!testCase.runTest) {
					skippedTests++;
					Logger::WriteMessage(std::format("[SKIP] {} (RunTest=false)\n", 
						testCase.testName).c_str());
					continue;
				}

				totalTests++;
				Logger::WriteMessage(std::format("\n=== Running Test {}/{}: {} ===\n", 
					totalTests, testCases.size() - skippedTests, testCase.testName).c_str());

				// Determine test location
				fs::path testPath = testCase.useAdditionalTestLocation
					? fs::path(GetAdditionalTestLocation().value_or(""))
					: fs::path("..") / ".." / "tests" / "gold";

				// Prepare debug CSV if requested
				std::optional<std::ofstream> debugCsv;
				if (testCase.produceDebugCsv) {
					auto outputDir = testPath.string();
					auto debugCsvName = testCase.testName + "-debug";
					debugCsv = CreateCsvFile(outputDir, debugCsvName);
					if (debugCsv.has_value()) {
						Logger::WriteMessage(std::format("  Producing debug CSV: {}-debug.csv\n", 
							testCase.testName).c_str());
					}
				}

				// Run the test
				bool testPassed = false;
				std::string errorMessage;

				try {
					testPassed = RunGoldCsvTest(testCase, testPath.string(), debugCsv, false); // Returns true/false instead of throwing
				}
				catch (const std::exception& e) {
					// Only unexpected failures throw exceptions
					testPassed = false;
					errorMessage = e.what();
				}

				// Handle the result
				if (testPassed) {
					if (testCase.isExpectedFailure) {
						// Test passed but was expected to fail - this is noteworthy!
						Logger::WriteMessage(std::format(
							"[UNEXPECTED PASS] Test passed but was marked as expected failure!\n"
							"  Expected failure reason: {}\n"
							"  ACTION: Update CSV to set IsExpectedFailure = false\n",
							testCase.failureReason).c_str());
					}
					else {
						Logger::WriteMessage(std::format("[PASS] {}\n", testCase.testName).c_str());
					}
					passedTests++;
				}
				else {
					// Test failed
					if (testCase.isExpectedFailure) {
						// Test failed as expected
						expectedFailures++;
						Logger::WriteMessage(std::format(
							"[EXPECTED FAIL] {}\n  Reason: {}\n",
							testCase.testName, testCase.failureReason).c_str());
					}
					else {
						// Unexpected failure
						failedTests++;
						std::string detail = std::format("[FAIL] {}: {}", 
							testCase.testName, errorMessage.empty() ? "Test failed" : errorMessage);
						failureDetails.push_back(detail);
						Logger::WriteMessage(std::format("{}\n", detail).c_str());
					}
				}

				// Close debug CSV if it was created
				if (debugCsv.has_value()) {
					debugCsv->close();
				}
			}

			// Print summary
			Logger::WriteMessage(std::format(
				"\n========================================\n"
				"Test Summary\n"
				"========================================\n"
				"Total Test Cases in CSV: {}\n"
				"Skipped (RunTest=false): {}\n"
				"Tests Run: {}\n"
				"  Passed: {}\n"
				"  Failed (Unexpected): {}\n"
				"  Failed (Expected): {}\n"
				"========================================\n",
				testCases.size(), skippedTests, totalTests, 
				passedTests, failedTests, expectedFailures).c_str());

			// Fail the overall test if there were unexpected failures
			if (failedTests > 0) {
				std::string summary = std::format(
					"\n{} of {} tests failed unexpectedly:\n\n", 
					failedTests, totalTests);
				for (const auto& detail : failureDetails) {
					summary += detail + "\n";
				}
				Assert::Fail(pmon::util::str::ToWide(summary).c_str());
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

		// Example: Run all tests from CSV file
		// This single test will run all test cases defined in the CSV
		// Use the RunTest column in CSV to selectively enable/disable tests
		TEST_METHOD(RunAllTestsFromCsv)
		{
			// CSV file is in the PresentMonAPI2Tests source directory
			// Working dir is build/Debug, so go up to source tree
			RunTestsFromCsv("..\\..\\IntelPresentMon\\PresentMonAPI2Tests\\test_cases.csv");
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
			// Simple test to verify we can create a session with an ETL file
			const auto etlFile = "..\\..\\tests\\gold\\test_case_0.etl";
			std::unique_ptr<pmapi::Session> pSession;
			auto result = SetupTestEnvironment(etlFile, "10000", pSession);
			Assert::IsTrue(result, L"SetupTestEnvironment failed");
		}
	};
}