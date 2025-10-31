// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include <vincentlaucsb-csv-parser/csv.hpp>
#include "../CommonUtilities/IntervalWaiter.h"
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
				.sampleClientMode = "MultiClient",
			};
			return args;
		}
	};

	class BlobReader
	{
		struct LookupInfo_
		{
			uint64_t offset;
			PM_DATA_TYPE type;
		};
	public:
		BlobReader(std::span<const PM_QUERY_ELEMENT> qels, std::shared_ptr<pmapi::intro::Root> pIntro)
		{
			for (auto& q : qels) {
				qInfo_.push_back({ q.dataOffset, pIntro->FindMetric(q.metric).GetDataTypeInfo().GetPolledType() });
			}
		}
		void Target(const pmapi::BlobContainer& blobs, uint32_t iBlob = 0)
		{
			pFirstByteTarget_ = blobs[iBlob];
		}
		template<typename T>
		T At(size_t iElement)
		{
			const auto off = qInfo_[iElement].offset;
			switch (qInfo_[iElement].type) {
			case PM_DATA_TYPE_BOOL:   return (T)reinterpret_cast<const bool&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_DOUBLE: return    reinterpret_cast<const double&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_ENUM:   return (T)reinterpret_cast<const int&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_INT32:  return (T)reinterpret_cast<const int32_t&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_STRING: return (T)-1;
			case PM_DATA_TYPE_UINT32: return (T)reinterpret_cast<const uint32_t&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_UINT64: return (T)reinterpret_cast<const uint64_t&>(pFirstByteTarget_[off]);
			case PM_DATA_TYPE_VOID:   return (T)-1;
			}
			return (T)-1;
		}
	private:
		const uint8_t* pFirstByteTarget_ = nullptr;
		std::vector<LookupInfo_> qInfo_;
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
		std::span<const PM_QUERY_ELEMENT> qels,
		const std::vector<std::vector<double>>& run0,
		const std::vector<std::vector<double>>& run1,
		double toleranceFactor)
	{
		std::vector<MetricCompareResult> results;
		for (auto&& [i, q] : vi::enumerate(qels)) {
			// triple tolerance for sensitive stats
			if (rn::contains(std::array{
				PM_STAT_MAX,
				PM_STAT_MIN,
				PM_STAT_PERCENTILE_01,
				PM_STAT_PERCENTILE_99,
				PM_STAT_MID_POINT }, q.stat)) {
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

	std::vector<std::vector<double>> LoadRunFromCsv(const std::string& path)
	{
		csv::CSVReader gold{ path };
		std::vector<std::vector<double>> dataRows;
		for (auto& row : gold) {
			std::vector<double> rowData;
			rowData.reserve(row.size());
			for (auto& field : row) {
				rowData.push_back(field.get<double>());
			}
			dataRows.push_back(std::move(rowData));
		}
		return dataRows;
	}

	std::vector<PM_QUERY_ELEMENT> BuildQueryElementSet(const pmapi::intro::Root& intro)
	{
		std::vector<PM_QUERY_ELEMENT> qels;
		for (const auto& m : intro.GetMetrics()) {
			// there is no reliable way of distinguishing CPU telemetry metrics from PresentData-based metrics via introspection
			// adding CPU device type is an idea, however that would require changing device id of the cpu metrics from 0 to
			// whatever id is assigned to cpu (probably an upper range like 1024+) and this might break existing code that just
			// hardcodes device id for the CPU metrics; for the time being use a hardcoded blacklist here
			if (rn::contains(std::array{
				PM_METRIC_CPU_UTILIZATION,
				PM_METRIC_CPU_POWER_LIMIT,
				PM_METRIC_CPU_POWER,
				PM_METRIC_CPU_TEMPERATURE,
				PM_METRIC_CPU_FREQUENCY,
				PM_METRIC_CPU_CORE_UTILITY,
				}, m.GetId())) {
				continue;
			}
			// only allow dynamic metrics
			if (m.GetType() != PM_METRIC_TYPE_DYNAMIC && m.GetType() != PM_METRIC_TYPE_DYNAMIC_FRAME) {
				continue;
			}
			// should be exactly 1 device (universal one)
			auto dmi = m.GetDeviceMetricInfo();
			if (dmi.size() != 1) {
				continue;
			}
			// device must be available and 0 (universal)
			if (!dmi.front().IsAvailable() || dmi.front().GetDevice().GetId() != 0) {
				continue;
			}
			// don't work on string data metrics
			if (m.GetDataTypeInfo().GetPolledType() == PM_DATA_TYPE_STRING) {
				continue;
			}
			for (const auto& s : m.GetStatInfo()) {
				// skip displayed fps (max) as it is broken now
				if (m.GetId() == PM_METRIC_DISPLAYED_FPS && s.GetStat() == PM_STAT_MAX) {
					continue;
				}
				qels.push_back(PM_QUERY_ELEMENT{ m.GetId(), s.GetStat() });
			}
		}
		return qels;
	}

	std::vector<std::string> MakeHeader(
		const std::vector<PM_QUERY_ELEMENT>& qels,
		const pmapi::intro::Root& intro)
	{
		std::vector<std::string> headerColumns{ "poll-time"s };
		for (auto& qel : qels) {
			headerColumns.push_back(std::format("{}({})",
				intro.FindMetric(qel.metric).Introspect().GetSymbol(),
				intro.FindEnum(PM_ENUM_STAT).FindKey((int)qel.stat).GetShortName()
			));
		}
		return headerColumns;
	}

	void WriteRunToCsv(
		const std::string& csvFilePath,
		const std::vector<std::string>& header,
		const std::vector<std::vector<double>>& runRows)
	{
		std::ofstream csvStream{ csvFilePath };
		auto csvWriter = csv::make_csv_writer(csvStream);
		csvWriter << header;
		for (auto& row : runRows) {
			csvWriter << row;
		}
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

	class TestClientModule
	{
	public:
		TestClientModule(const std::string& pipeName, double windowMs,
			double offsetMs, std::span<PM_QUERY_ELEMENT> qels)
			:
			session_{ pipeName },
			qels_{ qels },
			query_{ session_.RegisterDynamicQuery(qels_, windowMs, offsetMs) },
			blobs_{ query_.MakeBlobContainer(1) }
		{}
		std::vector<std::vector<double>> RecordPolling(uint32_t targetPid, double recordingStartSec,
			double recordingStopSec, double pollInterval)
		{
			auto pIntro = session_.GetIntrospectionRoot();
			// start tracking target
			auto tracker = session_.TrackProcess(targetPid);
			// get the waiter and the timer clocks ready
			using Clock = std::chrono::high_resolution_clock;
			const auto startTime = Clock::now();
			util::IntervalWaiter waiter{ pollInterval, 0.001 };
			// run polling loop and poll into vector
			std::vector<std::vector<double>> rows;
			std::vector<double> cells;
			BlobReader br{ qels_, pIntro };
			br.Target(blobs_);
			const auto recordingStart = recordingStartSec * 1s;
			const auto recordingStop = recordingStopSec * 1s;
			for (auto now = Clock::now(), start = Clock::now();
				now - start <= recordingStop; now = Clock::now()) {
				// skip recording while time has not reached start time
				if (now - start >= recordingStart) {
					cells.reserve(qels_.size() + 1);
					query_.Poll(tracker, blobs_);
					// first column is the time as measured in polling loop
					cells.push_back(std::chrono::duration<double>(now - start).count());
					// remaining columns are from the query
					for (size_t i = 0; i < qels_.size(); i++) {
						cells.push_back(br.At<double>(i));
					}
					rows.push_back(std::move(cells));
				}
				waiter.Wait();
			}
			return rows;
		}
	private:
		pmapi::Session session_;
		std::span<PM_QUERY_ELEMENT> qels_;
		pmapi::DynamicQuery query_;
		pmapi::BlobContainer blobs_;
	};

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

	auto DoPollingRunAndCompare(const std::string& ctrlPipe, std::span<PM_QUERY_ELEMENT> qels,
		uint32_t targetPid, double recordingStart, double recordingStop, double pollPeriod,
		const std::vector<std::string>& header, const std::vector<std::vector<double>>& gold,
		double toleranceFactor, const std::string& testName, const std::string& phaseName)
	{
		// execute a test run and record samples
		TestClientModule client{ ctrlPipe, 1000., 64., qels };
		auto run = client.RecordPolling(targetPid, recordingStart, recordingStop, pollPeriod);
		WriteRunToCsv(std::format("{}\\{}_{}.csv", outFolder_, testName, phaseName), header, run);
		// compare against gold
		auto compResults = CompareRuns(qels, run, gold, toleranceFactor);
		// record results for possible post-mortem
		WriteResults(std::format("{}\\{}_{}_rslt.csv", outFolder_, testName, phaseName),
			header, compResults);
		// return the results
		return compResults;
	}

	TEST_CLASS(PacedPollingTests)
	{
		TestFixture fixture_;
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup({
				"--etl-test-file"s, "Heaven-win-vsync-2080ti.etl"s,
				"--pace-playback"s,
			});
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		TEST_METHOD(PollDynamic)
		{
			const auto testName = "t0_hea"s;
			const auto goldCsvPath = R"(..\..\Tests\PacedGold\polled_gold.csv)"s;
			const uint32_t targetPid = 12820;
			const auto recordingStart = 1.;
			const auto recordingStop = 14.;
			const auto pollPeriod = 0.1;
			const auto sampleCount = (recordingStop - recordingStart) / pollPeriod;
			const size_t nRunsFull = 9;
			const size_t nRoundRobin = 12;
			const auto toleranceFactor = 0.02;
			const auto fullFailRatio = 0.667;

			auto& common = fixture_.GetCommonArgs();
			auto pIntro = pmapi::Session{ common.ctrlPipe }.GetIntrospectionRoot();
			auto qels = BuildQueryElementSet(*pIntro);
			auto header = MakeHeader(qels, *pIntro);

			// compare all runs against gold if exists
			if (std::filesystem::exists(goldCsvPath)) {
				auto gold = LoadRunFromCsv(goldCsvPath);
				// do one polling run and compare against gold
				const auto nFailOneshot = [&] {
					auto oneshotCompRes = DoPollingRunAndCompare(common.ctrlPipe, qels, targetPid, recordingStart,
						recordingStop, pollPeriod, header, gold, toleranceFactor, testName, "oneshot");
					return ValidateAndAggregateResults(sampleCount, testName + "_oneshot_agg.csv", { oneshotCompRes });
				}();
				// if oneshot run succeeds with zero failures, we finish here
				if (nFailOneshot == -1) {
					Logger::WriteMessage("One-shot success");
				}
				else {
					// oneshot failed, run N times and see if enough pass to seem plausible
					std::vector<std::vector<MetricCompareResult>> allResults;
					for (size_t i = 0; i < nRunsFull; i++) {
						// restart service to restart playback
						fixture_.RebootService();
						// do Nth polling run and compare against gold
						auto compRes = DoPollingRunAndCompare(common.ctrlPipe, qels, targetPid, recordingStart,
							recordingStop, pollPeriod, header, gold, toleranceFactor, testName, std::format("full_{}", i));
						allResults.push_back(std::move(compRes));
					}
					// validate comparison results
					const auto nFail = ValidateAndAggregateResults(sampleCount, testName + "full_agg", allResults);
					Assert::IsTrue(nFail < (int)std::round(nRunsFull * fullFailRatio),
						std::format(L"Failed [{}] runs (of {})", nFail, nRunsFull).c_str());
					Logger::WriteMessage(std::format(L"Retry success (failed [{}] of [{}])", nFail, nRunsFull).c_str());
				}
			}
			else { // if gold doesn't exist, do cartesian product comparison over many runs to genarate data for a new gold
				std::vector<std::vector<std::vector<double>>> allRobinRuns;
				for (size_t i = 0; i < nRoundRobin; i++) {
					// restart service to restart playback
					fixture_.RebootService();
					// execute polling run
					TestClientModule client{ common.ctrlPipe, 1000., 64., qels };
					auto run = client.RecordPolling(targetPid, recordingStart, recordingStop, pollPeriod);
					// record run samples for analysis
					WriteRunToCsv(std::format("{}\\{}_robin_{}.csv", outFolder_, testName, i), header, run);
					allRobinRuns.push_back(std::move(run));
				}
				// do cartesian product and record all results
				std::vector<std::vector<std::vector<MetricCompareResult>>> allRobinResults(allRobinRuns.size());
				for (size_t iA = 0; iA < allRobinRuns.size(); ++iA) {
					for (size_t iB = 0; iB < allRobinRuns.size(); ++iB) {
						if (iA == iB) continue;
						// compare run A vs run B
						auto results = CompareRuns(qels, allRobinRuns[iA], allRobinRuns[iB], toleranceFactor);
						// write per-pair results
						WriteResults(std::format("{}\\{}_robin_{}_{}_rslt.csv", outFolder_, testName, iA, iB), header, results);
						allRobinResults[iA].push_back(std::move(results));
					}
				}
				// aggregate for each candidate
				std::ofstream robinUberAggStream{ std::format("{}\\{}_robin_uber_agg.csv", outFolder_, testName) };
				auto aggWriter = csv::make_csv_writer(robinUberAggStream);
				aggWriter << std::array{ "#"s, "n-miss-total"s };
				Logger::WriteMessage("Round Robin Results\n===================\n");
				for (size_t i = 0; i < allRobinRuns.size(); i++) {
					const auto nFail = ValidateAndAggregateResults(sampleCount, 
						std::format("{}_robin_{}_agg.csv", testName, i), allRobinResults[i]);
					aggWriter << std::make_tuple(i, nFail);
					Logger::WriteMessage(std::format("#{}: {}", i, nFail).c_str());
				}
				// hardcode a fail because this execution path requires analysis and
				// selection of a gold result to lock in
				Assert::IsTrue(false, L"Run complete, analysis is required to select gold result.");
			}
		}
	};
}