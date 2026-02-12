#include "DynamicStat.h"
#include "DynamicQueryWindow.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/log/Log.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../../PresentData/PresentEventEnums.hpp"
#include <algorithm>
#include <cassert>
#include <optional>

namespace ipc = pmon::ipc;
namespace util = pmon::util;

namespace pmon::mid
{
    namespace detail
    {
        // TODO: consider ways of obviating this adapter construct
        template<typename T>
        struct SampleAdapter_
        {
            static bool HasValue(const T&)
            {
                return true;
            }
            static bool IsZero(const T& val)
            {
                return val == (T)0;
            }
            static double ToDouble(const T& val)
            {
                return (double)val;
            }
            static uint64_t ToUint64(const T& val)
            {
                return (uint64_t)val;
            }
        };

        template<typename U>
        struct SampleAdapter_<std::optional<U>>
        {
            static bool HasValue(const std::optional<U>& val)
            {
                return val.has_value();
            }
            static bool IsZero(const std::optional<U>& val)
            {
                return !val.has_value() || *val == (U)0;
            }
            static double ToDouble(const std::optional<U>& val)
            {
                return val.has_value() ? (double)*val : 0.0;
            }
            static uint64_t ToUint64(const std::optional<U>& val)
            {
                return val.has_value() ? (uint64_t)*val : 0;
            }
        };

        template<typename T>
        void WriteOptionalValueToBlob_(uint8_t* pBase, size_t offsetBytes, PM_DATA_TYPE outType, const std::optional<T>& value)
        {
            auto* pTarget = pBase + offsetBytes;
            const double doubleVal = value ? SampleAdapter_<T>::ToDouble(*value) : 0.0;
            const uint64_t uint64Val = value ? SampleAdapter_<T>::ToUint64(*value) : 0;
            switch (outType) {
            case PM_DATA_TYPE_DOUBLE:
                *reinterpret_cast<double*>(pTarget) = doubleVal;
                break;
            case PM_DATA_TYPE_INT32:
            case PM_DATA_TYPE_ENUM:
                *reinterpret_cast<int32_t*>(pTarget) = value ? (int32_t)doubleVal : 0;
                break;
            case PM_DATA_TYPE_UINT32:
                *reinterpret_cast<uint32_t*>(pTarget) = value ? (uint32_t)uint64Val : 0;
                break;
            case PM_DATA_TYPE_BOOL:
                *reinterpret_cast<bool*>(pTarget) = value ? doubleVal != 0.0 : false;
                break;
            case PM_DATA_TYPE_UINT64:
                *reinterpret_cast<uint64_t*>(pTarget) = value ? uint64Val : 0;
                break;
            default:
                pmlog_error("Unhandled data type case").pmwatch((int)outType);
                assert(false);
            }
        }

