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
#include "../CommonUtilities/mt/Thread.h"
#include "../CommonUtilities/log/Log.h"
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
    namespace rn = std::ranges;
    namespace vi = std::views;

    static constexpr size_t kFrameMetricsPerSwapChainCapacity = 4096u;

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
        pmlog_dbg("Registering dynamic query").pmwatch(queryElements.size()).pmwatch(windowSizeMs).pmwatch(metricOffsetMs);
        const auto qpcPeriod = util::GetTimestampPeriodSeconds();
        auto* query = new PM_DYNAMIC_QUERY{ queryElements, windowSizeMs, metricOffsetMs, qpcPeriod, *pComms_, *this };
        RegisterMetricUsage_(query, queryElements);
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
    }

    void Middleware::PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob)
    {
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
        return pQuery;
    }

    void mid::Middleware::FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery)
    {
        UnregisterMetricUsage_(pQuery);
        delete const_cast<PM_FRAME_QUERY*>(pQuery);
    }

    void mid::Middleware::ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames)
    {
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
