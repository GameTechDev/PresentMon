#pragma once
#include "MetricCapabilities.h"
#include "../../ControlLib/PresentMonPowerTelemetry.h"
#include "../../ControlLib/CpuTelemetryInfo.h"

namespace pmon::ipc::intro
{
    MetricCapabilities ConvertBitset(const GpuTelemetryBitset& bits);
    MetricCapabilities ConvertBitset(const CpuTelemetryBitset& bits);
}