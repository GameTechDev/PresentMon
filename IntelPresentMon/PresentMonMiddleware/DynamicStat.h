#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include "DynamicQueryWindow.h"
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::mid
{
    class DynamicStat
    {
    public:
        virtual ~DynamicStat();
        virtual bool NeedsUpdate() const = 0;
        virtual bool NeedsPointSample() const = 0;
        virtual bool NeedsSortedWindow() const = 0;
        virtual void AddSample(double val);
        virtual uint64_t GetSamplePoint(const DynamicQueryWindow& win) const;
        void SetSampledValue(double val);
        void SetSampledValue(int32_t val);
        virtual void InputSortedSamples(std::span<const double> sortedSamples);
        virtual void Populate(uint8_t* pBase) const;
    protected:
        DynamicStat(PM_DATA_TYPE dataType, size_t offsetBytes);
        void ThrowMalformed_(const char* msg) const;
        virtual void SetSampledValueDouble_(double val);
        virtual void SetSampledValueInt32_(int32_t val);
        PM_DATA_TYPE dataType_ = PM_DATA_TYPE_DOUBLE;
        size_t offsetBytes_ = 0;
    };

    std::unique_ptr<DynamicStat> CreateDynamicStat(PM_STAT stat, PM_DATA_TYPE dataType, size_t offsetBytes);
}
