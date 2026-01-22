#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include "DynamicQueryWindow.h"
#include "../CommonUtilities/Exception.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::mid
{
    class DynamicStatBase
    {
    public:
        virtual ~DynamicStatBase() = default;
        virtual bool NeedsUpdate() const = 0;
        virtual bool NeedsPointSample() const = 0;
        virtual bool NeedsSortedWindow() const = 0;
        virtual void AddSample(double)
        {
            ThrowMalformed_("DynamicStat::AddSample unsupported for this stat");
        }
        virtual void AddSample(int32_t)
        {
            ThrowMalformed_("DynamicStat::AddSample unsupported for this stat");
        }
        virtual void AddSample(bool)
        {
            ThrowMalformed_("DynamicStat::AddSample unsupported for this stat");
        }
        virtual uint64_t GetSamplePoint(const DynamicQueryWindow& win) const
        {
            ThrowMalformed_("DynamicStat::GetSamplePoint unsupported for this stat");
            return 0;
        }
        virtual void SetSampledValue(double)
        {
            ThrowMalformed_("DynamicStat::SetSampledValue unsupported for this stat");
        }
        virtual void SetSampledValue(int32_t)
        {
            ThrowMalformed_("DynamicStat::SetSampledValue unsupported for this stat");
        }
        virtual void SetSampledValue(bool)
        {
            ThrowMalformed_("DynamicStat::SetSampledValue unsupported for this stat");
        }
        virtual void InputSortedSamples(std::span<const double>)
        {
            ThrowMalformed_("DynamicStat::InputSortedSamples unsupported for this stat");
        }
        virtual void InputSortedSamples(std::span<const int32_t>)
        {
            ThrowMalformed_("DynamicStat::InputSortedSamples unsupported for this stat");
        }
        virtual void InputSortedSamples(std::span<const bool>)
        {
            ThrowMalformed_("DynamicStat::InputSortedSamples unsupported for this stat");
        }
        virtual void Populate(uint8_t* pBase) const
        {
            ThrowMalformed_("DynamicStat::Populate unsupported for this stat");
        }
    protected:
        DynamicStatBase(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
            : inType_{ inType },
            outType_{ outType },
            offsetBytes_{ offsetBytes }
        {
        }
        void ThrowMalformed_(const char* msg) const
        {
            throw pmon::util::Except<pmon::ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, msg);
        }
        PM_DATA_TYPE inType_ = PM_DATA_TYPE_DOUBLE;
        PM_DATA_TYPE outType_ = PM_DATA_TYPE_DOUBLE;
        size_t offsetBytes_ = 0;
    };

    template<typename T>
    class DynamicStat : public DynamicStatBase
    {
    public:
        void SetSampledValue(T val) override
        {
            SetSampledValue_(val);
        }
    protected:
        DynamicStat(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
            : DynamicStatBase(inType, outType, offsetBytes)
        {
        }
        virtual void SetSampledValue_(T)
        {
            ThrowMalformed_("DynamicStat::SetSampledValue unsupported for this stat");
        }
    };

    namespace detail
    {
        template<typename T>
        class DynamicStatAverage_ : public DynamicStat<T>
        {
        public:
            DynamicStatAverage_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, bool skipZero)
                : DynamicStat<T>{ inType, outType, offsetBytes },
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
            void Populate(uint8_t* pBase) const override
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
        class DynamicStatPercentile_ : public DynamicStat<T>
        {
        public:
            DynamicStatPercentile_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, double percentile)
                : DynamicStat<T>{ inType, outType, offsetBytes },
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
            void Populate(uint8_t* pBase) const override
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
        class DynamicStatMinMax_ : public DynamicStat<T>
        {
        public:
            DynamicStatMinMax_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, bool isMax)
                : DynamicStat<T>{ inType, outType, offsetBytes },
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
            void Populate(uint8_t* pBase) const override
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
        class DynamicStatPoint_ : public DynamicStat<T>
        {
        public:
            DynamicStatPoint_(PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, PM_STAT mode)
                : DynamicStat<T>{ inType, outType, offsetBytes },
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
            void Populate(uint8_t* pBase) const override
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
                default:
                    this->ThrowMalformed_("DynamicStat point expects double, int32, enum, or bool output value");
                }
            }
        private:
            void SetSampledValue_(T val) override
            {
                value_ = val;
                hasValue_ = true;
            }
            PM_STAT mode_ = PM_STAT_MID_POINT;
            bool hasValue_ = false;
            T value_ = (T)0;
        };
    }

    std::unique_ptr<DynamicStatBase> MakeDynamicStat(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
}
