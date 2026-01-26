#include "DynamicStat.h"
#include "DynamicQueryWindow.h"
#include "../CommonUtilities/Exception.h"
#include "../Interprocess/source/PmStatusError.h"
#include <algorithm>
#include <type_traits>

namespace ipc = pmon::ipc;
namespace util = pmon::util;

namespace pmon::mid
{
    namespace detail
    {
        template<typename T>
        class DynamicStatBase_ : public DynamicStat<T>
        {
        public:
            void AddSample(T) override
            {
                ThrowMalformed_("DynamicStat::AddSample unsupported for this stat");
            }
            uint64_t GetSamplePoint(const DynamicQueryWindow& win) const override
            {
                ThrowMalformed_("DynamicStat::GetSamplePoint unsupported for this stat");
                return 0;
            }
            void SetSampledValue(T) override
            {
                ThrowMalformed_("DynamicStat::SetSampledValue unsupported for this stat");
            }
            void InputSortedSamples(std::span<const T>) override
            {
                ThrowMalformed_("DynamicStat::InputSortedSamples unsupported for this stat");
            }
        protected:
            DynamicStatBase_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
                : inType_{ inType },
                outType_{ outType },
                offsetBytes_{ offsetBytes }
            {}
            void ThrowMalformed_(const char* msg) const
            {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, msg);
            }
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
                if (skipZero_ && val == (T)0) {
                    return;
                }
                sum_ += (double)val;
                ++count_;
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                if (this->outType_ != PM_DATA_TYPE_DOUBLE) {
                    this->ThrowMalformed_("DynamicStat average expects double output value");
                }
                double avg = 0.0;
                if (count_ > 0) {
                    avg = sum_ / (double)count_;
                }
                auto* pTarget = pBase + this->offsetBytes_;
                *reinterpret_cast<double*>(pTarget) = avg;
            }
        private:
            bool skipZero_ = false;
            double sum_ = 0.0;
            size_t count_ = 0;
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
                if (sortedSamples.empty()) {
                    value_ = 0.0;
                    hasValue_ = false;
                    return;
                }

