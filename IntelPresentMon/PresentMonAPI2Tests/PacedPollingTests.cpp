// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/str/String.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include <vincentlaucsb-csv-parser/csv.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <format>
#include <fstream>
#include <filesystem>
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;
using namespace pmon;

namespace PacedPolling
{
	class TestFixture : public CommonTestFixture
	{
	public:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-paced-polling-test-ctrl)",
				.shmNamePrefix = "pm_paced_polling_test_intro",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "PacedPlayback",
			};
			return args;
		}
	};

	struct CsvData
	{
		std::vector<std::string> header;
		std::vector<std::vector<double>> rows;
	};

	CsvData LoadRunFromCsv(const std::string& path)
	{
		csv::CSVReader gold{ path };
		CsvData data;
		data.header = gold.get_col_names();
		for (auto& row : gold) {
			std::vector<double> rowData;
			rowData.reserve(row.size());
			for (auto& field : row) {
				rowData.push_back(field.get<double>());
			}
			data.rows.push_back(std::move(rowData));
		}
		return data;
	}

	bool ValuesMatch(double expected, double actual)
	{
		return expected == actual || (std::isnan(expected) && std::isnan(actual));
	}

	constexpr double PollPeriodFromHz(double pollRateHz)
	{
		return 1.0 / pollRateHz;
	}

	std::string MakeMismatchReport(const CsvData& expected, const CsvData& actual)
	{
		std::string report;
		if (expected.header != actual.header) {
			report += std::format("Header mismatch: expected {} columns, actual {} columns\n",
				expected.header.size(), actual.header.size());
			const auto columnCount = std::min(expected.header.size(), actual.header.size());
			for (size_t i = 0; i < columnCount; ++i) {
				if (expected.header[i] != actual.header[i]) {
					report += std::format("  column {}: expected [{}], actual [{}]\n",
						i, expected.header[i], actual.header[i]);
				}
			}
		}

		if (expected.rows.size() != actual.rows.size()) {
			report += std::format("Row count mismatch: expected {}, actual {}\n",
				expected.rows.size(), actual.rows.size());
		}

		constexpr size_t maxMismatchesToReport = 100;
		size_t mismatchCount = 0;
		const auto rowCount = std::min(expected.rows.size(), actual.rows.size());
		for (size_t r = 0; r < rowCount; ++r) {
			if (expected.rows[r].size() != actual.rows[r].size()) {
				report += std::format("Row {} column count mismatch: expected {}, actual {}\n",
					r, expected.rows[r].size(), actual.rows[r].size());
				++mismatchCount;
				continue;
			}
			for (size_t c = 0; c < expected.rows[r].size(); ++c) {
				if (!ValuesMatch(expected.rows[r][c], actual.rows[r][c])) {
					++mismatchCount;
					if (mismatchCount <= maxMismatchesToReport) {
						const auto columnName = c < expected.header.size() ? expected.header[c] : "<unknown>"s;
						report += std::format("  row {}, column {} [{}]: expected {}, actual {}\n",
							r, c, columnName, expected.rows[r][c], actual.rows[r][c]);
					}
				}
			}
		}
		if (mismatchCount > maxMismatchesToReport) {
			report += std::format("  ... {} additional value mismatches omitted\n",
				mismatchCount - maxMismatchesToReport);
		}

		return report;
	}

	std::string MakeAnalyzeCommand(const std::string& testName)
	{
		const auto scriptPath = std::filesystem::absolute(R"(..\..\Tests\Scripts\analyze-paced.py)").string();
		const auto outputPath = std::filesystem::absolute(outFolder_).string();
		const auto goldPath = std::filesystem::absolute(R"(..\..\Tests\AuxData\Data)").string();
		return std::format(
			R"(python "{}" --folder "{}" --golds "{}" --name {})",
			scriptPath,
			outputPath,
			goldPath,
			testName);
	}

	void DoPollingRunAndCompare(TestFixture& fix,
		uint32_t targetPid, double recordingStart, double recordingStop, double pollPeriod,
		const CsvData& gold, const std::string& testName)
	{
		// build output file path
		auto outCsvPath = std::format("{}\\{}_actual.csv", outFolder_, testName);
		// execute a test run and record samples, sync on exit
		fix.LaunchClient({
			"--process-id"s, std::to_string(targetPid),
			"--output-path"s, outCsvPath,
			"--run-time"s, std::to_string(recordingStop - recordingStart),
			"--run-start"s, std::to_string(recordingStart),
			"--poll-period"s, std::to_string(pollPeriod),
			"--metric-offset"s, "500"s,
			"--window-size"s, "1000"s,
		});
		const auto analyzeCommand = MakeAnalyzeCommand(testName);
		Logger::WriteMessage(std::format("Analyze with: {}\n", analyzeCommand).c_str());
		// load up result
		auto run = LoadRunFromCsv(outCsvPath);
		if (const auto report = MakeMismatchReport(gold, run); !report.empty()) {
			const auto reportPath = std::format("{}\\{}_mismatch.txt", outFolder_, testName);
			std::ofstream reportStream{ reportPath };
			reportStream << report;
			Logger::WriteMessage(report.c_str());
			Assert::Fail(pmon::util::str::ToWide(
				std::format("Paced polling output differed from gold. See report: {}\nAnalyze with: {}",
					reportPath, analyzeCommand)).c_str());
		}
	}

	void ExecutePacedPollingTest(const std::string& testName, uint32_t targetPid, double recordingStart,
		double recordingStop, double pollPeriod, TestFixture& fixture)
	{
		// derived parameters
		const auto goldCsvPath = std::format(R"(..\..\Tests\AuxData\Data\{}_gold.csv)", testName);

		if (std::filesystem::exists(goldCsvPath)) {
			const auto gold = LoadRunFromCsv(goldCsvPath);
			DoPollingRunAndCompare(
				fixture,
				targetPid,
				recordingStart,
				recordingStop,
				pollPeriod,
				gold,
				testName
			);
			Logger::WriteMessage("Paced polling output matched gold\n");
		}
		else {
			const auto outCsvPath = std::format("{}\\{}_actual.csv", outFolder_, testName);
			fixture.LaunchClient({
				"--process-id"s, std::to_string(targetPid),
				"--output-path"s, outCsvPath,
				"--run-time"s, std::to_string(recordingStop - recordingStart),
				"--run-start"s, std::to_string(recordingStart),
				"--poll-period"s, std::to_string(pollPeriod),
				"--metric-offset"s, "500"s,
				"--window-size"s, "1000"s,
			});
			const auto analyzeCommand = MakeAnalyzeCommand(testName);
			Logger::WriteMessage(std::format("Analyze with: {}\n", analyzeCommand).c_str());
			Assert::Fail(pmon::util::str::ToWide(
				std::format("Gold CSV does not exist. Generated candidate: {}\nAnalyze with: {}",
					outCsvPath, analyzeCommand)).c_str());
		}
	}

