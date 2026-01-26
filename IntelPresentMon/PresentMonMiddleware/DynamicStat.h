#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::mid
{
    struct DynamicQueryWindow;

    template<typename T>
    class DynamicStat
    {
    public:
        virtual ~DynamicStat() = default;
        virtual bool NeedsUpdate() const = 0;
        virtual bool NeedsPointSample() const = 0;
        virtual bool NeedsSortedWindow() const = 0;
        virtual void GatherToBlob(uint8_t* pBlobBase) const = 0;
        virtual void AddSample(T) = 0;
        virtual uint64_t GetSamplePoint(const DynamicQueryWindow& win) const = 0;
        virtual void SetSampledValue(T) = 0;
        virtual void InputSortedSamples(std::span<const T>) = 0;
    protected:
        DynamicStat() = default;
    };

    template<typename T>
    std::unique_ptr<DynamicStat<T>> MakeDynamicStat(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t blobOffsetBytes);

    extern template std::unique_ptr<DynamicStat<double>> MakeDynamicStat<double>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    extern template std::unique_ptr<DynamicStat<int32_t>> MakeDynamicStat<int32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    extern template std::unique_ptr<DynamicStat<uint64_t>> MakeDynamicStat<uint64_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
    extern template std::unique_ptr<DynamicStat<bool>> MakeDynamicStat<bool>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes);
}
