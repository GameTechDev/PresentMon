#pragma once
#include "SharedMemoryTypes.h"
#include "ShmRing.h"
#include "TelemetryMap.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/log/Log.h"
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include "FrameDataPlaceholder.h"
#include <variant>

// these data stores intended to be hosted within StreamedDataSegment instances via template
// they provide the interface that the middleware will use to access frame/telemetry data
// as well as the interface the service will use to publish same

namespace pmon::ipc
{
	struct FrameDataStore
	{
		struct Statics
		{
			uint32_t processId;
			ShmString applicationName;
		} statics;
		ShmRing<PmFrameData> frameData;		
	};

    struct GpuDataStore
    {
        struct Statics
        {
            PM_DEVICE_VENDOR vendor;
            ShmString name;
            double sustainedPowerLimit;
            uint64_t memSize;
            uint64_t maxMemBandwidth;
        } statics;
        TelemetryMap telemetryData;
    };

    struct SystemDataStore
    {
        struct Statics
        {
            PM_DEVICE_VENDOR cpuVendor;
            ShmString cpuName;
            double cpuPowerLimit;
        } statics;
        TelemetryMap telemetryData;
    };
}