#define TEST_NAME P00HeaWin2080
	TEST_CLASS(TEST_NAME)
	{
		TestFixture fixture_;
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup({
				"--etl-test-file"s, std::format(R"(..\..\Tests\AuxData\Data\{}.etl)", STRINGIFY(TEST_NAME)),
				"--pace-playback"s,
			});
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		TEST_METHOD(PollDynamic)
		{
			// setup test parameters
			const uint32_t targetPid = 12820;
			const auto recordingStart = 1.;
			const auto recordingStop = 14.;
			constexpr auto pollPeriod = PollPeriodFromHz(150.0);
			// run test
			ExecutePacedPollingTest(STRINGIFY(TEST_NAME), targetPid, recordingStart, recordingStop,
				pollPeriod, fixture_);
		}
	};
#undef TEST_NAME

#define TEST_NAME P01TimeSpyDemoFS2080
	TEST_CLASS(TEST_NAME)
	{
		TestFixture fixture_;
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup({
				"--etl-test-file"s, std::format(R"(..\..\Tests\AuxData\Data\{}.etl)", STRINGIFY(TEST_NAME)),
				"--pace-playback"s,
				});
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		TEST_METHOD(PollDynamic)
		{
			// setup test parameters
			const uint32_t targetPid = 19736;
			const auto recordingStart = 1.;
			const auto recordingStop = 34.;
			constexpr auto pollPeriod = PollPeriodFromHz(140.0);
			// run test
			ExecutePacedPollingTest(STRINGIFY(TEST_NAME), targetPid, recordingStart, recordingStop,
				pollPeriod, fixture_);
		}
	};
#undef TEST_NAME
}
