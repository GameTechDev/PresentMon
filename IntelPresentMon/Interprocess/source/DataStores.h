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
        static constexpr size_t virtualSegmentSize = 50'000'000;
        FrameDataStore(ShmSegmentManager& segMan, size_t cap, bool backpressured)
            :
            frameData{ cap, segMan.get_allocator<FrameData>(), backpressured },
            statics{ .applicationName{ segMan.get_allocator<char>() } }
        {}
        // values that never change over the life of a target, available for use with metric queries
        // often lazy initialized upon receipt of the first present/frame
		struct Statics
		{
			ShmString applicationName;
		} statics;
        // values used for internal bookkeeping, often static (but not necessarily), typically not derived from frame data
        // and typically initialized once on first aquisition of target; may also feed into metric queries
        struct Bookkeeping
        {
            uint32_t processId;
            int64_t startQpc;
            bool staticInitComplete = false;
            bool bookkeepingInitComplete = false;
            bool isPlayback = false;
        } bookkeeping{};
		ShmRing<FrameData> frameData;
	};

    struct GpuDataStore
    {
        static constexpr size_t virtualSegmentSize = 2'000'000;
        GpuDataStore(ShmSegmentManager& segMan)
            :
            telemetryData{ segMan.get_allocator<TelemetryMap::AllocatorType::value_type>() },
            statics{
                .name{ segMan.get_allocator<char>() },
                .maxFanSpeedRpm{ segMan.get_allocator<int32_t>() } }
        {}
        struct Statics
        {
            PM_DEVICE_VENDOR vendor;
            ShmString name;
            double sustainedPowerLimit;
            uint64_t memSize;
            uint64_t maxMemBandwidth;
            ShmVector<int32_t> maxFanSpeedRpm;
        } statics;
        TelemetryMap telemetryData;
    };

    struct SystemDataStore
    {
        static constexpr size_t virtualSegmentSize = 1'000'000;
        SystemDataStore(ShmSegmentManager& segMan)
            :
            telemetryData{ segMan.get_allocator<TelemetryMap::AllocatorType::value_type>() },
            statics{ .cpuName{ segMan.get_allocator<char>() } }
        {}
        struct Statics
        {
            PM_DEVICE_VENDOR cpuVendor;
            ShmString cpuName;
            double cpuPowerLimit;
        } statics;
        TelemetryMap telemetryData;
    };
}