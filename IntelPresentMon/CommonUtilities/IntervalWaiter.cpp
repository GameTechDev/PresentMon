#include "IntervalWaiter.h"
#include <thread>

namespace pmon::util
{
	IntervalWaiter::IntervalWaiter(double intervalSeconds, int64_t syncTimestamp, double waitBuffer)
		:
		intervalSeconds_{ intervalSeconds },
		waiter_{ waitBuffer },
		timer_{ syncTimestamp }
	{}

	IntervalWaiter::IntervalWaiter(double intervalSeconds, double waitBuffer)
		:
		intervalSeconds_{ intervalSeconds },
		waiter_{ waitBuffer }
	{}

	void IntervalWaiter::SetInterval(double intervalSeconds)
	{
		intervalSeconds_ = intervalSeconds;
	}

	void IntervalWaiter::SetInterval(std::chrono::nanoseconds interval)
	{
		intervalSeconds_ = double(interval.count()) / 1'000'000'000.;
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
			res.errorSec = waiter_.Wait(waitTimeSeconds);
		}
		return res;
	}
}
