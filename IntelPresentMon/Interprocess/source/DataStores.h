#pragma once
#include "SharedMemoryTypes.h"
#include "HistoryRing.h"
#include "TelemetryMap.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/log/Log.h"
#include "../../CommonUtilities/mc/MetricsTypes.h"
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include <optional>
#include <variant>

// these data stores intended to be hosted within StreamedDataSegment instances via template
// they provide the interface that the middleware will use to access frame/telemetry data
// as well as the interface the service will use to publish same

namespace pmon::ipc
{
    using FrameData = util::metrics::FrameData;
    using FrameHistoryRing = HistoryRing<FrameData, &FrameData::presentStartTime>;

    class MetricCapabilities;
    namespace intro
    {
        struct IntrospectionRoot;
    }

    struct DataStoreSizingInfo
    {
        // Telemetry-only: introspection root + capability map.
        const intro::IntrospectionRoot* pRoot = nullptr;
        const MetricCapabilities* pCaps = nullptr;
        // Frame ring sample capacity (process segments) or telemetry ring capacity (GPU/system).
        size_t ringSamples = 0;
        // Process segment: telemetry ring sample capacity (PSO and future process telemetry).
        size_t telemetryRingSamples = 0;
        std::optional<size_t> overrideBytes;
        // Process (target) store: backpressure behavior for the frame ring only.
        bool backpressured = false;
    };

    using StaticMetricValue = std::variant<
        double,
        uint64_t,
        int32_t,
        uint32_t,
        bool,
        int64_t,
        const char*>;

	struct ProcessDataStore
	{
        ProcessDataStore(ShmSegmentManager& segMan, size_t cap, bool backpressured)
            :
            frameData{ cap, segMan.get_allocator<FrameData>(), backpressured },
            telemetryData{ segMan.get_allocator<TelemetryMap::AllocatorType::value_type>() },
            statics{ .applicationName{ segMan.get_allocator<char>() } }
        {}
        ProcessDataStore(ShmSegmentManager& segMan, const DataStoreSizingInfo& sizing)
            :
            frameData{ sizing.ringSamples, segMan.get_allocator<FrameData>(), sizing.backpressured },
            telemetryData{ segMan.get_allocator<TelemetryMap::AllocatorType::value_type>() },
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
		FrameHistoryRing frameData;
        TelemetryMap telemetryData;

        StaticMetricValue FindStaticMetric(PM_METRIC metric) const;

        static size_t CalculateSegmentBytes(const DataStoreSizingInfo& sizing);
	};

    struct GpuDataStore
    {
        GpuDataStore(ShmSegmentManager& segMan)
            :
            telemetryData{ segMan.get_allocator<TelemetryMap::AllocatorType::value_type>() },
            statics{
                .name{ segMan.get_allocator<char>() },
                .maxFanSpeedRpm{ segMan.get_allocator<int32_t>() } }
        {}
        GpuDataStore(ShmSegmentManager& segMan, const DataStoreSizingInfo&)
            : GpuDataStore(segMan)
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

        StaticMetricValue FindStaticMetric(PM_METRIC metric) const;

        static size_t CalculateSegmentBytes(const DataStoreSizingInfo& sizing);
    };

    struct SystemDataStore
    {
        SystemDataStore(ShmSegmentManager& segMan)
            :
            telemetryData{ segMan.get_allocator<TelemetryMap::AllocatorType::value_type>() },
            statics{ .cpuName{ segMan.get_allocator<char>() } }
        {}
        SystemDataStore(ShmSegmentManager& segMan, const DataStoreSizingInfo&)
            : SystemDataStore(segMan)
        {}
        struct Statics
        {
            PM_DEVICE_VENDOR cpuVendor;
            ShmString cpuName;
            double cpuPowerLimit;
        } statics;
        TelemetryMap telemetryData;

        StaticMetricValue FindStaticMetric(PM_METRIC metric) const;

        static size_t CalculateSegmentBytes(const DataStoreSizingInfo& sizing);
    };

    void PopulateTelemetryRings(TelemetryMap& telemetryData,
        const DataStoreSizingInfo& sizing,
        PM_DEVICE_TYPE deviceType);

    MetricCapabilities MakeProcessPsoTelemetryCapabilities();
}
