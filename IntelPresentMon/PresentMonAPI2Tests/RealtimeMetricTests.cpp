// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "FirstFrameWait.h"
#include "Folders.h"
#include "TestProcess.h"
#include "../PresentMonAPIWrapper/FixedQuery.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include <chrono>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;

namespace RealtimeMetricTests
{
	using namespace std::chrono_literals;
	using namespace std::string_literals;

	class TestFixture : public CommonTestFixture
	{
	public:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-rtmetric-test-ctrl)",
				.shmNamePrefix = "pm_rtmetric_test",
				.logLevel = "verbose",
				.logVerboseModules = "middleware dyninteg etwq",
				.logFolder = logFolder_,
				.sampleClientMode = "NONE",
				.etwSessionName = "RealtimeMetricSession",
			};
			return args;
		}
	};

	static std::unique_ptr<pmapi::Session> OpenSession_(const TestFixture& fixture)
	{
		try {
			return std::make_unique<pmapi::Session>(fixture.GetCommonArgs().ctrlPipe);
		}
		catch (const std::exception& e) {
			Logger::WriteMessage(std::format("Error opening session: {}\n", e.what()).c_str());
			Assert::Fail(L"Failed to connect to service via named pipe");
		}
		return {};
	}

	static pmapi::ProcessTracker TrackPresenterAndWaitForFirstFrame_(
		pmapi::Session& session,
		TestFixture& fixture,
		const PresenterProcess& presenter,
		const char* label,
		size_t minFrameCount = 1)
	{
		session.SetEtwFlushPeriod(8);
		std::this_thread::sleep_for(1ms);

		auto tracker = session.TrackProcess(presenter.GetId());
		Assert::IsTrue(fixture.service.has_value());
		const auto segmentName = pmon::tests::RequireFrameSegmentName(
			fixture.service->QueryStatus().frameSegmentNames, presenter.GetId());
		pmon::tests::WaitForFirstFrame(
			fixture.GetCommonArgs().ctrlPipe,
			presenter.GetId(),
			segmentName,
			label,
			pmon::tests::DefaultFirstFrameWaitLimit,
			minFrameCount);
		return tracker;
	}

	TEST_CLASS(RealtimeMetricTests)
	{
		TestFixture fixture_;
	public:

		TEST_METHOD_INITIALIZE(Setup)
		{
			Logger::WriteMessage(std::format("RealtimeMetric log folder: {}\n",
				fs::absolute(logFolder_).string()).c_str());
			Logger::WriteMessage(std::format("RealtimeMetric output folder: {}\n",
				fs::absolute(outFolder_).string()).c_str());
			fixture_.Setup();
		}

		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(RealtimeOpenSessionTest)
		{
			auto presenter = fixture_.LaunchPresenter();
			Logger::WriteMessage(std::format("RealtimeMetric presenter pid={}\n", presenter.GetId()).c_str());
			auto pSession = OpenSession_(fixture_);
			Assert::IsTrue((bool)pSession);
		}

		TEST_METHOD(RealtimeTrackProcessTest)
		{
			auto presenter = fixture_.LaunchPresenter();
			Logger::WriteMessage(std::format("RealtimeMetric presenter pid={}\n", presenter.GetId()).c_str());
			auto pSession = OpenSession_(fixture_);
			auto processTracker = TrackPresenterAndWaitForFirstFrame_(
				*pSession, fixture_, presenter, "rt-metric-track");
			processTracker.Reset();
		}
		TEST_METHOD(RealtimeFrameMetricsTest)
		{
			using namespace pmapi;

			auto presenter = fixture_.LaunchPresenter();
			Logger::WriteMessage(std::format("RealtimeMetric presenter pid={}\n", presenter.GetId()).c_str());
			auto pSession = OpenSession_(fixture_);
			auto processTracker = TrackPresenterAndWaitForFirstFrame_(
				*pSession, fixture_, presenter, "rt-metric-frame", 10);
			PM_BEGIN_FIXED_DYNAMIC_QUERY(MyDynamicQuery)
				FixedQueryElement qeCpuFtAvg{ this, PM_METRIC_CPU_FRAME_TIME, PM_STAT_AVG };
			PM_END_FIXED_QUERY dq{ *pSession, 1000, 101, 1, 1 };

			const int maxSamples = 10;
			for (int i = 0; i < maxSamples; i++)
			{
				dq.Poll(processTracker);
				Logger::WriteMessage(std::format("RealtimeFrameMetrics poll {} cpu_ft_avg={}\n",
					i, dq.qeCpuFtAvg.As<double>()).c_str());
				if (dq.qeCpuFtAvg.As<double>() == 0.) {
					std::this_thread::sleep_for(500ms);
				}
				else
				{
					break;
				}
			}

			// PresentBench2 defaults to a high-precision 10ms frame wait. Rendering
			// and scheduling still add a small amount of overhead to the interval.
			double expectedFtAvg = 10.;
			double ftTolerance = 0.5;
			Assert::AreEqual(expectedFtAvg, dq.qeCpuFtAvg.As<double>(), ftTolerance,
				L"CPU frame time avg not within tolerance (ms).");
			processTracker.Reset();
		}

		TEST_METHOD(PsoCompileMetricsAreZeroWithoutPsoWorkload)
		{
			using namespace pmapi;

			auto presenter = fixture_.LaunchPresenter();
			auto pSession = OpenSession_(fixture_);
			auto processTracker = TrackPresenterAndWaitForFirstFrame_(
				*pSession, fixture_, presenter, "rt-metric-pso-disabled", 5);
			PM_BEGIN_FIXED_FRAME_QUERY(PsoFrameQuery)
				FixedQueryElement psoCompileCount{ this, PM_METRIC_PSO_COMPILE_COUNT };
				FixedQueryElement psoCompileTime{ this, PM_METRIC_PSO_COMPILE_TIME };
				FixedQueryElement psoCompileBusy{ this, PM_METRIC_PSO_COMPILE_BUSY_PERCENT };
			PM_END_FIXED_QUERY query{ *pSession, 64 };

			bool sawZeroFrame = false;
			const auto deadline = std::chrono::steady_clock::now() + 2s;
			while (!sawZeroFrame && std::chrono::steady_clock::now() < deadline) {
				query.ForEachConsume(processTracker, [&] {
					const auto count = query.psoCompileCount.As<uint64_t>();
					const auto time = query.psoCompileTime.As<double>();
					const auto busy = query.psoCompileBusy.As<double>();
					if (count == 0 && time == 0. && busy == 0.) {
						sawZeroFrame = true;
					}
				});
				if (!sawZeroFrame) {
					std::this_thread::sleep_for(25ms);
				}
			}
			Assert::IsTrue(sawZeroFrame, L"Expected a frame without PSO compile activity");
		}

		TEST_METHOD(PsoCompileMetricsReportPb2Workload)
		{
			using namespace pmapi;

			auto presenter = fixture_.LaunchPresenter({
				"--coarse-frame-wait"s,
				"--pso-compiles"s, "1"s,
			});
			auto pSession = OpenSession_(fixture_);
			auto processTracker = TrackPresenterAndWaitForFirstFrame_(
				*pSession, fixture_, presenter, "rt-metric-pso-enabled", 5);
			PM_BEGIN_FIXED_FRAME_QUERY(PsoFrameQuery)
				FixedQueryElement psoCompileCount{ this, PM_METRIC_PSO_COMPILE_COUNT };
				FixedQueryElement psoCompileTime{ this, PM_METRIC_PSO_COMPILE_TIME };
				FixedQueryElement psoCompileBusy{ this, PM_METRIC_PSO_COMPILE_BUSY_PERCENT };
			PM_END_FIXED_QUERY query{ *pSession, 64 };

			bool sawCompileFrame = false;
			const auto deadline = std::chrono::steady_clock::now() + 2s;
			while (!sawCompileFrame && std::chrono::steady_clock::now() < deadline) {
				query.ForEachConsume(processTracker, [&] {
					const auto count = query.psoCompileCount.As<uint64_t>();
					const auto time = query.psoCompileTime.As<double>();
					const auto busy = query.psoCompileBusy.As<double>();
					if (count >= 1 && time > 0. && busy > 0.) {
						sawCompileFrame = true;
					}
				});
				if (!sawCompileFrame) {
					std::this_thread::sleep_for(25ms);
				}
			}
			Assert::IsTrue(sawCompileFrame, L"Expected PB2 PSO compile metrics");
		}
	};
}
