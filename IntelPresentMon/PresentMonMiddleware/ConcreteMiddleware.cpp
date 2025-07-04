// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ConcreteMiddleware.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdlib>
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
#include "../Interprocess/source/PmStatusError.h"
//#include "MockCommon.h"
#include "DynamicQuery.h"
#include "../ControlLib/PresentMonPowerTelemetry.h"
#include "../ControlLib/CpuTelemetryInfo.h"
#include "../PresentMonService/GlobalIdentifiers.h"
#include "FrameEventQuery.h"
#include "../CommonUtilities/mt/Thread.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Qpc.h"

#include "../CommonUtilities/log/GlogShim.h"

#include "ActionClient.h"

namespace pmon::mid
{
    using namespace ipc::intro;
    using namespace util;
    namespace rn = std::ranges;
    namespace vi = std::views;

    static const uint32_t kMaxRespBufferSize = 4096;
	static const uint64_t kClientFrameDeltaQPCThreshold = 50000000;
	ConcreteMiddleware::ConcreteMiddleware(std::optional<std::string> pipeNameOverride)
	{
        const auto pipeName = pipeNameOverride.transform(&std::string::c_str)
            .value_or(pmon::gid::defaultControlPipeName);

        // Try to open a named pipe; wait for it, if necessary
        try {
            if (!pipe::DuplexPipe::WaitForAvailability(pipeName + "-in"s, 500)) {
                throw std::runtime_error{ "Timeout waiting for service action pipe to become available" };
            }
            pActionClient = std::make_shared<ActionClient>(pipeName);
        }
        catch (...) {
            pmlog_error(util::ReportException()).diag();
            throw util::Except<ipc::PmStatusError>(PM_STATUS_PIPE_ERROR);
        }

        clientProcessId = GetCurrentProcessId();

        // discover introspection shm name
        auto res = pActionClient->DispatchSync(GetIntrospectionShmName::Params{});

        // connect to the introspection nsm
        pComms = ipc::MakeMiddlewareComms(std::move(res.name));

        // Get the introspection data
        try {
            auto& ispec = GetIntrospectionRoot();

            uint32_t gpuAdapterId = 0;
            auto deviceView = ispec.GetDevices();
            for (auto dev : deviceView) {
                if (dev.GetType() == PM_DEVICE_TYPE_GRAPHICS_ADAPTER)
                {
                    cachedGpuInfo.push_back({ dev.GetVendor(), dev.GetName(), dev.GetId(), gpuAdapterId, 0., 0, 0 });
                    gpuAdapterId++;
                }
            }
        }
        catch (...) {
            pmlog_error(ReportException("Problem acquiring introspection data"));
            throw;
        }

        // Update the static GPU metric data from the service
        GetStaticGpuMetrics();
        GetStaticCpuMetrics();
	}
    
    ConcreteMiddleware::~ConcreteMiddleware() = default;
    
    const PM_INTROSPECTION_ROOT* ConcreteMiddleware::GetIntrospectionData()
    {
        return pComms->GetIntrospectionRoot();
    }

    void ConcreteMiddleware::FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot)
    {
        free(const_cast<PM_INTROSPECTION_ROOT*>(pRoot));
    }

    PM_STATUS ConcreteMiddleware::StartStreaming(uint32_t targetPid)
    {
        try {
            auto res = pActionClient->DispatchSync(StartTracking::Params{ targetPid });
            // Initialize client in client map using returned nsm name
            auto iter = presentMonStreamClients.find(targetPid);
            if (iter == presentMonStreamClients.end()) {
                presentMonStreamClients.emplace(targetPid,
                    std::make_unique<StreamClient>(std::move(res.nsmFileName), false));
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

    PM_STATUS ConcreteMiddleware::StopStreaming(uint32_t targetPid)
    {
        try {
            pActionClient->DispatchSync(StopTracking::Params{ targetPid });
            // Remove client from map of clients
            auto iter = presentMonStreamClients.find(targetPid);
            if (iter != presentMonStreamClients.end()) {
                presentMonStreamClients.erase(std::move(iter));
            }
            // Remove the input to frame start data for this process.
            mPclI2FsManager.RemoveProcess(targetPid);
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
            return code;
        }

        pmlog_info(std::format("Stop tracking pid [{}]", targetPid)).diag();
        return PM_STATUS_SUCCESS;
    }

    void ConcreteMiddleware::GetStaticCpuMetrics()
    {
        try {
            auto metrics = pActionClient->DispatchSync(GetStaticCpuMetrics::Params{});

            const auto cpuNameLower = str::ToLower(metrics.cpuName);
            PM_DEVICE_VENDOR deviceVendor;
            if (cpuNameLower.contains("intel")) {
                deviceVendor = PM_DEVICE_VENDOR_INTEL;
            }
            else if (cpuNameLower.contains("amd")) {
                deviceVendor = PM_DEVICE_VENDOR_AMD;
            }
            else {
                deviceVendor = PM_DEVICE_VENDOR_UNKNOWN;
            }

            cachedCpuInfo.push_back({
                .deviceVendor = deviceVendor,
                .deviceName = std::move(metrics.cpuName),
                .cpuPowerLimit = metrics.cpuPowerLimit
            });
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
        }
    }

    std::string ConcreteMiddleware::GetProcessName(uint32_t processId)
    {
        HANDLE handle = NULL;
        std::string processName = "<UNKNOWN>";
        char path[MAX_PATH];
        DWORD numChars = sizeof(path);
        handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (handle) {
            if (QueryFullProcessImageNameA(handle, 0, path, &numChars)) {
                processName = PathFindFileNameA(path);
            }
            CloseHandle(handle);
        }
        return processName;
    }

    const pmapi::intro::Root& mid::ConcreteMiddleware::GetIntrospectionRoot()
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

    PM_DYNAMIC_QUERY* ConcreteMiddleware::RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs)
    { 
        // get introspection data for reference
        // TODO: cache this data so it's not required to be generated every time
        auto& ispec = GetIntrospectionRoot();

        // make the query object that will be managed by the handle
        auto pQuery = std::make_unique<PM_DYNAMIC_QUERY>();
        std::optional<uint32_t> cachedGpuInfoIndex;

        uint64_t offset = 0u;
        for (auto& qe : queryElements) {
            // A device of zero is NOT a graphics adapter.
            if (qe.deviceId != 0) {
                // If we have already set a device id in this query, check to
                // see if it's the same device id as previously set. Currently
                // we don't support querying multiple gpu devices in the one
                // query
                if (cachedGpuInfoIndex.has_value()) {
                    const auto cachedDeviceId = cachedGpuInfo[cachedGpuInfoIndex.value()].deviceId;
                    if (cachedDeviceId != qe.deviceId) {
                        pmlog_error(std::format("Multiple GPU devices not allowed in single query ({} and {})",
                            cachedDeviceId, qe.deviceId)).diag();
                        throw Except<util::Exception>("Multiple GPU devices not allowed in single query");
                    }
                }
                else {
                    // Go through the cached Gpus and see which device the client
                    // wants
                    if (auto i = rn::find(cachedGpuInfo, qe.deviceId, &DeviceInfo::deviceId);
                        i != cachedGpuInfo.end()) {
                        cachedGpuInfoIndex = uint32_t(i - cachedGpuInfo.begin());
                    }
                    else {
                        pmlog_error(std::format("unable to find device id [{}] while building dynamic query", qe.deviceId)).diag();
                        // TODO: shouldn't we throw here?
                    }
                }
            }

            auto metricView = ispec.FindMetric(qe.metric);
            switch (qe.metric) {
            case PM_METRIC_APPLICATION:
            case PM_METRIC_SWAP_CHAIN_ADDRESS:
            case PM_METRIC_PRESENT_MODE:
            case PM_METRIC_PRESENT_RUNTIME:
            case PM_METRIC_PRESENT_FLAGS:
            case PM_METRIC_SYNC_INTERVAL:
            case PM_METRIC_ALLOWS_TEARING:
            case PM_METRIC_FRAME_TYPE:
            case PM_METRIC_CPU_START_QPC:
            case PM_METRIC_CPU_BUSY:
            case PM_METRIC_CPU_WAIT:
            case PM_METRIC_CPU_FRAME_TIME:
            case PM_METRIC_GPU_LATENCY:
            case PM_METRIC_GPU_BUSY:
            case PM_METRIC_GPU_WAIT:
            case PM_METRIC_GPU_TIME:
            case PM_METRIC_DISPLAY_LATENCY:
            case PM_METRIC_DISPLAYED_TIME:
            case PM_METRIC_ANIMATION_ERROR:
            case PM_METRIC_PRESENTED_FPS:
            case PM_METRIC_APPLICATION_FPS:
            case PM_METRIC_DISPLAYED_FPS:
            case PM_METRIC_DROPPED_FRAMES:
            case PM_METRIC_CLICK_TO_PHOTON_LATENCY:
            case PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY:
            case PM_METRIC_INSTRUMENTED_LATENCY:
            case PM_METRIC_PRESENT_START_QPC:
            case PM_METRIC_BETWEEN_PRESENTS:
            case PM_METRIC_IN_PRESENT_API:
            case PM_METRIC_BETWEEN_DISPLAY_CHANGE:
            case PM_METRIC_UNTIL_DISPLAYED:
            case PM_METRIC_RENDER_PRESENT_LATENCY:
            case PM_METRIC_BETWEEN_SIMULATION_START:
            case PM_METRIC_PC_LATENCY:
            case PM_METRIC_DISPLAYED_FRAME_TIME:
            //case PM_METRIC_INSTRUMENTED_RENDER_DISPLAY_LATENCY:
                pQuery->accumFpsData = true;
                break;
            case PM_METRIC_GPU_POWER:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_power));
                break;
            case PM_METRIC_GPU_VOLTAGE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_voltage));
                break;
            case PM_METRIC_GPU_FREQUENCY:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_frequency));
                break;
            case PM_METRIC_GPU_TEMPERATURE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_temperature));
                break;
            case PM_METRIC_GPU_UTILIZATION:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_utilization));
                break;
            case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_render_compute_utilization));
                break;
            case PM_METRIC_GPU_MEDIA_UTILIZATION:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_media_utilization));
                break;
            case PM_METRIC_GPU_MEM_POWER:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_power));
                break;
            case PM_METRIC_GPU_MEM_VOLTAGE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_voltage));
                break;
            case PM_METRIC_GPU_MEM_FREQUENCY:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_frequency));
                break;
            case PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_effective_frequency));
                break;
            case PM_METRIC_GPU_MEM_TEMPERATURE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_temperature));
                break;
            case PM_METRIC_GPU_MEM_USED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_used));
                break;
            case PM_METRIC_GPU_MEM_UTILIZATION:
                // Gpu mem utilization is derived from mem size and mem used.
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_used));
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_size));
                break;
            case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_write_bandwidth));
                break;
            case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_read_bandwidth));
                break;
            case PM_METRIC_GPU_POWER_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_power_limited));
                break;
            case PM_METRIC_GPU_TEMPERATURE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_temperature_limited));
                break;
            case PM_METRIC_GPU_CURRENT_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_current_limited));
                break;
            case PM_METRIC_GPU_VOLTAGE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_voltage_limited));
                break;
            case PM_METRIC_GPU_UTILIZATION_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_utilization_limited));
                break;
            case PM_METRIC_GPU_MEM_POWER_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_power_limited));
                break;
            case PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_temperature_limited));
                break;
            case PM_METRIC_GPU_MEM_CURRENT_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_current_limited));
                break;
            case PM_METRIC_GPU_MEM_VOLTAGE_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_voltage_limited));
                break;
            case PM_METRIC_GPU_MEM_UTILIZATION_LIMITED:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::vram_utilization_limited));
                break;
            case PM_METRIC_GPU_FAN_SPEED:
                switch (qe.arrayIndex)
                {
                case 0:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_0));
                    break;
                case 1:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_1));
                    break;
                case 2:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_2));
                    break;
                case 3:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_3));
                    break;
                case 4:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::fan_speed_4));
                    break;
                }
                break;
            case PM_METRIC_GPU_FAN_SPEED_PERCENT:
                switch (qe.arrayIndex)
                {
                case 0:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::max_fan_speed_0));
                    break;
                case 1:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::max_fan_speed_1));
                    break;
                case 2:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::max_fan_speed_2));
                    break;
                case 3:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::max_fan_speed_3));
                    break;
                case 4:
                    pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::max_fan_speed_4));
                    break;
                }
                break;
            case PM_METRIC_GPU_EFFECTIVE_FREQUENCY:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_effective_frequency));
                break;
            case PM_METRIC_GPU_VOLTAGE_REGULATOR_TEMPERATURE:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_voltage_regulator_temperature));
                break;
            case PM_METRIC_GPU_MEM_EFFECTIVE_BANDWIDTH:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_mem_effective_bandwidth));
                break;
            case PM_METRIC_GPU_OVERVOLTAGE_PERCENT:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_overvoltage_percent));
                break;
            case PM_METRIC_GPU_TEMPERATURE_PERCENT:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_temperature_percent));
                break;
            case PM_METRIC_GPU_POWER_PERCENT:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_power_percent));
                break;
            case PM_METRIC_GPU_CARD_POWER:
                pQuery->accumGpuBits.set(static_cast<size_t>(GpuTelemetryCapBits::gpu_card_power));
                break;
            case PM_METRIC_CPU_UTILIZATION:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_utilization));
                break;
            case PM_METRIC_CPU_POWER:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_power));
                break;
            case PM_METRIC_CPU_TEMPERATURE:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_temperature));
                break;
            case PM_METRIC_CPU_FREQUENCY:
                pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_frequency));
                break;
            case PM_METRIC_CPU_CORE_UTILITY:
                //pQuery->accumCpuBits.set(static_cast<size_t>(CpuTelemetryCapBits::cpu_power));
                break;
            default:
                if (metricView.GetType() == PM_METRIC_TYPE_FRAME_EVENT) {
                    pmlog_warn(std::format("ignoring frame event metric [{}] while building dynamic query",
                        metricView.Introspect().GetSymbol())).diag();
                }
                break;
            }

            qe.dataOffset = offset;
            qe.dataSize = GetDataTypeSize(metricView.GetDataTypeInfo().GetPolledType());
            offset += qe.dataSize;
        }

        pQuery->metricOffsetMs = metricOffsetMs;
        pQuery->windowSizeMs = windowSizeMs;
        pQuery->elements = std::vector<PM_QUERY_ELEMENT>{ queryElements.begin(), queryElements.end() };
        pQuery->queryCacheSize = pQuery->elements[std::size(pQuery->elements) - 1].dataOffset + pQuery->elements[std::size(pQuery->elements) - 1].dataSize;
        if (cachedGpuInfoIndex.has_value())
        {
            pQuery->cachedGpuInfoIndex = cachedGpuInfoIndex.value();
        }

        return pQuery.release();
    }

