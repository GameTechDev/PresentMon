#include "Qpc.h"
#include "win/WinAPI.h"
#include "log/Log.h"
#include <thread>


namespace pmon::util
{
	int64_t GetCurrentTimestamp() noexcept
	{
		LARGE_INTEGER timestamp;
		if (!QueryPerformanceCounter(&timestamp)) {
			pmlog_error("qpc failed").hr().every(50);
		}
		return (int64_t)timestamp.QuadPart;
	}
	double GetTimestampFrequencyDouble() noexcept
	{
		return double(GetTimestampFrequencyUint64());
	}
	uint64_t GetTimestampFrequencyUint64() noexcept
	{
		LARGE_INTEGER freq;
		if (!QueryPerformanceFrequency(&freq)) {
			pmlog_error("qpc frequency failed").hr().every(5);
			return 0;
		}
		return (uint64_t)freq.QuadPart;
	}
	double GetTimestampPeriodSeconds() noexcept
	{
		const auto frequency = GetTimestampFrequencyDouble();
		return frequency == 0.0 ? 0.0 : 1.0 / frequency;
	}
	void SpinWaitUntilTimestamp(int64_t timestamp) noexcept
	{
		while (GetCurrentTimestamp() < timestamp) {
			std::this_thread::yield();
		}
	}
	double TimestampDeltaToSeconds(int64_t start, int64_t end, double period) noexcept
	{
		return double(end - start) * period;
	}

	double TimestampDeltaToMilliSeconds(uint64_t duration, uint64_t qpcFrequency) noexcept
	{
		return qpcFrequency == 0 ? 0.0 : (duration * 1000.0) / double(qpcFrequency);
	}

	double TimestampDeltaToMilliSeconds(uint64_t start, uint64_t end, uint64_t qpcFrequency) noexcept
	{
		return (end <= start || qpcFrequency == 0) ? 0.0 : TimestampDeltaToMilliSeconds(end - start, qpcFrequency);
	}

	double TimestampDeltaToSignedMilliSeconds(uint64_t start, uint64_t end, uint64_t qpcFrequency) noexcept
	{
		if (qpcFrequency == 0 || start == 0 || end == 0 || start == end) {
			return 0.0;
		}
		return end > start
			? TimestampDeltaToMilliSeconds(end - start, qpcFrequency)
			: -TimestampDeltaToMilliSeconds(start - end, qpcFrequency);
	}

	QpcTimer::QpcTimer() noexcept
	{
		performanceCounterPeriod_ = GetTimestampPeriodSeconds();
		Mark();
	}
	double QpcTimer::Mark() noexcept
	{
		const auto newTimestamp = GetCurrentTimestamp();
		const auto delta = TimestampDeltaToSeconds(startTimestamp_, newTimestamp, performanceCounterPeriod_);
		startTimestamp_ = newTimestamp;
		return delta;
	}
	double QpcTimer::Peek() const noexcept
	{
		const auto newTimestamp = GetCurrentTimestamp();
		const auto delta = TimestampDeltaToSeconds(startTimestamp_, newTimestamp, performanceCounterPeriod_);
		return delta;
	}
	int64_t QpcTimer::GetStartTimestamp() const noexcept
	{
		return startTimestamp_;
	}
	void QpcTimer::SpinWaitUntil(double seconds) const noexcept
	{
		while (Peek() < seconds) {
			std::this_thread::yield();
		}
	}

	// Duration in ticks -> ms
	double QpcConverter::TicksToMilliSeconds(uint64_t ticks) const noexcept
	{
		return ticks * msPerTick_;
	}

	// Unsigned delta (0 if end <= start or either is 0)
	double QpcConverter::DeltaUnsignedMilliSeconds(uint64_t start, uint64_t end) const noexcept
	{
		return (end <= start || start == 0 || end == 0) ? 0.0 : TicksToMilliSeconds(end - start);
	}

	// Signed delta (positive if end > start; negative if end < start; 0 if invalid)
	double QpcConverter::DeltaSignedMilliSeconds(uint64_t start, uint64_t end) const noexcept
	{
		if (start == 0 || end == 0 || start == end) {
			return 0.0;
		}
		return end > start ? TicksToMilliSeconds(end - start)
			: -TicksToMilliSeconds(start - end);
	}

	// Convenience: raw duration already a tick count (e.g. TimeInPresent)
	double QpcConverter::DurationMilliSeconds(uint64_t tickCount) const noexcept
	{
		return TicksToMilliSeconds(tickCount);
	}
}
