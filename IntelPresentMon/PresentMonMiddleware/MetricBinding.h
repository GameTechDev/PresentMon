#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include <ranges>
#include "DynamicMetric.h"
#include "DynamicQueryWindow.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/Meta.h"
#include "../CommonUtilities/mc/FrameMetricsMemberMap.h"
#include "../Interprocess/source/HistoryRing.h"
#include "../Interprocess/source/PmStatusError.h"

namespace pmon::ipc
{
    class MiddlewareComms;
}

namespace pmon::mid
{
    class SwapChainState;
    class Middleware;

    // container to bind and type erase a metric source type to one or more metrics
    // (telemetry rings are always 1 metric per ring, but the frame ring serves many metrics)
    class MetricBinding
    {
    public:
        virtual ~MetricBinding() = default;

        virtual void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
            const SwapChainState* pSwapChain, uint32_t processId) const = 0;
        virtual void Finalize() = 0;
        virtual void AddMetricStat(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro) = 0;
    };

    std::unique_ptr<MetricBinding> MakeFrameMetricBinding(PM_QUERY_ELEMENT& qel);
    std::unique_ptr<MetricBinding> MakeTelemetryMetricBinding(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro);
    std::unique_ptr<MetricBinding> MakeStaticMetricBinding(PM_QUERY_ELEMENT& qel, Middleware& middleware);
}