        template<typename T>
        class DynamicStatBase_ : public DynamicStat<T>
        {
        public:
            void AddSample(T) override
            {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "DynamicStat::AddSample unsupported for this stat");
            }
            uint64_t GetSamplePoint(const DynamicQueryWindow& win) const override
            {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "DynamicStat::GetSamplePoint unsupported for this stat");
            }
            void SetSampledValue(T) override
            {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "DynamicStat::SetSampledValue unsupported for this stat");
            }
            void InputSortedSamples(std::span<const T>) override
            {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "DynamicStat::InputSortedSamples unsupported for this stat");
            }
        protected:
            // functions
            DynamicStatBase_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes,
                std::optional<double> reciprocationFactor)
                : inType_{ inType },
                outType_{ outType },
                offsetBytes_{ offsetBytes },
                reciprocationFactor_{ reciprocationFactor }
            {}
            template<typename V>
            void WriteValue_(uint8_t* pBase, const std::optional<V>& value) const
            {
                // if not recip we can forward to Write function (it handles empty opt etc.)
                if (!reciprocationFactor_) {
                    WriteOptionalValueToBlob_(pBase, offsetBytes_, outType_, value);
                    return;
                }
                // if recip but no value, write nullopt (cannot reciprocate a nullopt)
                if (!value) {
                    WriteOptionalValueToBlob_(pBase, offsetBytes_, outType_, std::optional<double>{});
                    return;
                }
                const double rawValue = SampleAdapter_<V>::ToDouble(*value);
                // if value is present but zero, cannot recip zero so write nullopt
                if (rawValue == 0.0) {
                    WriteOptionalValueToBlob_(pBase, offsetBytes_, outType_, std::optional<double>{});
                    return;
                }
                // otherwise, perform reciprocation and then write
                const std::optional<double> adjusted = *reciprocationFactor_ / rawValue;
                WriteOptionalValueToBlob_(pBase, offsetBytes_, outType_, adjusted);
            }
            // data
            PM_DATA_TYPE inType_ = PM_DATA_TYPE_DOUBLE;
            PM_DATA_TYPE outType_ = PM_DATA_TYPE_DOUBLE;
            size_t offsetBytes_ = 0;
            std::optional<double> reciprocationFactor_;
        };

        template<typename T>
        class DynamicStatAverage_ : public DynamicStatBase_<T>
        {
        public:
            DynamicStatAverage_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes,
                std::optional<double> reciprocationFactor, bool skipZero)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes, reciprocationFactor },
                skipZero_{ skipZero }
            {
            }
            bool NeedsUpdate() const override { return true; }
            bool NeedsPointSample() const override { return false; }
            bool NeedsSortedWindow() const override { return false; }
            void AddSample(T val) override
            {
                if (!SampleAdapter_<T>::HasValue(val)) {
                    return;
                }
                if (skipZero_ && SampleAdapter_<T>::IsZero(val)) {
                    return;
                }
                sum_ += SampleAdapter_<T>::ToDouble(val);
                ++count_;
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                std::optional<double> avg;
                if (count_ > 0) {
                    avg = sum_ / (double)count_;
                }
                this->WriteValue_(pBase, avg);
                // reset for the next poll
                sum_ = 0;
                count_ = 0;
            }
        private:
            bool skipZero_ = false;
            mutable double sum_ = 0.0;
            mutable size_t count_ = 0;
        };

        template<typename T>
        class DynamicStatPercentile_ : public DynamicStatBase_<T>
        {
        public:
            DynamicStatPercentile_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes,
                std::optional<double> reciprocationFactor, double percentile)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes, reciprocationFactor },
                percentile_{ percentile }
            {
            }
            bool NeedsUpdate() const override { return true; }
            bool NeedsPointSample() const override { return false; }
            bool NeedsSortedWindow() const override { return true; }
            void InputSortedSamples(std::span<const T> sortedSamples) override
            {
                // Methodology / steps:
                //
                //  0) Find the first sample that "has value" (for std::optional and similar),
                //     assuming empties/invalids sort before valids in the already-sorted buffer.
                //
                //  1) Map p to a fractional index h in [0, N-1] using:
                //        h = p * (N - 1)
                //     This is the "linear interpolation of order statistics" mapping that
                //     is most intuitive for continuous metrics:
                //       - p = 0   => h = 0     => returns x[0]   (min)
                //       - p = 1   => h = N-1   => returns x[N-1] (max)
                //       - otherwise interpolates smoothly between neighbors
                //
                //  2) Split h into:
                //        i = floor(h)         (base index)
                //        g = h - i            (fraction in [0,1))
                //
                //  3) Retrieve neighbours: i and i+1 (or just i 2x if at end)
                // 
                //  4) Perform lerp:
                //        q = x[i] + g * (x[i+1] - x[i])
                //     (note that for p=1, g becomes 0.)

                // Step 0: locate the first valid value (ignore empties/invalids at the front).
                size_t firstValid = 0;
                while (firstValid < sortedSamples.size() &&
                    !SampleAdapter_<T>::HasValue(sortedSamples[firstValid])) {
                    ++firstValid;
                }
                const size_t validCount = sortedSamples.size() - firstValid;
                if (validCount == 0) {
                    return; // no valid samples => leave value_ unchanged (or set to NaN if desired)
                }

                // Step 1: p-to-index mapping (fractional index over [0, N-1]).
                const double h = percentile_ * double(validCount - 1);

                // Step 2: split into integer index + fractional part.
                // Since h is in [0, N-1] and non-negative, truncation is equivalent to floor.
                const size_t i = size_t(h);
                const double g = h - double(i);

                // Step 3: fetch neighbors
                // i is nearest index position less than or equal to target position
                // so interpolation always requires 2nd index i1 to be after i
                // (but if at the end of container, use i for both sides of interpolation)
                const size_t i1 = (i + 1 < validCount) ? (i + 1) : i;
                // retrieve both samples
                const double x0 = SampleAdapter_<T>::ToDouble(sortedSamples[firstValid + i]);
                const double x1 = SampleAdapter_<T>::ToDouble(sortedSamples[firstValid + i1]);
               
                // Step 4: perform linear interpolation
                value_ = x0 + g * (x1 - x0);
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                this->WriteValue_(pBase, value_);
                // reset for the next poll
                value_.reset();
            }
        private:
            double percentile_ = 0.0;
            mutable std::optional<double> value_;
        };

        template<typename T>
        class DynamicStatMinMax_ : public DynamicStatBase_<T>
        {
        public:
            DynamicStatMinMax_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes,
                std::optional<double> reciprocationFactor, bool isMax)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes, reciprocationFactor },
                isMax_{ isMax }
            {
            }
            bool NeedsUpdate() const override { return true; }
            bool NeedsPointSample() const override { return false; }
            bool NeedsSortedWindow() const override { return false; }
            void AddSample(T val) override
            {
                if (!SampleAdapter_<T>::HasValue(val)) {
                    return;
                }
                const double doubleVal = SampleAdapter_<T>::ToDouble(val);
                if (!value_) {
                    value_ = doubleVal;
                    return;
                }
                if (isMax_) {
                    if (doubleVal > *value_) {
                        value_ = doubleVal;
                    }
                }
                else {
                    if (doubleVal < *value_) {
                        value_ = doubleVal;
                    }
                }
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                this->WriteValue_(pBase, value_);
                // reset min/max
                value_.reset();
            }
        private:
            bool isMax_ = false;
            mutable std::optional<double> value_;
        };

        template<typename T>
        class DynamicStatPoint_ : public DynamicStatBase_<T>
        {
        public:
            DynamicStatPoint_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes,
                std::optional<double> reciprocationFactor, PM_STAT mode)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes, reciprocationFactor },
                mode_{ mode }
            {
            }
            bool NeedsUpdate() const override { return false; }
            bool NeedsPointSample() const override { return true; }
            bool NeedsSortedWindow() const override { return false; }
            uint64_t GetSamplePoint(const DynamicQueryWindow& win) const override
            {
                switch (mode_) {
                case PM_STAT_OLDEST_POINT:
                    return win.oldest;
                case PM_STAT_NEWEST_POINT:
                    return win.newest;
                case PM_STAT_MID_POINT:
                    return win.oldest + (win.newest - win.oldest) / 2;
                default:

                    pmlog_error("Unhandled point stat case").pmwatch((int)mode_);
                    assert(false);
                    return win.newest;
                }
            }
            void SetSampledValue(T val) override
            {
                value_ = val;
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                this->WriteValue_(pBase, value_);
                // reset for the next poll
                value_.reset();
            }
        private:
            PM_STAT mode_ = PM_STAT_MID_POINT;
            mutable std::optional<T> value_;
        };
    }

    namespace
    {
        template<typename T>
        std::unique_ptr<DynamicStat<T>> MakeDynamicStatTyped_(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType,
            size_t offsetBytes, std::optional<double> reciprocationFactor)
        {
            switch (stat) {
            case PM_STAT_AVG:
                return std::make_unique<detail::DynamicStatAverage_<T>>(inType, outType, offsetBytes, reciprocationFactor, false);
            case PM_STAT_NON_ZERO_AVG:
                return std::make_unique<detail::DynamicStatAverage_<T>>(inType, outType, offsetBytes, reciprocationFactor, true);
            case PM_STAT_PERCENTILE_99:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, reciprocationFactor, 0.99);
            case PM_STAT_PERCENTILE_95:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, reciprocationFactor, 0.95);
            case PM_STAT_PERCENTILE_90:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, reciprocationFactor, 0.90);
            case PM_STAT_PERCENTILE_01:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, reciprocationFactor, 0.01);
            case PM_STAT_PERCENTILE_05:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, reciprocationFactor, 0.05);
            case PM_STAT_PERCENTILE_10:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, reciprocationFactor, 0.10);
            case PM_STAT_MAX:
                return std::make_unique<detail::DynamicStatMinMax_<T>>(inType, outType, offsetBytes, reciprocationFactor, true);
            case PM_STAT_MIN:
                return std::make_unique<detail::DynamicStatMinMax_<T>>(inType, outType, offsetBytes, reciprocationFactor, false);
            case PM_STAT_MID_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, reciprocationFactor, PM_STAT_MID_POINT);
            case PM_STAT_NEWEST_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, reciprocationFactor, PM_STAT_NEWEST_POINT);
            case PM_STAT_OLDEST_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, reciprocationFactor, PM_STAT_OLDEST_POINT);
            case PM_STAT_NONE:
            case PM_STAT_MID_LERP:
            case PM_STAT_COUNT:
            default:
                pmlog_error("Unhandled stat case").pmwatch((int)stat);
                assert(false);
                return {};
            }
        }
    }

    template<typename T>
    std::unique_ptr<DynamicStat<T>> MakeDynamicStat(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes,
        std::optional<double> reciprocationFactor)
    {
        return MakeDynamicStatTyped_<T>(stat, inType, outType, offsetBytes, reciprocationFactor);
    }

    template std::unique_ptr<DynamicStat<double>> MakeDynamicStat<double>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<int32_t>> MakeDynamicStat<int32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<uint32_t>> MakeDynamicStat<uint32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<uint64_t>> MakeDynamicStat<uint64_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<bool>> MakeDynamicStat<bool>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<std::optional<double>>> MakeDynamicStat<std::optional<double>>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<::PresentMode>> MakeDynamicStat<::PresentMode>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<::Runtime>> MakeDynamicStat<::Runtime>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    template std::unique_ptr<DynamicStat<::FrameType>> MakeDynamicStat<::FrameType>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
}
