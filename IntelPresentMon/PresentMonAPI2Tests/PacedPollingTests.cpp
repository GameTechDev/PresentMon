// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/str/String.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include <vincentlaucsb-csv-parser/csv.hpp>
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include <string>
#include <iostream>
#include <format>
#include <fstream>
#include <filesystem>
#include <array>
#include <ranges>
#include <cmath>
#include <numeric>
#include <regex>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;
namespace rn = std::ranges;
namespace vi = rn::views;
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
				.introNsm = "pm_paced_polling_test_intro",
				.frameNsm = "pm_paced_polling_test_nsm",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "PacedPlayback",
			};
			return args;
		}
	};

	struct Mismatch
	{
		size_t sampleIndex;
		double val0;
		double val1;
	};

	struct MetricCompareResult
	{
		std::vector<Mismatch> mismatches;
		double meanSquareError;
	};

	std::pair<double, double> CalculateDynamicRange(
		const std::vector<double>& run0,
		const std::vector<double>& run1)
	{
		const auto [minIt0, maxIt0] = rn::minmax_element(run0);
		const auto [minIt1, maxIt1] = rn::minmax_element(run1);
		double lo = std::min(*minIt0, *minIt1);
		double hi = std::max(*maxIt0, *maxIt1);
		return { lo, hi };
	}

	MetricCompareResult CompareRunsForMetric(
		const std::vector<double>& run0,
		const std::vector<double>& run1,
		double toleranceFactor)
	{
		// 1) compute dynamic range & tolerance
		auto [lo, hi] = CalculateDynamicRange(run0, run1);
		double tolerance = (hi - lo) * toleranceFactor;

		// 2) loop over corresponding samples and compare for individual mismatch and mse
		MetricCompareResult result;
		double sumSq = 0.0;
		for (auto&& [i, v0, v1] : vi::zip(vi::iota(0ull), run0, run1)) {
			const auto diff = v0 - v1;
			sumSq += diff * diff;
			if (std::abs(diff) > tolerance) {
				result.mismatches.push_back({ i, v0, v1 });
			}
		}

		// 3) finish computing MSE
		if (run0.size() > 0) {
			result.meanSquareError = sumSq / double(run0.size());
		}
		else {
			result.meanSquareError = 0.;
		}

		return result;
	}

	std::vector<double> ExtractColumn(const std::vector<std::vector<double>>& mat, std::size_t i)
	{
		return mat | vi::transform([i](auto const& row) { return row[i]; }) | rn::to<std::vector>();
	}

	std::vector<MetricCompareResult> CompareRuns(
		std::span<const PM_STAT> qStats,
		const std::vector<std::vector<double>>& run0,
		const std::vector<std::vector<double>>& run1,
		double toleranceFactor)
	{
		std::vector<MetricCompareResult> results;
		for (auto&& [i, s] : vi::enumerate(qStats)) {
			// triple tolerance for sensitive stats
			if (rn::contains(std::array{
				PM_STAT_MAX,
				PM_STAT_MIN,
				PM_STAT_PERCENTILE_01,
				PM_STAT_PERCENTILE_99,
				PM_STAT_MID_POINT }, s)) {
				toleranceFactor *= 3.;
			}
			// compare columns for each query element
			results.push_back(CompareRunsForMetric(
				ExtractColumn(run0, i),
				ExtractColumn(run1, i),
				toleranceFactor
			));
		}
		return results;
	}

	auto LoadRunFromCsv(const std::string& path)
	{
 		csv::CSVReader gold{ path };
		auto header = gold.get_col_names();
		std::vector<std::vector<double>> dataRows;
		for (auto& row : gold) {
			std::vector<double> rowData;
			rowData.reserve(row.size());
			for (auto& field : row) {
				rowData.push_back(field.get<double>());
			}
			dataRows.push_back(std::move(rowData));
		}
		return std::make_pair(std::move(header), std::move(dataRows));
	}

	using StatMap = std::unordered_map<std::string, PM_STAT>;
	StatMap MakeStatMap(const pmapi::intro::Root& intro)
	{
		std::unordered_map<std::string, PM_STAT> statMap;
		for (auto s : intro.FindEnum(PM_ENUM_STAT).GetKeys()) {
			statMap[s.GetShortName()] = (PM_STAT)s.GetId();
		}
		return statMap;
	}

	std::vector<PM_STAT> HeaderToStats(std::span<const std::string> header, const StatMap& map)
	{
		// Capture text inside final parentheses, trimming optional whitespace.
		static const std::regex paren_capture{ R"(.*\(\s*([^)]+?)\s*\)\s*$)", std::regex::ECMAScript };

		std::vector<PM_STAT> stats;
		stats.reserve(header.size());

		for (const auto& col : header) {
			std::smatch m;
			if (!std::regex_match(col, m, paren_capture) || m.size() < 2) {
				continue;
			}

			const auto shortname = m[1].str();
			if (auto it = map.find(shortname); it != map.end()) {
				stats.push_back(it->second);
			}
			else {
				stats.push_back(PM_STAT_NONE);
				Logger::WriteMessage(std::format("Failed to look up stat: {}\n", shortname).c_str());
			}
		}

		return stats;
	}

	void WriteResults(
		const std::string& csvFilePath,
		const std::vector<std::string>& header,
		std::vector<MetricCompareResult> results)
	{
		// output results to csvs
		std::ofstream resStream{ csvFilePath };
		auto resWriter = csv::make_csv_writer(resStream);
		resWriter << std::array{ "metric"s, "n-miss"s, "mse"s };
		for (auto&& [i, res] : vi::enumerate(results)) {
			resWriter << std::make_tuple(header[i], res.mismatches.size(), res.meanSquareError);
		}
	}

	// works on the set of all results comparing one run (test) against another (gold)
	// outputs aggregate showing at a glance how each test run compares to the gold
	int ValidateAndAggregateResults(double sampleCount, std::string fileName,
		const std::vector<std::vector<MetricCompareResult>>& allResults)
	{
		// output aggregate results of all runs
		std::ofstream aggStream{ outFolder_ + "\\"s + fileName };
		auto aggWriter = csv::make_csv_writer(aggStream);
		aggWriter << std::array{ "#"s, "n-miss-total"s, "n-miss-max"s, "mse-total"s, "mse-max"s };
		int nFail = 0;
		for (auto&& [i, runResult] : vi::enumerate(allResults)) {
			size_t nMissTotal = 0;
			size_t nMissMax = 0;
			double mseTotal = 0.;
			double mseMax = 0.;
			for (auto& colRes : runResult) {
				nMissTotal += colRes.mismatches.size();
				nMissMax = std::max(colRes.mismatches.size(), nMissMax);
				mseTotal += colRes.meanSquareError;
				mseMax = std::max(colRes.meanSquareError, mseMax);
			}
			aggWriter << std::make_tuple(i, nMissTotal, nMissMax, mseTotal, mseMax);
			// factors to tweak the pass/fail decision points
			const auto overallMissRatio = 0.033;
			const auto perColumnMissRatio = 0.01;
			const auto mseTotalFactor = 2.5;
			const auto mseMaxFactor = 1.;
			// fail if any single column has too many mismatches, or if the total of all
			// columns exceeds a threshold (same idea for mse below as well)
			if (nMissTotal > size_t(sampleCount * overallMissRatio) ||
				nMissMax > size_t(sampleCount * perColumnMissRatio)) {
				nFail++;
			}
			else if (mseTotal > sampleCount * mseTotalFactor ||
				mseMax > sampleCount * mseMaxFactor) {
				nFail++;
			}
		}
		return nFail;
	}

	auto DoPollingRunAndCompare(TestFixture& fix, const std::string& ctrlPipe, const StatMap& smap,
		uint32_t targetPid, double recordingStart, double recordingStop, double pollPeriod,
		const std::vector<std::vector<double>>& gold, double toleranceFactor,
		const std::string& testName, const std::string& phaseName)
	{
		// build output file path
		auto outCsvPath = std::format("{}\\{}_{}.csv", outFolder_, testName, phaseName);
		// execute a test run and record samples, sync on exit
		fix.LaunchClient({
			"--process-id"s, std::to_string(targetPid),
			"--output-path"s, outCsvPath,
			"--run-time"s, std::to_string(recordingStop - recordingStart),
			"--run-start"s, std::to_string(recordingStart),
			"--poll-period"s, std::to_string(pollPeriod),
			"--metric-offset"s, "64"s,
			"--window-size"s, "1000"s,
		});
		// load up result
		auto [header, run] = LoadRunFromCsv(outCsvPath);
		// extract stats from header
		auto stats = HeaderToStats(header, smap);
		// compare against gold
		auto compResults = CompareRuns(stats, run, gold, toleranceFactor);
		// record results for possible post-mortem
		WriteResults(std::format("{}\\{}_{}_rslt.csv", outFolder_, testName, phaseName),
			header, compResults);
		// return the results
		return compResults;
	}

	void ExecutePacedPollingTest(const std::string& testName, uint32_t targetPid, double recordingStart,
		double recordingStop, double pollPeriod, double toleranceFactor, double fullFailRatio,
		TestFixture& fixture)
	{
		// hardcoded constants
		const size_t nRunsFull = 9;
		const size_t nRoundRobin = 12;

		// derived parameters
		const auto goldCsvPath = std::format(R"(..\..\Tests\PacedGold\{}_gold.csv)", testName);
		const auto sampleCount = (recordingStop - recordingStart) / pollPeriod;

		// script analysis command line info
		const auto rootPath = fs::current_path().parent_path().parent_path();
		const auto scriptPath = (rootPath / "Tests\\Scripts\\analyze-paced.py").string();
		const auto outPath = (rootPath / "build\\Debug\\TestOutput\\PacedPolling").string();
		const auto goldPath = (rootPath / "Tests\\PacedGold").string();

		auto& common = fixture.GetCommonArgs();
		const auto smap = [&] {
			pmapi::Session tempSession{ common.ctrlPipe };
			const auto pTempIntro = tempSession.GetIntrospectionRoot();
			return MakeStatMap(*pTempIntro);
		}();

		// compare all runs against gold if exists
		if (std::filesystem::exists(goldCsvPath)) {
			auto [gh, gold] = LoadRunFromCsv(goldCsvPath);
			// do one polling run and compare against gold
			const auto nFailOneshot = [&] {
				auto oneshotCompRes = DoPollingRunAndCompare(
					fixture,
					common.ctrlPipe,
					smap,
					targetPid,
					recordingStart,
					recordingStop,
					pollPeriod,
					gold,
					toleranceFactor,
					testName,
					"oneshot"
				);
				return ValidateAndAggregateResults(sampleCount, testName + "_oneshot_agg.csv", { oneshotCompRes });
			}();
			// if oneshot run succeeds with zero failures, we finish here
			if (nFailOneshot == 0) {
				Logger::WriteMessage("One-shot success");
			}
			else {
				// oneshot failed, run N times and see if enough pass to seem plausible
				std::vector<std::vector<MetricCompareResult>> allResults;
				for (size_t i = 0; i < nRunsFull; i++) {
					// restart service to restart playback
					fixture.RebootService();
					// do Nth polling run and compare against gold
					auto compRes = DoPollingRunAndCompare(
						fixture,
						common.ctrlPipe,
						smap,
						targetPid,
						recordingStart,
						recordingStop,
						pollPeriod,
						gold,
						toleranceFactor,
						testName,
						std::format("full_{:02}", i)
					);
					allResults.push_back(std::move(compRes));
				}
				// validate comparison results
				const auto nFail = ValidateAndAggregateResults(sampleCount, testName + "_full_agg.csv", allResults);
				// output analysis command
				Logger::WriteMessage("Analyze with:\n");
				Logger::WriteMessage(std::format(R"(python "{}" --folder "{}" --name {} --golds "{}")",
					scriptPath, outPath, testName, goldPath).c_str());
				Logger::WriteMessage("\n");
				Assert::IsTrue(nFail < (int)std::round(nRunsFull * fullFailRatio),
					std::format(L"Failed [{}] runs (of {})", nFail, nRunsFull).c_str());
				Logger::WriteMessage(std::format(L"Retry success (failed [{}] of [{}])", nFail, nRunsFull).c_str());
			}
		}
		else { // if gold doesn't exist, do cartesian product comparison over many runs to generate data for a new gold
			std::vector<std::vector<std::vector<double>>> allRobinRuns;
			std::vector<std::string> header;
			for (size_t i = 0; i < nRoundRobin; i++) {
				// restart service to restart playback
				fixture.RebootService();
				// execute a test run and record samples, sync on exit
				auto outCsvPath = std::format("{}\\{}_robin_{:02}.csv", outFolder_, testName, i);
				fixture.LaunchClient({
					"--process-id"s, std::to_string(targetPid),
					"--output-path"s, outCsvPath,
					});
				// load up result and collect in memory
				auto [runHeader, run] = LoadRunFromCsv(outCsvPath);
				if (header.empty()) {
					header = runHeader;
				}
				allRobinRuns.push_back(std::move(run));
			}
			const auto stats = HeaderToStats(header, smap);
			// do cartesian product and record all results
			std::vector<std::vector<std::vector<MetricCompareResult>>> allRobinResults(allRobinRuns.size());
			for (size_t iA = 0; iA < allRobinRuns.size(); ++iA) {
				for (size_t iB = 0; iB < allRobinRuns.size(); ++iB) {
					// compare run A vs run B
					auto results = CompareRuns(stats, allRobinRuns[iA], allRobinRuns[iB], toleranceFactor);
					// write per-pair results
					WriteResults(
						std::format("{}\\{}_robin_{:02}_{:02}_rslt.csv", outFolder_, testName, iA, iB),
						header,
						results
					);
					allRobinResults[iA].push_back(std::move(results));
				}
			}
			// aggregate for each candidate
			std::ofstream robinUberAggStream{ std::format("{}\\{}_robin_uber_agg.csv", outFolder_, testName) };
			auto aggWriter = csv::make_csv_writer(robinUberAggStream);
			aggWriter << std::array{ "#"s, "n-fail-total"s };
			Logger::WriteMessage("Round Robin Results\n===================\n");
			for (size_t i = 0; i < allRobinRuns.size(); i++) {
				const auto nFail = ValidateAndAggregateResults(
					sampleCount,
					std::format("{}_robin_{:02}_agg.csv", testName, i),
					allRobinResults[i]
				);
				aggWriter << std::make_tuple(i, nFail);
				Logger::WriteMessage(std::format("#{:02}: {}\n", i, nFail).c_str());
			}
			// output analysis command
			Logger::WriteMessage("Analyze with:\n");
			Logger::WriteMessage(std::format(R"(python "{}" --folder "{}" --name {})",
				scriptPath, outPath, testName).c_str());
			Logger::WriteMessage("\n");
			// hardcode a fail because this execution path requires analysis and
			// selection of a gold result to lock in
			Assert::IsTrue(false, L"Run complete, analysis is required to select gold result.");
		}
	}

	TEST_CLASS(P00HeaWin2080)
	{
		static constexpr const char* testName_ = "P00HeaWin2080";
		TestFixture fixture_;
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup({
				"--etl-test-file"s, std::format(R"(..\..\Tests\AuxData\PacedPolled\{}.etl)", testName_),
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
			const auto pollPeriod = 0.1;
			const auto toleranceFactor = 0.02;
			const auto fullFailRatio = 0.667;
			// run test
			ExecutePacedPollingTest(testName_, targetPid, recordingStart, recordingStop, pollPeriod,
				toleranceFactor, fullFailRatio, fixture_);
		}
	};
}