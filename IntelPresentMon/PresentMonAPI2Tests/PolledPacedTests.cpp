// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include <boost/process/v1/child.hpp>
#include <vincentlaucsb-csv-parser/csv.hpp>
#include "../PresentMonAPI2Loader/Loader.h"
#include "../PresentMonAPI2/Internal.h"
#include "../CommonUtilities/pipe/Pipe.h"
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
namespace bp = boost::process::v1;
namespace fs = std::filesystem;
namespace rn = std::ranges;
namespace vi = rn::views;
using namespace std::literals;
using namespace pmon;

namespace PacedPollingTests
{
	std::vector<double> ExtractColumn(const std::vector<std::vector<double>>& mat, std::size_t i)
	{
		return mat | vi::transform([i](auto const& row) { return row[i]; })
			| rn::to<std::vector>();
	}

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
		result.meanSquareError = sumSq / static_cast<double>(run0.size());
		return result;
	}

	std::vector<MetricCompareResult> CompareRuns(
		std::span<const PM_QUERY_ELEMENT> qels,
		const std::vector<std::vector<double>>& run0,
		const std::vector<std::vector<double>>& run1,
		double toleranceFactor)
	{
		std::vector<MetricCompareResult> results;
		for (auto&& [i, q] : vi::enumerate(qels)) {
			if (rn::contains(std::array{
				PM_STAT_MAX,
				PM_STAT_MIN,
				PM_STAT_PERCENTILE_01,
				PM_STAT_PERCENTILE_99,
				PM_STAT_MID_POINT }, q.stat)) {
				toleranceFactor *= 3.;
			}
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
			if (m.GetType() != PM_METRIC_TYPE_DYNAMIC && m.GetType() != PM_METRIC_TYPE_DYNAMIC_FRAME) {
				continue;
			}
			auto dmi = m.GetDeviceMetricInfo();
			if (dmi.size() != 1) {
				continue;
			}
			if (!dmi.front().IsAvailable() || dmi.front().GetDevice().GetId() != 0) {
				continue;
			}
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
			// we need to protect csv lib from inf values, it does not handle them well
			auto mse = std::isinf(res.meanSquareError) ? -0.0001 : res.meanSquareError;
			resWriter << std::make_tuple(header[i], res.mismatches.size(), mse);
		}
	}

	TEST_CLASS(PacedPollingTests)
	{
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
		}
		TEST_METHOD(PollDynamic)
		{
			const uint32_t targetPid = 12820;
			const auto recordingStart = 1s;
			const auto recordingStop = 14s;

			const auto pipeName = R"(\\.\pipe\pm-poll-test-act)"s;
			const auto etlName = "Heaven-win-vsync-2080ti.etl"s;
			const auto goldCsvPath = R"(..\..\Tests\PacedGold\polled_gold.csv)"s;
			const auto outputBasePath = R"(PacedTest\Polled1\)"s;
			const auto toleranceFactor = 0.02;

			std::filesystem::create_directories(outputBasePath);

			pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
			pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
			
			std::vector<std::vector<std::vector<double>>> allRuns;
			std::vector<PM_QUERY_ELEMENT> qels;
			std::vector<std::string> header;

			// generate playback polling data for N runs
			const auto Run = [&](int n) {
				for (int x = 0; x < n; x++) {
					// short sleep at the beginning of very run but the first
					// waiting to make sure previous svc has cleaned up 100%
					if (x > 0) {
						std::this_thread::sleep_for(50ms);
					}

					bp::child svc{ "PresentMonService.exe"s,
						"--control-pipe"s, pipeName,
						"--nsm-prefix"s, "pmon_poll_test_nsm"s,
						"--intro-nsm"s, "pm_poll_test_intro"s,
						"--etl-test-file"s, etlName,
						"--pace-playback" };
					// wait until child svc is ready to accept connections, fail if it takes too long
					Assert::IsTrue(pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500),
						L"Timeout waiting for service control pipe");
					// connect to svc and get introspection
					pmapi::Session api{ pipeName };
					auto pIntro = api.GetIntrospectionRoot();
					// build the query element set via introspect if not yet built (first run only)
					if (qels.empty()) {
						qels = BuildQueryElementSet(*pIntro);
					}
					// build the header if necessary
					if (header.empty()) {
						header = MakeHeader(qels, *pIntro);
					}
					// register query and create necessary blob
					auto query = api.RegisterDyanamicQuery(qels, 1000., 64.);
					auto blobs = query.MakeBlobContainer(1);
					// start tracking target
					auto tracker = api.TrackProcess(targetPid);
					// get the waiter and the timer clocks ready
					using Clock = std::chrono::high_resolution_clock;
					const auto startTime = Clock::now();
					util::IntervalWaiter waiter{ 0.1, 0.001 };
					// run polling loop and poll into vector
					std::vector<std::vector<double>> rows;
					std::vector<double> cells;
					BlobReader br{ qels, pIntro };
					br.Target(blobs);
					for (auto now = Clock::now(), start = Clock::now();
						now - start <= recordingStop; now = Clock::now()) {
						// skip recording while time has not reached start time
						if (now - start >= recordingStart) {
							cells.reserve(qels.size() + 1);
							query.Poll(tracker, blobs);
							// first column is the time as measured in polling loop
							cells.push_back(std::chrono::duration<double>(now - start).count());
							// remaining columns are from the query
							for (size_t i = 0; i < qels.size(); i++) {
								cells.push_back(br.At<double>(i));
							}
							rows.push_back(std::move(cells));
						}
						waiter.Wait();
					}
					// write the full run data to csv file
					WriteRunToCsv(std::format("{}test_run_{}.csv", outputBasePath, allRuns.size()), header, rows);
					// append run data to the vector of all runs
					allRuns.push_back(std::move(rows));
				}
			};

			// compare all runs against gold if exists
			if (std::filesystem::exists(goldCsvPath)) {
				std::vector<std::vector<MetricCompareResult>> allResults;
				// load gold csv
				auto gold = LoadRunFromCsv(goldCsvPath);
				const auto DoComparison = [&] {
					// loop over all runs in memory and compare with gold, write results
					for (auto&& [i, run] : vi::enumerate(allRuns)) {
						auto results = CompareRuns(qels, run, gold, toleranceFactor);
						WriteResults(std::format("{}results_{}.csv", outputBasePath, i), header, results);
						allResults.push_back(std::move(results));
					}
				};
				const auto ValidateAndWriteAggregateResults = [&] {
					// output aggregate results of all runs
					std::ofstream aggStream{ outputBasePath + "results_agg.csv"s };
					auto aggWriter = csv::make_csv_writer(aggStream);
					aggWriter << std::array{ "#"s, "n-miss-total"s, "n-miss-max"s, "mse-total"s, "mse-max"s };
					int nFail = 0;
					for (auto&& [i, columnResults] : vi::enumerate(allResults)) {
						size_t nMissTotal = 0;
						size_t nMissMax = 0;
						double mseTotal = 0.;
						double mseMax = 0.;
						for (auto& colRes : columnResults) {
							nMissTotal += colRes.mismatches.size();
							nMissMax = std::max(colRes.mismatches.size(), nMissMax);
							mseTotal += colRes.meanSquareError;
							mseMax = std::max(colRes.meanSquareError, mseMax);
						}
						aggWriter << std::make_tuple(i, nMissTotal, nMissMax, mseTotal, mseMax);
						const auto rowCount = allRuns[i].size();
						// fail if any single column has too many mismatches, or if the total of all
						// columns exceeds a threshold (same idea for mse below as well)
						if (nMissTotal > size_t(rowCount / 33.) || nMissMax > size_t(rowCount / 100.)) {
							nFail++;
						}
						else if (mseTotal > rowCount * 2.5 || mseMax > double(rowCount)) {
							nFail++;
						}
					}
					return nFail;
				};
				// first attempt to compare single run against gold
				Run(1);
				DoComparison();
				if (ValidateAndWriteAggregateResults() == 0) {
					Logger::WriteMessage("One-shot success");
				}
				else {
					// first single run fails, run N times and see if enough pass to seem plausible
					allResults.clear();
					const int nRuns = 9;
					Run(nRuns);
					DoComparison();
					const auto nFail = ValidateAndWriteAggregateResults();
					Assert::IsTrue(nFail < (int)std::roundf(nRuns * 0.667f),
						std::format(L"Failed [{}] runs (of {})", nFail, nRuns).c_str());
					Logger::WriteMessage(std::format(L"Retry success (failed [{}] of [{}])", nFail, nRuns).c_str());
				}
			}
			else { // if gold doesn't exist, do cartesian product comparison of all
				const int nRuns = 9;
				Run(nRuns);
				std::vector<size_t> mismatchTotals(allRuns.size(), 0);
				for (size_t iA = 0; iA < allRuns.size(); ++iA) {
					for (size_t iB = iA + 1; iB < allRuns.size(); ++iB) {
						// compare run A vs run B
						auto results = CompareRuns(qels, allRuns[iA], allRuns[iB], toleranceFactor);
						// write per-pair results
						WriteResults(std::format("{}round_robin_{}_{}.csv", outputBasePath, iA, iB), header, results);

						// accumulate total mismatches for ranking
						size_t sumMiss = 0;
						for (const auto& res : results) {
							sumMiss += res.mismatches.size();
						}
						mismatchTotals[iA] += sumMiss;
						mismatchTotals[iB] += sumMiss;
					}
				}

				// write aggregate ranking of runs by total mismatches
				std::ofstream aggStream{ outputBasePath + "round_robin_agg.csv"s };
				auto aggWriter = csv::make_csv_writer(aggStream);
				aggWriter << std::array{ "#"s, "n-miss-total"s };
				for (size_t i = 0; i < mismatchTotals.size(); ++i) {
					aggWriter << std::make_tuple(i, mismatchTotals[i]);
				}
				// hardcode a fail because this execution path requires analysis and
				// selection of a gold result to lock in
				Assert::IsTrue(false, L"Run complete, analysis is required to select gold result.");
			}
		}
	};
}