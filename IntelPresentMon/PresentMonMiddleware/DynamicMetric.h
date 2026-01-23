#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>
#include <ranges>
#include "DynamicQueryWindow.h"
#include "DynamicStat.h"
#include "../CommonUtilities/Exception.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"

namespace pmapi::intro
{
    class Root;
    class MetricView;
    class StatView;
}

namespace pmon::mid
{
    // container for multiple stats connected to a single metric
    // implements shared stat calculation facilities (i.e. sorted sample buffer for %)
    template<typename S>
    class DynamicMetric
    {
    public:
        DynamicMetric() = default;
        ~DynamicMetric() = default;

        virtual void AddSample(const S& sample) = 0;
        virtual const std::vector<uint64_t>& GetRequestedSamplePoints(const DynamicQueryWindow& window) const = 0;
        virtual void InputRequestedPointSamples(const std::vector<const S*>& samples) = 0;
        virtual void GatherToBlob(uint8_t* pBlobBase) const = 0;
        virtual uint32_t AddStat(PM_STAT stat, uint32_t blobByteOffset, const pmapi::intro::Root& intro) = 0;
        virtual void FinalizeStats() = 0;
        virtual bool NeedsFullTraversal() const = 0;
    };

    template<typename S, typename T, T S::* MemberPtr>
    class DynamicMetricBinding : public DynamicMetric<S>
    {
    public:
        DynamicMetricBinding(PM_METRIC metric)
            :
            metric_{ metric }
        {
            static_assert(std::is_same_v<T, double> || std::is_same_v<T, int32_t> || std::is_same_v<T, bool>,
                "DynamicMetricBinding only supports double, int32_t, and bool sample types.");
        }

        void AddSample(const S& sample) override
        {
            const auto& value = sample.*MemberPtr;
            // if samples has reserved size, it is needed
            if (samples_.capacity()) {
                samples_.push_back(value);
            }
            for (auto* stat : needsUpdatePtrs_) {
                stat->AddSample(value);
            }
        }

        const std::vector<uint64_t>& GetRequestedSamplePoints(const DynamicQueryWindow& window) const override
        {
            if (!needsSamplePtrs_.empty()) {
                requestedSamplePoints_.clear();
                for (auto* stat : needsSamplePtrs_) {
                    requestedSamplePoints_.push_back(stat->GetSamplePoint(window));
                }
            }
            return requestedSamplePoints_;
        }

        void InputRequestedPointSamples(const std::vector<const S*>& samplePtrs) override
        {
            if (samplePtrs.size() != needsSamplePtrs_.size()) {
                throw pmon::util::Except<pmon::ipc::PmStatusError>(PM_STATUS_FAILURE,
                    "DynamicMetricBinding received unexpected number of point samples.");
            }

            for (auto&& [sample, stat] : std::views::zip(samplePtrs, needsSamplePtrs_)) {
                if (sample == nullptr) {
                    throw pmon::util::Except<pmon::ipc::PmStatusError>(PM_STATUS_FAILURE,
                        "DynamicMetricBinding received null point sample.");
                }
                stat->SetSampledValue(sample->*MemberPtr);
            }
        }

        void GatherToBlob(uint8_t* pBlobBase) const override
        {
            if (!needsSortedWindowPtrs_.empty()) {
                std::ranges::sort(samples_);
                for (auto pStat : needsSortedWindowPtrs_) {
                    pStat->InputSortedSamples(samples_);
                }
            }
            // clear the sample sorting buffer for the next poll
            samples_.clear();
            for (auto& pStat : statPtrs_) {
                pStat->GatherToBlob(pBlobBase);
            }
        }

        uint32_t AddStat(PM_STAT stat, uint32_t blobByteOffset, const pmapi::intro::Root& intro) override
        {
            const auto metricView = intro.FindMetric(metric_);
            if (!IsStatSupported_(stat, metricView)) {
                throw pmon::util::Except<pmon::ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                    "Dynamic metric stat not supported by metric.");
            }

            const auto inType = GetSampleType_();
            auto outType = SelectOutputType_(stat, metricView.GetDataTypeInfo().GetPolledType());
            auto statPtr = MakeDynamicStat<T>(stat, inType, outType, blobByteOffset);
            auto* rawPtr = statPtr.get();
            statPtrs_.push_back(std::move(statPtr));

            if (rawPtr->NeedsPointSample()) {
                needsSamplePtrs_.push_back(rawPtr);
            }
            else if (rawPtr->NeedsSortedWindow()) {
                needsSortedWindowPtrs_.push_back(rawPtr);
            }
            else if (!rawPtr->NeedsSortedWindow() && rawPtr->NeedsUpdate()) {
                needsUpdatePtrs_.push_back(rawPtr);
            }

            return (uint32_t)ipc::intro::GetDataTypeSize(outType);
        }

        void FinalizeStats() override
        {
            requestedSamplePoints_.reserve(needsSamplePtrs_.size());
            if (!needsSortedWindowPtrs_.empty()) {
                // a decent middleground based on typical 1 sec window and 144 fps typical upper fps
                samples_.reserve(150);
            }
        }

        ~DynamicMetricBinding() = default;

    private:
        static constexpr PM_DATA_TYPE GetSampleType_()
        {
            if constexpr (std::is_same_v<T, double>) {
                return PM_DATA_TYPE_DOUBLE;
            }
            else if constexpr (std::is_same_v<T, int32_t>) {
                return PM_DATA_TYPE_INT32;
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return PM_DATA_TYPE_BOOL;
            }
            else {
                return PM_DATA_TYPE_VOID;
            }
        }

        static PM_DATA_TYPE SelectOutputType_(PM_STAT stat, PM_DATA_TYPE metricOutType)
        {
            if (stat == PM_STAT_AVG || stat == PM_STAT_NON_ZERO_AVG) {
                return PM_DATA_TYPE_DOUBLE;
            }
            return metricOutType;
        }

        static bool IsStatSupported_(PM_STAT stat, const pmapi::intro::MetricView& metricView)
        {
            for (auto statInfo : metricView.GetStatInfo()) {
                if (statInfo.GetStat() == stat) {
                    return true;
                }
            }
            return false;
        }

        PM_METRIC metric_;
        mutable std::vector<T> samples_;
        std::vector<std::unique_ptr<DynamicStat<T>>> statPtrs_;
        std::vector<DynamicStat<T>*> needsUpdatePtrs_;
        std::vector<DynamicStat<T>*> needsSamplePtrs_;
        std::vector<DynamicStat<T>*> needsSortedWindowPtrs_;
        mutable std::vector<uint64_t> requestedSamplePoints_;
    };
}
