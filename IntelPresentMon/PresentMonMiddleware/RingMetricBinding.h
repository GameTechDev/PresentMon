#pragma once
#include <cstddef>
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include "DynamicMetric.h"
#include "DynamicQueryWindow.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/Meta.h"
#include "../CommonUtilities/mc/FrameMetricsMemberMap.h"
#include "../Interprocess/source/HistoryRing.h"
#include "../Interprocess/source/PmStatusError.h"

namespace pmon::mid
{
    class RingMetricBindingBase
    {
    public:
        RingMetricBindingBase() = default;
        virtual ~RingMetricBindingBase() = default;

        virtual void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase) const = 0;
        virtual void Finalize() = 0;
        virtual void AddMetricStat(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro) = 0;
    };

    // container to bind and type erase a single metric ring to one or more metrics
    // (telemetry rings are always 1 metric per ring, but the frame ring serves many metrics)
    template<class S, uint64_t S::*TimestampMember>
    class RingMetricBinding : public RingMetricBindingBase
    {
    public:
        RingMetricBinding();
        ~RingMetricBinding();
        void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase) const override
        {
            // traverse ring once to handle all metrics
            ring_.ForEachInTimestampRange(window.oldest, window.newest, [this](const S& sample) {
                for (auto pMetric : needsFullTraversalMetricPtrs_) {
                    pMetric->AddSample(sample);
                }
            });
            for (const std::unique_ptr<DynamicMetric<S>>& pMetric : metricPtrs_) {
                const auto& requestedPoints = pMetric->GetRequestedSamplePoints(window);
                if (!requestedPoints.empty()) {
                    sampledPtrs_.clear();
                    if (!ring_.Empty()) {
                        for (auto point : requestedPoints) {
                            sampledPtrs_.push_back(&ring_.Nearest(point));
                        }
                        pMetric->InputRequestedPointSamples(sampledPtrs_);
                    }
                    // if ring is empty, do not input point samples and the stats can fallback to last value or null/zero
                }
                pMetric->GatherToBlob(pBlobBase);
            }
        }
        void AddMetricStat(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro) override
        {
        }
        void Finalize() override
        {
            needsFullTraversalMetricPtrs_.clear();
            for (const auto& metric : metricPtrs_) {
                if (metric->NeedsFullTraversal()) {
                    needsFullTraversalMetricPtrs_.push_back(metric.get());
                }
            }
        }
    private:
        std::vector<std::unique_ptr<DynamicMetric<S>>> metricPtrs_;
        std::vector<PM_METRIC> metricIds_;
        std::vector<DynamicMetric<S>*> needsFullTraversalMetricPtrs_;
        ipc::HistoryRing<S, TimestampMember>& ring_;
        std::vector<const S*> sampledPtrs_;
    };
}
