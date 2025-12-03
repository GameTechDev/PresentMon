#include "MetricCapabilitiesShim.h"
#include "IntrospectionCapsLookup.h"
#include "../../CommonUtilities/third/reflect.hpp"

#include <type_traits>

namespace pmon::ipc::intro
{
    namespace detail
    {
        using MetricEnum = PM_METRIC;
        using MetricUnderlying = std::underlying_type_t<MetricEnum>;

        // Probe underlying values in [0, MaxMetricUnderlying)
        constexpr MetricUnderlying MaxMetricUnderlying = 256;

        // Is this underlying value actually one of the enum's declared enumerators?
        template<MetricUnderlying Value>
        consteval bool IsValidMetricEnum()
        {
            constexpr auto e = static_cast<MetricEnum>(Value);
            constexpr auto name = reflect::enum_name(e);
            return !name.empty();
        }

        // xxxCapBits is std::bitset<N>
        template<typename BitsType, typename Index>
        bool HasCap(const BitsType& bits, Index index)
        {
            return bits.test(static_cast<std::size_t>(index));
        }

        // GPU per-metric accumulation (only instantiated for valid enum values)
        template<MetricUnderlying Value>
        void AccumulateGpuCapability(MetricCapabilities& caps,
            const GpuTelemetryBitset& bits)
        {
            constexpr auto metricEnum = static_cast<MetricEnum>(Value);
            using Lookup = IntrospectionCapsLookup<metricEnum>;

            // Single GPU capability bit -> metric present if bit set
            if constexpr (IsGpuDeviceMetric<Lookup>) {
                if (HasCap(bits, Lookup::gpuCapBit)) {
                    caps.Set(metricEnum, 1);
                }
            }

            // Array GPU capability bits (fan speeds, etc.)
            if constexpr (IsGpuDeviceMetricArray<Lookup>) {
                std::size_t count = 0;
                for (auto flag : Lookup::gpuCapBitArray) {
                    if (HasCap(bits, flag)) {
                        ++count;
                    }
                }
                if (count > 0) {
                    caps.Set(metricEnum, count);
                }
            }

            // Static GPU metrics: name/vendor/etc.
            if constexpr (IsGpuDeviceStaticMetric<Lookup>) {
                caps.Set(metricEnum, 1);
            }
        }

        // CPU per-metric accumulation (only instantiated for valid enum values)
        template<MetricUnderlying Value>
        void AccumulateCpuCapability(MetricCapabilities& caps,
            const CpuTelemetryBitset& bits)
        {
            constexpr auto metricEnum = static_cast<MetricEnum>(Value);
            using Lookup = IntrospectionCapsLookup<metricEnum>;

            // CPU metrics gated by a capability bit
            if constexpr (IsCpuMetric<Lookup>) {
                if (HasCap(bits, Lookup::cpuCapBit)) {
                    caps.Set(metricEnum, 1);
                }
            }

            // Metrics that exist but are intended for manual disable by default
            if constexpr (IsManualDisableMetric<Lookup>) {
                caps.Set(metricEnum, 1);
            }
        }

        // Compile-time recursion over underlying values [0, MaxMetricUnderlying)
        // GPU: only call AccumulateGpuCapability when Value is a real enumerator
        template<MetricUnderlying Value>
        void ConvertGpuBitsRecursive(MetricCapabilities& caps,
            const GpuTelemetryBitset& bits)
        {
            if constexpr (Value < MaxMetricUnderlying) {
                if constexpr (IsValidMetricEnum<Value>()) {
                    AccumulateGpuCapability<Value>(caps, bits);
                }
                ConvertGpuBitsRecursive<Value + 1>(caps, bits);
            }
        }

        // CPU: same pattern
        template<MetricUnderlying Value>
        void ConvertCpuBitsRecursive(MetricCapabilities& caps,
            const CpuTelemetryBitset& bits)
        {
            if constexpr (Value < MaxMetricUnderlying) {
                if constexpr (IsValidMetricEnum<Value>()) {
                    AccumulateCpuCapability<Value>(caps, bits);
                }
                ConvertCpuBitsRecursive<Value + 1>(caps, bits);
            }
        }
    } // namespace detail

    MetricCapabilities ConvertBitset(const GpuTelemetryBitset& bits)
    {
        MetricCapabilities caps;
        detail::ConvertGpuBitsRecursive<0>(caps, bits);
        return caps;
    }

    MetricCapabilities ConvertBitset(const CpuTelemetryBitset& bits)
    {
        MetricCapabilities caps;
        detail::ConvertCpuBitsRecursive<0>(caps, bits);
        return caps;
    }
}