                const double clamped = std::clamp(percentile_, 0.0, 1.0);
                const size_t last = sortedSamples.size() - 1;
                const double position = clamped * (double)last;
                const size_t index = (size_t)(position + 0.5);
                value_ = (double)sortedSamples[index];
                hasValue_ = true;
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                auto* pTarget = pBase + this->offsetBytes_;
                switch (this->outType_) {
                case PM_DATA_TYPE_DOUBLE:
                    *reinterpret_cast<double*>(pTarget) = hasValue_ ? value_ : 0.0;
                    break;
                case PM_DATA_TYPE_INT32:
                case PM_DATA_TYPE_ENUM:
                    *reinterpret_cast<int32_t*>(pTarget) = hasValue_ ? (int32_t)value_ : 0;
                    break;
                case PM_DATA_TYPE_BOOL:
                    *reinterpret_cast<bool*>(pTarget) = hasValue_ ? value_ != 0.0 : false;
                    break;
                case PM_DATA_TYPE_UINT64:
                    *reinterpret_cast<uint64_t*>(pTarget) = hasValue_ ? (uint64_t)value_ : 0;
                    break;
                default:
                    this->ThrowMalformed_("DynamicStat percentile expects double, int32, enum, or bool output value");
                }
            }
        private:
            double percentile_ = 0.0;
            double value_ = 0.0;
            bool hasValue_ = false;
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
                const double doubleVal = (double)val;
                if (!hasValue_) {
                    value_ = doubleVal;
                    hasValue_ = true;
                    return;
                }
                if (isMax_) {
                    if (doubleVal > value_) {
                        value_ = doubleVal;
                    }
                }
                else {
                    if (doubleVal < value_) {
                        value_ = doubleVal;
                    }
                }
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                auto* pTarget = pBase + this->offsetBytes_;
                switch (this->outType_) {
                case PM_DATA_TYPE_DOUBLE:
                    *reinterpret_cast<double*>(pTarget) = hasValue_ ? value_ : 0.0;
                    break;
                case PM_DATA_TYPE_INT32:
                case PM_DATA_TYPE_ENUM:
                    *reinterpret_cast<int32_t*>(pTarget) = hasValue_ ? (int32_t)value_ : 0;
                    break;
                case PM_DATA_TYPE_BOOL:
                    *reinterpret_cast<bool*>(pTarget) = hasValue_ ? value_ != 0.0 : false;
                    break;
                case PM_DATA_TYPE_UINT64:
                    *reinterpret_cast<uint64_t*>(pTarget) = hasValue_ ? (uint64_t)value_ : 0;
                    break;
                default:
                    this->ThrowMalformed_("DynamicStat min/max expects double, int32, enum, or bool output value");
                }
            }
        private:
            bool isMax_ = false;
            double value_ = 0.0;
            bool hasValue_ = false;
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
                    this->ThrowMalformed_("DynamicStat point expects point mode");
                    return win.newest;
                }
            }
            void GatherToBlob(uint8_t* pBase) const override
            {
                auto* pTarget = pBase + this->offsetBytes_;
                const double doubleVal = (double)value_;
                switch (this->outType_) {
                case PM_DATA_TYPE_DOUBLE:
                    *reinterpret_cast<double*>(pTarget) = hasValue_ ? doubleVal : 0.0;
                    break;
                case PM_DATA_TYPE_INT32:
                case PM_DATA_TYPE_ENUM:
                    *reinterpret_cast<int32_t*>(pTarget) = hasValue_ ? (int32_t)doubleVal : 0;
                    break;
                case PM_DATA_TYPE_BOOL:
                    *reinterpret_cast<bool*>(pTarget) = hasValue_ ? doubleVal != 0.0 : false;
                    break;
                case PM_DATA_TYPE_UINT64:
                    *reinterpret_cast<uint64_t*>(pTarget) = hasValue_ ? (uint64_t)value_ : 0;
                    break;
                default:
                    this->ThrowMalformed_("DynamicStat point expects double, int32, enum, or bool output value");
                }
            }
        private:
            void SetSampledValue(T val) override
            {
                value_ = val;
                hasValue_ = true;
            }
            PM_STAT mode_ = PM_STAT_MID_POINT;
            bool hasValue_ = false;
            T value_ = (T)0;
        };
    }

    namespace
    {
        template<typename T>
        void ValidateInputType_(PM_DATA_TYPE inType)
        {
            if constexpr (std::is_same_v<T, double>) {
                if (inType != PM_DATA_TYPE_DOUBLE) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Unsupported dynamic stat input data type");
                }
            }
            else if constexpr (std::is_same_v<T, int32_t>) {
                if (inType != PM_DATA_TYPE_INT32 && inType != PM_DATA_TYPE_ENUM) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Unsupported dynamic stat input data type");
                }
            }
            else if constexpr (std::is_same_v<T, uint64_t>) {
                if (inType != PM_DATA_TYPE_UINT64) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Unsupported dynamic stat input data type");
                }
            }
            else if constexpr (std::is_same_v<T, bool>) {
                if (inType != PM_DATA_TYPE_BOOL) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Unsupported dynamic stat input data type");
                }
            }
            else {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                    "Unsupported dynamic stat input data type");
            }
        }

        bool IsAvgStat_(PM_STAT stat)
        {
            return stat == PM_STAT_AVG || stat == PM_STAT_NON_ZERO_AVG;
        }

        bool IsSupportedOutputType_(PM_DATA_TYPE outType, bool allowBool, bool allowUint64)
        {
            switch (outType) {
            case PM_DATA_TYPE_DOUBLE:
            case PM_DATA_TYPE_INT32:
            case PM_DATA_TYPE_ENUM:
                return true;
            case PM_DATA_TYPE_BOOL:
                return allowBool;
            case PM_DATA_TYPE_UINT64:
                return allowUint64;
            default:
                return false;
            }
        }

        void ValidateOutputType_(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType)
        {
            if (IsAvgStat_(stat)) {
                if (outType != PM_DATA_TYPE_DOUBLE) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Dynamic stat average expects double output value");
                }
                return;
            }

            const bool allowBool = inType == PM_DATA_TYPE_BOOL;
            const bool allowUint64 = inType == PM_DATA_TYPE_UINT64;
            if (!IsSupportedOutputType_(outType, allowBool, allowUint64)) {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                    "Unsupported dynamic stat output data type");
            }
        }

        template<typename T>
        std::unique_ptr<DynamicStat<T>> MakeDynamicStatTyped_(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
        {
            ValidateOutputType_(stat, inType, outType);

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
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                    "Unsupported dynamic stat");
            }
        }
    }

    template<typename T>
    std::unique_ptr<DynamicStat<T>> MakeDynamicStat(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
    {
        ValidateInputType_<T>(inType);
        return MakeDynamicStatTyped_<T>(stat, inType, outType, offsetBytes);
    }

    template std::unique_ptr<DynamicStat<double>> MakeDynamicStat<double>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<int32_t>> MakeDynamicStat<int32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<uint64_t>> MakeDynamicStat<uint64_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    template std::unique_ptr<DynamicStat<bool>> MakeDynamicStat<bool>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
}
