#pragma once
#include "ShmRing.h"

namespace pmon::ipc
{
    // wrapper around ShmRing that adds the ability to search/address by timestamp
    // meant for use with telemetry data
    template<typename T, size_t ReadBufferSize = 4>
    class HistoryRing
    {
    public:
        // types
        struct Sample
        {
            T value;
            uint64_t timestamp;
        };
        // functions
        HistoryRing(size_t capacity, ShmVector<Sample>::allocator_type alloc)
            :
            samples_{ capacity, alloc }
        {
            if (capacity < ReadBufferSize * 2) {
                throw std::logic_error{ "The capacity of a ShmRing must be at least double its ReadBufferSize" };
            }
        }
        void Push(const T& value, uint64_t timestamp)
        {
            samples_.Push({ value, timestamp });
        }
        const Sample& Newest() const
        {
            auto&&[first, last] = samples_.GetSerialRange();
            return At(last - 1);
        }
        const Sample& At(size_t serial) const
        {
            return samples_.At(serial);
        }
        const Sample& Nearest(uint64_t timestamp) const
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
        // First serial with timestamp >= given timestamp.
        // If all samples have timestamp < given timestamp, returns last (one past end).
        size_t LowerBoundSerial(uint64_t timestamp) const
        {
            return BoundSerial_<BoundKind::Lower>(timestamp);
        }
        // First serial with timestamp > given timestamp.
        // If all samples have timestamp <= given timestamp, returns last (one past end).
        size_t UpperBoundSerial(uint64_t timestamp) const
        {
            return BoundSerial_<BoundKind::Upper>(timestamp);
        }
        // Find the serial whose timestamp is closest to the given timestamp.
        // If the timestamp is outside the stored range, clamps to first/last.
        size_t NearestSerial(uint64_t timestamp) const
        {
            const auto range = samples_.GetSerialRange();
            if (range.first == range.second) {
                return range.first;
            }

            // First serial with timestamp >= requested
            size_t serial = LowerBoundSerial(timestamp);

            if (serial >= range.second) {
                return range.second - 1;
            }

            // Check whether the previous sample is actually closer.
            // but only if there is a sample available before this one
            if (serial > range.first) {
                const auto nextTimestamp = At(serial).timestamp;
                const auto prevTimestamp = At(serial - 1).timestamp;
                const uint64_t dPrev = timestamp - prevTimestamp;
                const uint64_t dNext = nextTimestamp - timestamp;
                if (dPrev <= dNext) {
                    --serial;
                }
            }

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
                const Sample& s = At(serial);
                if (s.timestamp > end) {
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
        enum class BoundKind
        {
            Lower,
            Upper
        };
        // functions
        // Shared binary search implementation for LowerBoundSerial / UpperBoundSerial.
        template<BoundKind Kind>
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
                const Sample& s = At(mid);

                if constexpr (Kind == BoundKind::Lower) {
                    // First with s.timestamp >= timestamp
                    if (s.timestamp < timestamp) {
                        lo = mid + 1;
                    }
                    else {
                        hi = mid;
                    }
                }
                else {
                    // First with s.timestamp > timestamp
                    if (s.timestamp <= timestamp) {
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
        ShmRing<Sample> samples_;
    };

}
