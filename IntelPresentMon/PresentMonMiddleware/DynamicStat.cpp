#include "DynamicStat.h"
#include "../CommonUtilities/Exception.h"
#include "../Interprocess/source/PmStatusError.h"
#include <algorithm>

namespace ipc = pmon::ipc;
namespace util = pmon::util;

namespace pmon::mid
{
    namespace
    {
        // only supports double
        class DynamicStatAverage_ : public DynamicStat
        {
        public:
            DynamicStatAverage_(PM_DATA_TYPE dataType, size_t offsetBytes, bool skipZero)
                : DynamicStat{ dataType, offsetBytes },
                skipZero_{ skipZero }
            {
            }
            bool NeedsUpdate() const override { return true; }
            bool NeedsPointSample() const override { return false; }
            bool NeedsSortedWindow() const override { return false; }
            void AddSample(double val) override
            {
                if (skipZero_ && val == 0.0) {
                    return;
                }
                sum_ += val;
                ++count_;
            }
            void Populate(uint8_t* pBase) const override
            {
                double avg = 0.0;
                if (count_ > 0) {
                    avg = sum_ / (double)count_;
                }
                auto* pTarget = pBase + offsetBytes_;
                *reinterpret_cast<double*>(pTarget) = avg;
            }
        private:
            bool skipZero_ = false;
            double sum_ = 0.0;
            size_t count_ = 0;
        };

        // only supports double
        class DynamicStatPercentile_ : public DynamicStat
        {
        public:
            DynamicStatPercentile_(PM_DATA_TYPE dataType, size_t offsetBytes, double percentile)
                : DynamicStat{ dataType, offsetBytes },
                percentile_{ percentile }
            {
            }
            bool NeedsUpdate() const override { return true; }
            bool NeedsPointSample() const override { return false; }
            bool NeedsSortedWindow() const override { return true; }
            void InputSortedSamples(std::span<const double> sortedSamples) override
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
                value_ = sortedSamples[index];
                hasValue_ = true;
            }
            void Populate(uint8_t* pBase) const override
            {
                auto* pTarget = pBase + offsetBytes_;
                *reinterpret_cast<double*>(pTarget) = hasValue_ ? value_ : 0.0;
            }
        private:
            double percentile_ = 0.0;
            double value_ = 0.0;
            bool hasValue_ = false;
        };

        // only supports double
        class DynamicStatMinMax_ : public DynamicStat
        {
        public:
            DynamicStatMinMax_(PM_DATA_TYPE dataType, size_t offsetBytes, bool isMax)
                : DynamicStat{ dataType, offsetBytes },
                isMax_{ isMax }
            {
            }
            bool NeedsUpdate() const override { return true; }
            bool NeedsPointSample() const override { return false; }
            bool NeedsSortedWindow() const override { return false; }
            void AddSample(double val) override
            {
                if (!hasValue_) {
                    value_ = val;
                    hasValue_ = true;
                    return;
                }
                if (isMax_) {
                    if (val > value_) {
                        value_ = val;
                    }
                }
                else {
                    if (val < value_) {
                        value_ = val;
                    }
                }
            }
            void Populate(uint8_t* pBase) const override
            {
                auto* pTarget = pBase + offsetBytes_;
                *reinterpret_cast<double*>(pTarget) = hasValue_ ? value_ : 0.0;
            }
        private:
            bool isMax_ = false;
            double value_ = 0.0;
            bool hasValue_ = false;
        };

