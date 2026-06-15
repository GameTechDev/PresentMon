#include "../CommonUtilities/win/WinAPI.h"
#include "MetricBinding.h"
#include "FrameMetricsSource.h"
#include "Middleware.h"
#include "ProcessDataRate.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../CommonUtilities/Memory.h"
#include "../CommonUtilities/mc/FrameMetricsMemberMap.h"
#include <cassert>
#include <unordered_map>
#include <utility>

namespace pmon::mid
{
    using namespace std::literals;

    namespace
    {
        template<typename T>
        inline constexpr bool IsTelemetryRingValue_ =
            std::is_same_v<T, double> ||
            std::is_same_v<T, uint32_t> ||
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
                for (const auto& metric : metricPtrs_) {
                    metric->FinalizeStats();
                }

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
                const SwapChainState* pSwapChain, uint32_t processId) const override
            {
                (void)pSwapChain;
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
                const SwapChainState* pSwapChain, uint32_t processId) const override
            {
                (void)comms;
                (void)processId;

                if (pSwapChain == nullptr) {
                    auto forEachFunc = [](uint64_t, uint64_t, auto&&) {};
                    auto nearestFunc = [](uint64_t) -> const util::metrics::FrameMetrics* {
                        return nullptr;
                    };
                    this->ProcessSamples_(window, pBlobBase, forEachFunc, nearestFunc, false);
                    return;
                }

                auto forEachFunc = [pSwapChain](uint64_t start, uint64_t end, auto&& func) {
                    pSwapChain->ForEachInTimestampRange(start, end, std::forward<decltype(func)>(func));
                };
                auto nearestFunc = [pSwapChain](uint64_t point) -> const util::metrics::FrameMetrics* {
                    const size_t index = pSwapChain->NearestIndex(point);
                    return &pSwapChain->At(index);
                };

                const bool hasSamples = pSwapChain->CountInTimestampRange(window.oldest, window.newest) > 0;
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
                const SwapChainState* pSwapChain, uint32_t processId) const override
            {
                (void)window;
                (void)comms;
                (void)pSwapChain;

                if (needsConversion_) {
                    alignas(alignof(uint64_t)) uint8_t scratch[kScratchSize_]{};
                    const PM_QUERY_ELEMENT element{
                        .metric = metricId_,
                        .stat = PM_STAT_NONE,
                        .deviceId = deviceId_,
                        .arrayIndex = arrayIndex_,
                        .dataOffset = 0,
                        .dataSize = frameDataSize_,
                    };

                    middleware_.PollStaticQuery(element, processId, scratch);
                    ConvertStaticMetricValue_(pBlobBase + dataOffset_, outputType_, scratch, frameType_);
                    return;
                }

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
                const auto typeInfo = metricView.GetDataTypeInfo();
                frameType_ = typeInfo.GetFrameType();
                outputType_ = typeInfo.GetPolledType();
                const auto dataSize = ipc::intro::GetDataTypeSizeChecked(outputType_);
                qel.dataSize = (uint64_t)dataSize;
                qel.dataOffset = (uint64_t)util::PadToAlignment((size_t)qel.dataOffset, dataSize);

                dataOffset_ = qel.dataOffset;
                dataSize_ = qel.dataSize;
                frameDataSize_ = (uint64_t)ipc::intro::GetDataTypeSizeChecked(frameType_);
                needsConversion_ = frameType_ != outputType_;
            }

        private:
            static constexpr size_t kScratchSize_ = ipc::intro::DataTypeToStaticType_sz<PM_DATA_TYPE_STRING>;

            template<typename T>
            static void WriteConvertedStaticValue_(uint8_t* pTarget, PM_DATA_TYPE outType, T value)
            {
                switch (outType) {
                case PM_DATA_TYPE_DOUBLE:
                    *reinterpret_cast<double*>(pTarget) = (double)value;
                    break;
                case PM_DATA_TYPE_INT32:
                case PM_DATA_TYPE_ENUM:
                    *reinterpret_cast<int32_t*>(pTarget) = (int32_t)value;
                    break;
                case PM_DATA_TYPE_UINT32:
                    *reinterpret_cast<uint32_t*>(pTarget) = (uint32_t)value;
                    break;
                case PM_DATA_TYPE_UINT64:
                    *reinterpret_cast<uint64_t*>(pTarget) = (uint64_t)value;
                    break;
                case PM_DATA_TYPE_BOOL:
                    *reinterpret_cast<bool*>(pTarget) = value != (T)0;
                    break;
                default:
                    assert(false);
                    break;
                }
            }

            static void ConvertStaticMetricValue_(uint8_t* pTarget, PM_DATA_TYPE outType, const uint8_t* pSource, PM_DATA_TYPE inType)
            {
                if (inType == PM_DATA_TYPE_STRING || outType == PM_DATA_TYPE_STRING ||
                    inType == PM_DATA_TYPE_VOID || outType == PM_DATA_TYPE_VOID) {
                    assert(false);
                    return;
                }

                switch (inType) {
                case PM_DATA_TYPE_DOUBLE:
                    WriteConvertedStaticValue_(pTarget, outType, *reinterpret_cast<const double*>(pSource));
                    break;
                case PM_DATA_TYPE_INT32:
                    WriteConvertedStaticValue_(pTarget, outType, *reinterpret_cast<const int32_t*>(pSource));
                    break;
                case PM_DATA_TYPE_UINT32:
                    WriteConvertedStaticValue_(pTarget, outType, *reinterpret_cast<const uint32_t*>(pSource));
                    break;
                case PM_DATA_TYPE_UINT64:
                    WriteConvertedStaticValue_(pTarget, outType, *reinterpret_cast<const uint64_t*>(pSource));
                    break;
                case PM_DATA_TYPE_ENUM:
                    WriteConvertedStaticValue_(pTarget, outType, *reinterpret_cast<const int32_t*>(pSource));
                    break;
                case PM_DATA_TYPE_BOOL:
                    WriteConvertedStaticValue_(pTarget, outType, *reinterpret_cast<const bool*>(pSource));
                    break;
                default:
                    assert(false);
                    break;
                }
            }

