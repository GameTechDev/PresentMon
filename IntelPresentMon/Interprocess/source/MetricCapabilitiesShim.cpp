#include "MetricCapabilitiesShim.h"
#include "IntrospectionCapsLookup.h"
#include "../../CommonUtilities/Meta.h"

namespace pmon::ipc::intro
{
    namespace detail
    {
        using MetricEnum = PM_METRIC;
        using MetricUnderlying = std::underlying_type_t<MetricEnum>;

        // Probe underlying values in [0, MaxMetricUnderlying)
        constexpr MetricUnderlying MaxMetricUnderlying = 256;

        // xxxCapBits is std::bitset<N>
        template<typename BitsType, typename Index>
        bool HasCap(const BitsType& bits, Index index)
        {
            return bits.test(static_cast<std::size_t>(index));
        }

        // GPU per-metric accumulation (only instantiated for valid enum values)
        template<MetricEnum Metric>
        void AccumulateGpuCapability(MetricCapabilities& caps, const GpuTelemetryBitset& bits)
        {
            using Lookup = IntrospectionCapsLookup<Metric>;

            // Single GPU capability bit -> metric present if bit set
            if constexpr (IsGpuDeviceMetric<Lookup>) {
                if (HasCap(bits, Lookup::gpuCapBit)) {
                    caps.Set(Metric, 1);
                }
            }

            // Array GPU capability bits (fan speeds, etc.)
            if constexpr (IsGpuDeviceMetricArray<Lookup> && !IsManualDisableMetric<Lookup>) {
                std::size_t count = 0;
                for (auto flag : Lookup::gpuCapBitArray) {
                    if (HasCap(bits, flag)) {
                        ++count;
                    }
                }
                if (count > 0) {
                    caps.Set(Metric, count);
                }
            }

            // Static GPU metrics: name/vendor/etc.
            if constexpr (IsGpuDeviceStaticMetric<Lookup>) {
                caps.Set(Metric, 1);
            }
        }

        // CPU per-metric accumulation (only instantiated for valid enum values)
        template<MetricEnum Metric>
        void AccumulateCpuCapability(MetricCapabilities& caps, const CpuTelemetryBitset& bits)
        {
            using Lookup = IntrospectionCapsLookup<Metric>;

            // CPU metrics gated by a capability bit
            if constexpr (IsCpuMetric<Lookup> && !IsManualDisableMetric<Lookup>) {
                if (HasCap(bits, Lookup::cpuCapBit)) {
                    caps.Set(Metric, 1);
                }
            }
        }
    } // namespace detail

    MetricCapabilities ConvertBitset(const GpuTelemetryBitset& bits)
    {
        MetricCapabilities caps;
        util::ForEachEnumValue<detail::MetricEnum, detail::MaxMetricUnderlying>(
            [&]<detail::MetricEnum Metric>() {
                detail::AccumulateGpuCapability<Metric>(caps, bits);
            });
        return caps;
    }

    MetricCapabilities ConvertBitset(const CpuTelemetryBitset& bits)
    {
        MetricCapabilities caps;
        util::ForEachEnumValue<detail::MetricEnum, detail::MaxMetricUnderlying>(
            [&]<detail::MetricEnum Metric>() {
                detail::AccumulateCpuCapability<Metric>(caps, bits);
            });
        return caps;
    }
}
