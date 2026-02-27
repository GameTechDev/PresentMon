// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Middleware.h"
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdlib>
#include <numeric>
#include <algorithm>
#include <unordered_set>
#include <type_traits>
#include "../CommonUtilities/mt/Thread.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/Verbose.h"
#include "../CommonUtilities/Qpc.h"
#include "../Interprocess/source/IntrospectionTransfer.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/IntrospectionCloneAllocators.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../PresentMonService/GlobalIdentifiers.h"
#include "FrameMetricsSource.h"
#include "FrameEventQuery.h"
#include "DynamicQuery.h"
#include "QueryValidation.h"
#include "ActionClient.h"

namespace pmon::mid
{
    using namespace ipc::intro;
    using namespace util;
    using v = util::log::V;
    namespace rn = std::ranges;
    namespace vi = std::views;

    static constexpr size_t kFrameMetricsPerSwapChainCapacity = 4096u;

    namespace
    {
        const char* GetQueryTypeName_(PM_METRIC_TYPE queryType) noexcept
        {
            switch (queryType) {
            case PM_METRIC_TYPE_DYNAMIC:
                return "dynamic";
            case PM_METRIC_TYPE_FRAME_EVENT:
                return "frame";
            default:
                return "unknown";
            }
        }

        std::string GetMetricSymbol_(const pmapi::intro::Root& introRoot, PM_METRIC metric)
        {
            try {
                return introRoot.FindMetric(metric).Introspect().GetSymbol();
            }
            catch (...) {
                return "UnknownMetric";
            }
        }

        std::string GetStatSymbol_(const pmapi::intro::Root& introRoot, PM_STAT stat)
        {
            try {
                return introRoot.FindEnumKey(PM_ENUM_STAT, int(stat)).GetSymbol();
            }
            catch (...) {
                return "UnknownStat";
            }
        }

        std::string GetDeviceName_(const pmapi::intro::Root& introRoot, uint32_t deviceId)
        {
            if (deviceId == ipc::kUniversalDeviceId) {
                return "Universal";
            }
            if (deviceId == ipc::kSystemDeviceId) {
                return "System";
            }
            try {
                return introRoot.FindDevice(deviceId).GetName();
            }
            catch (...) {
                return "UnknownDevice";
            }
        }

        const auto DescribePointerArg_ = []<typename T>(const T* p, bool dereferenceScalar = true) -> std::string
        {
            if (!p) {
                return "null";
            }
            using U = std::remove_cv_t<T>;
            constexpr bool isCharLike = std::is_same_v<U, char> || std::is_same_v<U, signed char> || std::is_same_v<U, unsigned char>;
            if constexpr ((std::is_integral_v<U> && !isCharLike) || std::is_floating_point_v<U>) {
                if (dereferenceScalar) {
                    if constexpr (std::is_integral_v<U>) {
                        if constexpr (std::is_signed_v<U>) {
                            return std::to_string((long long)*p);
                        }
                        else {
                            return std::to_string((unsigned long long)*p);
                        }
                    }
                    else {
                        return std::to_string((double)*p);
                    }
                }
            }
            return "set";
        };

        void LogQueryRegistration_(PM_METRIC_TYPE queryType, const pmapi::intro::Root& introRoot,
            const void* queryHandle, std::span<const PM_QUERY_ELEMENT> queryElements)
        {
            pmlog_dbg("Registered query")
                .watch("query_type", GetQueryTypeName_(queryType))
                .pmwatch(queryHandle)
                .pmwatch(queryElements.size());

            for (size_t elementIndex = 0; elementIndex < queryElements.size(); ++elementIndex) {
                const auto& qel = queryElements[elementIndex];
                const auto metricSymbol = GetMetricSymbol_(introRoot, qel.metric);
                const auto statSymbol = GetStatSymbol_(introRoot, qel.stat);
                const auto deviceName = GetDeviceName_(introRoot, qel.deviceId);

                pmlog_dbg("Registered query element")
                    .watch("query_type", GetQueryTypeName_(queryType))
                    .pmwatch(queryHandle)
                    .pmwatch(elementIndex)
                    .pmwatch((int)qel.metric)
                    .watch("metric_symbol", metricSymbol)
                    .pmwatch((int)qel.stat)
                    .watch("stat_symbol", statSymbol)
                    .pmwatch(qel.deviceId)
                    .watch("device_name", deviceName)
                    .pmwatch(qel.arrayIndex)
                    .pmwatch(qel.dataOffset)
                    .pmwatch(qel.dataSize);
            }
        }
    }

