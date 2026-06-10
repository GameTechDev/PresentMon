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

	static PresenterProcess LaunchPresenter_(TestFixture& fixture)
	{
		return fixture.LaunchPresenter({ "/FrameSleep=10"s });
	}

	static pmapi::ProcessTracker TrackPresenterAndWaitForFirstFrame_(
		pmapi::Session& session,
		const TestFixture& fixture,
		const PresenterProcess& presenter,
		const char* label,
		size_t minFrameCount = 1)
	{
		session.SetEtwFlushPeriod(8);
		std::this_thread::sleep_for(1ms);

		auto tracker = session.TrackProcess(presenter.GetId());
		pmon::tests::WaitForFirstFrame(
			fixture.GetCommonArgs().ctrlPipe,
			presenter.GetId(),
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
			fixture_.Setup({ "--etw-session-name"s, "RealtimeMetricSession"s });
		}

		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(RealtimeOpenSessionTest)
		{
			auto presenter = LaunchPresenter_(fixture_);
			Logger::WriteMessage(std::format("RealtimeMetric presenter pid={}\n", presenter.GetId()).c_str());
			auto pSession = OpenSession_(fixture_);
			Assert::IsTrue((bool)pSession);
		}

		TEST_METHOD(RealtimeTrackProcessTest)
		{
			auto presenter = LaunchPresenter_(fixture_);
			Logger::WriteMessage(std::format("RealtimeMetric presenter pid={}\n", presenter.GetId()).c_str());
			auto pSession = OpenSession_(fixture_);
			auto processTracker = TrackPresenterAndWaitForFirstFrame_(
				*pSession, fixture_, presenter, "rt-metric-track");
			processTracker.Reset();
		}
		TEST_METHOD(RealtimeFrameMetricsTest)
		{
			using namespace pmapi;

			auto presenter = LaunchPresenter_(fixture_);
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

			// PresentBench is paced with Sleep(10), which typically yields an
			// average CPU frame time near 15.3ms due to OS timer granularity.
			double expectedFtAvg = 15.3;
			double ftTolerance = 2.5;
			Assert::AreEqual(expectedFtAvg, dq.qeCpuFtAvg.As<double>(), ftTolerance,
				L"CPU frame time avg not within tolerance (ms).");
			processTracker.Reset();
		}
	};
}
