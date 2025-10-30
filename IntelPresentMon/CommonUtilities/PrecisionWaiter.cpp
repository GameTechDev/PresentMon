#include "PrecisionWaiter.h"
#include "log/Log.h"
#include "win/WinAPI.h"
#include "Exception.h"
#include "PrecisionWaiter.h"


namespace pmon::util
{
	PrecisionWaiter::PrecisionWaiter(double waitBuffer) noexcept
		:
		defaultWaitBuffer_{ waitBuffer }
	{
		try {
			waitableTimer_ = win::Handle(CreateWaitableTimerExW(
				nullptr,
				nullptr,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
				TIMER_ALL_ACCESS
			));
		}
		catch (...) {
			pmlog_error(ReportException("Failed creating timer"));
		}
	}
	void PrecisionWaiter::Wait(double seconds, bool alertable) noexcept
	{
		WaitWithBuffer(seconds, defaultWaitBuffer_, alertable);
	}
	void PrecisionWaiter::WaitUnbuffered(double seconds, bool alertable) noexcept
	{
		WaitWithBuffer(seconds, 0., alertable);
	}
	void PrecisionWaiter::WaitWithBuffer(double seconds, double buffer, bool alertable) noexcept
	{
		try {
			if (waitableTimer_) {
				// mark the start time of the wait if we're doing buffer with precision spin
				if (buffer > 0.) {
					qpcTimer_.Mark();
				}
				// wait slightly (buffer seconds) shorter than required to compensate for overwait error
				const LARGE_INTEGER waitTime100ns{
					.QuadPart = -LONGLONG(double(seconds - defaultWaitBuffer_) * 10'000'000.)
				};
				// set the timer deadline
				if (!SetWaitableTimerEx(
					waitableTimer_,
					&waitTime100ns,
					0,
					nullptr,
					nullptr,
					nullptr,
					0
				)) {
					pmlog_error("Failed setting high resolution timer").hr().raise<Exception>();
				}
				// wait on the timer
				if (WaitForSingleObject(waitableTimer_, INFINITE)) {
					pmlog_error("Failed waiting on high resolution timer").hr().raise<Exception>();
				}
				// spin wait for the remainder buffer time
				if (buffer > 0.) {
					qpcTimer_.SpinWaitUntil(seconds);
				}
				// return so we don't do the fallback Sleep() wait
				return;
			}
		}
		catch (...) {
			pmlog_error(ReportException());
			// clear timer on error so that subsequent calls use the fallback
			try { waitableTimer_.Clear(); }
			catch (...) { pmlog_error(ReportException()); }
			// fall through to the fallback
		}
		// fallback if we fail high performance timing
		Sleep(DWORD(seconds * 1000.f));
	}
}