            Middleware& middleware_;
            PM_METRIC metricId_;
            uint32_t deviceId_;
            uint32_t arrayIndex_;
            uint64_t dataOffset_ = 0;
            uint64_t dataSize_ = 0;
            uint64_t frameDataSize_ = 0;
            PM_DATA_TYPE frameType_ = PM_DATA_TYPE_VOID;
            PM_DATA_TYPE outputType_ = PM_DATA_TYPE_VOID;
            bool needsConversion_ = false;
        };

        class ProcessDataMetricBinding_ : public MetricBinding
        {
        public:
            explicit ProcessDataMetricBinding_(double qpcPeriodSeconds)
                :
                qpcPeriodSeconds_{ qpcPeriodSeconds }
            {
            }

            void Poll(const DynamicQueryWindow& window, uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
                const SwapChainState* pSwapChain, uint32_t processId) const override
            {
                (void)pSwapChain;

                const ipc::ProcessDataHistoryRing& ring = comms.GetProcessDataStore(processId).processData;
                uint64_t compileCount = 0;
                double compileDurationMsSum = 0.;
                std::vector<PsoCompileQpcInterval> busyIntervals;
                const bool needBusyPercent = avgOffsets_.find(PM_METRIC_D3D12_PSO_COMPILE_BUSY_PERCENT) != avgOffsets_.end();
                if (needBusyPercent) {
                    busyIntervals.reserve(ring.Size());
                }
                const auto serialRange = ring.GetSerialRange();
                for (size_t serial = serialRange.first; serial < serialRange.second; ++serial) {
                    const ipc::ProcessDataSample& sample = ring.At(serial);
                    const uint64_t endQpc = sample.eventCompleteQpc;
                    const uint64_t durationQpc = PsoCompileDurationMsToQpc(
                        sample.psoCompileDurationMs, qpcPeriodSeconds_);
                    const uint64_t startQpc = endQpc >= durationQpc ? endQpc - durationQpc : 0;
                    if (startQpc >= window.oldest && startQpc <= window.newest) {
                        ++compileCount;
                    }
                    uint64_t clipStart = 0;
                    uint64_t clipEnd = 0;
                    if (PsoCompileClipToWindow(startQpc, endQpc, window.oldest, window.newest, clipStart, clipEnd)) {
                        compileDurationMsSum += PsoCompileQpcToDurationMs(clipEnd - clipStart, qpcPeriodSeconds_);
                        if (needBusyPercent) {
                            busyIntervals.push_back(PsoCompileQpcInterval{ clipStart, clipEnd });
                        }
                    }
                }

                const uint64_t windowQpc = window.newest >= window.oldest ?
                    window.newest - window.oldest : 0;
                const uint64_t mergedBusyQpc = needBusyPercent ? MergePsoCompileBusyQpc(std::move(busyIntervals)) : 0;

                for (const auto& [metricId, dataOffset] : avgOffsets_) {
                    double value = 0.;
                    if (metricId == PM_METRIC_D3D12_PSO_COMPILE_COUNT) {
                        value = PsoCompileCountRate(compileCount, windowQpc, qpcPeriodSeconds_);
                    }
                    else if (metricId == PM_METRIC_D3D12_PSO_COMPILE_TIME) {
                        value = PsoCompileTimeRateMsPerSecond(compileDurationMsSum, windowQpc, qpcPeriodSeconds_);
                    }
                    else if (metricId == PM_METRIC_D3D12_PSO_COMPILE_BUSY_PERCENT) {
                        value = PsoCompileBusyPercent(mergedBusyQpc, windowQpc);
                    }
                    *reinterpret_cast<double*>(pBlobBase + dataOffset) = value;
                }
            }

            void Finalize() override
            {
            }

            void AddMetricStat(PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& intro) override
            {
                (void)intro;
                if (qel.stat != PM_STAT_AVG) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "D3D12 PSO compile metrics only support PM_STAT_AVG.");
                }
                qel.dataSize = sizeof(double);
                qel.dataOffset = (uint64_t)util::PadToAlignment((size_t)qel.dataOffset, sizeof(double));
                avgOffsets_[qel.metric] = qel.dataOffset;
            }

        private:
            double qpcPeriodSeconds_;
            std::unordered_map<PM_METRIC, uint64_t> avgOffsets_;
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

    std::unique_ptr<MetricBinding> MakeProcessDataMetricBinding(PM_QUERY_ELEMENT& qel, double qpcPeriodSeconds)
    {
        (void)qel;
        return std::make_unique<ProcessDataMetricBinding_>(qpcPeriodSeconds);
    }
}
