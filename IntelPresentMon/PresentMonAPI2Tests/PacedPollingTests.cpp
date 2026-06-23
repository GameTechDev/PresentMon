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
#include <array>
#include <algorithm>
#include <format>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <map>
#include <optional>

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

	std::string MakeAbsolutePath(const std::string& path)
	{
		return std::filesystem::absolute(path).string();
	}

	struct StatColumnSet
	{
		std::optional<size_t> min;
		std::optional<size_t> p01;
		std::optional<size_t> p05;
		std::optional<size_t> p10;
		std::optional<size_t> p90;
		std::optional<size_t> p95;
		std::optional<size_t> p99;
		std::optional<size_t> max;
	};

	std::optional<std::pair<std::string, std::string>> ParseMetricStatColumn(const std::string& column)
	{
		const auto open = column.rfind('(');
		const auto close = column.rfind(')');
		if (open == std::string::npos || close == std::string::npos || close <= open) {
			return std::nullopt;
		}
		return std::pair{
			column.substr(0, open),
			column.substr(open + 1, close - open - 1),
		};
	}

	bool Ordered(double left, double right)
	{
		constexpr auto tolerance = 1e-9;
		return left <= right || std::abs(left - right) <= tolerance;
	}

	std::string MakeStatOrderingReport(const CsvData& run)
	{
		std::map<std::string, StatColumnSet> statColumnsByMetric;
		for (size_t i = 0; i < run.header.size(); ++i) {
			const auto parsed = ParseMetricStatColumn(run.header[i]);
			if (!parsed) {
				continue;
			}
			auto& columns = statColumnsByMetric[parsed->first];
			if (parsed->second == "min") {
				columns.min = i;
			}
			else if (parsed->second == "1%") {
				columns.p01 = i;
			}
			else if (parsed->second == "5%") {
				columns.p05 = i;
			}
			else if (parsed->second == "10%") {
				columns.p10 = i;
			}
			else if (parsed->second == "90%") {
				columns.p90 = i;
			}
			else if (parsed->second == "95%") {
				columns.p95 = i;
			}
			else if (parsed->second == "99%") {
				columns.p99 = i;
			}
			else if (parsed->second == "max") {
				columns.max = i;
			}
		}

		std::string report;
		size_t checkedMetricCount = 0;
		for (const auto& [metric, columns] : statColumnsByMetric) {
			if (!columns.min || !columns.p01 || !columns.p05 || !columns.p10 ||
				!columns.p90 || !columns.p95 || !columns.p99 || !columns.max) {
				continue;
			}
			++checkedMetricCount;
			const std::array<std::pair<const char*, size_t>, 8> orderedColumns{
				std::pair{ "min", *columns.min },
				std::pair{ "1%", *columns.p01 },
				std::pair{ "5%", *columns.p05 },
				std::pair{ "10%", *columns.p10 },
				std::pair{ "90%", *columns.p90 },
				std::pair{ "95%", *columns.p95 },
				std::pair{ "99%", *columns.p99 },
				std::pair{ "max", *columns.max },
			};
			for (size_t rowIndex = 0; rowIndex < run.rows.size(); ++rowIndex) {
				const auto& row = run.rows[rowIndex];
				if (row.size() != run.header.size()) {
					report += std::format("Row {} column count mismatch: expected {}, actual {}\n",
						rowIndex, run.header.size(), row.size());
					continue;
				}
				for (size_t i = 1; i < orderedColumns.size(); ++i) {
					const auto& previous = orderedColumns[i - 1];
					const auto& current = orderedColumns[i];
					const auto previousValue = row[previous.second];
					const auto currentValue = row[current.second];
					if (std::isnan(previousValue) || std::isnan(currentValue)) {
						continue;
					}
					if (!Ordered(previousValue, currentValue)) {
						report += std::format(
							"row {}, metric {}: {} ({}) > {} ({})\n",
							rowIndex, metric, previous.first, previousValue,
							current.first, currentValue);
					}
				}
			}
		}

		if (checkedMetricCount == 0) {
			report += "No metrics with complete min/percentile/max stat columns were found\n";
		}

		return report;
	}

	void ExecutePacedPollingOrderingTest(const std::string& testName, uint32_t targetPid,
		double recordingStart, double recordingStop, double pollPeriod, TestFixture& fixture)
	{
		const auto outCsvPath = MakeAbsolutePath(std::format("{}\\{}_ordering_actual.csv", outFolder_, testName));
		Logger::WriteMessage(std::format("Paced polling ordering output csv: {}\n",
			outCsvPath).c_str());
		fixture.LaunchClient({
			"--process-id"s, std::to_string(targetPid),
			"--output-path"s, outCsvPath,
			"--run-time"s, std::to_string(recordingStop - recordingStart),
			"--run-start"s, std::to_string(recordingStart),
			"--poll-period"s, std::to_string(pollPeriod),
			"--metric-offset"s, "500"s,
			"--window-size"s, "1000"s,
			"--min-max-percentile-stats-only"s,
		});

		auto run = LoadRunFromCsv(outCsvPath);
		Assert::IsTrue(!run.rows.empty(), L"Paced polling ordering run produced no rows");
		if (const auto report = MakeStatOrderingReport(run); !report.empty()) {
			const auto reportPath = MakeAbsolutePath(std::format("{}\\{}_ordering_mismatch.txt", outFolder_, testName));
			std::ofstream reportStream{ reportPath };
			reportStream << report;
			Logger::WriteMessage(std::format("Paced polling ordering mismatch report: {}\n",
				reportPath).c_str());
			Logger::WriteMessage(report.c_str());
			Assert::Fail(pmon::util::str::ToWide(
				"Paced polling stat ordering failed.").c_str());
		}
	}

	void DoPollingRunAndCompare(TestFixture& fix,
		uint32_t targetPid, double recordingStart, double recordingStop, double pollPeriod,
		const CsvData& gold, const std::string& testName)
	{
		// build output file path
		auto outCsvPath = MakeAbsolutePath(std::format("{}\\{}_actual.csv", outFolder_, testName));
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
			const auto reportPath = MakeAbsolutePath(std::format("{}\\{}_mismatch.txt", outFolder_, testName));
			std::ofstream reportStream{ reportPath };
			reportStream << report;
			Logger::WriteMessage(std::format("Paced polling mismatch report: {}\n",
				reportPath).c_str());
			Logger::WriteMessage(report.c_str());
			Assert::Fail(pmon::util::str::ToWide(
				"Paced polling output differed from gold.").c_str());
		}
	}

	void ExecutePacedPollingTest(const std::string& testName, uint32_t targetPid, double recordingStart,
		double recordingStop, double pollPeriod, TestFixture& fixture)
	{
		// derived parameters
		const auto goldCsvPath = MakeAbsolutePath(std::format(R"(..\..\Tests\AuxData\Data\{}_gold.csv)", testName));

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
			const auto outCsvPath = MakeAbsolutePath(std::format("{}\\{}_actual.csv", outFolder_, testName));
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
			Logger::WriteMessage(std::format("Gold CSV search path: {}\n", goldCsvPath).c_str());
			Logger::WriteMessage(std::format("Generated candidate CSV: {}\n", outCsvPath).c_str());
			Logger::WriteMessage(std::format("Analyze with: {}\n", analyzeCommand).c_str());
			Assert::Fail(pmon::util::str::ToWide(
				"Gold CSV does not exist.").c_str());
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
		TEST_METHOD(MinPercentileMaxOrdering)
		{
			const uint32_t targetPid = 12820;
			const auto recordingStart = 1.;
			const auto recordingStop = 5.;
			constexpr auto pollPeriod = PollPeriodFromHz(150.0);
			ExecutePacedPollingOrderingTest(STRINGIFY(TEST_NAME), targetPid, recordingStart,
				recordingStop, pollPeriod, fixture_);
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
		TEST_METHOD(MinPercentileMaxOrdering)
		{
			const uint32_t targetPid = 19736;
			const auto recordingStart = 1.;
			const auto recordingStop = 5.;
			constexpr auto pollPeriod = PollPeriodFromHz(140.0);
			ExecutePacedPollingOrderingTest(STRINGIFY(TEST_NAME), targetPid, recordingStart,
				recordingStop, pollPeriod, fixture_);
		}
	};
#undef TEST_NAME

#define TEST_NAME P02NomadDemoFS750
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
			const uint32_t targetPid = 5756;
			const auto recordingStart = 14.;
			const auto recordingStop = 17.;
			constexpr auto pollPeriod = PollPeriodFromHz(140.0);
			ExecutePacedPollingTest(STRINGIFY(TEST_NAME), targetPid, recordingStart, recordingStop,
				pollPeriod, fixture_);
		}
	};
#undef TEST_NAME
}