namespace {

struct FakePMTraceSession {
    double mMilliSecondsPerTimestamp = 0.0;

    double TimestampDeltaToMilliSeconds(uint64_t qpcDelta) const
    {
        return mMilliSecondsPerTimestamp * qpcDelta;
    }

    double TimestampDeltaToUnsignedMilliSeconds(uint64_t qpcFrom, uint64_t qpcTo) const
    {
        return qpcFrom == 0 || qpcTo <= qpcFrom ? 0.0 : TimestampDeltaToMilliSeconds(qpcTo - qpcFrom);
    }

    double TimestampDeltaToMilliSeconds(uint64_t qpcFrom, uint64_t qpcTo) const
    {
        return qpcFrom == 0 || qpcTo == 0 || qpcFrom == qpcTo ? 0.0 :
            qpcTo > qpcFrom ? TimestampDeltaToMilliSeconds(qpcTo - qpcFrom) :
                             -TimestampDeltaToMilliSeconds(qpcFrom - qpcTo);
    }
};

// Copied from: PresentMon/PresentMon.hpp
// Metrics computed per-frame.  Duration and Latency metrics are in milliseconds.
struct FrameMetrics {
    double mMsBetweenPresents;
    double mMsInPresentApi;
    double mMsUntilDisplayed;
    double mMsBetweenDisplayChange;
    double mMsUntilRenderComplete;

    uint64_t mCPUStart;
    double mCPUBusy;
    double mCPUWait;
    double mGPULatency;
    double mGPUBusy;
    double mVideoBusy;
    double mGPUWait;
    double mDisplayLatency;
    double mDisplayedTime;
    double mAnimationError;
    double mClickToPhotonLatency;
    double mAllInputPhotonLatency;
    FrameType mFrameType;
    double mInstrumentedDisplayLatency;
    double mPcLatency;
    double mMsBetweenSimStarts;

