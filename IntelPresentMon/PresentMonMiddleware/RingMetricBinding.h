#pragma once
#include <cstddef>
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
    // container to bind and type erase a metric ring static type to one or more metrics
    // (telemetry rings are always 1 metric per ring, but the frame ring serves many metrics)
    class RingMetricBinding
    {
    public:
        virtual ~RingMetricBinding() = default;

        virtual void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase, ipc::MiddlewareComms& comms, std::optional<uint32_t> pid) const = 0;
        virtual void Finalize() = 0;
        virtual void AddMetricStat(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro) = 0;
    };

    std::unique_ptr<RingMetricBinding> MakeRingMetricBinding(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro);
}
