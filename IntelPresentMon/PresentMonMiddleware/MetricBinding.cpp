#include "MetricBinding.h"
#include "FrameMetricsSource.h"
#include "Middleware.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../CommonUtilities/Memory.h"
#include <cassert>
#include <utility>

namespace pmon::mid
{
    namespace
    {
        template<typename T>
        inline constexpr bool IsTelemetryRingValue_ =
            std::is_same_v<T, double> ||
            std::is_same_v<T, uint64_t> ||
            std::is_same_v<T, bool> ||
            std::is_same_v<T, int>;

        template<typename S>
        class MetricBindingBase_ : public MetricBinding
        {
        public:
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
                    assert(pMetric != nullptr);
                    if (pMetric == nullptr) {
                        return;
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

        protected:
            template<typename ForEachFunc, typename NearestFunc>
            void ProcessSamples_(const DynamicQueryWindow& window, uint8_t* pBlobBase,
                ForEachFunc&& forEachFunc, NearestFunc&& nearestFunc, bool hasSamples) const
            {
                forEachFunc(window.oldest, window.newest, [this](const S& sample) {
                    for (auto pMetric : needsFullTraversalMetricPtrs_) {
                        pMetric->AddSample(sample);
                    }
                });

                for (const std::unique_ptr<DynamicMetric<S>>& pMetric : metricPtrs_) {
                    const auto& requestedPoints = pMetric->GetRequestedSamplePoints(window);
                    if (!requestedPoints.empty()) {
                        sampledPtrs_.clear();
                        if (hasSamples) {
                            for (auto point : requestedPoints) {
                                const S* samplePtr = nearestFunc(point);
                                if (samplePtr == nullptr) {
                                    sampledPtrs_.clear();
                                    break;
                                }
                                sampledPtrs_.push_back(samplePtr);
                            }
                            if (sampledPtrs_.size() == requestedPoints.size()) {
                                pMetric->InputRequestedPointSamples(sampledPtrs_);
                            }
                        }
                        // if no samples, do not input point samples and the stats can fallback to last value or null/zero
                    }
                    pMetric->GatherToBlob(pBlobBase);
                }
            }

            std::vector<std::unique_ptr<DynamicMetric<S>>> metricPtrs_;
            std::vector<DynamicMetric<S>*> needsFullTraversalMetricPtrs_;
            mutable std::vector<const S*> sampledPtrs_;
        };

        template<typename S, uint64_t S::*TimestampMember>
        class TelemetryMetricBinding_ : public MetricBindingBase_<S>
        {
        public:
            explicit TelemetryMetricBinding_(const PM_QUERY_ELEMENT& qel)
                :
                deviceId_{ qel.deviceId },
                arrayIndex_{ qel.arrayIndex },
                metricId_{ qel.metric }
            {
            }

            void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
                FrameMetricsSource* pFrameSource, uint32_t processId) const override
            {
                (void)pFrameSource;
                (void)processId;

                const ipc::HistoryRing<S, TimestampMember>* pRing = nullptr;
                using ValueType = typename S::value_type;
                if (deviceId_ == ipc::kSystemDeviceId) {
                    pRing = &comms.GetSystemDataStore().telemetryData.FindRing<ValueType>(metricId_).at(arrayIndex_);
                }
                else {
                    pRing = &comms.GetGpuDataStore(deviceId_).telemetryData.FindRing<ValueType>(metricId_).at(arrayIndex_);
                }

                auto forEachFunc = [pRing](uint64_t start, uint64_t end, auto&& func) {
                    pRing->ForEachInTimestampRange(start, end, std::forward<decltype(func)>(func));
                };
                auto nearestFunc = [pRing](uint64_t point) -> const S* {
                    return &pRing->Nearest(point);
                };

                this->ProcessSamples_(window, pBlobBase, forEachFunc, nearestFunc, !pRing->Empty());
            }

        private:
            uint32_t deviceId_;
            uint32_t arrayIndex_;
            PM_METRIC metricId_;
        };

        class FrameMetricBinding_ : public MetricBindingBase_<util::metrics::FrameMetrics>
        {
        public:
            explicit FrameMetricBinding_(const PM_QUERY_ELEMENT&)
            {
            }

