#include "DynamicStat.h"
#include "DynamicQueryWindow.h"
#include "../CommonUtilities/Exception.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
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
            DynamicStatBase_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
                : inType_{ inType },
                outType_{ outType },
                offsetBytes_{ offsetBytes }
            {}
            PM_DATA_TYPE inType_ = PM_DATA_TYPE_DOUBLE;
            PM_DATA_TYPE outType_ = PM_DATA_TYPE_DOUBLE;
            size_t offsetBytes_ = 0;
        };

        template<typename T>
        class DynamicStatAverage_ : public DynamicStatBase_<T>
        {
        public:
            DynamicStatAverage_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, bool skipZero)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes },
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
                WriteOptionalValueToBlob_(pBase, this->offsetBytes_, this->outType_, avg);
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
            DynamicStatPercentile_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, double percentile)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes },
                percentile_{ percentile }
            {
            }
            bool NeedsUpdate() const override { return true; }
            bool NeedsPointSample() const override { return false; }
            bool NeedsSortedWindow() const override { return true; }
            void InputSortedSamples(std::span<const T> sortedSamples) override
            {
                // TODO: review interaction of optional values with percentile sorted buffer
                size_t firstValid = 0;
                while (firstValid < sortedSamples.size() && !SampleAdapter_<T>::HasValue(sortedSamples[firstValid])) {
                    ++firstValid;
                }
                const size_t validCount = sortedSamples.size() - firstValid;
                if (validCount == 0) {
                    return;
                }

                const size_t last = validCount - 1;
                const double position = percentile_ * (double)last;
                const size_t index = (size_t)(position + 0.5);
                value_ = SampleAdapter_<T>::ToDouble(sortedSamples[firstValid + index]);
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                WriteOptionalValueToBlob_(pBase, this->offsetBytes_, this->outType_, value_);
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
            DynamicStatMinMax_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, bool isMax)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes },
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
                WriteOptionalValueToBlob_(pBase, this->offsetBytes_, this->outType_, value_);
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
            DynamicStatPoint_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, PM_STAT mode)
                : DynamicStatBase_<T>{ inType, outType, offsetBytes },
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
                WriteOptionalValueToBlob_(pBase, this->offsetBytes_, this->outType_, value_);
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
        std::unique_ptr<DynamicStat<T>> MakeDynamicStatTyped_(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
        {
            switch (stat) {
            case PM_STAT_AVG:
                return std::make_unique<detail::DynamicStatAverage_<T>>(inType, outType, offsetBytes, false);
            case PM_STAT_NON_ZERO_AVG:
                return std::make_unique<detail::DynamicStatAverage_<T>>(inType, outType, offsetBytes, true);
            case PM_STAT_PERCENTILE_99:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.99);
            case PM_STAT_PERCENTILE_95:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.95);
            case PM_STAT_PERCENTILE_90:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.90);
            case PM_STAT_PERCENTILE_01:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.01);
            case PM_STAT_PERCENTILE_05:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.05);
            case PM_STAT_PERCENTILE_10:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.10);
            case PM_STAT_MAX:
                return std::make_unique<detail::DynamicStatMinMax_<T>>(inType, outType, offsetBytes, true);
            case PM_STAT_MIN:
                return std::make_unique<detail::DynamicStatMinMax_<T>>(inType, outType, offsetBytes, false);
            case PM_STAT_MID_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, PM_STAT_MID_POINT);
            case PM_STAT_NEWEST_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, PM_STAT_NEWEST_POINT);
            case PM_STAT_OLDEST_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, PM_STAT_OLDEST_POINT);
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
    std::unique_ptr<DynamicStat<T>> MakeDynamicStat(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
    {
        return MakeDynamicStatTyped_<T>(stat, inType, outType, offsetBytes);
    }

    template std::unique_ptr<DynamicStat<double>> MakeDynamicStat<double>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<int32_t>> MakeDynamicStat<int32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<uint32_t>> MakeDynamicStat<uint32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<uint64_t>> MakeDynamicStat<uint64_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<bool>> MakeDynamicStat<bool>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<std::optional<double>>> MakeDynamicStat<std::optional<double>>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<::PresentMode>> MakeDynamicStat<::PresentMode>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<::Runtime>> MakeDynamicStat<::Runtime>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<::FrameType>> MakeDynamicStat<::FrameType>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
}
