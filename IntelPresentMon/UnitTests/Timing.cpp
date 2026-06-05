// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include <Core/source/win/WinAPI.h>
#include <CommonUtilities/Qpc.h>
#include <CommonUtilities/PrecisionWaiter.h>
#include <CommonUtilities/IntervalWaiter.h>
#include <CommonUtilities/win/Handle.h>
#include <CommonUtilities/test/MachineExpectations.h>
#include <format>
#include <chrono>
#include <cmath>
#include <string_view>
#include <thread>
#include <vector>

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilityTests
{
	using namespace std::literals;
	using namespace pmon;

	double ScaleTolerance(double tolerance)
	{
		return tolerance * util::test::GetWaitMultiplier();
	}

	void AssertWithinTolerance(double test, double expected, double tolerance)
	{
		const auto scaledTolerance = ScaleTolerance(tolerance);
		const auto diff = test - expected;
		Assert::IsTrue(diff <= scaledTolerance, std::format(L"{} - {} = {} | exceeds {}", test, expected, diff, scaledTolerance).c_str());
	}

	double SecondsToMilliseconds(double seconds)
	{
		return seconds * 1000.;
	}

	void AssertErrorWithinTolerance(std::wstring_view name, int iteration, double actual, double expected, double error, double tolerance)
	{
		const auto scaledTolerance = ScaleTolerance(tolerance);
		const auto absError = std::abs(error);
		if (absError > scaledTolerance) {
			Logger::WriteMessage(std::format(
				L"{} iteration {} failed: actual={:.6f}ms expected={:.6f}ms error={:.6f}ms tolerance={:.6f}ms\n",
				name,
				iteration,
				SecondsToMilliseconds(actual),
				SecondsToMilliseconds(expected),
				SecondsToMilliseconds(error),
				SecondsToMilliseconds(scaledTolerance)).c_str());
		}
		Assert::IsTrue(absError <= scaledTolerance, std::format(
			L"{} iteration {}: actual={:.6f}ms expected={:.6f}ms error={:.6f}ms tolerance={:.6f}ms",
			name,
			iteration,
			SecondsToMilliseconds(actual),
			SecondsToMilliseconds(expected),
			SecondsToMilliseconds(error),
			SecondsToMilliseconds(scaledTolerance)).c_str());
	}

	void AssertAggregateErrorWithinTolerance(std::wstring_view name, double actual, double expected, double aggregateError, double tolerance)
	{
		const auto scaledTolerance = ScaleTolerance(tolerance);
		if (aggregateError > scaledTolerance) {
			Logger::WriteMessage(std::format(
				L"{} total failed: actual={:.6f}ms expected={:.6f}ms avgError={:.6f}ms tolerance={:.6f}ms\n",
				name,
				SecondsToMilliseconds(actual),
				SecondsToMilliseconds(expected),
				SecondsToMilliseconds(aggregateError),
				SecondsToMilliseconds(scaledTolerance)).c_str());
		}
		Assert::IsTrue(aggregateError <= scaledTolerance, std::format(
			L"{} total: actual={:.6f}ms expected={:.6f}ms avgError={:.6f}ms tolerance={:.6f}ms",
			name,
			SecondsToMilliseconds(actual),
			SecondsToMilliseconds(expected),
			SecondsToMilliseconds(aggregateError),
			SecondsToMilliseconds(scaledTolerance)).c_str());
	}

	struct WaitIterationData
	{
		double waitSeconds = 0.;
		double expectedSeconds = 0.;
		double errorSeconds = 0.;
	};

	struct LateWaitIterationData
	{
		int iteration = 0;
		bool late = false;
		double workSeconds = 0.;
		double waitSeconds = 0.;
		double iterationSeconds = 0.;
		double expectedIterationSeconds = 0.;
		double iterationErrorSeconds = 0.;
		double targetSeconds = 0.;
		double waiterErrorSeconds = 0.;
		double expectedWaiterErrorSeconds = 0.;
	};

	struct IntervalWaiterPhaseStepData
	{
		int step = 0;
		double workSeconds = 0.;
		double auxiliaryWorkSeconds = 0.;
		double waitSeconds = 0.;
		double iterationSeconds = 0.;
		double expectedIterationSeconds = 0.;
		double targetSeconds = 0.;
		double expectedTargetSeconds = 0.;
		double waiterErrorSeconds = 0.;
		double expectedWaiterErrorSeconds = 0.;
		double targetAdvanceSeconds = 0.;
	};

	void LogIntervalWaiterPhaseSteps(std::wstring_view testName, const std::vector<IntervalWaiterPhaseStepData>& steps)
	{
		for (const auto& result : steps) {
			if (result.iterationSeconds > 0. || result.expectedIterationSeconds > 0.) {
				Logger::WriteMessage(std::format(
					L"{} step {}: work={:.6f}ms auxWork={:.6f}ms wait={:.6f}ms total={:.6f}ms expectedTotal={:.6f}ms target={:.6f}ms expectedTarget={:.6f}ms waiterError={:.6f}ms targetAdvance={:.6f}ms\n",
					testName,
					result.step,
					SecondsToMilliseconds(result.workSeconds),
					SecondsToMilliseconds(result.auxiliaryWorkSeconds),
					SecondsToMilliseconds(result.waitSeconds),
					SecondsToMilliseconds(result.iterationSeconds),
					SecondsToMilliseconds(result.expectedIterationSeconds),
					SecondsToMilliseconds(result.targetSeconds),
					SecondsToMilliseconds(result.expectedTargetSeconds),
					SecondsToMilliseconds(result.waiterErrorSeconds),
					SecondsToMilliseconds(result.targetAdvanceSeconds)).c_str());
			}
			else {
				Logger::WriteMessage(std::format(
					L"{} step {}: work={:.6f}ms auxWork={:.6f}ms wait={:.6f}ms target={:.6f}ms expectedTarget={:.6f}ms waiterError={:.6f}ms targetAdvance={:.6f}ms\n",
					testName,
					result.step,
					SecondsToMilliseconds(result.workSeconds),
					SecondsToMilliseconds(result.auxiliaryWorkSeconds),
					SecondsToMilliseconds(result.waitSeconds),
					SecondsToMilliseconds(result.targetSeconds),
					SecondsToMilliseconds(result.expectedTargetSeconds),
					SecondsToMilliseconds(result.waiterErrorSeconds),
					SecondsToMilliseconds(result.targetAdvanceSeconds)).c_str());
			}
		}
	}

	struct HighResolutionWaitableTimerIterationData
	{
		double waitSeconds = 0.;
		double expectedSeconds = 0.;
		double errorSeconds = 0.;
		bool timerSet = false;
		DWORD waitResult = WAIT_FAILED;
	};

	template<typename F>
	void RunWaitAccuracyTest(std::wstring_view name, double expectedSeconds, int iterations, double singleErrorThreshold, double aggregateErrorThresholdPerIteration, F&& wait)
	{
		std::vector<WaitIterationData> results;
		results.reserve(iterations);
		double totalWaitSeconds = 0.;
		double aggregateError = 0.;
		util::QpcTimer timer;
		for (int i = 0; i < iterations; i++) {
			timer.Mark();
			wait();
			const auto waitSeconds = timer.Peek();
			const auto error = waitSeconds - expectedSeconds;
			totalWaitSeconds += waitSeconds;
			aggregateError += std::abs(error);
			results.push_back({ waitSeconds, expectedSeconds, error });
		}

		for (int i = 0; i < int(results.size()); i++) {
			const auto& result = results[i];
			Logger::WriteMessage(std::format(
				L"{} iteration {}: wait={:.6f}ms expected={:.6f}ms error={:.6f}ms\n",
				name,
				i + 1,
				SecondsToMilliseconds(result.waitSeconds),
				SecondsToMilliseconds(result.expectedSeconds),
				SecondsToMilliseconds(result.errorSeconds)).c_str());
		}

		const auto expectedTotalSeconds = expectedSeconds * iterations;
		const auto averageError = aggregateError / iterations;
		Logger::WriteMessage(std::format(
			L"{} total: wait={:.6f}ms expected={:.6f}ms avgError={:.6f}ms iterations={}\n",
			name,
			SecondsToMilliseconds(totalWaitSeconds),
			SecondsToMilliseconds(expectedTotalSeconds),
			SecondsToMilliseconds(averageError),
			iterations).c_str());

		for (int i = 0; i < int(results.size()); i++) {
			const auto& result = results[i];
			AssertErrorWithinTolerance(name, i + 1, result.waitSeconds, result.expectedSeconds, result.errorSeconds, singleErrorThreshold);
		}
		AssertAggregateErrorWithinTolerance(name, totalWaitSeconds, expectedTotalSeconds, averageError, aggregateErrorThresholdPerIteration);
	}

	TEST_CLASS(TestTiming)
	{
	public:
		TEST_METHOD(QpcTimerPeekCumulativeSleep)
		{
			const auto periodMs = 1000. / 60.;
			const auto sleepPeriodMs = periodMs * 5;
			util::QpcTimer timer;
			for (int i = 0; i < 4; i++) {
				Sleep(DWORD(sleepPeriodMs));
				AssertWithinTolerance(timer.Peek() * 1000., sleepPeriodMs * (i+1), periodMs * (i+1));
			}
		}
		TEST_METHOD(QpcTimerMarkPeekSleep)
		{
			const auto periodMs = 1000. / 60.;
			const auto sleepPeriodMs = periodMs * 5;
			util::QpcTimer timer;
			for (int i = 0; i < 4; i++) {
				timer.Mark();
				Sleep(DWORD(sleepPeriodMs));
				AssertWithinTolerance(timer.Peek() * 1000., sleepPeriodMs, periodMs);
			}
		}
		TEST_METHOD(QpcTimerMarkPeekChrono)
		{
			using Timer = std::chrono::high_resolution_clock;
			const auto periodMs = 1000. / 60.;
			const auto sleepPeriodMs = periodMs * 3;
			util::QpcTimer timer;
			for (int i = 0; i < 4; i++) {
				const auto start = Timer::now();
				timer.Mark();
				std::this_thread::sleep_for(1ms * sleepPeriodMs);
				const auto dur = std::chrono::duration<double>(Timer::now() - start).count();
				AssertWithinTolerance(timer.Peek(), dur, 0.000'050);
			}
		}
		TEST_METHOD(QpcTimerSpinChrono)
		{
			using Timer = std::chrono::high_resolution_clock;
			const auto totalWait = 0.040;
			const auto start = Timer::now();
			util::QpcTimer timer;
			std::this_thread::sleep_for(16ms);
			timer.SpinWaitUntil(totalWait);
			const auto dur = std::chrono::duration<double>(Timer::now() - start).count();
			AssertWithinTolerance(dur, totalWait, 0.000'100);
		}
		TEST_METHOD(QpcTimerMarkPeekHighResolutionWaitableTimer)
		{
			const auto waitPeriod = 0.010;
			const auto iterations = 4;
			const auto singleErrorThreshold = 0.001'100;
			const auto aggregateErrorThresholdPerIteration = 0.000'650;
			std::vector<HighResolutionWaitableTimerIterationData> results;
			results.reserve(iterations);
			double totalWait = 0.;
			double aggregateError = 0.;
			util::win::Handle highResolutionWaitableTimer{ CreateWaitableTimerExW(
				nullptr,
				nullptr,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
				TIMER_ALL_ACCESS
			) };
			util::QpcTimer timer;
			for (int i = 0; i < iterations; i++) {
				timer.Mark();
				const LARGE_INTEGER waitTime100ns{
					.QuadPart = -LONGLONG(waitPeriod * 10'000'000.)
				};
				const auto timerSet = SetWaitableTimerEx(
					highResolutionWaitableTimer,
					&waitTime100ns,
					0,
					nullptr,
					nullptr,
					nullptr,
					0
				);
				const auto waitResult = timerSet
					? WaitForSingleObject(highResolutionWaitableTimer, INFINITE)
					: DWORD(WAIT_FAILED);
				const auto wait = timerSet && waitResult == WAIT_OBJECT_0 ? timer.Peek() : 0.;
				const auto error = wait - waitPeriod;
				totalWait += wait;
				aggregateError += std::abs(error);
				results.push_back({ wait, waitPeriod, error, bool(timerSet), waitResult });
			}

			for (int i = 0; i < int(results.size()); i++) {
				const auto& result = results[i];
				Logger::WriteMessage(std::format(
					L"QpcTimerMarkPeekHighResolutionWaitableTimer iteration {}: wait={:.6f}ms expected={:.6f}ms error={:.6f}ms\n",
					i + 1,
					SecondsToMilliseconds(result.waitSeconds),
					SecondsToMilliseconds(result.expectedSeconds),
					SecondsToMilliseconds(result.errorSeconds)).c_str());
			}

			const auto expectedTotalWait = waitPeriod * iterations;
			const auto averageError = aggregateError / iterations;
			Logger::WriteMessage(std::format(
				L"QpcTimerMarkPeekHighResolutionWaitableTimer total: wait={:.6f}ms expected={:.6f}ms avgError={:.6f}ms iterations={}\n",
				SecondsToMilliseconds(totalWait),
				SecondsToMilliseconds(expectedTotalWait),
				SecondsToMilliseconds(averageError),
				iterations).c_str());
			Assert::IsTrue(bool(highResolutionWaitableTimer), L"Failed creating high resolution waitable timer");
			for (int i = 0; i < int(results.size()); i++) {
				const auto& result = results[i];
				Assert::IsTrue(result.timerSet, L"Failed setting high resolution waitable timer");
				Assert::AreEqual(DWORD(WAIT_OBJECT_0), result.waitResult, L"Failed waiting on high resolution waitable timer");
				AssertErrorWithinTolerance(L"QpcTimerMarkPeekHighResolutionWaitableTimer", i + 1, result.waitSeconds, result.expectedSeconds, result.errorSeconds, singleErrorThreshold);
			}
			AssertAggregateErrorWithinTolerance(
				L"QpcTimerMarkPeekHighResolutionWaitableTimer",
				totalWait,
				expectedTotalWait,
				averageError,
				aggregateErrorThresholdPerIteration);
		}
		TEST_METHOD(PrecisionWaiterChrono)
		{
			constexpr auto totalWait = 0.016'666;
			constexpr auto iterations = 5;
			constexpr auto singleErrorThreshold = 0.000'180;
			constexpr auto aggregateErrorThresholdPerIteration = 0.000'070;
			util::PrecisionWaiter waiter;
			RunWaitAccuracyTest(L"PrecisionWaiterChrono", totalWait, iterations, singleErrorThreshold, aggregateErrorThresholdPerIteration, [&]() {
				waiter.Wait(totalWait);
			});
		}
		TEST_METHOD(IntervalWaiterChrono)
		{
			constexpr auto waitInterval = 0.040;
			constexpr auto iterations = 5;
			constexpr auto workMillis = 16;
			constexpr auto singleErrorThreshold = 0.000'180;
			constexpr auto aggregateErrorThresholdPerIteration = 0.000'070;
			util::IntervalWaiter waiter{ waitInterval, util::IntervalWaiter::Options{
				.mechanism = util::WaitMechanism::HighPrecisionTimer,
				.spinBufferSeconds = util::PrecisionWaiter::standardWaitBuffer,
			} };
			RunWaitAccuracyTest(L"IntervalWaiterChrono", waitInterval, iterations, singleErrorThreshold, aggregateErrorThresholdPerIteration, [&]() {
				Sleep(workMillis);
				waiter.Wait();
			});
		}
		TEST_METHOD(IntervalWaiterLateChrono)
		{
			constexpr auto waitInterval = 0.010;
			constexpr auto onTimeWorkSeconds = 0.004;
			constexpr auto lateWorkSeconds = 0.012;
			constexpr auto iterations = 8;
			constexpr auto latePeriod = 4;
			constexpr auto immediateReturnThreshold = 0.000'060;
			constexpr auto singleCadenceErrorThreshold = 0.000'180;
			constexpr auto aggregateCadenceErrorThresholdPerIteration = 0.000'070;
			constexpr auto reportedLateErrorThreshold = immediateReturnThreshold;
			constexpr auto lateOverrunSeconds = lateWorkSeconds - waitInterval;

			std::vector<LateWaitIterationData> results;
			results.reserve(iterations);
			double totalIterationSeconds = 0.;
			double expectedTotalIterationSeconds = 0.;
			double aggregateIterationError = 0.;
			double totalOnTimeCadenceSeconds = 0.;
			double aggregateOnTimeCadenceError = 0.;
			int onTimeIterations = 0;
			util::IntervalWaiter waiter{ waitInterval, util::IntervalWaiter::Options{
				.mechanism = util::WaitMechanism::HighPrecisionTimer,
				.spinBufferSeconds = util::PrecisionWaiter::standardWaitBuffer,
			} };
			util::QpcTimer iterationTimer;
			util::QpcTimer workTimer;
			util::QpcTimer waitTimer;
			for (int i = 0; i < iterations; i++) {
				const auto iteration = i + 1;
				const auto late = iteration % latePeriod == 0;
				const auto recoveryAfterLate = !late && iteration > 1 && ((iteration - 1) % latePeriod == 0);
				const auto workSeconds = late ? lateWorkSeconds : onTimeWorkSeconds;
				const auto expectedIterationSeconds = late ? lateWorkSeconds
					: recoveryAfterLate ? waitInterval - lateOverrunSeconds
					: waitInterval;
				iterationTimer.Mark();
				workTimer.Mark();
				workTimer.SpinWaitUntil(workSeconds);
				const auto actualWorkSeconds = workTimer.Peek();
				waitTimer.Mark();
				const auto result = waiter.Wait();
				const auto waitSeconds = waitTimer.Peek();
				const auto iterationSeconds = iterationTimer.Peek();
				const auto expectedReportedError = waitInterval - actualWorkSeconds;
				const auto iterationError = iterationSeconds - expectedIterationSeconds;
				totalIterationSeconds += iterationSeconds;
				expectedTotalIterationSeconds += expectedIterationSeconds;
				aggregateIterationError += std::abs(iterationError);
				results.push_back({
					iteration,
					late,
					actualWorkSeconds,
					waitSeconds,
					iterationSeconds,
					expectedIterationSeconds,
					iterationError,
					result.targetSec,
					result.errorSec,
					expectedReportedError,
				});

				if (!late) {
					const auto expectedCadenceSeconds = recoveryAfterLate ? waitInterval - lateOverrunSeconds : waitInterval;
					const auto cadenceError = iterationSeconds - expectedCadenceSeconds;
					totalOnTimeCadenceSeconds += iterationSeconds;
					aggregateOnTimeCadenceError += std::abs(cadenceError);
					onTimeIterations++;
				}
			}

			for (const auto& result : results) {
				Logger::WriteMessage(std::format(
					L"IntervalWaiterLateChrono iteration {}: work={:.6f}ms wait={:.6f}ms total={:.6f}ms expected={:.6f}ms totalError={:.6f}ms target={:.6f}ms waiterError={:.6f}ms late={}\n",
					result.iteration,
					SecondsToMilliseconds(result.workSeconds),
					SecondsToMilliseconds(result.waitSeconds),
					SecondsToMilliseconds(result.iterationSeconds),
					SecondsToMilliseconds(result.expectedIterationSeconds),
					SecondsToMilliseconds(result.iterationErrorSeconds),
					SecondsToMilliseconds(result.targetSeconds),
					SecondsToMilliseconds(result.waiterErrorSeconds),
					result.late).c_str());
			}
			double expectedOnTimeCadenceSeconds = 0.;
			for (const auto& result : results) {
				if (!result.late) {
					const auto recoveryAfterLate = result.iteration > 1 && ((result.iteration - 1) % latePeriod == 0);
					expectedOnTimeCadenceSeconds += recoveryAfterLate ? waitInterval - lateOverrunSeconds : waitInterval;
				}
			}
			const auto averageIterationError = aggregateIterationError / iterations;
			const auto averageOnTimeCadenceError = aggregateOnTimeCadenceError / onTimeIterations;
			Logger::WriteMessage(std::format(
				L"IntervalWaiterLateChrono total: wait={:.6f}ms expected={:.6f}ms avgError={:.6f}ms iterations={}\n",
				SecondsToMilliseconds(totalIterationSeconds),
				SecondsToMilliseconds(expectedTotalIterationSeconds),
				SecondsToMilliseconds(averageIterationError),
				iterations).c_str());
			Logger::WriteMessage(std::format(
				L"IntervalWaiterLateChrono on-time total: wait={:.6f}ms expected={:.6f}ms avgError={:.6f}ms iterations={}\n",
				SecondsToMilliseconds(totalOnTimeCadenceSeconds),
				SecondsToMilliseconds(expectedOnTimeCadenceSeconds),
				SecondsToMilliseconds(averageOnTimeCadenceError),
				onTimeIterations).c_str());

			for (const auto& result : results) {
				if (result.late) {
					AssertErrorWithinTolerance(L"IntervalWaiterLateChrono late immediate wait", result.iteration, result.waitSeconds, 0., result.waitSeconds, immediateReturnThreshold);
					AssertErrorWithinTolerance(L"IntervalWaiterLateChrono late reported error", result.iteration, result.waiterErrorSeconds, result.expectedWaiterErrorSeconds, result.waiterErrorSeconds - result.expectedWaiterErrorSeconds, reportedLateErrorThreshold);
				}
				else {
					const auto recoveryAfterLate = result.iteration > 1 && ((result.iteration - 1) % latePeriod == 0);
					const auto expectedCadenceSeconds = recoveryAfterLate ? waitInterval - lateOverrunSeconds : waitInterval;
					AssertErrorWithinTolerance(L"IntervalWaiterLateChrono on-time cadence", result.iteration, result.iterationSeconds, expectedCadenceSeconds, result.iterationSeconds - expectedCadenceSeconds, singleCadenceErrorThreshold);
				}
			}
			AssertAggregateErrorWithinTolerance(
				L"IntervalWaiterLateChrono total",
				totalIterationSeconds,
				expectedTotalIterationSeconds,
				averageIterationError,
				aggregateCadenceErrorThresholdPerIteration);

			AssertAggregateErrorWithinTolerance(
				L"IntervalWaiterLateChrono on-time cadence",
				totalOnTimeCadenceSeconds,
				expectedOnTimeCadenceSeconds,
				averageOnTimeCadenceError,
				aggregateCadenceErrorThresholdPerIteration);
		}
		TEST_METHOD(IntervalWaiterPhaseRecoveryPreservesGrid)
		{
			constexpr auto waitInterval = 0.010;
			constexpr auto onTimeWorkSeconds = 0.004;
			constexpr auto moderateLateWorkSeconds = 0.012;
			constexpr auto lateOverrunSeconds = moderateLateWorkSeconds - waitInterval;
			constexpr auto immediateReturnThreshold = 0.000'060;
			constexpr auto targetThreshold = 0.000'001;
			constexpr auto cadenceThreshold = 0.000'180;
			constexpr std::wstring_view testName = L"IntervalWaiterPhaseRecoveryPreservesGrid";

			std::vector<IntervalWaiterPhaseStepData> results;
			results.reserve(4);

			util::IntervalWaiter waiter{ waitInterval, util::IntervalWaiter::Options{
				.mechanism = util::WaitMechanism::HighPrecisionTimer,
				.spinBufferSeconds = util::PrecisionWaiter::standardWaitBuffer,
			} };
			util::QpcTimer workTimer;
			util::QpcTimer waitTimer;
			util::QpcTimer iterationTimer;

			workTimer.Mark();
			workTimer.SpinWaitUntil(onTimeWorkSeconds);
			const auto primeWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto prime = waiter.Wait();
			results.push_back({
				.step = 1,
				.workSeconds = primeWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.targetSeconds = prime.targetSec,
				.expectedTargetSeconds = waitInterval,
				.waiterErrorSeconds = prime.errorSec,
			});

			workTimer.Mark();
			workTimer.SpinWaitUntil(moderateLateWorkSeconds);
			const auto lateWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto late = waiter.Wait();
			results.push_back({
				.step = 2,
				.workSeconds = lateWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.targetSeconds = late.targetSec,
				.expectedTargetSeconds = 2. * waitInterval,
				.waiterErrorSeconds = late.errorSec,
			});
			const auto expectedRecoveryIterationSeconds = waitInterval + late.errorSec;

			iterationTimer.Mark();
			workTimer.Mark();
			workTimer.SpinWaitUntil(onTimeWorkSeconds);
			const auto recoveryWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto recovery = waiter.Wait();
			results.push_back({
				.step = 3,
				.workSeconds = recoveryWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.iterationSeconds = iterationTimer.Peek(),
				.expectedIterationSeconds = expectedRecoveryIterationSeconds,
				.targetSeconds = recovery.targetSec,
				.expectedTargetSeconds = 3. * waitInterval,
				.waiterErrorSeconds = recovery.errorSec,
			});

			iterationTimer.Mark();
			workTimer.Mark();
			workTimer.SpinWaitUntil(onTimeWorkSeconds);
			const auto resumedWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto resumed = waiter.Wait();
			results.push_back({
				.step = 4,
				.workSeconds = resumedWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.iterationSeconds = iterationTimer.Peek(),
				.expectedIterationSeconds = waitInterval,
				.targetSeconds = resumed.targetSec,
				.expectedTargetSeconds = 4. * waitInterval,
				.waiterErrorSeconds = resumed.errorSec,
			});

			LogIntervalWaiterPhaseSteps(testName, results);

			AssertErrorWithinTolerance(testName, 1, results[0].targetSeconds, results[0].expectedTargetSeconds, results[0].targetSeconds - results[0].expectedTargetSeconds, targetThreshold);
			AssertErrorWithinTolerance(testName, 2, results[1].targetSeconds, results[1].expectedTargetSeconds, results[1].targetSeconds - results[1].expectedTargetSeconds, targetThreshold);
			AssertErrorWithinTolerance(L"IntervalWaiterPhaseRecoveryPreservesGrid late immediate wait", 2, results[1].waitSeconds, 0., results[1].waitSeconds, immediateReturnThreshold);
			Assert::IsTrue(results[1].waiterErrorSeconds < 0.);
			Assert::IsTrue(results[1].waiterErrorSeconds > -(waitInterval + lateOverrunSeconds));
			AssertErrorWithinTolerance(testName, 3, results[2].targetSeconds, results[2].expectedTargetSeconds, results[2].targetSeconds - results[2].expectedTargetSeconds, targetThreshold);
			AssertErrorWithinTolerance(
				L"IntervalWaiterPhaseRecoveryPreservesGrid recovery cadence",
				3,
				results[2].iterationSeconds,
				results[2].expectedIterationSeconds,
				results[2].iterationSeconds - results[2].expectedIterationSeconds,
				cadenceThreshold);
			AssertErrorWithinTolerance(testName, 4, results[3].targetSeconds, results[3].expectedTargetSeconds, results[3].targetSeconds - results[3].expectedTargetSeconds, targetThreshold);
			AssertErrorWithinTolerance(
				L"IntervalWaiterPhaseRecoveryPreservesGrid resumed cadence",
				4,
				results[3].iterationSeconds,
				results[3].expectedIterationSeconds,
				results[3].iterationSeconds - results[3].expectedIterationSeconds,
				cadenceThreshold);
		}
		TEST_METHOD(IntervalWaiterPhaseRecoveryConsecutiveModerateLate)
		{
			constexpr auto waitInterval = 0.010;
			constexpr auto onTimeWorkSeconds = 0.004;
			constexpr auto moderateLateWorkSeconds = 0.012;
			constexpr auto immediateReturnThreshold = 0.000'060;
			constexpr auto targetThreshold = 0.000'001;
			constexpr std::wstring_view testName = L"IntervalWaiterPhaseRecoveryConsecutiveModerateLate";

			std::vector<IntervalWaiterPhaseStepData> results;
			results.reserve(3);

			util::IntervalWaiter waiter{ waitInterval, util::IntervalWaiter::Options{
				.mechanism = util::WaitMechanism::HighPrecisionTimer,
				.spinBufferSeconds = util::PrecisionWaiter::standardWaitBuffer,
			} };
			util::QpcTimer workTimer;
			util::QpcTimer waitTimer;

			for (int step = 1; step <= 2; step++) {
				workTimer.Mark();
				workTimer.SpinWaitUntil(onTimeWorkSeconds);
				const auto onTimeWorkSecondsActual = workTimer.Peek();
				waiter.Wait();
				workTimer.Mark();
				workTimer.SpinWaitUntil(moderateLateWorkSeconds);
				const auto moderateLateWorkSecondsActual = workTimer.Peek();
				waitTimer.Mark();
				const auto late = waiter.Wait();
				results.push_back({
					.step = step,
					.workSeconds = moderateLateWorkSecondsActual,
					.auxiliaryWorkSeconds = onTimeWorkSecondsActual,
					.waitSeconds = waitTimer.Peek(),
					.targetSeconds = late.targetSec,
					.expectedTargetSeconds = double(step) * 2. * waitInterval,
					.waiterErrorSeconds = late.errorSec,
				});
			}

			workTimer.Mark();
			workTimer.SpinWaitUntil(onTimeWorkSeconds);
			const auto nextWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto next = waiter.Wait();
			results.push_back({
				.step = 3,
				.workSeconds = nextWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.targetSeconds = next.targetSec,
				.expectedTargetSeconds = 5. * waitInterval,
				.waiterErrorSeconds = next.errorSec,
			});

			LogIntervalWaiterPhaseSteps(testName, results);

			for (int step = 1; step <= 2; step++) {
				const auto& result = results[step - 1];
				AssertErrorWithinTolerance(testName, step, result.targetSeconds, result.expectedTargetSeconds, result.targetSeconds - result.expectedTargetSeconds, targetThreshold);
				AssertErrorWithinTolerance(L"IntervalWaiterPhaseRecoveryConsecutiveModerateLate late immediate wait", step, result.waitSeconds, 0., result.waitSeconds, immediateReturnThreshold);
			}
			const auto& nextResult = results[2];
			AssertErrorWithinTolerance(testName, 3, nextResult.targetSeconds, nextResult.expectedTargetSeconds, nextResult.targetSeconds - nextResult.expectedTargetSeconds, targetThreshold);
		}
		TEST_METHOD(IntervalWaiterPhaseResetMissesNextBus)
		{
			constexpr auto waitInterval = 0.010;
			constexpr auto onTimeWorkSeconds = 0.004;
			constexpr auto phaseResetWorkSeconds = 0.022;
			constexpr auto immediateReturnThreshold = 0.000'060;
			constexpr auto reportedLateErrorThreshold = 0.000'180;
			constexpr auto targetThreshold = 0.000'001;
			constexpr auto cadenceThreshold = 0.000'180;
			constexpr auto phaseAdvanceThreshold = 0.000'180;
			constexpr std::wstring_view testName = L"IntervalWaiterPhaseResetMissesNextBus";

			std::vector<IntervalWaiterPhaseStepData> results;
			results.reserve(3);

			util::IntervalWaiter waiter{ waitInterval, util::IntervalWaiter::Options{
				.mechanism = util::WaitMechanism::HighPrecisionTimer,
				.spinBufferSeconds = util::PrecisionWaiter::standardWaitBuffer,
			} };
			util::QpcTimer workTimer;
			util::QpcTimer waitTimer;
			util::QpcTimer iterationTimer;

			workTimer.Mark();
			workTimer.SpinWaitUntil(onTimeWorkSeconds);
			const auto primeWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto prime = waiter.Wait();
			results.push_back({
				.step = 1,
				.workSeconds = primeWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.targetSeconds = prime.targetSec,
				.expectedTargetSeconds = waitInterval,
				.waiterErrorSeconds = prime.errorSec,
			});

			workTimer.Mark();
			workTimer.SpinWaitUntil(phaseResetWorkSeconds);
			const auto actualResetWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto missedBuses = waiter.Wait();
			results.push_back({
				.step = 2,
				.workSeconds = actualResetWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.targetSeconds = missedBuses.targetSec,
				.expectedTargetSeconds = 2. * waitInterval,
				.waiterErrorSeconds = missedBuses.errorSec,
				.expectedWaiterErrorSeconds = waitInterval - actualResetWorkSeconds,
			});

			iterationTimer.Mark();
			workTimer.Mark();
			workTimer.SpinWaitUntil(onTimeWorkSeconds);
			const auto afterResetWorkSeconds = workTimer.Peek();
			waitTimer.Mark();
			const auto afterReset = waiter.Wait();
			results.push_back({
				.step = 3,
				.workSeconds = afterResetWorkSeconds,
				.waitSeconds = waitTimer.Peek(),
				.iterationSeconds = iterationTimer.Peek(),
				.expectedIterationSeconds = waitInterval,
				.targetSeconds = afterReset.targetSec,
				.waiterErrorSeconds = afterReset.errorSec,
				.targetAdvanceSeconds = afterReset.targetSec - missedBuses.targetSec,
			});

			LogIntervalWaiterPhaseSteps(testName, results);

			AssertErrorWithinTolerance(testName, 1, results[0].targetSeconds, results[0].expectedTargetSeconds, results[0].targetSeconds - results[0].expectedTargetSeconds, targetThreshold);
			AssertErrorWithinTolerance(testName, 2, results[1].targetSeconds, results[1].expectedTargetSeconds, results[1].targetSeconds - results[1].expectedTargetSeconds, targetThreshold);
			AssertErrorWithinTolerance(L"IntervalWaiterPhaseResetMissesNextBus late immediate wait", 2, results[1].waitSeconds, 0., results[1].waitSeconds, immediateReturnThreshold);
			Assert::IsTrue(results[1].waiterErrorSeconds < -waitInterval);
			AssertErrorWithinTolerance(
				L"IntervalWaiterPhaseResetMissesNextBus late reported error",
				2,
				results[1].waiterErrorSeconds,
				results[1].expectedWaiterErrorSeconds,
				results[1].waiterErrorSeconds - results[1].expectedWaiterErrorSeconds,
				reportedLateErrorThreshold);
			Assert::IsTrue(results[2].targetAdvanceSeconds > waitInterval + phaseAdvanceThreshold);
			AssertErrorWithinTolerance(
				L"IntervalWaiterPhaseResetMissesNextBus post-reset cadence",
				3,
				results[2].iterationSeconds,
				results[2].expectedIterationSeconds,
				results[2].iterationSeconds - results[2].expectedIterationSeconds,
				cadenceThreshold);
		}
		TEST_METHOD(IntervalWaiterPhaseResetVersusRecoveryTargetAdvance)
		{
			constexpr auto waitInterval = 0.010;
			constexpr auto onTimeWorkSeconds = 0.004;
			constexpr auto moderateLateWorkSeconds = 0.012;
			constexpr auto phaseResetWorkSeconds = 0.022;
			constexpr auto phaseAdvanceThreshold = 0.000'180;
			constexpr std::wstring_view testName = L"IntervalWaiterPhaseResetVersusRecoveryTargetAdvance";

			auto measureTargetAdvance = [&](double lateWorkSeconds, int step) {
				util::IntervalWaiter waiter{ waitInterval, util::IntervalWaiter::Options{
					.mechanism = util::WaitMechanism::HighPrecisionTimer,
					.spinBufferSeconds = util::PrecisionWaiter::standardWaitBuffer,
				} };
				util::QpcTimer workTimer;
				workTimer.Mark();
				workTimer.SpinWaitUntil(onTimeWorkSeconds);
				waiter.Wait();
				workTimer.Mark();
				workTimer.SpinWaitUntil(lateWorkSeconds);
				const auto actualLateWorkSeconds = workTimer.Peek();
				const auto late = waiter.Wait();
				workTimer.Mark();
				workTimer.SpinWaitUntil(onTimeWorkSeconds);
				const auto next = waiter.Wait();
				return IntervalWaiterPhaseStepData{
					.step = step,
					.workSeconds = actualLateWorkSeconds,
					.targetSeconds = late.targetSec,
					.expectedTargetSeconds = next.targetSec,
					.waiterErrorSeconds = late.errorSec,
					.targetAdvanceSeconds = next.targetSec - late.targetSec,
				};
			};

			std::vector<IntervalWaiterPhaseStepData> results;
			results.push_back(measureTargetAdvance(moderateLateWorkSeconds, 1));
			results.push_back(measureTargetAdvance(phaseResetWorkSeconds, 2));

			LogIntervalWaiterPhaseSteps(testName, results);
			Logger::WriteMessage(std::format(
				L"{} summary: recoveryAdvance={:.6f}ms resetAdvance={:.6f}ms interval={:.6f}ms\n",
				testName,
				SecondsToMilliseconds(results[0].targetAdvanceSeconds),
				SecondsToMilliseconds(results[1].targetAdvanceSeconds),
				SecondsToMilliseconds(waitInterval)).c_str());

			AssertErrorWithinTolerance(
				L"IntervalWaiterPhaseResetVersusRecoveryTargetAdvance recovery advance",
				1,
				results[0].targetAdvanceSeconds,
				waitInterval,
				results[0].targetAdvanceSeconds - waitInterval,
				phaseAdvanceThreshold);
			Assert::IsTrue(results[1].targetAdvanceSeconds > waitInterval + phaseAdvanceThreshold);
			Assert::IsTrue(results[1].targetAdvanceSeconds > results[0].targetAdvanceSeconds + phaseAdvanceThreshold);
		}
	};
}