            void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
                FrameMetricsSource* pFrameSource, uint32_t processId) const override
            {
                (void)comms;
                (void)processId;

                if (pFrameSource == nullptr) {
                    throw pmon::util::Except<ipc::PmStatusError>(PM_STATUS_FAILURE,
                        "Frame metrics source missing for dynamic query.");
                }

                pFrameSource->Update();

                auto forEachFunc = [pFrameSource](uint64_t start, uint64_t end, auto&& func) {
                    pFrameSource->ForEachInActiveTimestampRange(start, end, std::forward<decltype(func)>(func));
                };
                auto nearestFunc = [pFrameSource, &window](uint64_t point) -> const util::metrics::FrameMetrics* {
                    return pFrameSource->FindNearestActive(window.oldest, window.newest, point);
                };

                const bool hasSamples = pFrameSource->HasActiveSwapChainSamples(window.oldest, window.newest);
                this->ProcessSamples_(window, pBlobBase, forEachFunc, nearestFunc, hasSamples);
            }
        };

        class StaticMetricBinding_ : public MetricBinding
        {
        public:
            StaticMetricBinding_(Middleware& middleware, const PM_QUERY_ELEMENT& qel)
                :
                middleware_{ middleware },
                metricId_{ qel.metric },
                deviceId_{ qel.deviceId },
                arrayIndex_{ qel.arrayIndex }
            {
            }

            void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
                FrameMetricsSource* pFrameSource, uint32_t processId) const override
            {
                (void)window;
                (void)comms;
                (void)pFrameSource;

                const PM_QUERY_ELEMENT element{
                    .metric = metricId_,
                    .stat = PM_STAT_NONE,
                    .deviceId = deviceId_,
                    .arrayIndex = arrayIndex_,
                    .dataOffset = dataOffset_,
                    .dataSize = dataSize_,
                };

                middleware_.PollStaticQuery(element, processId, pBlobBase + dataOffset_);
            }

            void Finalize() override
            {
            }

            void AddMetricStat(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro) override
            {
                const auto metricView = intro.FindMetric(qel.metric);
                const auto dataType = metricView.GetDataTypeInfo().GetPolledType();
                const auto dataSize = ipc::intro::GetDataTypeSize(dataType);
                qel.dataSize = (uint64_t)dataSize;
                qel.dataOffset = (uint64_t)util::PadToAlignment((size_t)qel.dataOffset, dataSize);

                dataOffset_ = qel.dataOffset;
                dataSize_ = qel.dataSize;
            }

        private:
            Middleware& middleware_;
            PM_METRIC metricId_;
            uint32_t deviceId_;
            uint32_t arrayIndex_;
            uint64_t dataOffset_ = 0;
            uint64_t dataSize_ = 0;
        };

        template<PM_DATA_TYPE dt, PM_ENUM enumId>
        struct TelemetryBindingBridger_
        {
            static std::unique_ptr<MetricBinding> Invoke(PM_ENUM, PM_QUERY_ELEMENT& qel)
            {
                using ValueType = typename ipc::intro::DataTypeToStaticType<dt>::type;
                if constexpr (IsTelemetryRingValue_<ValueType>) {
                    using SampleType = ipc::TelemetrySample<ValueType>;
                    return std::make_unique<TelemetryMetricBinding_<SampleType, &SampleType::timestamp>>(qel);
                }
                else {
                    assert(false);
                    return {};
                }
            }

            static std::unique_ptr<MetricBinding> Default(PM_QUERY_ELEMENT&)
            {
                assert(false);
                return {};
            }
        };
    }

    std::unique_ptr<MetricBinding> MakeFrameMetricBinding(PM_QUERY_ELEMENT& qel)
    {
        return std::make_unique<FrameMetricBinding_>(qel);
    }

    std::unique_ptr<MetricBinding> MakeTelemetryMetricBinding(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro)
    {
        const auto metricView = intro.FindMetric(qel.metric);
        const auto typeInfo = metricView.GetDataTypeInfo();
        return ipc::intro::BridgeDataTypeWithEnum<TelemetryBindingBridger_>(
            typeInfo.GetFrameType(), typeInfo.GetEnumId(), qel);
    }

    std::unique_ptr<MetricBinding> MakeStaticMetricBinding(PM_QUERY_ELEMENT& qel, Middleware& middleware)
    {
        return std::make_unique<StaticMetricBinding_>(middleware, qel);
    }
}
