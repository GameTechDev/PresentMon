// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "Qpc.h"
#include "PrecisionWaiter.h"
#include <chrono>

namespace pmon::util
{
	class IntervalWaiter
	{
	public:
		// types
		struct WaitResult
		{
			double targetSec;
			double errorSec;
		};
		// functions
		IntervalWaiter(double intervalSeconds, int64_t syncTimestamp, double waitBuffer = PrecisionWaiter::standardWaitBuffer);
		IntervalWaiter(double intervalSeconds, double waitBuffer = PrecisionWaiter::standardWaitBuffer);
		IntervalWaiter(const IntervalWaiter&) = delete;
		IntervalWaiter & operator=(const IntervalWaiter&) = delete;
		IntervalWaiter(IntervalWaiter&&) = delete;
		IntervalWaiter & operator=(IntervalWaiter&&) = delete;
		~IntervalWaiter() = default;
		void SetInterval(double intervalSeconds);
		void SetInterval(std::chrono::nanoseconds interval);
		WaitResult Wait();
	private:
		double intervalSeconds_;
		double lastTargetTime_ = 0.;
		PrecisionWaiter waiter_;
		QpcTimer timer_;
	};
}