    double mInstrumentedRenderLatency;
    double mInstrumentedSleep;
    double mInstrumentedGpuLatency;
    double mInstrumentedReadyTimeToDisplayLatency;
};

// Copied from: PresentMon/OutputThread.cpp
void UpdateChain(
    fpsSwapChainData* chain,
    PmNsmPresentEvent const& p)
{
    if (p.FinalState == PresentResult::Presented) {
        if (p.DisplayedCount > 0) {
            if (p.Displayed_FrameType[p.DisplayedCount - 1] == FrameType::NotSet ||
                p.Displayed_FrameType[p.DisplayedCount - 1] == FrameType::Application) {
                // If the chain animation error source has been set to either
                // app provider or PCL latency then set the last displayed simulation start time and the
                // first app simulation start time based on the animation error source type.
                if (chain->mAnimationErrorSource == AnimationErrorSource::AppProvider) {
                    chain->mLastDisplayedSimStartTime = p.AppSimStartTime;
                    chain->mLastDisplayedAppScreenTime = p.Displayed_ScreenTime[p.DisplayedCount - 1];
                } else if (chain->mAnimationErrorSource == AnimationErrorSource::PCLatency) {
                    // In the case of PCLatency only set values if pcl sim start time is not zero.
                    if (p.PclSimStartTime != 0) {
                        chain->mLastDisplayedSimStartTime = p.PclSimStartTime;
                        chain->mLastDisplayedAppScreenTime = p.Displayed_ScreenTime[p.DisplayedCount - 1];
                    }
                } else {
                    // Currently sourcing animation error from CPU start time, however check
                    // to see if we have a valid app provider or PCL sim start time and set the
                    // new animation source and set the first app sim start time
                    if (p.AppSimStartTime != 0) {
                        chain->mAnimationErrorSource = AnimationErrorSource::AppProvider;
                        chain->mLastDisplayedSimStartTime = p.AppSimStartTime;
                        chain->mLastDisplayedAppScreenTime = p.Displayed_ScreenTime[p.DisplayedCount - 1];
                    } else if (p.PclSimStartTime != 0) {
                        chain->mAnimationErrorSource = AnimationErrorSource::PCLatency;
                        chain->mLastDisplayedSimStartTime = p.PclSimStartTime;
                        chain->mLastDisplayedAppScreenTime = p.Displayed_ScreenTime[p.DisplayedCount - 1];
                    } else {
                        if (chain->mLastAppPresentIsValid == true) {
                            chain->mLastDisplayedSimStartTime = chain->mLastAppPresent.PresentStartTime +
                                chain->mLastAppPresent.TimeInPresent;
                        }
                        chain->mLastDisplayedAppScreenTime = p.Displayed_ScreenTime[p.DisplayedCount - 1];
                    }
                }
            }
        }
        // TODO: This used to be p.Displayed_ScreenTime[0]. That seems incorrect.
        uint64_t lastDisplayedScreenTime = p.DisplayedCount == 0 ? 0 : 
                                           p.Displayed_ScreenTime[p.DisplayedCount - 1];

        // IntelPresentMon specifics:
        if (chain->display_count == 0) {
            chain->display_0_screen_time = lastDisplayedScreenTime;
        }
        chain->mLastDisplayedScreenTime = lastDisplayedScreenTime;
        chain->display_count += 1;
    }

    if (p.DisplayedCount > 0) {
        if (p.Displayed_FrameType[p.DisplayedCount - 1] == FrameType::NotSet ||
            p.Displayed_FrameType[p.DisplayedCount - 1] == FrameType::Application) {
            chain->mLastAppPresent = p;
            chain->mLastAppPresentIsValid = true;
        }
    } else {
        // If the displayed count is zero, assuming this is an application
        // present
        chain->mLastAppPresent = p;
        chain->mLastAppPresentIsValid = true;
    }

    // Set chain->mLastSimStartTime to either p->PclSimStartTime or p->AppSimStartTime depending on
    // if either are not zero. If both are zero, do not set.
    if (p.PclSimStartTime != 0) {
        chain->mLastSimStartTime = p.PclSimStartTime;
    } else if (p.AppSimStartTime != 0) {
        chain->mLastSimStartTime = p.AppSimStartTime;
    }

    chain->mLastPresent = p;
    chain->mLastPresentIsValid = true;
    chain->mIncludeFrameData = true;
}

// Copied from: PresentMon/OutputThread.cpp
static void ReportMetricsHelper(
    FakePMTraceSession const& pmSession,
    InputToFsManager& pclI2FsManager,
    fpsSwapChainData* chain,
    PmNsmPresentEvent* p,
    PmNsmPresentEvent const* nextDisplayedPresent)
{
    // Figure out what display index to start processing.
    //
    // The following cases are expected:
    // p.Displayed empty and nextDisplayedPresent == nullptr:       process p as not displayed
    // p.Displayed with size N and nextDisplayedPresent == nullptr: process p.Displayed[0..N-2] as displayed, postponing N-1
    // p.Displayed with size N and nextDisplayedPresent != nullptr: process p.Displayed[N-1]    as displayed
    auto displayCount = p->DisplayedCount;
    bool displayed = p->FinalState == PresentResult::Presented && displayCount > 0;
    size_t displayIndex = displayed && nextDisplayedPresent != nullptr ? displayCount - 1 : 0;

    // Figure out what display index to attribute cpu work, gpu work, animation error, and input
    // latency to. Start looking from the current display index.
    size_t appIndex = std::numeric_limits<size_t>::max();
    if (displayCount > 0) {
        for (size_t i = displayIndex; i < displayCount; ++i) {
            if (p->Displayed_FrameType[i] == FrameType::NotSet ||
                p->Displayed_FrameType[i] == FrameType::Application) {
                appIndex = i;
                break;
            }
        }
    } else {
        // If there are no displayed frames
        appIndex = 0;
    }

    do {
        // PB = PresentStartTime
        // PE = PresentEndTime
        // D  = ScreenTime
        //
        // chain->mLastPresent:    PB--PE----D
        // p:                          |        PB--PE----D
        // ...                         |        |   |     |     PB--PE
        // nextDisplayedPresent:       |        |   |     |             PB--PE----D
        //                             |        |   |     |                       |
        // mCPUStart/mCPUBusy:         |------->|   |     |                       |
        // mCPUWait:                            |-->|     |                       |
        // mDisplayLatency:            |----------------->|                       |
        // mDisplayedTime:                                |---------------------->|

        // Lookup the ScreenTime and next ScreenTime
        uint64_t screenTime = 0;
        uint64_t nextScreenTime = 0;
        if (displayed) {
            screenTime = p->Displayed_ScreenTime[displayIndex];

            if (displayIndex + 1 < displayCount) {
                nextScreenTime = p->Displayed_ScreenTime[displayIndex + 1];
            } else if (nextDisplayedPresent != nullptr) {
                nextScreenTime = nextDisplayedPresent->Displayed_ScreenTime[0];
            } else {
                return;
            }
        }

        double msGPUDuration = 0.0;

        FrameMetrics metrics;
        metrics.mCPUStart = 0;

        // Calculate these metrics for every present
        metrics.mMsBetweenPresents = chain->mLastPresentIsValid == false ? 0 :
            pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastPresent.PresentStartTime, p->PresentStartTime);
        metrics.mMsInPresentApi = pmSession.TimestampDeltaToMilliSeconds(p->TimeInPresent);
        metrics.mMsUntilRenderComplete = pmSession.TimestampDeltaToMilliSeconds(p->PresentStartTime, p->ReadyTime);

        if (chain->mLastAppPresentIsValid == true) {
            if (chain->mLastAppPresent.AppPropagatedPresentStartTime != 0) {
                metrics.mCPUStart = chain->mLastAppPresent.AppPropagatedPresentStartTime + chain->mLastAppPresent.AppPropagatedTimeInPresent;
            }
            else {
                metrics.mCPUStart = chain->mLastAppPresent.PresentStartTime + chain->mLastAppPresent.TimeInPresent;
            }
        } else {
            metrics.mCPUStart = chain->mLastPresentIsValid == false ? 0 : chain->mLastPresent.PresentStartTime + p->TimeInPresent;
        }

        if (displayIndex == appIndex) {
            if (p->AppPropagatedPresentStartTime != 0) {
                msGPUDuration = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->AppPropagatedGPUStartTime, p->AppPropagatedReadyTime);
                metrics.mCPUBusy = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, p->AppPropagatedPresentStartTime);
                metrics.mCPUWait = pmSession.TimestampDeltaToMilliSeconds(p->AppPropagatedTimeInPresent);
                metrics.mGPULatency = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, p->AppPropagatedGPUStartTime);
                metrics.mGPUBusy = pmSession.TimestampDeltaToMilliSeconds(p->AppPropagatedGPUDuration);
                metrics.mVideoBusy = pmSession.TimestampDeltaToMilliSeconds(p->AppPropagatedGPUVideoDuration);
                metrics.mGPUWait = std::max(0.0, msGPUDuration - metrics.mGPUBusy);
            } else {
                msGPUDuration = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->GPUStartTime, p->ReadyTime);
                metrics.mCPUBusy = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, p->PresentStartTime);
                metrics.mCPUWait = pmSession.TimestampDeltaToMilliSeconds(p->TimeInPresent);
                metrics.mGPULatency = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, p->GPUStartTime);
                metrics.mGPUBusy = pmSession.TimestampDeltaToMilliSeconds(p->GPUDuration);
                metrics.mVideoBusy = pmSession.TimestampDeltaToMilliSeconds(p->GPUVideoDuration);
                metrics.mGPUWait = std::max(0.0, msGPUDuration - metrics.mGPUBusy);
            }
            // Need both AppSleepStart and AppSleepEnd to calculate XellSleep
            metrics.mInstrumentedSleep  = (p->AppSleepEndTime == 0 || p->AppSleepStartTime == 0) ? 0 :
                pmSession.TimestampDeltaToUnsignedMilliSeconds(p->AppSleepStartTime, p->AppSleepEndTime);
            // If there isn't a valid sleep end time use the sim start time
            auto instrumentedStartTime  = p->AppSleepEndTime != 0 ? p->AppSleepEndTime : p->AppSimStartTime;
            // If neither the sleep end time or sim start time is valid, there is no
            // way to calculate the Xell Gpu latency
            metrics.mInstrumentedGpuLatency = instrumentedStartTime == 0 ? 0 :
                                      pmSession.TimestampDeltaToUnsignedMilliSeconds(instrumentedStartTime, p->GPUStartTime);
            
            // If we have both a valid pcl sim start time and a valid app sim start time, we use the pcl sim start time.
            if (p->PclSimStartTime != 0) {
                metrics.mMsBetweenSimStarts = pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastSimStartTime, p->PclSimStartTime);
            }
            else if (p->AppSimStartTime != 0) {
                metrics.mMsBetweenSimStarts = pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastSimStartTime, p->AppSimStartTime);
            }
        } else {
            metrics.mCPUBusy                = 0;
            metrics.mCPUWait                = 0;
            metrics.mGPULatency             = 0;
            metrics.mGPUBusy                = 0;
            metrics.mVideoBusy              = 0;
            metrics.mGPUWait                = 0;
            metrics.mInstrumentedSleep      = 0;
            metrics.mInstrumentedGpuLatency = 0;
            metrics.mMsBetweenSimStarts     = 0;
        }

        // If the frame was displayed regardless of how it was produced, calculate the following
        // metrics
        if (displayed) {
            metrics.mDisplayLatency = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, screenTime);
            metrics.mDisplayedTime  = pmSession.TimestampDeltaToUnsignedMilliSeconds(screenTime, nextScreenTime);
            metrics.mMsUntilDisplayed = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->PresentStartTime, screenTime);
            metrics.mMsBetweenDisplayChange = chain->mLastDisplayedScreenTime == 0 ? 0 :
                pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastDisplayedScreenTime, screenTime);

            // If AppRenderSubmitStart is valid calculate the render latency
            metrics.mInstrumentedRenderLatency = p->AppRenderSubmitStartTime == 0 ? 0 :
                pmSession.TimestampDeltaToUnsignedMilliSeconds(p->AppRenderSubmitStartTime, screenTime);
            metrics.mInstrumentedReadyTimeToDisplayLatency = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->ReadyTime, screenTime);
            // If there isn't a valid sleep end time use the sim start time
            auto InstrumentedStartTime = p->AppSleepEndTime != 0 ? p->AppSleepEndTime : p->AppSimStartTime;
            // If neither the sleep end time or sim start time is valid, there is no
            // way to calculate the Xell Gpu latency
            metrics.mInstrumentedDisplayLatency = InstrumentedStartTime == 0 ? 0 :
                pmSession.TimestampDeltaToUnsignedMilliSeconds(InstrumentedStartTime, screenTime);

            metrics.mPcLatency = 0.f;
            // Check to see if we have a valid pc latency sim start time
            if (p->PclSimStartTime != 0) {
                if (p->PclInputPingTime == 0) {
                    if (chain->mAccumulatedInput2FrameStartTime != 0) {
                        // This frame was displayed but we don't have a pc latency input time. However, there is accumulated time
                        // so there is a pending input that will now hit the screen. Add in the time from the last not
                        // displayed pc simulation start to this frame's pc simulation start.
                        chain->mAccumulatedInput2FrameStartTime +=
                            pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedPclSimStart, p->PclSimStartTime);
                        // Add all of the accumlated time to the average input to frame start time.
                        pclI2FsManager.AddI2FsValueForProcess(
                            p->ProcessId,
                            chain->mLastReceivedNotDisplayedPclInputTime,
                            chain->mAccumulatedInput2FrameStartTime);
                        // Reset the tracking variables for when we have a dropped frame with a pc latency input
                        chain->mAccumulatedInput2FrameStartTime = 0.f;
                        chain->mLastReceivedNotDisplayedPclSimStart = 0;
                        chain->mLastReceivedNotDisplayedPclInputTime = 0;
                    }
                } else {
                    pclI2FsManager.AddI2FsValueForProcess(
                        p->ProcessId,
                        p->PclInputPingTime,
                        pmSession.TimestampDeltaToUnsignedMilliSeconds(p->PclInputPingTime, p->PclSimStartTime));
                }
            }
            // If we have a non-zero average input to frame start time and a PC Latency simulation
            // start time calculate the PC Latency
            auto i2Fs = pclI2FsManager.GetI2FsForProcess(p->ProcessId);
            auto simStartTime = p->PclSimStartTime != 0 ? p->PclSimStartTime : chain->mLastSimStartTime;
            if (i2Fs != 0.f && simStartTime != 0) {
                metrics.mPcLatency = i2Fs + pmSession.TimestampDeltaToMilliSeconds(simStartTime, screenTime);
            }
        } else {
            metrics.mDisplayLatency                         = 0;
            metrics.mDisplayedTime                          = 0;
            metrics.mMsUntilDisplayed                       = 0;
            metrics.mMsBetweenDisplayChange                 = 0;
            metrics.mInstrumentedRenderLatency              = 0;
            metrics.mInstrumentedReadyTimeToDisplayLatency  = 0;
            metrics.mInstrumentedDisplayLatency             = 0;
            metrics.mPcLatency                              = 0;
            if (p->PclSimStartTime != 0) {
                if (p->PclInputPingTime != 0) {
                    // This frame was dropped but we have valid pc latency input and simulation start
                    // times. Calculate the initial input to sim start time.
                    chain->mAccumulatedInput2FrameStartTime =
                        pmSession.TimestampDeltaToUnsignedMilliSeconds(p->PclInputPingTime, p->PclSimStartTime);
                    chain->mLastReceivedNotDisplayedPclInputTime = p->PclInputPingTime;
                } else if (chain->mAccumulatedInput2FrameStartTime != 0.f) {
                    // This frame was also dropped and there is no pc latency input time. However, since we have
                    // accumulated time this means we have a pending input that has had multiple dropped frames
                    // and has not yet hit the screen. Calculate the time between the last not displayed sim start and
                    // this sim start and add it to our accumulated total
                    chain->mAccumulatedInput2FrameStartTime +=
                        pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedPclSimStart, p->PclSimStartTime);
                }
                chain->mLastReceivedNotDisplayedPclSimStart = p->PclSimStartTime;
            }
        }

        // The following metrics use both the frame's displayed and origin information.
        metrics.mClickToPhotonLatency   = 0;
        metrics.mAllInputPhotonLatency  = 0;
        metrics.mAnimationError         = 0;

        if (displayIndex == appIndex) {
            if (displayed) {
                // For all input device metrics check to see if there were any previous device input times 
                // that were attached to a dropped frame and if so use the last received times for the
                // metric calculations
                auto updatedInputTime = chain->mLastReceivedNotDisplayedAllInputTime == 0 ? 0 :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedAllInputTime, screenTime);
                metrics.mAllInputPhotonLatency = p->InputTime == 0 ? updatedInputTime :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(p->InputTime, screenTime);

                updatedInputTime = chain->mLastReceivedNotDisplayedMouseClickTime == 0 ? 0 :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedMouseClickTime, screenTime);
                metrics.mClickToPhotonLatency = p->MouseClickTime == 0 ? updatedInputTime :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(p->MouseClickTime, screenTime);

                // Reset all last received device times
                chain->mLastReceivedNotDisplayedAllInputTime = 0;
                chain->mLastReceivedNotDisplayedMouseClickTime = 0;

                // Next calculate the animation error and animation time. First calculate the simulation
                // start time. Simulation start can be either an app provided sim start time via the provider or
                // PCL stats or, if not present,the cpu start.
                uint64_t simStartTime = 0;
                if (chain->mAnimationErrorSource == AnimationErrorSource::PCLatency) {
                    // If the pcl latency is the source of the animation error then use the pcl sim start time.
                    simStartTime = p->PclSimStartTime;
                }
                else if (chain->mAnimationErrorSource == AnimationErrorSource::AppProvider) {
                    // If the app provider is the source of the animation error then use the app sim start time.
                    simStartTime = p->AppSimStartTime;
                }
                else if (chain->mAnimationErrorSource == AnimationErrorSource::CpuStart) {
                    // If the cpu start time is the source of the animation error then use the cpu start time.
                    simStartTime = metrics.mCPUStart;
                }
                
                if (chain->mLastDisplayedSimStartTime != 0) {
                    // If the simulation start time is less than the last displayed simulation start time it means
                    // we are transitioning to app provider events.
                    if (simStartTime > chain->mLastDisplayedSimStartTime) {
                        metrics.mAnimationError = pmSession.TimestampDeltaToMilliSeconds(screenTime - chain->mLastDisplayedScreenTime,
                            simStartTime - chain->mLastDisplayedSimStartTime);
                        chain->mAnimationError.push_back(std::abs(metrics.mAnimationError));
                    }
                }
            }
            else {
                if (p->InputTime != 0) {
                    chain->mLastReceivedNotDisplayedAllInputTime = p->InputTime;
                }
                if (p->MouseClickTime != 0) {
                    chain->mLastReceivedNotDisplayedMouseClickTime = p->MouseClickTime;
                }
            }
        }

        if (p->DisplayedCount == 0) {
            metrics.mFrameType = FrameType::NotSet;
        } else {
            metrics.mFrameType = p->Displayed_FrameType[displayIndex];
        }

        // Push back all Present() information regardless of the source and if
        // the present was displayed
        chain->mMsBetweenPresents.push_back(metrics.mMsBetweenPresents);
        chain->mMsInPresentApi.push_back(metrics.mMsInPresentApi);
        chain->mMsUntilRenderComplete.push_back(metrics.mMsUntilRenderComplete);

        // Push back application based metrics regardless if the present was displayed
        if (displayIndex == appIndex) {
            chain->mCPUBusy               .push_back(metrics.mCPUBusy);
            chain->mCPUWait               .push_back(metrics.mCPUWait);
            chain->mGPULatency            .push_back(metrics.mGPULatency);
            chain->mGPUBusy               .push_back(metrics.mGPUBusy);
            chain->mVideoBusy             .push_back(metrics.mVideoBusy);
            chain->mGPUWait               .push_back(metrics.mGPUWait);
            chain->mInstrumentedSleep     .push_back(metrics.mInstrumentedSleep);
            chain->mInstrumentedGpuLatency.push_back(metrics.mInstrumentedGpuLatency);
        }

        // Push back all Display() information regardless of the source
        if (displayed) {
            chain->mDisplayLatency                       .push_back(metrics.mDisplayLatency);
            chain->mDisplayedTime                        .push_back(metrics.mDisplayedTime);
            chain->mMsUntilDisplayed                     .push_back(metrics.mMsUntilDisplayed);
            chain->mDropped                              .push_back(0.0);
            if (metrics.mMsBetweenDisplayChange != 0) {
                // Only push back the mMsBetweenDisplayChange if it is non-zero. 
                // mMsBetweenDisplayChange will be zero on the first use of the incoming 
                // swap chain parameter.
                chain->mMsBetweenDisplayChange.push_back(metrics.mMsBetweenDisplayChange);
            }
        } else {
            chain->mDropped       .push_back(1.0);
        }

        if (displayed) {
            if (chain->mAppDisplayedTime.empty() || displayIndex == appIndex) {
                chain->mAppDisplayedTime.push_back(metrics.mDisplayedTime);
            }
            else {
                chain->mAppDisplayedTime.back() += metrics.mDisplayedTime;
            }
        } else {
            chain->mDropped       .push_back(1.0);
        }

        if (displayed && displayIndex == appIndex) {
            if (metrics.mAllInputPhotonLatency != 0) {
                chain->mAllInputToPhotonLatency.push_back(metrics.mAllInputPhotonLatency);
            }
            if (metrics.mClickToPhotonLatency != 0) {
                chain->mClickToPhotonLatency.push_back(metrics.mClickToPhotonLatency);
            }
            if (metrics.mInstrumentedRenderLatency != 0) {
                chain->mInstrumentedRenderLatency.push_back(metrics.mInstrumentedRenderLatency);
            }
            if (metrics.mInstrumentedDisplayLatency != 0) {
                chain->mInstrumentedDisplayLatency.push_back(metrics.mInstrumentedDisplayLatency);
            }
            if (metrics.mInstrumentedReadyTimeToDisplayLatency != 0) {
                chain->mInstrumentedReadyTimeToDisplayLatency.push_back(metrics.mInstrumentedReadyTimeToDisplayLatency);
            }
            if (metrics.mPcLatency != 0) {
                chain->mMsPcLatency.push_back(metrics.mPcLatency);
            }
        }

        displayIndex += 1;
    } while (displayIndex < displayCount);

    UpdateChain(chain, *p);
}

