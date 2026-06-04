#include "IntervalWaiter.h"
#include "win/WinAPI.h"
#include <cmath>
#include <thread>

namespace pmon::util
{
	IntervalWaiter::IntervalWaiter(double intervalSeconds, int64_t syncTimestamp, Options options)
		:
		intervalSeconds_{ intervalSeconds },
		options_{ options },
		waiter_{ options.spinBufferSeconds },
		timer_{ syncTimestamp }
	{}

	IntervalWaiter::IntervalWaiter(double intervalSeconds, Options options)
		:
		intervalSeconds_{ intervalSeconds },
		options_{ options },
		waiter_{ options.spinBufferSeconds }
	{}

	IntervalWaiter::IntervalWaiter(double intervalSeconds, int64_t syncTimestamp, double waitBuffer)
		:
		IntervalWaiter{ intervalSeconds, syncTimestamp, Options{
			.mechanism = WaitMechanism::HighPrecisionTimer,
			.spinBufferSeconds = waitBuffer,
		} }
	{}

	IntervalWaiter::IntervalWaiter(double intervalSeconds, double waitBuffer)
		:
		IntervalWaiter{ intervalSeconds, Options{
			.mechanism = WaitMechanism::HighPrecisionTimer,
			.spinBufferSeconds = waitBuffer,
		} }
	{}

	void IntervalWaiter::SetInterval(double intervalSeconds)
	{
		intervalSeconds_ = intervalSeconds;
	}

	void IntervalWaiter::SetInterval(std::chrono::nanoseconds interval)
	{
		intervalSeconds_ = double(interval.count()) / 1'000'000'000.;
	}

	void IntervalWaiter::SetOptions(Options options)
	{
		options_ = options;
	}

	IntervalWaiter::Options IntervalWaiter::GetOptions() const
	{
		return options_;
	}

	IntervalWaiter::WaitResult IntervalWaiter::Wait()
	{
		WaitResult res{};
		const auto waitTimeSeconds = [=, this, &res] {
			const auto t = timer_.Peek();
			res.targetSec = lastTargetTime_ + intervalSeconds_;
			// if we are on-time
			if (t <= res.targetSec) {
				lastTargetTime_ = res.targetSec;
				return res.targetSec - t;
			}
			// if we are late, reset target to NOW and do not wait
			lastTargetTime_ = t;
			res.errorSec = res.targetSec - t;
			return 0.;
		}();
		if (waitTimeSeconds > 0.) {
			res.errorSec = WaitFor(waitTimeSeconds);
		}
		return res;
	}

	int64_t IntervalWaiter::TargetTimeToTimestamp(double targetTime) const
	{
		return timer_.TimeToTimestamp(targetTime);
	}

	double IntervalWaiter::WaitFor(double seconds)
	{
		switch (options_.mechanism) {
		case WaitMechanism::HighPrecisionTimer:
			return waiter_.WaitWithBuffer(seconds, options_.spinBufferSeconds);
		case WaitMechanism::Sleep:
		default:
			Sleep((DWORD)std::ceil(seconds * 1000.));
			return 0.;
		}
	}
}
