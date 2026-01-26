#include "RingMetricBinding.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../Interprocess/source/SystemDeviceId.h"

namespace pmon::mid
{
    template<class S, uint64_t S::*TimestampMember>
    class RingMetricBindingBound : public RingMetricBinding
    {
    public:
        RingMetricBindingBound(const PM_QUERY_ELEMENT& qel)
            :
            deviceId_{ qel.deviceId },
            arrayIndex_{ qel.arrayIndex },
            metricIds_{ { qel.metric } }
        {}
        void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
            std::optional<uint32_t> pid) const override
        {
            // find the history ring for this metric(s)
            const ipc::HistoryRing<S, TimestampMember>* pRing = nullptr;
            if constexpr (std::is_same_v<S, ipc::FrameData>) {
                if (!pid) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Frame metrics require a process id.");
                }
                pRing = &comms.GetFrameDataStore(*pid).frameData;
            }
            else {
                if (pid) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Telemetry metrics do not accept a process id.");
                }
                using ValueType = typename S::value_type;
                if (deviceId_ == ipc::kSystemDeviceId) {
                    pRing = &comms.GetSystemDataStore().telemetryData.FindRing<ValueType>(metricIds_.front()).at(arrayIndex_);
                }
                else {
                    pRing = &comms.GetGpuDataStore(deviceId_).telemetryData.FindRing<ValueType>(metricIds_.front()).at(arrayIndex_);
                }
            }
            // traverse ring once to handle all metrics
            pRing->ForEachInTimestampRange(window.oldest, window.newest, [this](const S& sample) {
                for (auto pMetric : needsFullTraversalMetricPtrs_) {
                    pMetric->AddSample(sample);
                }
            });
            // handle metrics having point-sampled stats
            for (const std::unique_ptr<DynamicMetric<S>>& pMetric : metricPtrs_) {
                const auto& requestedPoints = pMetric->GetRequestedSamplePoints(window);
                if (!requestedPoints.empty()) {
                    sampledPtrs_.clear();
                    if (!pRing->Empty()) {
                        for (auto point : requestedPoints) {
                            sampledPtrs_.push_back(&pRing->Nearest(point));
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
            DynamicMetric<S>* pMetric = nullptr;
            // if metric already exists, we add the stat to it
            if (auto it = std::ranges::find(metricPtrs_,
                qel.metric, [](const auto& p) { return p->GetMetricId(); }); it != metricPtrs_.end()) {
                pMetric = it->get();
            }

            // if metric doesn't exist, we must make it
            if (!pMetric) {
                auto pNewMetric = MakeDynamicMetric<S>(qel);
                pMetric = pNewMetric.get();
                if (pMetric == nullptr) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Unsupported metric for dynamic query.");
                }
                metricPtrs_.push_back(std::move(pNewMetric));
            }

            // now add the stat
            pMetric->AddStat(qel, intro);
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
        // device id only required for telemetry ring lookup
        uint32_t deviceId_;
        // index currently only applies to telemetry ring lookup
        uint32_t arrayIndex_;
        std::vector<std::unique_ptr<DynamicMetric<S>>> metricPtrs_;
        std::vector<PM_METRIC> metricIds_;
        std::vector<DynamicMetric<S>*> needsFullTraversalMetricPtrs_;
        mutable std::vector<const S*> sampledPtrs_;
    };

    namespace
    {
        template<typename T>
        inline constexpr bool IsTelemetryRingValue_ =
            std::is_same_v<T, double> ||
            std::is_same_v<T, uint64_t> ||
            std::is_same_v<T, bool> ||
            std::is_same_v<T, int>;

        template<PM_DATA_TYPE dt, PM_ENUM enumId>
        struct TelemetryRingBindingBridger_
        {
            static std::unique_ptr<RingMetricBinding> Invoke(PM_ENUM, PM_QUERY_ELEMENT& qel)
            {
                using ValueType = typename ipc::intro::DataTypeToStaticType<dt>::type;
                if constexpr (IsTelemetryRingValue_<ValueType>) {
                    using SampleType = ipc::TelemetrySample<ValueType>;
                    return std::make_unique<RingMetricBindingBound<SampleType, &SampleType::timestamp>>(qel);
                }
                else {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Unsupported telemetry ring data type for dynamic query.");
                }
            }
            static std::unique_ptr<RingMetricBinding> Default(PM_QUERY_ELEMENT&)
            {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                    "Unknown telemetry ring data type for dynamic query.");
            }
        };
    }

    std::unique_ptr<RingMetricBinding> MakeRingMetricBinding(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro)
    {
        // handle frame metric store case
        if (qel.deviceId == ipc::kUniversalDeviceId) {
            return std::make_unique<RingMetricBindingBound<ipc::FrameData, &ipc::FrameData::presentStartTime>>(qel);
        }
        // handle telemetry metric store case
        const auto metricView = intro.FindMetric(qel.metric);
        const auto typeInfo = metricView.GetDataTypeInfo();
        return ipc::intro::BridgeDataTypeWithEnum<TelemetryRingBindingBridger_>(
            typeInfo.GetFrameType(), typeInfo.GetEnumId(), qel);
    }
}
