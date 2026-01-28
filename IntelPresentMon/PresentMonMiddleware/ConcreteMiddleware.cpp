// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ConcreteMiddleware.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdlib>
#include <type_traits>
#include <Shlwapi.h>
#include <numeric>
#include <algorithm>
#include "../PresentMonUtils/QPCUtils.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
// TODO: don't need transfer if we can somehow get the PM_ struct generation working without inheritance
// needed right now because even if we forward declare, we don't have the inheritance info
#include "../Interprocess/source/IntrospectionTransfer.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/IntrospectionCloneAllocators.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/PmStatusError.h"
#include "DynamicQuery.h"
#include "../ControlLib/PresentMonPowerTelemetry.h"
#include "../ControlLib/CpuTelemetryInfo.h"
#include "../PresentMonService/GlobalIdentifiers.h"
#include "FrameEventQuery.h"
#include "FrameMetricsSource.h"
#include "../CommonUtilities/mt/Thread.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Qpc.h"

#include "../CommonUtilities/log/GlogShim.h"

#include "ActionClient.h"
#include "QueryValidation.h"

namespace pmon::mid
{
    using namespace ipc::intro;
    using namespace util;
    namespace rn = std::ranges;
    namespace vi = std::views;

    static const uint32_t kMaxRespBufferSize = 4096;
	static const uint64_t kClientFrameDeltaQPCThreshold = 50000000;
    static constexpr size_t kFrameMetricsPerSwapChainCapacity = 4096u;

	ConcreteMiddleware::ConcreteMiddleware(std::optional<std::string> pipeNameOverride)
	{
        const auto pipeName = pipeNameOverride.transform(&std::string::c_str)
            .value_or(pmon::gid::defaultControlPipeName);

        // Try to open a named pipe to action server; wait for it, if necessary
        try {
            if (!pipe::DuplexPipe::WaitForAvailability(pipeName, 500)) {
                throw std::runtime_error{ "Timeout waiting for service action pipe to become available" };
            }
            pActionClient = std::make_shared<ActionClient>(pipeName);
        }
        catch (...) {
            pmlog_error(util::ReportException()).diag();
            throw util::Except<ipc::PmStatusError>(PM_STATUS_PIPE_ERROR);
        }

        // connect to the shm server
        pComms = ipc::MakeMiddlewareComms(pActionClient->GetShmPrefix(), pActionClient->GetShmSalt());

        // Get and cache the introspection data
        try {
            auto& ispec = GetIntrospectionRoot_();
        }
        catch (...) {
            pmlog_error(ReportException("Problem acquiring introspection data"));
            throw;
        }
	}
    
    ConcreteMiddleware::~ConcreteMiddleware() = default;
    
    const PM_INTROSPECTION_ROOT* ConcreteMiddleware::GetIntrospectionData()
    {
        // TODO: consider updating cache or otherwise connecting to middleware intro cache here
        return pComms->GetIntrospectionRoot();
    }