	Middleware::Middleware(std::optional<std::string> pipeNameOverride)
	{
        const auto pipeName = pipeNameOverride.transform(&std::string::c_str)
            .value_or(pmon::gid::defaultControlPipeName);

        // Try to open a named pipe to action server; wait for it, if necessary
        if (!pipe::DuplexPipe::WaitForAvailability(pipeName, 500)) {
            throw util::Except<ipc::PmStatusError>(PM_STATUS_PIPE_ERROR,
                "Timeout waiting for service action pipe to become available");
        }
        pActionClient_ = std::make_shared<ActionClient>(pipeName);

        // connect to the shm server
        pComms_ = ipc::MakeMiddlewareComms(pActionClient_->GetShmPrefix(), pActionClient_->GetShmSalt());

        // Get and cache the introspection data
        (void)GetIntrospectionRoot_();
	}
    
    Middleware::~Middleware() = default;
    
    const PM_INTROSPECTION_ROOT* Middleware::GetIntrospectionData()
    {
        // TODO: consider updating cache or otherwise connecting to middleware intro cache here
        return pComms_->GetIntrospectionRoot();
    }

    void Middleware::FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot)
    {
        free(const_cast<PM_INTROSPECTION_ROOT*>(pRoot));
    }

    void Middleware::StartTracking(uint32_t targetPid)
    {
        if (frameMetricsSources_.contains(targetPid)) {
            throw util::Except<ipc::PmStatusError>(PM_STATUS_ALREADY_TRACKING_PROCESS,
                std::format("Process [{}] is already being tracked", targetPid));
        }
        pActionClient_->DispatchSync(StartTracking::Params{ targetPid });
        frameMetricsSources_.emplace(targetPid,
            std::make_unique<FrameMetricsSource>(*pComms_, targetPid, kFrameMetricsPerSwapChainCapacity));

        pmlog_info(std::format("Started tracking pid [{}]", targetPid)).diag();
    }

    void Middleware::StartPlaybackTracking(uint32_t targetPid, bool isBackpressured)
    {
        if (frameMetricsSources_.contains(targetPid)) {
            throw util::Except<ipc::PmStatusError>(PM_STATUS_ALREADY_TRACKING_PROCESS,
                std::format("Process [{}] is already being tracked", targetPid));
        }
        pActionClient_->DispatchSync(StartTracking::Params{
            .targetPid = targetPid,
            .isPlayback = true,
            .isBackpressured = isBackpressured
        });
        frameMetricsSources_.emplace(targetPid,
            std::make_unique<FrameMetricsSource>(*pComms_, targetPid, kFrameMetricsPerSwapChainCapacity));

        pmlog_info(std::format("Started playback tracking pid [{}]", targetPid)).diag();
    }

    void Middleware::StopTracking(uint32_t targetPid)
    {
        auto it = frameMetricsSources_.find(targetPid);
        if (it == frameMetricsSources_.end()) {
            throw util::Except<ipc::PmStatusError>(PM_STATUS_INVALID_PID,
                std::format("Process [{}] is not currently being tracked", targetPid));
        }
        pActionClient_->DispatchSync(StopTracking::Params{ targetPid });
        frameMetricsSources_.erase(it);

        pmlog_info(std::format("Stopped tracking pid [{}]", targetPid)).diag();
    }

    const pmapi::intro::Root& mid::Middleware::GetIntrospectionRoot_()
    {
        if (!pIntroRoot_) {
            pmlog_info("Creating and cacheing introspection root object").diag();
            pIntroRoot_ = std::make_unique<pmapi::intro::Root>(GetIntrospectionData(), [this](auto p){FreeIntrospectionData(p);});
        }
        return *pIntroRoot_;
    }

    void Middleware::SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t timeMs)
    {
        // note: deviceId is being ignored for the time being, but might be used in the future
        pActionClient_->DispatchSync(SetTelemetryPeriod::Params{ timeMs });
    }

    void Middleware::SetEtwFlushPeriod(std::optional<uint32_t> periodMs)
    {
        pActionClient_->DispatchSync(acts::SetEtwFlushPeriod::Params{ periodMs });
    }

    void Middleware::FlushFrames(uint32_t processId)
    {
        if (auto it = frameMetricsSources_.find(processId); it != frameMetricsSources_.end() && it->second) {
            it->second->Flush();
        }
    }

    PM_DYNAMIC_QUERY* Middleware::RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements,
        double windowSizeMs, double metricOffsetMs)
    {
        const auto qpcPeriod = util::GetTimestampPeriodSeconds();
        auto* query = new PM_DYNAMIC_QUERY{ queryElements, windowSizeMs, metricOffsetMs, qpcPeriod, *pComms_, *this };
        RegisterMetricUsage_(query, queryElements);
        LogQueryRegistration_(PM_METRIC_TYPE_DYNAMIC, GetIntrospectionRoot_(), query, queryElements);
        return query;
    }

    void Middleware::FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery)
    {
        if (pQuery == nullptr) {
            return;
        }
        UnregisterMetricUsage_(pQuery);
        delete pQuery;
    }

    void Middleware::PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId,
        uint8_t* pBlob, uint32_t* numSwapChains, std::optional<uint64_t> nowTimestamp)
    {
        const auto requestedSwapChains = numSwapChains ? *numSwapChains : 0u;
        pmlog_verb(v::middleware)("Middleware poll dynamic query")
            .pmwatch(pQuery)
            .pmwatch(processId)
            .watch("numSwapChains", DescribePointerArg_(numSwapChains))
            .pmwatch(requestedSwapChains)
            .pmwatch(nowTimestamp.has_value())
            .pmwatch(nowTimestamp.value_or(0ull));

        if (pQuery == nullptr) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "pQuery pointer is null.");
        }
        if (numSwapChains == nullptr) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "numSwapChains pointer is null.");
        }

        const uint32_t maxSwapChains = *numSwapChains;
        if (maxSwapChains == 0) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "numSwapChains is zero.");
        }
        if (pBlob == nullptr) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "pBlob pointer is null.");
        }
        if (processId == 0 && pQuery->HasFrameMetrics()) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT,
                "processId is zero but query requires frame metrics.");
        }

        *numSwapChains = 0;

        FrameMetricsSource* pFrameSource = nullptr;
        if (processId != 0) {
            pFrameSource = &GetFrameMetricSource_(processId);
            pFrameSource->Update();
        }

        const auto now = nowTimestamp.value_or((uint64_t)util::GetCurrentTimestamp());
        *numSwapChains = pQuery->Poll(pBlob, *pComms_, now, pFrameSource, processId, maxSwapChains);

        pmlog_verb(v::middleware)("Middleware poll dynamic query complete")
            .pmwatch(pQuery)
            .pmwatch(processId)
            .pmwatch(*numSwapChains);
    }

    void Middleware::PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob)
    {
        pmlog_verb(v::middleware)("Middleware poll static query")
            .pmwatch(processId)
            .pmwatch((int)element.metric)
            .pmwatch((int)element.stat)
            .pmwatch(element.deviceId)
            .pmwatch(element.arrayIndex)
            .watch("pBlob", DescribePointerArg_(pBlob, false));

        if (pBlob == nullptr) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "pBlob pointer is null.");
        }
        const ipc::StaticMetricValue value = [&]() {
            if (element.deviceId == ipc::kSystemDeviceId) {
                return pComms_->GetSystemDataStore().FindStaticMetric(element.metric);
            }
            if (element.deviceId == ipc::kUniversalDeviceId) {
                return pComms_->GetFrameDataStore(processId).FindStaticMetric(element.metric);
            }
            return pComms_->GetGpuDataStore(element.deviceId).FindStaticMetric(element.metric);
        }();

        std::visit([&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            // need stringcopy instead of memcpy for string type data (null terminator)
            if constexpr (std::is_same_v<T, const char*>) {
                strncpy_s(reinterpret_cast<char*>(pBlob), PM_MAX_PATH, v, _TRUNCATE);
            }
            else {
                std::memcpy(pBlob, &v, sizeof(v));
            }
        }, value);
    }

    PM_FRAME_QUERY* mid::Middleware::RegisterFrameEventQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t& blobSize)
    {
        auto pQuery = new PM_FRAME_QUERY{ queryElements, *this, *pComms_, GetIntrospectionRoot_() };
        blobSize = (uint32_t)pQuery->GetBlobSize();
        RegisterMetricUsage_(pQuery, queryElements);
        LogQueryRegistration_(PM_METRIC_TYPE_FRAME_EVENT, GetIntrospectionRoot_(), pQuery, queryElements);
        return pQuery;
    }

    void mid::Middleware::FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery)
    {
        UnregisterMetricUsage_(pQuery);
        delete const_cast<PM_FRAME_QUERY*>(pQuery);
    }

    void mid::Middleware::ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames)
    {
        const auto requestedFrames = numFrames;
        pmlog_verb(v::middleware)("Middleware consume frame events")
            .pmwatch(pQuery)
            .pmwatch(processId)
            .watch("pBlob", DescribePointerArg_(pBlob, false))
            .pmwatch(requestedFrames);

        if (pQuery == nullptr) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "pQuery pointer is null.");
        }
        if (numFrames > 0 && pBlob == nullptr) {
            throw Except<ipc::PmStatusError>(PM_STATUS_BAD_ARGUMENT, "pBlob pointer is null.");
        }
        const auto framesToCopy = numFrames;
        numFrames = 0;
        if (framesToCopy == 0) {
            return;
        }

        // TODO: consider making consume return one frame at a time (eliminate need for heap alloc)
        auto frames = GetFrameMetricSource_(processId).Consume(framesToCopy);
        assert(frames.size() <= framesToCopy);
        for (const auto& frameMetrics : frames) {
            pQuery->GatherToBlob(pBlob, processId, frameMetrics);
            pBlob += pQuery->GetBlobSize();
        }

        numFrames = uint32_t(frames.size());

        pmlog_verb(v::middleware)("Middleware consume frame events complete")
            .pmwatch(pQuery)
            .pmwatch(processId)
            .pmwatch(numFrames);
    }

    void Middleware::StopPlayback()
    {
        pActionClient_->DispatchSync(StopPlayback::Params{});
    }

    uint32_t Middleware::StartEtlLogging()
    {
        return pActionClient_->DispatchSync(StartEtlLogging::Params{}).etwLogSessionHandle;
    }

    std::string Middleware::FinishEtlLogging(uint32_t etlLogSessionHandle)
    {
        return pActionClient_->DispatchSync(FinishEtlLogging::Params{ etlLogSessionHandle }).etlFilePath;
    }

    bool Middleware::ServiceConnected() const
    {
        return pActionClient_->IsRunning();
    }

    FrameMetricsSource& Middleware::GetFrameMetricSource_(uint32_t pid) const
    {        
        if (auto it = frameMetricsSources_.find(pid);
            it == frameMetricsSources_.end() || it->second == nullptr) {
            pmlog_error("Frame metrics source for process {} doesn't exist. Call pmStartTracking to initialize the client.").diag();
            throw Except<util::Exception>(std::format("Failed to find frame metrics source for pid {}", pid));
        }
        else {
            return *it->second;
        }
    }

    void Middleware::RegisterMetricUsage_(const void* queryHandle, std::span<const PM_QUERY_ELEMENT> queryElements)
    {
        if (queryHandle == nullptr) {
            pmlog_warn("Attempting to register metric usage with null query handle");
            return;
        }
        std::vector<QueryMetricKey> keys;
        keys.reserve(queryElements.size());
        for (const auto& element : queryElements) {
            keys.push_back(QueryMetricKey{
                .metric = element.metric,
                .deviceId = element.deviceId,
                .arrayIndex = element.arrayIndex,
            });
        }
        queryMetricUsage_[queryHandle] = std::move(keys);
        UpdateMetricUsage_();
    }

    void Middleware::UnregisterMetricUsage_(const void* queryHandle)
    {
        if (queryHandle == nullptr) {
            pmlog_warn("Attempting to unregister metric usage with null query handle");
            return;
        }
        if (queryMetricUsage_.erase(queryHandle) > 0) {
            UpdateMetricUsage_();
        }
    }

    void Middleware::UpdateMetricUsage_()
    {
        std::unordered_set<svc::acts::MetricUse> usage;
        // TODO: remove intro here as it is not necessary
        const auto& introRoot = GetIntrospectionRoot_();
        for (const auto& [handle, elements] : queryMetricUsage_) {
            for (const auto& element : elements) {
                usage.insert(svc::acts::MetricUse{
                    .metricId = element.metric,
                    .deviceId = element.deviceId,
                    .arrayIdx = element.arrayIndex,
                });
            }
        }
        pActionClient_->DispatchSync(svc::acts::ReportMetricUse::Params{ std::move(usage) });
    }
}
