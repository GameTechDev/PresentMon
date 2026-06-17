// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "Qpc.h"
#include "PrecisionWaiter.h"
#include <chrono>

namespace pmon::util
{
	enum class WaitMechanism
	{
		Sleep,
		HighPrecisionTimer,
	};

	class IntervalWaiter
	{
	public:
		// types
		struct WaitResult
		{
			// Scheduled tick for this Wait() in timer seconds (on-time grid instant).
			double targetSec;
			// On-time: non-negative wait overrun from WaitFor. Late: negative lateness (targetSec - now).
			double errorSec;
		};
		struct Options
		{
			WaitMechanism mechanism = WaitMechanism::Sleep;
			// HighPrecisionTimer: spin buffer after timer wait. Sleep: wake this much early (no spin).
			double spinBufferSeconds = 0.;
		};
		// functions
		IntervalWaiter(double intervalSeconds, int64_t syncTimestamp, Options options = {});
		IntervalWaiter(double intervalSeconds, Options options = {});
		IntervalWaiter(double intervalSeconds, int64_t syncTimestamp, double waitBuffer);
		IntervalWaiter(double intervalSeconds, double waitBuffer);
		IntervalWaiter(const IntervalWaiter&) = delete;
		IntervalWaiter & operator=(const IntervalWaiter&) = delete;
		IntervalWaiter(IntervalWaiter&&) = delete;
		IntervalWaiter & operator=(IntervalWaiter&&) = delete;
		~IntervalWaiter() = default;
		void SetInterval(double intervalSeconds);
		void SetInterval(std::chrono::nanoseconds interval);
		void SetOptions(Options options);
		Options GetOptions() const;
		WaitResult Wait();
		int64_t TargetTimeToTimestamp(double targetTime) const;
	private:
		double WaitFor(double seconds);
		double intervalSeconds_;
		// Cadence anchor: scheduled time of the last completed (or re-synced) tick.
		double lastTargetTime_ = 0.;
		Options options_;
		PrecisionWaiter waiter_;
		QpcTimer timer_;
	};
}