static void ReportMetrics(
    FakePMTraceSession const& pmSession,
    InputToFsManager& pclI2FsManager,
    fpsSwapChainData* chain,
    PmNsmPresentEvent* p)
{
    // For the chain's first present, we just initialize mLastPresent to give a baseline for the
    // first frame.
    if (!chain->mLastPresentIsValid) {
        UpdateChain(chain, *p);
        return;
    }

    // If chain->mPendingPresents is non-empty, then it contains a displayed present followed by
    // some number of discarded presents.  If the displayed present has multiple Displayed entries,
    // all but the last have already been handled.
    //
    // If p is displayed, then we can complete all pending presents, and complete any flips in p
    // except for the last one, but then we have to add p to the pending list to wait for the next
    // displayed frame.
    //
    // If p is not displayed, we can process it now unless it is blocked behind an earlier present
    // waiting for the next displayed one, in which case we need to add it to the pending list as
    // well.
    if (p->FinalState == PresentResult::Presented) {
        for (auto& p2 : chain->mPendingPresents) {
            ReportMetricsHelper(pmSession, pclI2FsManager, chain, &p2, p);
        }
        ReportMetricsHelper(pmSession, pclI2FsManager, chain, p, nullptr);
        chain->mPendingPresents.clear();
        chain->mPendingPresents.push_back(*p);
    } else {
        if (chain->mPendingPresents.empty()) {
            ReportMetricsHelper(pmSession, pclI2FsManager, chain, p, nullptr);
        } else {
            chain->mPendingPresents.push_back(*p);
        }
    }
}

}

    void ConcreteMiddleware::PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains)
    {
        std::unordered_map<uint64_t, fpsSwapChainData> swapChainData;
        std::unordered_map<PM_METRIC, MetricInfo> metricInfo;
        bool allMetricsCalculated = false;
        bool fpsMetricsCalculated = false;

        if (*numSwapChains == 0) {
            return;
        }

        if (pQuery->cachedGpuInfoIndex.has_value())
        {
            if (pQuery->cachedGpuInfoIndex.value() != currentGpuInfoIndex)
            {
                // Set the adapter id 
                SetActiveGraphicsAdapter(cachedGpuInfo[pQuery->cachedGpuInfoIndex.value()].deviceId);
                // Set the current index to the queried one
                currentGpuInfoIndex = pQuery->cachedGpuInfoIndex.value();
            }
        }

        auto iter = presentMonStreamClients.find(processId);
        if (iter == presentMonStreamClients.end()) {
            return;
        }

        // Get the named shared memory associated with the stream client
        StreamClient* client = iter->second.get();
        auto nsm_view = client->GetNamedSharedMemView();
        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            // TODO: Do we want to inform the client if the server has destroyed the
            // named shared memory?
            // Server destroyed the named shared memory due to process exit. Destroy the
            // mapped view from client side.
            //StopStreamProcess(process_id);
            //return PM_STATUS::PM_STATUS_PROCESS_NOT_EXIST;
            return;
        }

        uint64_t index = 0;
        double adjusted_window_size_in_ms = pQuery->windowSizeMs;
        auto result = queryFrameDataDeltas.emplace(std::pair(std::pair(pQuery, processId), uint64_t()));
        auto queryToFrameDataDelta = &result.first->second;
        
        PmNsmFrameData* frame_data = GetFrameDataStart(client, index, SecondsDeltaToQpc(pQuery->metricOffsetMs/1000., client->GetQpcFrequency()), *queryToFrameDataDelta, adjusted_window_size_in_ms);
        if (frame_data == nullptr) {
            pmlog_warn("Filling cached data in dynamic metric poll due to nullptr from GetFrameDataStart").diag();
            CopyMetricCacheToBlob(pQuery, processId, pBlob);
            return;
        }

        // Calculate the end qpc based on the current frame's qpc and
        // requested window size coverted to a qpc
        // then loop from the most recent frame data until we either run out of data or
        // we meet the window size requirements sent in by the client
        uint64_t end_qpc =
            frame_data->present_event.PresentStartTime -
            SecondsDeltaToQpc(adjusted_window_size_in_ms/1000., client->GetQpcFrequency());

        std::vector<PmNsmFrameData*> frames;
        while (frame_data->present_event.PresentStartTime > end_qpc) {
            frames.push_back(frame_data);

            // Get the index of the next frame
            if (DecrementIndex(nsm_view, index) == false) {
                // We have run out of data to process, time to go
                break;
            }
            frame_data = client->ReadFrameByIdx(index, true);
            if (frame_data == nullptr) {
                break;
            }
        }

        FakePMTraceSession pmSession;
        pmSession.mMilliSecondsPerTimestamp = 1000.0 / client->GetQpcFrequency().QuadPart;

        for (const auto& frame_data : frames | std::views::reverse) {
            if (pQuery->accumFpsData)
            {
                auto result = swapChainData.emplace(
                    frame_data->present_event.SwapChainAddress, fpsSwapChainData());
                auto swap_chain = &result.first->second;

                auto presentEvent = &frame_data->present_event;
                auto chain = swap_chain;

                // The following code block copied from: PresentMon/OutputThread.cpp
                if (chain->mLastPresentIsValid) {
                    ReportMetrics(pmSession, mPclI2FsManager, chain, presentEvent);
                } else {
                    pmon::mid::UpdateChain(chain, *presentEvent);
                }
                // end
            }

            for (size_t i = 0; i < pQuery->accumGpuBits.size(); ++i) {
                if (pQuery->accumGpuBits[i])
                {
                    GetGpuMetricData(i, frame_data->power_telemetry, metricInfo);
                }
            }

            for (size_t i = 0; i < pQuery->accumCpuBits.size(); ++i) {
                if (pQuery->accumCpuBits[i])
                {
                    GetCpuMetricData(i, frame_data->cpu_telemetry, metricInfo);
                }
            }
        }

        CalculateMetrics(pQuery, processId, pBlob, numSwapChains, client->GetQpcFrequency(), swapChainData, metricInfo);
    }

    std::optional<size_t> ConcreteMiddleware::GetCachedGpuInfoIndex(uint32_t deviceId)
    {
        for (std::size_t i = 0; i < cachedGpuInfo.size(); ++i)
        {
            if (cachedGpuInfo[i].deviceId == deviceId)
            {
                if (cachedGpuInfo[i].adapterId.has_value())
                    return cachedGpuInfo[i].adapterId.value();
                else {
                    return std::nullopt;
                }
            }
        }

        return std::nullopt;
    }

    void ConcreteMiddleware::CopyStaticMetricData(PM_METRIC metric, uint32_t deviceId, uint8_t* pBlob, uint64_t blobOffset, size_t sizeInBytes)
    {
        switch (metric)
        {
        case PM_METRIC_CPU_NAME:
        {
            strncpy_s(reinterpret_cast<char*>(&pBlob[blobOffset]), sizeInBytes, cachedCpuInfo[0].deviceName.c_str(), _TRUNCATE);
        }
            break;
        case PM_METRIC_CPU_VENDOR:
        {
            auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[blobOffset]);
            output = cachedCpuInfo[0].deviceVendor;
        }
            break;
        case PM_METRIC_CPU_POWER_LIMIT:
        {
            auto& output = reinterpret_cast<double&>(pBlob[blobOffset]);
            output = cachedCpuInfo[0].cpuPowerLimit.has_value() ? cachedCpuInfo[0].cpuPowerLimit.value() : 0.;
        }
            break;
        case PM_METRIC_GPU_NAME:
        {
            auto index = GetCachedGpuInfoIndex(deviceId);
            if (index.has_value())
            {
                strncpy_s(reinterpret_cast<char*>(&pBlob[blobOffset]), sizeInBytes, cachedGpuInfo[index.value()].deviceName.c_str(), _TRUNCATE);
            }
        }
            break;
        case PM_METRIC_GPU_VENDOR:
        {
            auto index = GetCachedGpuInfoIndex(deviceId);
            auto& output = reinterpret_cast<PM_DEVICE_VENDOR&>(pBlob[blobOffset]);
            output = index.has_value() ? cachedGpuInfo[index.value()].deviceVendor : PM_DEVICE_VENDOR_UNKNOWN;
        }
            break;
        case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
        {
            auto& output = reinterpret_cast<double&>(pBlob[blobOffset]);
            auto index = GetCachedGpuInfoIndex(deviceId);
            if (index.has_value())
            {
                output = cachedGpuInfo[index.value()].gpuMemoryMaxBandwidth.has_value() ? 
                    cachedGpuInfo[index.value()].gpuMemoryMaxBandwidth.value() : 0.;
            }
            else
            {
                output = 0.;
            }
        }
            break;
        case PM_METRIC_GPU_MEM_SIZE:
        {
            auto& output = reinterpret_cast<double&>(pBlob[blobOffset]);
            auto index = GetCachedGpuInfoIndex(deviceId);
            if (index.has_value())
            {
                output = cachedGpuInfo[index.value()].gpuMemorySize.has_value() ?
                    static_cast<double>(cachedGpuInfo[index.value()].gpuMemorySize.value()) : 0.;
            }
            else
            {
                output = 0.;
            }
        }
            break;
        case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
        {
            auto& output = reinterpret_cast<double&>(pBlob[blobOffset]);
            auto index = GetCachedGpuInfoIndex(deviceId);
            if (index.has_value())
            {
                output = cachedGpuInfo[index.value()].gpuSustainedPowerLimit.has_value() ?
                    cachedGpuInfo[index.value()].gpuSustainedPowerLimit.value() : 0.f;
            }
            else
            {
                output = 0.f;
            }
        }
            break;
        default:
            break;
        }
        return;
    }

    void ConcreteMiddleware::PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob)
    {
        auto& ispec = GetIntrospectionRoot();
        auto metricView = ispec.FindMetric(element.metric);
        if (metricView.GetType() != int(PM_METRIC_TYPE_STATIC)) {
            pmlog_error(std::format("dynamic metric [{}] in static query poll", metricView.Introspect().GetSymbol())).diag();
            throw Except<util::Exception>("dynamic metric in static query poll");
        }

        auto elementSize = GetDataTypeSize(metricView.GetDataTypeInfo().GetPolledType());

        CopyStaticMetricData(element.metric, element.deviceId, pBlob, 0, elementSize);

        return;
    }

    PM_FRAME_QUERY* mid::ConcreteMiddleware::RegisterFrameEventQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t& blobSize)
    {
        const auto pQuery = new PM_FRAME_QUERY{ queryElements };
        blobSize = (uint32_t)pQuery->GetBlobSize();
        return pQuery;
    }

    void mid::ConcreteMiddleware::FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery)
    {
        delete const_cast<PM_FRAME_QUERY*>(pQuery);
    }

    void mid::ConcreteMiddleware::ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames)
    {
        PM_STATUS status = PM_STATUS::PM_STATUS_SUCCESS;

        const auto frames_to_copy = numFrames;
        // We have saved off the number of frames to copy, now set
        // to zero in case we error out along the way BEFORE we
        // copy frames into the buffer. If a successful copy occurs
        // we'll set to actual number copied.
        uint32_t frames_copied = 0;
        numFrames = 0;

        StreamClient* pShmClient = nullptr;
        try {
            pShmClient = presentMonStreamClients.at(processId).get();
        }
        catch (...) {
            LOG(INFO)
                << "Stream client for process " << processId
                << " doesn't exist. Please call pmStartStream to initialize the "
                "client.";
            pmlog_error("Stream client for process {} doesn't exist. Please call pmStartStream to initialize the client.").diag();
            throw Except<util::Exception>(std::format("Failed to find stream for pid {} in ConsumeFrameEvents", processId));
        }

        const auto nsm_view = pShmClient->GetNamedSharedMemView();
        const auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            StopStreaming(processId);
            pmlog_dbg("Process death detected while consuming frame events").diag();
            throw Except<ipc::PmStatusError>(PM_STATUS_INVALID_PID, "Process died cannot consume frame events");
        }

        const auto last_frame_idx = pShmClient->GetLatestFrameIndex();
        if (last_frame_idx == UINT_MAX) {
            // There are no frames available, no error frames copied = 0
            return;
        }

        // make sure active device is the one referenced in this query
        if (auto devId = pQuery->GetReferencedDevice()) {
            SetActiveGraphicsAdapter(*devId);
        }

        FrameTimingData currentFrameTimingData{};
        auto iter = frameTimingData.find(processId);
        if (iter != frameTimingData.end()) {
            currentFrameTimingData = iter->second;
        }

        FakePMTraceSession pmSession;
        pmSession.mMilliSecondsPerTimestamp = 1000.0 / pShmClient->GetQpcFrequency().QuadPart;

        // context transmits various data that applies to each gather command in the query
        PM_FRAME_QUERY::Context ctx{ 
            nsm_hdr->start_qpc,
            pShmClient->GetQpcFrequency().QuadPart,
            currentFrameTimingData };

        while (frames_copied < frames_to_copy) {
            const PmNsmFrameData* pCurrentFrameData = nullptr;
            const PmNsmFrameData* pNextFrameData = nullptr;
            const PmNsmFrameData* pFrameDataOfLastPresented = nullptr;
            const PmNsmFrameData* pFrameDataOfLastAppPresented = nullptr;
            const PmNsmFrameData* pFrameDataOfNextDisplayed = nullptr;
            const PmNsmFrameData* pFrameDataOfLastDisplayed = nullptr;
            const PmNsmFrameData* pFrameDataOfLastAppDisplayed = nullptr;
            const PmNsmFrameData* pFrameDataOfPreviousAppFrameOfLastAppDisplayed = nullptr;
            const auto status = pShmClient->ConsumePtrToNextNsmFrameData(&pCurrentFrameData, &pNextFrameData,
                &pFrameDataOfNextDisplayed, &pFrameDataOfLastPresented, &pFrameDataOfLastAppPresented,
                &pFrameDataOfLastDisplayed, &pFrameDataOfLastAppDisplayed, &pFrameDataOfPreviousAppFrameOfLastAppDisplayed);
            if (status != PM_STATUS::PM_STATUS_SUCCESS) {
                pmlog_error("Error while trying to get frame data from shared memory").diag();
                throw Except<util::Exception>("Error while trying to get frame data from shared memory");
            }
            if (!pCurrentFrameData) {
                break;
            }
            if (pFrameDataOfLastPresented && pFrameDataOfLastAppPresented && pFrameDataOfNextDisplayed) {
                ctx.UpdateSourceData(pCurrentFrameData,
                    pFrameDataOfNextDisplayed,
                    pFrameDataOfLastPresented,
                    pFrameDataOfLastAppPresented,
                    pFrameDataOfLastDisplayed,
                    pFrameDataOfLastAppDisplayed,
                    pFrameDataOfPreviousAppFrameOfLastAppDisplayed);

                if (ctx.dropped && ctx.pSourceFrameData->present_event.DisplayedCount == 0) {
                        pQuery->GatherToBlob(ctx, pBlob);
                        pBlob += pQuery->GetBlobSize();
                        frames_copied++;
                } else {
                    while (ctx.sourceFrameDisplayIndex < ctx.pSourceFrameData->present_event.DisplayedCount) {
                        if (ctx.pSourceFrameData->present_event.PclSimStartTime != 0) {
                            // If we are calculating PC Latency then we need to update the input to frame start
                            // time.
                            if (ctx.pSourceFrameData->present_event.PclInputPingTime == 0) {
                                if (ctx.mAccumulatedInput2FrameStartTime != 0) {
                                    // This frame was displayed but we don't have a pc latency input time. However, there is accumulated time
                                    // so there is a pending input that will now hit the screen. Add in the time from the last not
                                    // displayed pc simulation start to this frame's pc simulation start.
                                    ctx.mAccumulatedInput2FrameStartTime +=
                                        pmSession.TimestampDeltaToUnsignedMilliSeconds(
                                            ctx.mLastReceivedNotDisplayedPclSimStart,
                                            ctx.pSourceFrameData->present_event.PclSimStartTime);
                                    // Add all of the accumlated time to the average input to frame start time.
                                    mPclI2FsManager.AddI2FsValueForProcess(
                                        ctx.pSourceFrameData->present_event.ProcessId,
                                        ctx.mLastReceivedNotDisplayedPclInputTime,
                                        ctx.mAccumulatedInput2FrameStartTime);
                                    // Reset the tracking variables for when we have a dropped frame with a pc latency input
                                    ctx.mAccumulatedInput2FrameStartTime = 0.f;
                                    ctx.mLastReceivedNotDisplayedPclSimStart = 0;
                                    ctx.mLastReceivedNotDisplayedPclInputTime = 0;
                                }
                            } else {
                                mPclI2FsManager.AddI2FsValueForProcess(
                                    ctx.pSourceFrameData->present_event.ProcessId,
                                    ctx.pSourceFrameData->present_event.PclInputPingTime,
                                    pmSession.TimestampDeltaToUnsignedMilliSeconds(
                                        ctx.pSourceFrameData->present_event.PclInputPingTime,
                                        ctx.pSourceFrameData->present_event.PclSimStartTime));
                            }
                        }
                        ctx.avgInput2Fs = mPclI2FsManager.GetI2FsForProcess(ctx.pSourceFrameData->present_event.ProcessId);
                        pQuery->GatherToBlob(ctx, pBlob);
                        pBlob += pQuery->GetBlobSize();
                        frames_copied++;
                        ctx.sourceFrameDisplayIndex++;
                    }
                }

            }
            // Check to see if the next frame produces more frames than we can store in the
            // the blob.
            if (frames_copied + pNextFrameData->present_event.DisplayedCount >= frames_to_copy) {
                break;
            }
        }
        // Set to the actual number of frames copied
        numFrames = frames_copied;
        // Trim off any old flip delay data that resides in the FrameTimingData::flipDelayDataMap map
        // that is older than the last displayed frame id.
        for (auto it = ctx.frameTimingData.flipDelayDataMap.begin(); it != ctx.frameTimingData.flipDelayDataMap.end();) {
            if (it->first < ctx.frameTimingData.lastDisplayedFrameId) {
                it = ctx.frameTimingData.flipDelayDataMap.erase(it); // Erase and move to the next element
            } else {
                ++it; // Move to the next element
            }
        }
        frameTimingData[processId] = ctx.frameTimingData;
    }

    void ConcreteMiddleware::StopPlayback()
    {
        pActionClient->DispatchSync(StopPlayback::Params{});
    }

    void ConcreteMiddleware::CalculateFpsMetric(fpsSwapChainData& swapChain, const PM_QUERY_ELEMENT& element, uint8_t* pBlob, LARGE_INTEGER qpcFrequency)
    {
        auto& output = reinterpret_cast<double&>(pBlob[element.dataOffset]);

        switch (element.metric)
        {
        case PM_METRIC_APPLICATION:
            strncpy_s(reinterpret_cast<char*>(&pBlob[element.dataOffset]), 260, swapChain.mLastPresent.application, _TRUNCATE);
            break;
        case PM_METRIC_PRESENT_MODE:
            reinterpret_cast<PM_PRESENT_MODE&>(pBlob[element.dataOffset]) = (PM_PRESENT_MODE)swapChain.mLastPresent.PresentMode;
            break;
        case PM_METRIC_PRESENT_RUNTIME:
            reinterpret_cast<PM_GRAPHICS_RUNTIME&>(pBlob[element.dataOffset]) = (PM_GRAPHICS_RUNTIME)swapChain.mLastPresent.Runtime;
            break;
        case PM_METRIC_PRESENT_FLAGS:
            reinterpret_cast<uint32_t&>(pBlob[element.dataOffset]) = swapChain.mLastPresent.PresentFlags;
            break;
        case PM_METRIC_SYNC_INTERVAL:
            reinterpret_cast<uint32_t&>(pBlob[element.dataOffset]) = swapChain.mLastPresent.SyncInterval;
            break;
        case PM_METRIC_ALLOWS_TEARING:
            reinterpret_cast<bool&>(pBlob[element.dataOffset]) = swapChain.mLastPresent.SupportsTearing;
            break;
        case PM_METRIC_FRAME_TYPE:
            reinterpret_cast<PM_FRAME_TYPE&>(pBlob[element.dataOffset]) = (PM_FRAME_TYPE)(swapChain.mLastPresent.DisplayedCount == 0 ? FrameType::NotSet : swapChain.mLastPresent.Displayed_FrameType[0]);
            break;
        case PM_METRIC_CPU_BUSY:
            output = CalculateStatistic(swapChain.mCPUBusy, element.stat);
            break;
        case PM_METRIC_CPU_WAIT:
            output = CalculateStatistic(swapChain.mCPUWait, element.stat);
            break;
        case PM_METRIC_CPU_FRAME_TIME:
        {
            std::vector<double> frame_times(swapChain.mCPUBusy.size());
            for (size_t i = 0; i < swapChain.mCPUBusy.size(); ++i) {
                frame_times[i] = swapChain.mCPUBusy[i] + swapChain.mCPUWait[i];
            }
            output = CalculateStatistic(frame_times, element.stat);
            break;
        }
        case PM_METRIC_GPU_LATENCY:
            output = CalculateStatistic(swapChain.mGPULatency, element.stat);
            break;
        case PM_METRIC_GPU_BUSY:
            output = CalculateStatistic(swapChain.mGPUBusy, element.stat);
            break;
        case PM_METRIC_GPU_WAIT:
            output = CalculateStatistic(swapChain.mGPUWait, element.stat);
            break;
        case PM_METRIC_GPU_TIME:
        {
            std::vector<double> gpu_duration(swapChain.mGPUBusy.size());
            for (size_t i = 0; i < swapChain.mGPUBusy.size(); ++i) {
                gpu_duration[i] = swapChain.mGPUBusy[i] + swapChain.mGPUWait[i];
            }
            output = CalculateStatistic(gpu_duration, element.stat);
            break;
        }
        case PM_METRIC_DISPLAY_LATENCY:
            output = CalculateStatistic(swapChain.mDisplayLatency, element.stat);
            break;
        case PM_METRIC_DISPLAYED_TIME:
            output = CalculateStatistic(swapChain.mDisplayedTime, element.stat);
            break;
        case PM_METRIC_ANIMATION_ERROR:
            output = CalculateStatistic(swapChain.mAnimationError, element.stat);
            break;
        case PM_METRIC_PRESENTED_FPS:
        {
            output = CalculateStatistic(swapChain.mMsBetweenPresents, element.stat, true);
            output = output == 0 ? 0 : 1000.0 / output;
            break;
        }
        case PM_METRIC_APPLICATION_FPS:
        {
            std::vector<double> frame_times(swapChain.mCPUBusy.size());
            for (size_t i = 0; i < swapChain.mCPUBusy.size(); ++i) {
                frame_times[i] = swapChain.mCPUBusy[i] + swapChain.mCPUWait[i];
            }
            output = CalculateStatistic(frame_times, element.stat);
            output = output == 0 ? 0 : 1000.0 / output;
            break;
        }
        case PM_METRIC_DISPLAYED_FPS:
        {
            output = CalculateStatistic(swapChain.mMsBetweenDisplayChange, element.stat, true);
            output = output == 0 ? 0 : 1000.0 / output;
            break;
        }
        case PM_METRIC_DROPPED_FRAMES:
            output = CalculateStatistic(swapChain.mDropped, element.stat);
            break;
        case PM_METRIC_CLICK_TO_PHOTON_LATENCY:
            output = CalculateStatistic(swapChain.mClickToPhotonLatency, element.stat);
            break;
        case PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY:
            output = CalculateStatistic(swapChain.mAllInputToPhotonLatency, element.stat);
            break;
        case PM_METRIC_INSTRUMENTED_LATENCY:
            output = CalculateStatistic(swapChain.mInstrumentedDisplayLatency, element.stat);
            break;
        case PM_METRIC_BETWEEN_PRESENTS:
            output = CalculateStatistic(swapChain.mMsBetweenPresents, element.stat);
            break;
        case PM_METRIC_IN_PRESENT_API:
            output = CalculateStatistic(swapChain.mMsInPresentApi, element.stat);
            break;
        case PM_METRIC_UNTIL_DISPLAYED:
            output = CalculateStatistic(swapChain.mMsUntilDisplayed, element.stat);
            break;
        case PM_METRIC_BETWEEN_DISPLAY_CHANGE:
            output = CalculateStatistic(swapChain.mMsBetweenDisplayChange, element.stat);
            break;
        case PM_METRIC_RENDER_PRESENT_LATENCY:
            output = CalculateStatistic(swapChain.mMsUntilRenderComplete, element.stat);
            break;
        case PM_METRIC_BETWEEN_SIMULATION_START:
            output = CalculateStatistic(swapChain.mMsBetweenSimStarts, element.stat);
            break;
        case PM_METRIC_PC_LATENCY:
            output = CalculateStatistic(swapChain.mMsPcLatency, element.stat);
            break;
        case PM_METRIC_PRESENTED_FRAME_TIME:
            output = CalculateStatistic(swapChain.mMsBetweenPresents, element.stat);
            break;
        case PM_METRIC_DISPLAYED_FRAME_TIME:
            output = CalculateStatistic(swapChain.mMsBetweenDisplayChange, element.stat);
            break;
        default:
            output = 0.;
            break;
        }
    }

    void ConcreteMiddleware::CalculateGpuCpuMetric(std::unordered_map<PM_METRIC, MetricInfo>& metricInfo, const PM_QUERY_ELEMENT& element, uint8_t* pBlob)
    {
        auto& output = reinterpret_cast<double&>(pBlob[element.dataOffset]);
        output = 0.;

        auto it = metricInfo.find(element.metric);
        if (it != metricInfo.end())
        {
            MetricInfo& mi = it->second;
            auto it2 = mi.data.find(element.arrayIndex);
            if (it2 != mi.data.end())
            {
                output = CalculateStatistic(it2->second, element.stat);
            }
        }
        return;
    }
    
    double ConcreteMiddleware::CalculateStatistic(std::vector<double>& inData, PM_STAT stat, bool invert) const
    {
        if (inData.size() == 1) {
            return inData[0];
        }

        if (inData.size() >= 1) {
            switch (stat) {
            case PM_STAT_NONE:
                break;
            case PM_STAT_AVG:
            {
                double sum = 0.0;
                for (auto element : inData) {
                    sum += element;
                }
                return sum / inData.size();
            }
            case PM_STAT_PERCENTILE_99: return CalculatePercentile(inData, 0.99, invert);
            case PM_STAT_PERCENTILE_95: return CalculatePercentile(inData, 0.95, invert);
            case PM_STAT_PERCENTILE_90: return CalculatePercentile(inData, 0.90, invert);
            case PM_STAT_PERCENTILE_01: return CalculatePercentile(inData, 0.01, invert);
            case PM_STAT_PERCENTILE_05: return CalculatePercentile(inData, 0.05, invert);
            case PM_STAT_PERCENTILE_10: return CalculatePercentile(inData, 0.10, invert);
            case PM_STAT_MAX:
            {
                double max = inData[0];
                for (size_t i = 1; i < inData.size(); ++i) {
                    if (invert) {
                        max = std::min(max, inData[i]);
                    } else {
                        max = std::max(max, inData[i]);
                    }
                }
                return max;
            }
            case PM_STAT_MIN:
            {
                double min = inData[0];
                for (size_t i = 1; i < inData.size(); ++i) {
                    if (invert) {
                        min = std::max(min, inData[i]);
                    }
                    else {
                        min = std::min(min, inData[i]);
                    }
                }
                return min;
            }
            case PM_STAT_MID_POINT:
            {
                size_t middle_index = inData.size() / 2;
                return inData[middle_index];
            }
            case PM_STAT_MID_LERP:
                // TODO: Not yet implemented
                break;
            case PM_STAT_NEWEST_POINT:
                // TODO: Not yet implemented
                break;
            case PM_STAT_OLDEST_POINT:
                // TODO: Not yet implemented
                break;
            case PM_STAT_COUNT:
                // TODO: Not yet implemented
                break;
            case PM_STAT_NON_ZERO_AVG:
            {
                double sum = 0.0;
                size_t num = 0;
                for (auto element : inData) {
                    sum += element;
                    num += element == 0.0 ? 0 : 1;
                }
                return num == 0 ? 0.0 : sum / num;
            }
            }
        }

        return 0.0;
    }

    // Calculate percentile using linear interpolation between the closet ranks
    double ConcreteMiddleware::CalculatePercentile(std::vector<double>& inData, double percentile, bool invert) const
    {
        if (invert) {
            percentile = 1.0 - percentile;
        }
        percentile = std::min(std::max(percentile, 0.), 1.);

        double integral_part_as_double;
        double fractpart =
            modf(percentile * static_cast<double>(inData.size()),
                &integral_part_as_double);

        uint32_t idx = static_cast<uint32_t>(integral_part_as_double);
        if (idx >= inData.size() - 1) {
            return CalculateStatistic(inData, PM_STAT_MAX);
        }

        std::sort(inData.begin(), inData.end());
        return inData[idx] + (fractpart * (inData[idx + 1] - inData[idx]));
    }

    PmNsmFrameData* ConcreteMiddleware::GetFrameDataStart(StreamClient* client, uint64_t& index, uint64_t queryMetricsDataOffset, uint64_t& queryFrameDataDelta, double& window_sample_size_in_ms)
    {

        PmNsmFrameData* frame_data = nullptr;
        index = 0;
        if (client == nullptr) {
            return nullptr;
        }

        auto nsm_view = client->GetNamedSharedMemView();
        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            return nullptr;
        }

        index = client->GetLatestFrameIndex();
        frame_data = client->ReadFrameByIdx(index, true);
        if (frame_data == nullptr) {
            index = 0;
            return nullptr;
        }

        if (queryMetricsDataOffset == 0) {
            // Client has not specified a metric offset. Return back the most
            // most recent frame data
            return frame_data;
        }

        LARGE_INTEGER client_qpc = {};
        QueryPerformanceCounter(&client_qpc);
        uint64_t adjusted_qpc = GetAdjustedQpc(
            client_qpc.QuadPart, frame_data->present_event.PresentStartTime,
            queryMetricsDataOffset, client->GetQpcFrequency(), queryFrameDataDelta);

        if (adjusted_qpc > frame_data->present_event.PresentStartTime) {
            // Need to adjust the size of the window sample size
            double ms_adjustment =
                QpcDeltaToMs(adjusted_qpc - frame_data->present_event.PresentStartTime,
                    client->GetQpcFrequency());
            window_sample_size_in_ms = window_sample_size_in_ms - ms_adjustment;
            if (window_sample_size_in_ms <= 0.0) {
                return nullptr;
            }
            pmlog_dbg("Adjusting dynamic stats window due to possible excursion").pmwatch(ms_adjustment);
        }
        else {
            // Find the frame with the appropriate time based on the adjusted
            // qpc
            for (;;) {

                if (DecrementIndex(nsm_view, index) == false) {
                    // Increment index to match up with the frame_data read below
                    index++;
                    break;
                }
                frame_data = client->ReadFrameByIdx(index, true);
                if (frame_data == nullptr) {
                    return nullptr;
                }
                if (adjusted_qpc >= frame_data->present_event.PresentStartTime) {
                    break;
                }
            }
        }

        return frame_data;
    }

    uint64_t ConcreteMiddleware::GetAdjustedQpc(uint64_t current_qpc, uint64_t frame_data_qpc, uint64_t queryMetricsOffset, LARGE_INTEGER frequency, uint64_t& queryFrameDataDelta) {
        // Calculate how far behind the frame data qpc is compared
        // to the client qpc
        uint64_t current_qpc_delta = current_qpc - frame_data_qpc;
        if (queryFrameDataDelta == 0) {
            queryFrameDataDelta = current_qpc_delta;
        }
        else {
            if (_abs64(queryFrameDataDelta - current_qpc_delta) >
                kClientFrameDeltaQPCThreshold) {
                queryFrameDataDelta = current_qpc_delta;
            }
        }

        // Add in the client set metric offset in qpc ticks
        return current_qpc -
            (queryFrameDataDelta + queryMetricsOffset);
    }

    bool ConcreteMiddleware::DecrementIndex(NamedSharedMem* nsm_view, uint64_t& index) {

        if (nsm_view == nullptr) {
            return false;
        }

        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            return false;
        }

        uint64_t current_max_entries =
            (nsm_view->IsFull()) ? nsm_hdr->max_entries - 1 : nsm_hdr->tail_idx;
        index = (index == 0) ? current_max_entries : index - 1;
        if (index == nsm_hdr->head_idx) {
            return false;
        }

        return true;
    }

    bool ConcreteMiddleware::GetGpuMetricData(size_t telemetry_item_bit, PresentMonPowerTelemetryInfo& power_telemetry_info, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo)
    {
        bool validGpuMetric = true;
        GpuTelemetryCapBits bit =
            static_cast<GpuTelemetryCapBits>(telemetry_item_bit);
        switch (bit) {
        case GpuTelemetryCapBits::time_stamp:
            // This is a valid telemetry cap bit but we do not produce metrics for
            // it.
            validGpuMetric = false;
            break;
        case GpuTelemetryCapBits::gpu_power:
            metricInfo[PM_METRIC_GPU_POWER].data[0].emplace_back(power_telemetry_info.gpu_power_w);
            break;
        case GpuTelemetryCapBits::gpu_voltage:
            metricInfo[PM_METRIC_GPU_VOLTAGE].data[0].emplace_back(power_telemetry_info.gpu_voltage_v);
            break;
        case GpuTelemetryCapBits::gpu_frequency:
            metricInfo[PM_METRIC_GPU_FREQUENCY].data[0].emplace_back(power_telemetry_info.gpu_frequency_mhz);
            break;
        case GpuTelemetryCapBits::gpu_temperature:
            metricInfo[PM_METRIC_GPU_TEMPERATURE].data[0].emplace_back(power_telemetry_info.gpu_temperature_c);
            break;
        case GpuTelemetryCapBits::gpu_utilization:
            metricInfo[PM_METRIC_GPU_UTILIZATION].data[0].emplace_back(power_telemetry_info.gpu_utilization);
            break;
        case GpuTelemetryCapBits::gpu_render_compute_utilization:
            metricInfo[PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION].data[0].emplace_back(power_telemetry_info.gpu_render_compute_utilization);
            break;
        case GpuTelemetryCapBits::gpu_media_utilization:
            metricInfo[PM_METRIC_GPU_MEDIA_UTILIZATION].data[0].emplace_back(power_telemetry_info.gpu_media_utilization);
            break;
        case GpuTelemetryCapBits::vram_power:
            metricInfo[PM_METRIC_GPU_MEM_POWER].data[0].emplace_back(power_telemetry_info.vram_power_w);
            break;
        case GpuTelemetryCapBits::vram_voltage:
            metricInfo[PM_METRIC_GPU_MEM_VOLTAGE].data[0].emplace_back(power_telemetry_info.vram_voltage_v);
            break;
        case GpuTelemetryCapBits::vram_frequency:
            metricInfo[PM_METRIC_GPU_MEM_FREQUENCY].data[0].emplace_back(power_telemetry_info.vram_frequency_mhz);
            break;
        case GpuTelemetryCapBits::vram_effective_frequency:
            metricInfo[PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY].data[0].emplace_back(power_telemetry_info.vram_effective_frequency_gbps);
            break;
        case GpuTelemetryCapBits::vram_temperature:
            metricInfo[PM_METRIC_GPU_MEM_TEMPERATURE].data[0].emplace_back(power_telemetry_info.vram_temperature_c);
            break;
        case GpuTelemetryCapBits::fan_speed_0:
            metricInfo[PM_METRIC_GPU_FAN_SPEED].data[0].emplace_back(power_telemetry_info.fan_speed_rpm[0]);
            break;
        case GpuTelemetryCapBits::fan_speed_1:
            metricInfo[PM_METRIC_GPU_FAN_SPEED].data[1].emplace_back(power_telemetry_info.fan_speed_rpm[1]);
            break;
        case GpuTelemetryCapBits::fan_speed_2:
            metricInfo[PM_METRIC_GPU_FAN_SPEED].data[2].emplace_back(power_telemetry_info.fan_speed_rpm[2]);
            break;
        case GpuTelemetryCapBits::fan_speed_3:
            metricInfo[PM_METRIC_GPU_FAN_SPEED].data[3].emplace_back(power_telemetry_info.fan_speed_rpm[3]);
            break;
        case GpuTelemetryCapBits::fan_speed_4:
            metricInfo[PM_METRIC_GPU_FAN_SPEED].data[4].emplace_back(power_telemetry_info.fan_speed_rpm[4]);
            break;
        case GpuTelemetryCapBits::max_fan_speed_0:
            metricInfo[PM_METRIC_GPU_FAN_SPEED_PERCENT].data[0].emplace_back(
                power_telemetry_info.fan_speed_rpm[0] / double(power_telemetry_info.max_fan_speed_rpm[0]));
            break;
        case GpuTelemetryCapBits::max_fan_speed_1:
            metricInfo[PM_METRIC_GPU_FAN_SPEED_PERCENT].data[1].emplace_back(
                power_telemetry_info.fan_speed_rpm[0] / double(power_telemetry_info.max_fan_speed_rpm[1]));
            break;
        case GpuTelemetryCapBits::max_fan_speed_2:
            metricInfo[PM_METRIC_GPU_FAN_SPEED_PERCENT].data[2].emplace_back(
                power_telemetry_info.fan_speed_rpm[0] / double(power_telemetry_info.max_fan_speed_rpm[2]));
            break;
        case GpuTelemetryCapBits::max_fan_speed_3:
            metricInfo[PM_METRIC_GPU_FAN_SPEED_PERCENT].data[3].emplace_back(
                power_telemetry_info.fan_speed_rpm[0] / double(power_telemetry_info.max_fan_speed_rpm[3]));
            break;
        case GpuTelemetryCapBits::max_fan_speed_4:
            metricInfo[PM_METRIC_GPU_FAN_SPEED_PERCENT].data[4].emplace_back(
                power_telemetry_info.fan_speed_rpm[0] / double(power_telemetry_info.max_fan_speed_rpm[4]));
            break;
        case GpuTelemetryCapBits::gpu_mem_used:
            metricInfo[PM_METRIC_GPU_MEM_USED].data[0].emplace_back(static_cast<double>(power_telemetry_info.gpu_mem_used_b));
            break;
        case GpuTelemetryCapBits::gpu_mem_write_bandwidth:
            metricInfo[PM_METRIC_GPU_MEM_WRITE_BANDWIDTH].data[0].emplace_back(power_telemetry_info.gpu_mem_write_bandwidth_bps);
            break;
        case GpuTelemetryCapBits::gpu_mem_read_bandwidth:
            metricInfo[PM_METRIC_GPU_MEM_READ_BANDWIDTH].data[0].emplace_back(power_telemetry_info.gpu_mem_read_bandwidth_bps);
            break;
        case GpuTelemetryCapBits::gpu_power_limited:
            metricInfo[PM_METRIC_GPU_POWER_LIMITED].data[0].emplace_back(power_telemetry_info.gpu_power_limited);
            break;
        case GpuTelemetryCapBits::gpu_temperature_limited:
            metricInfo[PM_METRIC_GPU_TEMPERATURE_LIMITED].data[0].emplace_back(power_telemetry_info.gpu_temperature_limited);
            break;
        case GpuTelemetryCapBits::gpu_current_limited:
            metricInfo[PM_METRIC_GPU_CURRENT_LIMITED].data[0].emplace_back(power_telemetry_info.gpu_current_limited);
            break;
        case GpuTelemetryCapBits::gpu_voltage_limited:
            metricInfo[PM_METRIC_GPU_VOLTAGE_LIMITED].data[0].emplace_back(power_telemetry_info.gpu_voltage_limited);
            break;
        case GpuTelemetryCapBits::gpu_utilization_limited:
            metricInfo[PM_METRIC_GPU_UTILIZATION_LIMITED].data[0].emplace_back(power_telemetry_info.gpu_utilization_limited);
            break;
        case GpuTelemetryCapBits::vram_power_limited:
            metricInfo[PM_METRIC_GPU_MEM_POWER_LIMITED].data[0].emplace_back(power_telemetry_info.vram_power_limited);
            break;
        case GpuTelemetryCapBits::vram_temperature_limited:
            metricInfo[PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED].data[0].emplace_back(power_telemetry_info.vram_temperature_limited);
            break;
        case GpuTelemetryCapBits::vram_current_limited:
            metricInfo[PM_METRIC_GPU_MEM_CURRENT_LIMITED].data[0].emplace_back(power_telemetry_info.vram_current_limited);
            break;
        case GpuTelemetryCapBits::vram_voltage_limited:
            metricInfo[PM_METRIC_GPU_MEM_VOLTAGE_LIMITED].data[0].emplace_back(power_telemetry_info.vram_voltage_limited);
            break;
        case GpuTelemetryCapBits::vram_utilization_limited:
            metricInfo[PM_METRIC_GPU_MEM_UTILIZATION_LIMITED].data[0].emplace_back(power_telemetry_info.vram_utilization_limited);
            break;
        case GpuTelemetryCapBits::gpu_effective_frequency:
            metricInfo[PM_METRIC_GPU_EFFECTIVE_FREQUENCY].data[0].emplace_back(power_telemetry_info.gpu_effective_frequency_mhz);
            break;
        case GpuTelemetryCapBits::gpu_voltage_regulator_temperature:
            metricInfo[PM_METRIC_GPU_VOLTAGE_REGULATOR_TEMPERATURE].data[0].emplace_back(power_telemetry_info.gpu_voltage_regulator_temperature_c);
            break;
        case GpuTelemetryCapBits::gpu_mem_effective_bandwidth:
            metricInfo[PM_METRIC_GPU_MEM_EFFECTIVE_BANDWIDTH].data[0].emplace_back(power_telemetry_info.gpu_mem_effective_bandwidth_gbps);
            break;
        case GpuTelemetryCapBits::gpu_overvoltage_percent:
            metricInfo[PM_METRIC_GPU_OVERVOLTAGE_PERCENT].data[0].emplace_back(power_telemetry_info.gpu_overvoltage_percent);
            break;
        case GpuTelemetryCapBits::gpu_temperature_percent:
            metricInfo[PM_METRIC_GPU_TEMPERATURE_PERCENT].data[0].emplace_back(power_telemetry_info.gpu_temperature_percent);
            break;
        case GpuTelemetryCapBits::gpu_power_percent:
            metricInfo[PM_METRIC_GPU_POWER_PERCENT].data[0].emplace_back(power_telemetry_info.gpu_power_percent);
            break;
        case GpuTelemetryCapBits::gpu_card_power:
            metricInfo[PM_METRIC_GPU_CARD_POWER].data[0].emplace_back(power_telemetry_info.gpu_card_power_w);
            break;
        default:
            validGpuMetric = false;
            break;
        }
        return validGpuMetric;
    }

    bool ConcreteMiddleware::GetCpuMetricData(size_t telemetryBit, CpuTelemetryInfo& cpuTelemetry, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo)
    {
        bool validCpuMetric = true;
        CpuTelemetryCapBits bit =
            static_cast<CpuTelemetryCapBits>(telemetryBit);
        switch (bit) {
        case CpuTelemetryCapBits::cpu_utilization:
            metricInfo[PM_METRIC_CPU_UTILIZATION].data[0].emplace_back(cpuTelemetry.cpu_utilization);
            break;
        case CpuTelemetryCapBits::cpu_power:
            metricInfo[PM_METRIC_CPU_POWER].data[0].emplace_back(cpuTelemetry.cpu_power_w);
            break;
        case CpuTelemetryCapBits::cpu_temperature:
            metricInfo[PM_METRIC_CPU_TEMPERATURE].data[0].emplace_back(cpuTelemetry.cpu_temperature);
            break;
        case CpuTelemetryCapBits::cpu_frequency:
            metricInfo[PM_METRIC_CPU_FREQUENCY].data[0].emplace_back(cpuTelemetry.cpu_frequency);
            break;
        default:
            validCpuMetric = false;
            break;
        }

        return validCpuMetric;
    }

    void ConcreteMiddleware::SaveMetricCache(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob)
    {
        auto it = cachedMetricDatas.find(std::pair(pQuery, processId));
        if (it != cachedMetricDatas.end())
        {
            auto& uniquePtr = it->second;
            std::copy(pBlob, pBlob + pQuery->queryCacheSize, uniquePtr.get());
        }
        else
        {
            auto dataArray = std::make_unique<uint8_t[]>(pQuery->queryCacheSize);
            std::copy(pBlob, pBlob + pQuery->queryCacheSize, dataArray.get());
            cachedMetricDatas.emplace(std::pair(pQuery, processId), std::move(dataArray));
        }
    }

    void ConcreteMiddleware::CopyMetricCacheToBlob(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob)
    {
        auto it = cachedMetricDatas.find(std::pair(pQuery, processId));
        if (it != cachedMetricDatas.end())
        {
            auto& uniquePtr = it->second;
            std::copy(uniquePtr.get(), uniquePtr.get() + pQuery->queryCacheSize, pBlob);
        }
    }

    // This code currently doesn't support the copying of multiple swap chains. If a second swap chain
    // is encountered it will update the numSwapChains to the correct number and then copy the swap
    // chain frame information with the most presents. If the client does happen to specify two swap
    // chains this code will incorrectly copy the data. WIP.
    void ConcreteMiddleware::CalculateMetrics(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains, LARGE_INTEGER qpcFrequency, std::unordered_map<uint64_t, fpsSwapChainData>& swapChainData, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo)
    {
        // Find the swapchain with the most frame metrics
        auto CalcGpuMemUtilization = [this, metricInfo](PM_STAT stat)
            {
                double output = 0.;
                if (cachedGpuInfo[currentGpuInfoIndex].gpuMemorySize.has_value()) {
                    auto gpuMemSize = static_cast<double>(cachedGpuInfo[currentGpuInfoIndex].gpuMemorySize.value());
                    if (gpuMemSize != 0.)
                    {
                        std::vector<double> memoryUtilization;
                        auto it = metricInfo.find(PM_METRIC_GPU_MEM_USED);
                        if (it != metricInfo.end()) {
                            auto memUsed = it->second;
                            auto memUsedVector = memUsed.data[0];
                            for (auto memUsed : memUsedVector) {
                                memoryUtilization.push_back(100. * (memUsed / gpuMemSize));
                            }
                            output = CalculateStatistic(memoryUtilization, stat);
                        }
                    }
                }
                return output;
            };

        uint32_t maxSwapChainPresents = 0;
        uint32_t maxSwapChainPresentsIndex = 0;
        uint32_t currentSwapChainIndex = 0;
        for (auto& pair : swapChainData) {
            auto& swapChain = pair.second;
            auto numFrames = (uint32_t)swapChain.mCPUBusy.size();
            if (numFrames > maxSwapChainPresents)
            {
                maxSwapChainPresents = numFrames;
                maxSwapChainPresentsIndex = currentSwapChainIndex;
            }
            currentSwapChainIndex++;
        }

        currentSwapChainIndex = 0;
        bool copyAllMetrics = true;
        bool useCache = false;
        bool allMetricsCalculated = false;

        // If the number of swap chains found in the frame data is greater than the number passed
        // in update the passed in number to notify the client there is more data present than
        // can be returned
        if (swapChainData.size() > *numSwapChains)
        {
            *numSwapChains = static_cast<uint32_t>(swapChainData.size());
            copyAllMetrics = false;
        }

        // If the client chose to monitor frame information then this loop
        // will calculate and store all metrics.
        for (auto& pair : swapChainData) {
            auto& swapChain = pair.second;

            // There are couple reasons where we will not be able to produce
            // fps metric data. The first is if all of the frames are dropped.
            // The second is if in the requested sample window there are
            // no presents.
            auto numFrames = (uint32_t)swapChain.mCPUBusy.size();
            if ((swapChain.display_count <= 1) && (numFrames == 0)) {
                useCache = true;
                pmlog_dbg("Filling cached data in dynamic metric poll")
                    .pmwatch(numFrames).pmwatch(swapChain.display_count).diag();
                break;
            }

            // If we are unable to copy all of the metrics to the blob and the current swap
            // chain isn't the one with the most presents, skip by it
            if ((copyAllMetrics == false) && (currentSwapChainIndex != maxSwapChainPresentsIndex))
            {
                continue;
            }
            for (auto& qe : pQuery->elements) {
                switch (qe.metric)
                {
                case PM_METRIC_SWAP_CHAIN_ADDRESS:
                {
                    auto& output = reinterpret_cast<uint64_t&>(pBlob[qe.dataOffset]);
                    output = pair.first;
                }
                    break;

                case PM_METRIC_CPU_START_QPC:
                case PM_METRIC_PRESENT_MODE:
                case PM_METRIC_PRESENT_RUNTIME:
                case PM_METRIC_PRESENT_FLAGS:
                case PM_METRIC_SYNC_INTERVAL:
                case PM_METRIC_ALLOWS_TEARING:
                case PM_METRIC_FRAME_TYPE:
                case PM_METRIC_GPU_LATENCY:
                case PM_METRIC_GPU_WAIT:
                case PM_METRIC_GPU_BUSY:
                case PM_METRIC_DISPLAY_LATENCY:
                case PM_METRIC_CLICK_TO_PHOTON_LATENCY:
                case PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY:
                case PM_METRIC_PRESENTED_FPS:
                case PM_METRIC_APPLICATION_FPS:
                case PM_METRIC_DISPLAYED_FPS:
                case PM_METRIC_DROPPED_FRAMES:
                case PM_METRIC_CPU_FRAME_TIME:
                case PM_METRIC_CPU_BUSY:
                case PM_METRIC_CPU_WAIT:
                case PM_METRIC_GPU_TIME:
                case PM_METRIC_DISPLAYED_TIME:
                case PM_METRIC_ANIMATION_ERROR:
                case PM_METRIC_APPLICATION:
                case PM_METRIC_PRESENT_START_QPC:
                case PM_METRIC_BETWEEN_PRESENTS:
                case PM_METRIC_IN_PRESENT_API:
                case PM_METRIC_BETWEEN_DISPLAY_CHANGE:
                case PM_METRIC_UNTIL_DISPLAYED:
                case PM_METRIC_RENDER_PRESENT_LATENCY:
                case PM_METRIC_BETWEEN_SIMULATION_START:
                case PM_METRIC_PC_LATENCY:
                case PM_METRIC_INSTRUMENTED_LATENCY:
                case PM_METRIC_PRESENTED_FRAME_TIME:
                case PM_METRIC_DISPLAYED_FRAME_TIME:
                    CalculateFpsMetric(swapChain, qe, pBlob, qpcFrequency);
                    break;
                case PM_METRIC_CPU_VENDOR:
                case PM_METRIC_CPU_POWER_LIMIT:
                case PM_METRIC_GPU_VENDOR:
                case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
                case PM_METRIC_GPU_MEM_SIZE:
                case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
                    CopyStaticMetricData(qe.metric, qe.deviceId, pBlob, qe.dataOffset);
                    break;
                case PM_METRIC_CPU_NAME:
                case PM_METRIC_GPU_NAME:
                    CopyStaticMetricData(qe.metric, qe.deviceId, pBlob, qe.dataOffset, 260);
                    break;
                case PM_METRIC_GPU_MEM_UTILIZATION:
                {
                    auto& output = reinterpret_cast<double&>(pBlob[qe.dataOffset]);
                    output = CalcGpuMemUtilization(qe.stat);
                }
                    break;
                default:
                    if (qe.dataSize == sizeof(double)) {
                        CalculateGpuCpuMetric(metricInfo, qe, pBlob);
                    }
                    break;
                }
            }

            allMetricsCalculated = true;
            currentSwapChainIndex++;
        }

        if (useCache == true) {
            CopyMetricCacheToBlob(pQuery, processId, pBlob);
            return;
        }

        if (allMetricsCalculated == false)
        {
            for (auto& qe : pQuery->elements)
            {
                switch (qe.metric)
                {
                case PM_METRIC_GPU_POWER:
                case PM_METRIC_GPU_FAN_SPEED:
                case PM_METRIC_GPU_FAN_SPEED_PERCENT:
                case PM_METRIC_GPU_VOLTAGE:
                case PM_METRIC_GPU_FREQUENCY:
                case PM_METRIC_GPU_TEMPERATURE:
                case PM_METRIC_GPU_UTILIZATION:
                case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
                case PM_METRIC_GPU_MEDIA_UTILIZATION:
                case PM_METRIC_GPU_MEM_POWER:
                case PM_METRIC_GPU_MEM_VOLTAGE:
                case PM_METRIC_GPU_MEM_FREQUENCY:
                case PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY:
                case PM_METRIC_GPU_MEM_TEMPERATURE:
                case PM_METRIC_GPU_MEM_USED:
                case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
                case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
                case PM_METRIC_GPU_POWER_LIMITED:
                case PM_METRIC_GPU_TEMPERATURE_LIMITED:
                case PM_METRIC_GPU_CURRENT_LIMITED:
                case PM_METRIC_GPU_VOLTAGE_LIMITED:
                case PM_METRIC_GPU_UTILIZATION_LIMITED:
                case PM_METRIC_GPU_MEM_POWER_LIMITED:
                case PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED:
                case PM_METRIC_GPU_MEM_CURRENT_LIMITED:
                case PM_METRIC_GPU_MEM_VOLTAGE_LIMITED:
                case PM_METRIC_GPU_MEM_UTILIZATION_LIMITED:
                case PM_METRIC_CPU_UTILIZATION:
                case PM_METRIC_CPU_POWER:
                case PM_METRIC_CPU_TEMPERATURE:
                case PM_METRIC_CPU_FREQUENCY:
                case PM_METRIC_CPU_CORE_UTILITY:
                case PM_METRIC_GPU_EFFECTIVE_FREQUENCY:
                case PM_METRIC_GPU_VOLTAGE_REGULATOR_TEMPERATURE:
                case PM_METRIC_GPU_MEM_EFFECTIVE_BANDWIDTH:
                case PM_METRIC_GPU_OVERVOLTAGE_PERCENT:
                case PM_METRIC_GPU_TEMPERATURE_PERCENT:
                case PM_METRIC_GPU_POWER_PERCENT:
                case PM_METRIC_GPU_CARD_POWER:
                    CalculateGpuCpuMetric(metricInfo, qe, pBlob);
                    break;
                case PM_METRIC_CPU_VENDOR:
                case PM_METRIC_CPU_POWER_LIMIT:
                case PM_METRIC_GPU_VENDOR:
                case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
                case PM_METRIC_GPU_MEM_SIZE:
                case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
                    CopyStaticMetricData(qe.metric, qe.deviceId, pBlob, qe.dataOffset);
                    break;
                case PM_METRIC_CPU_NAME:
                case PM_METRIC_GPU_NAME:
                    CopyStaticMetricData(qe.metric, qe.deviceId, pBlob, qe.dataOffset, 260);
                    break;
                case PM_METRIC_GPU_MEM_UTILIZATION:
                {
                    auto& output = reinterpret_cast<double&>(pBlob[qe.dataOffset]);
                    output = CalcGpuMemUtilization(qe.stat);
                    break;
                }
                default:
                    break;
                }
            }
        }

        // Save calculated metrics blob to cache
        SaveMetricCache(pQuery, processId, pBlob);
    }

    PM_STATUS ConcreteMiddleware::SetActiveGraphicsAdapter(uint32_t deviceId)
    {
        std::optional<uint64_t> adapterIndex;
        try {
            // if the requested deviceId does not exist in the cache of gpu devices return error
            adapterIndex = GetCachedGpuInfoIndex(deviceId);
            if (!adapterIndex.has_value()) {
                pmlog_error(std::format("Client specified invalid deviceId for adapter [{}]", deviceId));
                return PM_STATUS_INVALID_ADAPTER_ID;
            }

            pActionClient->DispatchSync(SelectAdapter::Params{ (uint32_t)*adapterIndex });
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
            return code;
        }

        pmlog_info(std::format("Active adapter change request completed, changed to [{}] (svc id) [{}] (API id)", *adapterIndex, deviceId));
        return PM_STATUS_SUCCESS;
    }

    void ConcreteMiddleware::GetStaticGpuMetrics()
    {
        try {
            const auto res = pActionClient->DispatchSync(EnumerateAdapters::Params{});

            // check that adapter count matches introspection
            if (res.adapters.size() != cachedGpuInfo.size()) {
                pmlog_warn(std::format("Queried adapter count {} did not match count from introspection {}",
                    res.adapters.size(), cachedGpuInfo.size())).diag();
            }

            // For each cached gpu search through the returned adapter information and set the returned
            // static gpu metrics where a match is found
            for (auto& gpuInfo : cachedGpuInfo) {
                for (auto& adapter : res.adapters) {
                    if (gpuInfo.adapterId == adapter.id) {
                        gpuInfo.gpuSustainedPowerLimit = adapter.gpuSustainedPowerLimit;
                        gpuInfo.gpuMemorySize = adapter.gpuMemorySize;
                        gpuInfo.gpuMemoryMaxBandwidth = adapter.gpuMemoryMaxBandwidth;
                        break;
                    }
                }
            }
        }
        catch (...) {
            const auto code = util::GeneratePmStatus();
            pmlog_error(util::ReportException()).code(code).diag();
        }
    }
}
