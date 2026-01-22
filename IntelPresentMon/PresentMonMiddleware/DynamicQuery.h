#pragma once
#include <vector>
#include <bitset>
#include <map>
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../ControlLib/CpuTelemetryInfo.h"
#include "../ControlLib/PresentMonPowerTelemetry.h"

namespace pmapi::intro
{
	class Root;
	class MetricView;
}

namespace pmon::mid
{
	class Middleware;
}

namespace pmon::ipc
{
	class MiddlewareComms;
	class TelemetryMap;
}

struct PM_DYNAMIC_QUERY
{
public:
	PM_DYNAMIC_QUERY();

private:
};