    void ConcreteMiddleware::FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot)
    {
        free(const_cast<PM_INTROSPECTION_ROOT*>(pRoot));
    }

    // TODO: rename => tracking
    PM_STATUS ConcreteMiddleware::StartStreaming(uint32_t targetPid)
    {
        try {
            auto res = pActionClient->DispatchSync(StartTracking::Params{ targetPid });
            // TODO: error when already tracking
            auto sourceIter = frameMetricsSources.find(targetPid);
            if (sourceIter == frameMetricsSources.end()) {
                frameMetricsSources.emplace(targetPid,
                    std::make_unique<FrameMetricsSource>(*pComms, targetPid, kFrameMetricsPerSwapChainCapacity));
            }
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
            return code;
        }

        pmlog_info(std::format("Started tracking pid [{}]", targetPid)).diag();
        return PM_STATUS_SUCCESS;
    }

    PM_STATUS ConcreteMiddleware::StartPlaybackTracking(uint32_t targetPid, bool isBackpressured)
    {
        try {
            auto res = pActionClient->DispatchSync(StartTracking::Params{
                .targetPid = targetPid,
                .isPlayback = true,
                .isBackpressured = isBackpressured
            });
            // TODO: error when already tracking
            auto sourceIter = frameMetricsSources.find(targetPid);
            if (sourceIter == frameMetricsSources.end()) {
                frameMetricsSources.emplace(targetPid,
                    std::make_unique<FrameMetricsSource>(*pComms, targetPid, kFrameMetricsPerSwapChainCapacity));
            }
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
            return code;
        }

        pmlog_info(std::format("Started playback tracking pid [{}]", targetPid)).diag();
        return PM_STATUS_SUCCESS;
    }

    // TODO: rename => tracking
    PM_STATUS ConcreteMiddleware::StopStreaming(uint32_t targetPid)
    {
        try {
            // TODO: error when not tracking (returns 0 not 1)
            frameMetricsSources.erase(targetPid);
            pActionClient->DispatchSync(StopTracking::Params{ targetPid });
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
            return code;
        }

        pmlog_info(std::format("Stopped tracking pid [{}]", targetPid)).diag();
        return PM_STATUS_SUCCESS;
    }

    const pmapi::intro::Root& mid::ConcreteMiddleware::GetIntrospectionRoot_()
    {
        if (!pIntroRoot) {
            pmlog_info("Creating and cacheing introspection root object").diag();
            pIntroRoot = std::make_unique<pmapi::intro::Root>(GetIntrospectionData(), [this](auto p){FreeIntrospectionData(p);});
        }
        return *pIntroRoot;
    }

    PM_STATUS ConcreteMiddleware::SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t timeMs)
    {
        try {
            // note: deviceId is being ignored for the time being, but might be used in the future
            pActionClient->DispatchSync(SetTelemetryPeriod::Params{ timeMs });
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
            return code;
        }
        return PM_STATUS_SUCCESS;
    }

    PM_STATUS ConcreteMiddleware::SetEtwFlushPeriod(std::optional<uint32_t> periodMs)
    {
        try {
            pActionClient->DispatchSync(acts::SetEtwFlushPeriod::Params{ periodMs });
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
            return code;
        }
        return PM_STATUS_SUCCESS;
    }

    PM_DYNAMIC_QUERY* ConcreteMiddleware::RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements,
        double windowSizeMs, double metricOffsetMs)
    {
        pmlog_dbg("Registering dynamic query").pmwatch(queryElements.size()).pmwatch(windowSizeMs).pmwatch(metricOffsetMs);
        const auto qpcPeriod = util::GetTimestampPeriodSeconds();
        return new PM_DYNAMIC_QUERY{ queryElements, windowSizeMs, metricOffsetMs, qpcPeriod, *pComms };
    }

    void ConcreteMiddleware::PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains)
    {
        // TODO: implement multi-swap handling
        // locate frame metric source for target process if required
        FrameMetricsSource* pFrameSource = nullptr;
        if (processId) {
            pFrameSource = &GetFrameMetricSource_(processId);
        }
        // execute the dynamic poll operation
        pQuery->Poll(pBlob, *pComms, (uint64_t)util::GetCurrentTimestamp(), pFrameSource);
    }

    void ConcreteMiddleware::PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob)
    {
        const ipc::StaticMetricValue value = [&]() {
            if (element.deviceId == ipc::kSystemDeviceId) {
                return pComms->GetSystemDataStore().FindStaticMetric(element.metric);
            }
            if (element.deviceId == ipc::kUniversalDeviceId) {
                return pComms->GetFrameDataStore(processId).FindStaticMetric(element.metric);
            }
            return pComms->GetGpuDataStore(element.deviceId).FindStaticMetric(element.metric);
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

    PM_FRAME_QUERY* mid::ConcreteMiddleware::RegisterFrameEventQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t& blobSize)
    {
        auto pQuery = new PM_FRAME_QUERY{ queryElements, *this, *pComms, GetIntrospectionRoot_() };
        blobSize = (uint32_t)pQuery->GetBlobSize();
        return pQuery;
    }

    void mid::ConcreteMiddleware::FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery)
    {
        delete const_cast<PM_FRAME_QUERY*>(pQuery);
    }

    void mid::ConcreteMiddleware::ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames)
    {
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

    void ConcreteMiddleware::StopPlayback()
    {
        pActionClient->DispatchSync(StopPlayback::Params{});
    }

    uint32_t ConcreteMiddleware::StartEtlLogging()
    {
        return pActionClient->DispatchSync(StartEtlLogging::Params{}).etwLogSessionHandle;
    }

    std::string ConcreteMiddleware::FinishEtlLogging(uint32_t etlLogSessionHandle)
    {
        return pActionClient->DispatchSync(FinishEtlLogging::Params{ etlLogSessionHandle }).etlFilePath;
    }

    FrameMetricsSource& ConcreteMiddleware::GetFrameMetricSource_(uint32_t pid) const
    {        
        if (auto it = frameMetricsSources.find(pid);
            it == frameMetricsSources.end() || it->second == nullptr) {
            pmlog_error("Frame metrics source for process {} doesn't exist. Call pmStartTracking to initialize the client.").diag();
            throw Except<util::Exception>(std::format("Failed to find frame metrics source for pid {}", pid));
        }
        else {
            return *it->second;
        }
    }
}
