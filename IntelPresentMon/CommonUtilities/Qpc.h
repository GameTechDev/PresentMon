// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace pmon::util
{
	int64_t GetCurrentTimestamp() noexcept;
	double GetTimestampPeriodSeconds() noexcept;
	void SpinWaitUntilTimestamp(int64_t timestamp) noexcept;
	double TimestampDeltaToSeconds(int64_t start, int64_t end, double period) noexcept;
	double TimestampDeltaToMilliSeconds(uint64_t duration, uint64_t qpcFrequency) noexcept;
    double TimestampDeltaToMilliSeconds(uint64_t start, uint64_t end, uint64_t qpcFrequency) noexcept;
    double TimestampDeltaToSignedMilliSeconds(uint64_t start, uint64_t end, uint64_t qpcFrequency) noexcept;

	class QpcTimer
	{
	public:
		QpcTimer() noexcept;
		double Mark() noexcept;
		double Peek() const noexcept;
		int64_t GetStartTimestamp() const noexcept;
		void SpinWaitUntil(double seconds) const noexcept;
	private:
		double performanceCounterPeriod_;
		int64_t startTimestamp_ = 0;
	};

    class QpcConverter
    {
    public:
		QpcConverter(uint64_t qpcFrequency, uint64_t sessionStartTimestamp = 0) noexcept
			: qpcFrequency_(qpcFrequency)
			, msPerTick_(qpcFrequency_ == 0 ? 0.0 : 1000.0 / double(qpcFrequency_))
			, sessionStartTimestamp_(sessionStartTimestamp)
		{
		}

        uint64_t GetFrequency() const noexcept { return qpcFrequency_; }
        double   GetMilliSecondsPerTick() const noexcept { return msPerTick_; }
        uint64_t GetSessionStartTimestamp() const noexcept { return sessionStartTimestamp_; }

        // Duration in ticks -> ms
        double TicksToMilliSeconds(uint64_t ticks) const noexcept;

        // Unsigned delta (0 if end <= start or either is 0)
        double DeltaUnsignedMilliSeconds(uint64_t start, uint64_t end) const noexcept;

        // Signed delta (positive if end > start; negative if end < start; 0 if invalid)
        double DeltaSignedMilliSeconds(uint64_t start, uint64_t end) const noexcept;

        // Convenience: raw duration already a tick count (e.g. TimeInPresent)
        double DurationMilliSeconds(uint64_t tickCount) const noexcept;

    private:
        uint64_t qpcFrequency_{ 0 };
        double   msPerTick_{ 0.0 };
        uint64_t sessionStartTimestamp_{ 0 };
    };
}