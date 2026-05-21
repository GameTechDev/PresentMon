// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include <Core/source/win/WinAPI.h>
#include <CommonUtilities/Qpc.h>
#include <CommonUtilities/PrecisionWaiter.h>
#include <CommonUtilities/IntervalWaiter.h>
#include <format>
#include <chrono>
#include <thread>

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilityTests
{
	using namespace std::literals;
	using namespace pmon;

	void AssertWithinTolerance(double test, double expected, double tolerance)
	{
		const auto diff = test - expected;
		Assert::IsTrue(diff <= tolerance, std::format(L"{} - {} = {} | exceeds {}", test, expected, diff, tolerance).c_str());
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
				AssertWithinTolerance(timer.Peek(), dur, 0.000'020);
			}
		}
		TEST_METHOD(QpcTimerSpinChrono)
		{
			using Timer = std::chrono::high_resolution_clock;
			const auto totalWait = 0.075;
			const auto start = Timer::now();
			util::QpcTimer timer;
			std::this_thread::sleep_for(32ms);
			timer.SpinWaitUntil(totalWait);
			const auto dur = std::chrono::duration<double>(Timer::now() - start).count();
			AssertWithinTolerance(dur, totalWait, 0.000'020);
		}
		TEST_METHOD(PrecisionWaiterChrono)
		{
			using Timer = std::chrono::high_resolution_clock;
			const auto totalWait = 0.016'666;
			util::PrecisionWaiter waiter;
			for (int i = 0; i < 5; i++) {
				const auto start = Timer::now();
				waiter.Wait(totalWait);
				const auto dur = std::chrono::duration<double>(Timer::now() - start).count();
				AssertWithinTolerance(dur, totalWait, 0.000'020);
			}
		}
		TEST_METHOD(IntervalWaiterChrono)
		{
			using Timer = std::chrono::high_resolution_clock;
			const auto waitInterval = 0.040;
			const auto start = Timer::now();
			util::IntervalWaiter waiter{ waitInterval };
			for (int i = 0; i < 5; i++) {
				Sleep(16);
				waiter.Wait();
				const auto dur = std::chrono::duration<double>(Timer::now() - start).count();
				AssertWithinTolerance(dur, waitInterval * (i + 1), 0.000'020);
			}
		}
		// TODO: interval waiter test with late Wait() call
	};
}
