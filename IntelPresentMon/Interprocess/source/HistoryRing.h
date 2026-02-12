#pragma once
#include "ShmRing.h"
#include "../../CommonUtilities/log/Verbose.h"
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace pmon::ipc
{
    // wrapper around ShmRing that adds the ability to search/address by timestamp
    // meant for use with telemetry data
    template<typename T, uint64_t T::*TimestampMember, size_t ReadBufferSize = 4>
    class HistoryRing
    {
    public:
        // functions
        HistoryRing(size_t capacity, ShmVector<T>::allocator_type alloc, bool backpressured = false)
            :
            samples_{ capacity, alloc, backpressured }
        {
            if (capacity < ReadBufferSize * 2) {
                throw std::logic_error{ "The capacity of a ShmRing must be at least double its ReadBufferSize" };
            }
        }
        bool Push(const T& sample, std::optional<uint32_t> timeoutMs = {})
        {
            return samples_.Push(sample, timeoutMs);
        }
        template<typename U = T,
            typename ValueT = std::decay_t<decltype(std::declval<U>().value)>,
            typename = std::enable_if_t<std::is_constructible_v<U, ValueT, uint64_t>>>
        bool Push(const ValueT& value, uint64_t timestamp, std::optional<uint32_t> timeoutMs = {})
        {
            return samples_.Push(U{ value, timestamp }, timeoutMs);
        }
        const T& Newest() const
        {
            auto&&[first, last] = samples_.GetSerialRange();
            return At(last - 1);
        }
        const T& At(size_t serial) const
        {
            return samples_.At(serial);
        }
        const T& Nearest(uint64_t timestamp) const
        {
            return samples_.At(NearestSerial(timestamp));
        }
        std::pair<size_t, size_t> GetSerialRange() const
        {
            return samples_.GetSerialRange();
        }
        bool Empty() const
        {
            return samples_.Empty();
        }
        size_t Size() const
        {
            const auto range = GetSerialRange();
            return range.second - range.first;
        }
        void MarkNextRead(size_t serial) const
        {
            samples_.MarkNextRead(serial);
        }
        // First serial with timestamp >= given timestamp.
        // If all samples have timestamp < given timestamp, returns last (one past end).
        size_t LowerBoundSerial(uint64_t timestamp) const
        {
            return BoundSerial_<BoundKind_::Lower>(timestamp);
        }
        // First serial with timestamp > given timestamp.
        // If all samples have timestamp <= given timestamp, returns last (one past end).
        size_t UpperBoundSerial(uint64_t timestamp) const
        {
            return BoundSerial_<BoundKind_::Upper>(timestamp);
        }
        // Find the serial whose timestamp is closest to the given timestamp.
        // If the timestamp is outside the stored range, clamps to first/last.
        size_t NearestSerial(uint64_t timestamp) const
        {
            const auto range = samples_.GetSerialRange();
            // empty ring case
            if (range.first == range.second) {
                pmlog_verb(util::log::V::ipc_ring)("Reading from empty history ring");
                return range.first;
            }

            // First serial with timestamp >= requested
            size_t serial = LowerBoundSerial(timestamp);

            // case where requested timestamp is newer than newest sample
            if (serial >= range.second) {
                // log timing and dump ring contents in case where ring has insufficient samples
                if (util::log::GlobalPolicy::VCheck(util::log::V::ipc_ring)) {
                    std::string recentSamples;
                    const size_t sampleCount = range.second - range.first;
                    const size_t maxSamples = 12;
                    const size_t dumpCount = sampleCount < maxSamples ? sampleCount : maxSamples;
                    const size_t startSerial = range.second - dumpCount;
                    for (size_t s = startSerial; s < range.second; ++s) {
                        const auto& sample = At(s);
                        if (!recentSamples.empty()) {
                            recentSamples += "\n";
                        }
                        if constexpr (HasValueMember_<T>::value) {
                            using ValueMemberT = std::decay_t<decltype(std::declval<T>().value)>;
                            if constexpr (IsFormattable_<ValueMemberT>::value) {
                                recentSamples += std::format("ts={} value={}", TimestampOf_(sample), sample.value);
                            }
                            else {
                                recentSamples += std::format("ts={}", TimestampOf_(sample));
                            }
                        }
                        else {
                            recentSamples += std::format("ts={}", TimestampOf_(sample));
                        }
                    }
                    pmlog_verb(util::log::V::ipc_ring)("Target timestamp past end of history ring")
                        .pmwatch(timestamp)
                        .pmwatch(range.second)
                        .pmwatch(int64_t(TimestampOf_(At(serial - 1))) - int64_t(timestamp))
                        .watch("recent_samples", recentSamples);
                }

                return range.second - 1;
            }

            // Check whether the previous sample is actually closer.
            // but only if there is a sample available before this one
            if (serial > range.first) {
                const auto nextTimestamp = TimestampOf_(At(serial));
                const auto prevTimestamp = TimestampOf_(At(serial - 1));
                const uint64_t dPrev = timestamp - prevTimestamp;
                const uint64_t dNext = nextTimestamp - timestamp;
                if (dPrev <= dNext) {
                    --serial;
                }
            }

            pmlog_verb(util::log::V::ipc_ring)("Found nearest sample")
                .pmwatch(timestamp)
                .pmwatch(serial)
                .pmwatch(int64_t(TimestampOf_(At(serial))) - int64_t(timestamp));
            return serial;
        }
        // Calls func(sample) for each sample whose timestamp is in [start, end].
        // Intended use case is calculation of stats (avg, min, %)
        // Returns the number of samples visited.
        template<typename F>
        size_t ForEachInTimestampRange(uint64_t start, uint64_t end, F&& func) const
        {
            const auto range = samples_.GetSerialRange();

            // Find the first sample with timestamp >= start
            size_t serial = LowerBoundSerial(start);

            size_t count = 0;
            // Walk forward until we leave the [start, end] window or hit last
            for (; serial < range.second; ++serial) {
                const T& s = At(serial);
                if (TimestampOf_(s) > end) {
                    break;
                }
                // s.timestamp is guaranteed >= start by LowerBoundSerial
                std::forward<F>(func)(s);
                ++count;
            }

            return count;
        }

    private:
        // types
        enum class BoundKind_
        {
            Lower,
            Upper
        };
        template<typename U, typename = void>
        struct HasValueMember_ : std::false_type {};
        template<typename U>
        struct HasValueMember_<U, std::void_t<decltype(std::declval<U>().value)>> : std::true_type {};
        template<typename U, typename = void>
        struct IsFormattable_ : std::false_type {};
        template<typename U>
        struct IsFormattable_<U, std::void_t<decltype(std::format("{}", std::declval<U>()))>> : std::true_type {};
        // functions
        static uint64_t TimestampOf_(const T& sample)
        {
            return sample.*TimestampMember;
        }
        // Shared binary search implementation for LowerBoundSerial / UpperBoundSerial.
        template<BoundKind_ Kind>
        size_t BoundSerial_(uint64_t timestamp) const
        {
            auto range = samples_.GetSerialRange();
            size_t first = range.first;
            size_t last = range.second; // one past end

            size_t lo = first;
            size_t hi = last;

            // Standard lower/upper bound style search over [first, last)
            while (lo < hi) {
                size_t mid = lo + (hi - lo) / 2;
                const T& s = At(mid);

                if constexpr (Kind == BoundKind_::Lower) {
                    // First with s.timestamp >= timestamp
                    if (TimestampOf_(s) < timestamp) {
                        lo = mid + 1;
                    }
                    else {
                        hi = mid;
                    }
                }
                else {
                    // First with s.timestamp > timestamp
                    if (TimestampOf_(s) <= timestamp) {
                        lo = mid + 1;
                    }
                    else {
                        hi = mid;
                    }
                }
            }

            return lo; // in [first, last]
        }
        // data
        ShmRing<T> samples_;
    };

    template<typename TValue>
    struct TelemetrySample
    {
        using value_type = TValue;
        TValue value;
        uint64_t timestamp;
    };
    // Alias for telemetry convenience; matches the prior HistoryRing behavior.
    template<typename TValue, size_t ReadBufferSize = 4>
    using SampleHistoryRing = HistoryRing<TelemetrySample<TValue>, &TelemetrySample<TValue>::timestamp, ReadBufferSize>;
}
