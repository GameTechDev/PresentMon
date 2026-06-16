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
		// Decide how long to block (if at all) and update the cadence anchor (lastTargetTime_).
		// lastTargetTime_ is the scheduled instant of the last completed tick; the next tick is
		// always lastTargetTime_ + intervalSeconds_. res.targetSec is that next tick and is always
		// the on-time deadline for this call, even when we arrive late.
		const auto waitTimeSeconds = [=, this, &res] {
			const auto t = timer_.Peek();
			res.targetSec = lastTargetTime_ + intervalSeconds_;

			if (t <= res.targetSec) {
				// On time: wait until the scheduled tick, then advance the anchor to that tick.
				lastTargetTime_ = res.targetSec;
				return res.targetSec - t;
			}

			// Late for res.targetSec. Do not wait; report how far past the deadline we are.
			// errorSec < 0 means lateness (targetSec - t). Consumers can map targetSec to a
			// timestamp via TargetTimeToTimestamp() for poll/window alignment on that grid slot.
			res.errorSec = res.targetSec - t;

			const auto nextTargetSec = res.targetSec + intervalSeconds_;
			if (t <= nextTargetSec) {
				// Moderately late: we missed this tick but can still hit the following one on the
				// same phase grid. Anchor stays on the missed tick so the next Wait() targets
				// nextTargetSec (shorter sleep catches up).
				lastTargetTime_ = res.targetSec;
			}
			else {
				// Too late for the next tick as well; abandon the old grid and re-anchor from now
				// so the next Wait() schedules intervalSeconds_ from t instead of chasing missed slots.
				lastTargetTime_ = t;
			}
			return 0.;
		}();

		// On-time path only: sleep/spin until the deadline. errorSec is wait overrun (>= 0), not lateness.
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
