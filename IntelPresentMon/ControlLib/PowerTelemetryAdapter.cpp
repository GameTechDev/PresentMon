#include "PowerTelemetryAdapter.h"
#include "Logging.h"
#include "../CommonUtilities/ref/WrapReflect.h"
#include "../CommonUtilities/ref/StaticReflection.h"
#include <format>


namespace pwr
{
    using namespace std::literals;
    using namespace pmon::util;
    using v = ::pmon::util::log::V;
    using ::pmon::util::log::GlobalPolicy;

    void PowerTelemetryAdapter::SetTelemetryCapBit(GpuTelemetryCapBits telemetryCapBit) noexcept
    {
        if (GlobalPolicy::VCheck(v::tele_gpu)) {
            if (!gpuTelemetryCapBits_.test(static_cast<size_t>(telemetryCapBit))) {
                pmlog_verb(v::tele_gpu)("Telemetry cap bit being set: "s + std::string(reflect::enum_name(telemetryCapBit)))
                    .pmwatch(GetName());
            }
        }
        gpuTelemetryCapBits_.set(static_cast<size_t>(telemetryCapBit));
    }
    PowerTelemetryAdapter::SetTelemetryCapBitset PowerTelemetryAdapter::GetPowerTelemetryCapBits()
    {
        pmlog_verb(v::tele_gpu)("Telemetry cap bits being retrieved").pmwatch(GetName())
            .pmwatch(ref::DumpEnumBitset<GpuTelemetryCapBits>(gpuTelemetryCapBits_));
        return gpuTelemetryCapBits_;
    }
    bool PowerTelemetryAdapter::HasTelemetryCapBit(GpuTelemetryCapBits bit) const
    {
        pmlog_verb(v::tele_gpu)("Telemetry cap bit being retrieved: "s + std::string(reflect::enum_name(bit)))
            .pmwatch(GetName()).pmwatch(gpuTelemetryCapBits_.test(size_t(bit)));
        return gpuTelemetryCapBits_.test(size_t(bit));
    }
}