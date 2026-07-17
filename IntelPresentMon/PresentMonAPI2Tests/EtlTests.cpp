// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include <fstream>
#include "CppUnitTest.h"
#include "Folders.h"
#include "StatusComparison.h"
#include "TestProcess.h"
#include "CsvHelper.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../CommonUtilities/str/String.h"
#include <string>
#include <format>
#include <filesystem>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;

namespace EtlTests
{
	static constexpr const char* controlPipe_ = R"(\\.\pipe\test-pipe-pmsvc-2)";
	static constexpr const char* shmNamePrefix = "pm_etl_test_shm";
	static constexpr const char* testCasesCsvPath_ =
		R"(..\..\IntelPresentMon\PresentMonAPI2Tests\test_cases.csv)";

	// Test case data structure - loaded from CSV file
	struct TestCaseData {
		std::string testName;
		uint32_t processId;
		std::string processName;
		std::string etlFile;
		std::wstring goldCsvFile;
		int pollCount;
		int waitTimeSecs;
		bool useAdditionalTestLocation;   // Load from additional test directory (runsettings)
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
	// CSV Format: TestName,ProcessID,ProcessName,EtlFile,GoldCsvFile,PollCount,WaitTimeSecs,IsExpectedFailure,FailureReason,UseAdditionalTestLocation,RunTest
	std::vector<TestCaseData> LoadTestCasesFromCsv(const std::string& csvFilePath) {
		std::vector<TestCaseData> testCases;
		
		// Convert to absolute path for better error reporting
		fs::path absolutePath = fs::absolute(csvFilePath);
		
		std::ifstream file(absolutePath);
        auto fileOpenResult = file.is_open();
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

		if (!file.good()) {
			throw std::runtime_error(std::format(
				"File opened but stream is in bad state:\n"
				"  Path: {}",
				absolutePath.string()));
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
				if (fields.size() < 11) {
					throw std::runtime_error(std::format(
						"Line {}: Expected at least 11 fields, got {}",
						lineNumber, fields.size()));
				}

				TestCaseData testCase;
				testCase.testName = fields[0];
				testCase.processId = std::stoul(fields[1]);
				testCase.processName = fields[2];
				testCase.etlFile = fields[3];
				testCase.goldCsvFile = pmon::util::str::ToWide(fields[4]);
				testCase.pollCount = std::stoi(fields[5]);
				// this is now maximum wait time, not absolute, so no more need to tune this per test
				testCase.waitTimeSecs = 5;
				testCase.useAdditionalTestLocation = ParseBool(fields[9]);
				testCase.runTest = ParseBool(fields[10]);

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
		std::optional<std::ofstream>& outputCsvFile, int pollCount, int waitTimeSecs) {
		using namespace std::chrono_literals;
		pmapi::ProcessTracker processTracker;
		static constexpr uint32_t numberOfBlobs = 2000;

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

		processTracker = pSession->TrackProcess(processId, true, true);

		const auto firstFrameDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(waitTimeSecs);
		bool gotFirstFrame = false;
		while (std::chrono::steady_clock::now() < firstFrameDeadline) {
			frameQuery.Consume(processTracker, blobs);
			if (blobs.GetNumBlobsPopulated() > 0) {
				gotFirstFrame = true;
				goldCsvFile.VerifyBlobAgainstCsv(processName, processId, queryElements, blobs, outputCsvFile);
				break;
			}
			std::this_thread::sleep_for(8ms);
		}
		if (!gotFirstFrame) {
			throw CsvException("Timeout waiting to consume first frame");
		}

		int emptyPollCount = 0;
		while (1) {
			frameQuery.Consume(processTracker, blobs);
			if (blobs.GetNumBlobsPopulated() == 0) {
				// if we poll 10 times in a row and get no new frames, consider this ETL finished
				if (++emptyPollCount >= pollCount) {
					break;
				}
				std::this_thread::sleep_for(8ms);
			}
			else {
				emptyPollCount = 0;
				goldCsvFile.VerifyBlobAgainstCsv(processName, processId, queryElements, blobs, outputCsvFile);
			}
		}
		// Drain the backpressured playback ring so the service output thread cannot
		// block indefinitely during session teardown.
		int drainEmptyPollCount = 0;
		while (drainEmptyPollCount < pollCount) {
			frameQuery.Consume(processTracker, blobs);
			if (blobs.GetNumBlobsPopulated() == 0) {
				if (++drainEmptyPollCount >= pollCount) {
					break;
				}
				std::this_thread::sleep_for(8ms);
			}
			else {
				drainEmptyPollCount = 0;
				goldCsvFile.VerifyBlobAgainstCsv(processName, processId, queryElements, blobs, outputCsvFile);
			}
		}
		processTracker.FlushFrames();
	}

	TEST_CLASS(GoldEtlCsvTests)
	{
	private:
		class EtlTestFixture : public CommonTestFixture
		{
		public:
			const CommonProcessArgs& GetCommonArgs() const override
			{
				return commonArgs_;
			}
		private:
			CommonProcessArgs commonArgs_{
				controlPipe_,
				shmNamePrefix,
				"debug",
				std::nullopt,
				logFolder_,
				"frames",
			};
		};

		EtlTestFixture fixture_;

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

			fixture_.Setup({
				"--timed-stop"s, timedStop,
				"--etl-test-file"s, etlFile,
			});

			try {
				outSession = std::make_unique<pmapi::Session>(controlPipe_);
				return true;
			}
			catch (const std::exception& e) {
				Logger::WriteMessage(std::format("Error: {}\n", e.what()).c_str());
				Assert::Fail(L"Failed to connect to service via named pipe");
				CleanupTestEnvironment();
				return false;
			}
		}
		void CleanupTestEnvironment()
		{
			fixture_.Cleanup();
		}
		TestCaseData LoadTestCaseFromCsvRow(const std::string& csvFilePath, size_t rowIndex)
		{
			auto testCases = LoadTestCasesFromCsv(csvFilePath);
			if (rowIndex >= testCases.size()) {
				throw std::runtime_error(std::format(
					"CSV row index {} is out of range for {} test cases",
					rowIndex, testCases.size()));
			}
			return testCases[rowIndex];
		}
		fs::path GetTestPath(const TestCaseData& testCase)
		{
			return testCase.useAdditionalTestLocation
				? fs::path(GetAdditionalTestLocation().value_or(R"(..\..\Tests\AuxData\Data)"))
				: fs::path("..") / ".." / "tests" / "gold";
		}
		std::ofstream CreateOutputCsv(const TestCaseData& testCase)
		{
			auto outputDir = std::string{ outFolder_ };
			auto outputCsvName = testCase.testName + "-actual";
			auto outputCsv = CreateCsvFile(outputDir, outputCsvName);
			if (!outputCsv.has_value()) {
				Assert::Fail(pmon::util::str::ToWide(std::format(
					"Failed to create ETL output CSV in {}", outFolder_)).c_str());
			}
			Logger::WriteMessage(std::format("ETL output CSV folder: {}\n",
				fs::absolute(outFolder_).string()).c_str());
			return std::move(*outputCsv);
		}
		// Returns true if test passed, false if test failed
		// When throwOnFailure=false, returns status instead of throwing
		// When throwOnFailure=true, throws Assert::Fail on failure
		bool RunGoldCsvTest(const TestCaseData& testCase, const std::string& goldPath, std::optional<std::ofstream>& outputCsv, bool throwOnFailure = true)
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
			try {
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
			}
			catch (...) {
				CleanupTestEnvironment();
				throw;
			}

			// Track if we encountered an assertion failure
			bool testPassed = true;
			std::string exceptionMessage;

			try {
				RunTestCaseV2(std::move(pSession), testCase.processId, testCase.processName,
					goldCsvFile, outputCsv, testCase.pollCount, testCase.waitTimeSecs);
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

			pSession.reset();
			goldCsvFile.Close();
			CleanupTestEnvironment();

			if (!testPassed) {
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

		void RunTestCaseFromCsvRow(const std::string& csvFilePath, size_t rowIndex)
		{
			TestCaseData testCase;
			try {
				testCase = LoadTestCaseFromCsvRow(csvFilePath, rowIndex);
			}
			catch (const std::exception& e) {
				Assert::Fail(pmon::util::str::ToWide(
					std::format("Failed to load test cases CSV: {}", e.what())).c_str());
				return;
			}

			if (!testCase.runTest) {
				Logger::WriteMessage(std::format("[SKIP] {} (RunTest=false)\n",
					testCase.testName).c_str());
				return;
			}

			Logger::WriteMessage(std::format("\n=== Running ETL CSV Test {}: {} ===\n",
				rowIndex + 1, testCase.testName).c_str());

			auto testPath = GetTestPath(testCase);
			std::optional<std::ofstream> outputCsv;
			outputCsv.emplace(CreateOutputCsv(testCase));

			try {
				RunGoldCsvTest(testCase, testPath.string(), outputCsv);
			}
			catch (...) {
				CleanupTestEnvironment();
				throw;
			}

			if (outputCsv.has_value()) {
				outputCsv->close();
			}
		}
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
			CleanupTestEnvironment();
		}
		TEST_METHOD(OpenCsvTest)
		{
			const auto goldCsvName = L"..\\..\\tests\\gold\\test_case_0.csv";
			CsvParser goldCsvFile;
			goldCsvFile.Open(goldCsvName, 1268);
			goldCsvFile.Close();
		}

#if __has_include("EtlCsvTestCases.g.h")
#include "EtlCsvTestCases.g.h"
#endif

		TEST_METHOD(OpenServiceTest)
		{
			const auto etlName = "..\\..\\tests\\gold\\test_case_0.etl";
			std::unique_ptr<pmapi::Session> pSession;
			Assert::IsTrue(SetupTestEnvironment(etlName, "10000", pSession),
				L"SetupTestEnvironment failed");
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