        // supports double AND int32/enum
        class DynamicStatPoint_ : public DynamicStat
        {
        public:
            DynamicStatPoint_(PM_DATA_TYPE dataType, size_t offsetBytes, PM_STAT mode)
                : DynamicStat{ dataType, offsetBytes },
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
                }
                return win.newest;
            }
            void SetSampledValueDouble_(double val) override
            {
                if (dataType_ != PM_DATA_TYPE_DOUBLE) {
                    ThrowMalformed_("DynamicStat point expects int32 or enum value");
                }
                valueDouble_ = val;
                hasValue_ = true;
            }
            void SetSampledValueInt32_(int32_t val) override
            {
                if (dataType_ != PM_DATA_TYPE_INT32 && dataType_ != PM_DATA_TYPE_ENUM) {
                    ThrowMalformed_("DynamicStat point expects double value");
                }
                valueInt32_ = val;
                hasValue_ = true;
            }
            void Populate(uint8_t* pBase) const override
            {
                if (dataType_ == PM_DATA_TYPE_DOUBLE) {
                    auto* pTarget = pBase + offsetBytes_;
                    *reinterpret_cast<double*>(pTarget) = hasValue_ ? valueDouble_ : 0.0;
                }
                else {
                    auto* pTarget = pBase + offsetBytes_;
                    *reinterpret_cast<int32_t*>(pTarget) = hasValue_ ? valueInt32_ : 0;
                }
            }
        private:
            PM_STAT mode_ = PM_STAT_NEWEST_POINT;
            bool hasValue_ = false;
            double valueDouble_ = 0.0;
            int32_t valueInt32_ = 0;
        };
    }

    DynamicStat::DynamicStat(PM_DATA_TYPE dataType, size_t offsetBytes)
        : dataType_{ dataType },
        offsetBytes_{ offsetBytes }
    {
    }

    DynamicStat::~DynamicStat() = default;

    void DynamicStat::ThrowMalformed_(const char* msg) const
    {
        throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, msg);
    }

    void DynamicStat::AddSample(double)
    {
        ThrowMalformed_("DynamicStat::AddSample unsupported for this stat");
    }

    uint64_t DynamicStat::GetSamplePoint(const DynamicQueryWindow&) const
    {
        ThrowMalformed_("DynamicStat::GetSamplePoint unsupported for this stat");
        return 0;
    }

    void DynamicStat::SetSampledValue(double val)
    {
        SetSampledValueDouble_(val);
    }

    void DynamicStat::SetSampledValue(int32_t val)
    {
        SetSampledValueInt32_(val);
    }

    void DynamicStat::SetSampledValueDouble_(double)
    {
        ThrowMalformed_("DynamicStat::SetSampledValueDouble unsupported for this stat");
    }

    void DynamicStat::SetSampledValueInt32_(int32_t)
    {
        ThrowMalformed_("DynamicStat::SetSampledValueInt32 unsupported for this stat");
    }

    void DynamicStat::InputSortedSamples(std::span<const double>)
    {
        ThrowMalformed_("DynamicStat::InputSortedSamples unsupported for this stat");
    }

    void DynamicStat::Populate(uint8_t*) const
    {
        ThrowMalformed_("DynamicStat::Populate unsupported for this stat");
    }

    std::unique_ptr<DynamicStat> CreateDynamicStat(PM_STAT stat, PM_DATA_TYPE dataType, size_t offsetBytes)
    {
        const bool isPointStat = stat == PM_STAT_MID_POINT
            || stat == PM_STAT_NEWEST_POINT
            || stat == PM_STAT_OLDEST_POINT;
        if (isPointStat) {
            switch (dataType) {
            case PM_DATA_TYPE_DOUBLE:
            case PM_DATA_TYPE_INT32:
            case PM_DATA_TYPE_ENUM:
                break;
            default:
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Unsupported dynamic stat data type");
            }
        }
        else {
            if (dataType != PM_DATA_TYPE_DOUBLE) {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Unsupported dynamic stat data type");
            }
        }

        switch (stat) {
        case PM_STAT_AVG:
            return std::make_unique<DynamicStatAverage_>(dataType, offsetBytes, false);
        case PM_STAT_NON_ZERO_AVG:
            return std::make_unique<DynamicStatAverage_>(dataType, offsetBytes, true);
        case PM_STAT_PERCENTILE_99:
            return std::make_unique<DynamicStatPercentile_>(dataType, offsetBytes, 0.99);
        case PM_STAT_PERCENTILE_95:
            return std::make_unique<DynamicStatPercentile_>(dataType, offsetBytes, 0.95);
        case PM_STAT_PERCENTILE_90:
            return std::make_unique<DynamicStatPercentile_>(dataType, offsetBytes, 0.90);
        case PM_STAT_PERCENTILE_01:
            return std::make_unique<DynamicStatPercentile_>(dataType, offsetBytes, 0.01);
        case PM_STAT_PERCENTILE_05:
            return std::make_unique<DynamicStatPercentile_>(dataType, offsetBytes, 0.05);
        case PM_STAT_PERCENTILE_10:
            return std::make_unique<DynamicStatPercentile_>(dataType, offsetBytes, 0.10);
        case PM_STAT_MAX:
            return std::make_unique<DynamicStatMinMax_>(dataType, offsetBytes, true);
        case PM_STAT_MIN:
            return std::make_unique<DynamicStatMinMax_>(dataType, offsetBytes, false);
        case PM_STAT_MID_POINT:
            return std::make_unique<DynamicStatPoint_>(dataType, offsetBytes, PM_STAT_MID_POINT);
        case PM_STAT_NEWEST_POINT:
            return std::make_unique<DynamicStatPoint_>(dataType, offsetBytes, PM_STAT_NEWEST_POINT);
        case PM_STAT_OLDEST_POINT:
            return std::make_unique<DynamicStatPoint_>(dataType, offsetBytes, PM_STAT_OLDEST_POINT);
        case PM_STAT_NONE:
        case PM_STAT_MID_LERP:
        case PM_STAT_COUNT:
        default:
            throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Unsupported dynamic stat");
        }
    }
}
