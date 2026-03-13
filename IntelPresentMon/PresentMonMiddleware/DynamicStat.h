#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::mid
{
    struct DynamicQueryWindow;

    namespace detail
    {
        template<typename T>
        struct DynamicStatSampleAdapter
        {
            static bool HasValue(const T& val)
            {
                if constexpr (std::is_floating_point_v<T>) {
                    return !std::isnan(val);
                }
                else {
                    return true;
                }
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

    }

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
    std::unique_ptr<DynamicStat<T>> MakeDynamicStat(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType,
        size_t blobOffsetBytes, std::optional<double> reciprocationFactor);

    extern template std::unique_ptr<DynamicStat<double>> MakeDynamicStat<double>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    extern template std::unique_ptr<DynamicStat<int32_t>> MakeDynamicStat<int32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    extern template std::unique_ptr<DynamicStat<uint32_t>> MakeDynamicStat<uint32_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    extern template std::unique_ptr<DynamicStat<uint64_t>> MakeDynamicStat<uint64_t>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
    extern template std::unique_ptr<DynamicStat<bool>> MakeDynamicStat<bool>(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes, std::optional<double> reciprocationFactor);
}
