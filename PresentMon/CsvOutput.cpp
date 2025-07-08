// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#include "PresentMon.hpp"

static FILE* gGlobalOutputCsv = nullptr;
static uint32_t gRecordingCount = 1;

void IncrementRecordingCount()
{
    gRecordingCount += 1;
}

const char* PresentModeToString(PresentMode mode)
{
    switch (mode) {
    case PresentMode::Hardware_Legacy_Flip: return "Hardware: Legacy Flip";
    case PresentMode::Hardware_Legacy_Copy_To_Front_Buffer: return "Hardware: Legacy Copy to front buffer";
    case PresentMode::Hardware_Independent_Flip: return "Hardware: Independent Flip";
    case PresentMode::Composed_Flip: return "Composed: Flip";
    case PresentMode::Composed_Copy_GPU_GDI: return "Composed: Copy with GPU GDI";
    case PresentMode::Composed_Copy_CPU_GDI: return "Composed: Copy with CPU GDI";
    case PresentMode::Hardware_Composed_Independent_Flip: return "Hardware Composed: Independent Flip";
    default: return "Other";
    }
}

const char* RuntimeToString(Runtime rt)
{
    switch (rt) {
    case Runtime::DXGI: return "DXGI";
    case Runtime::D3D9: return "D3D9";
    default: return "Other";
    }
}

// If frame type debug is necessary uncomment
// the following line. If we want to have
// this based on _DEBUG we will need to double
// the number of ULT cases.
// #define DEBUG_FRAME_TYPE

const char* FrameTypeToString(FrameType ft)
{
    switch (ft) {
    #ifdef DEBUG_FRAME_TYPE
    case FrameType::NotSet:      return "NotSet";
    case FrameType::Repeated:    return "Repeated";
    #else
    case FrameType::NotSet:
    case FrameType::Repeated:
    #endif
    case FrameType::Application: return "Application";
    case FrameType::Intel_XEFG:  return "Intel XeSS-FG";
    case FrameType::AMD_AFMF:    return "AMD AFMF";
    }

    return "Unknown";
}

// v1.x only:
static const char* FinalStateToDroppedString(PresentResult res)
{
    switch (res) {
    case PresentResult::Presented: return "0";
    default: return "1";
    }
}

/* This text is reproduced in the readme, modify both if there are changes:

By default, PresentMon creates a CSV file named `PresentMon-TIME.csv`, where
`TIME` is the creation time in ISO 8601 format.  To specify your own output
location, use the `-output_file PATH` command line argument.

If `-multi_csv` is used, then one CSV is created for each process captured with
`-PROCESSNAME` appended to the file name.

If `-hotkey` is used, then one CSV is created each time recording is started
with `-INDEX` appended to the file name.

If `-include_mixed_reality` is used, a second CSV file will be generated with
`_WMR` appended to the filename containing the WMR data.
*/
static void GenerateFilename(wchar_t* path, std::wstring const& processName, uint32_t processId)
{
    auto const& args = GetCommandLineArgs();

    wchar_t ext[_MAX_EXT];
    int pathLength = MAX_PATH;

    #define ADD_TO_PATH(...) do { \
        if (path != nullptr) { \
            auto result = _snwprintf_s(path, pathLength, _TRUNCATE, __VA_ARGS__); \
            if (result == -1) path = nullptr; else { path += result; pathLength -= result; } \
        } \
    } while (0)

    // Generate base filename.
    if (args.mOutputCsvFileName) {
        wchar_t drive[_MAX_DRIVE];
        wchar_t dir[_MAX_DIR];
        wchar_t name[_MAX_FNAME];
        _wsplitpath_s(args.mOutputCsvFileName, drive, dir, name, ext);
        ADD_TO_PATH(L"%s%s%s", drive, dir, name);
    } else {
        struct tm tm;
        time_t time_now = time(NULL);
        localtime_s(&tm, &time_now);
        ADD_TO_PATH(L"PresentMon-%4d-%02d-%02dT%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        wcscpy_s(ext, L".csv");
    }

    // Append -PROCESSNAME if applicable.
    if (args.mMultiCsv) {
        if (processName != L"<unknown>") {
            ADD_TO_PATH(L"-%s", processName.c_str());
        }
        ADD_TO_PATH(L"-%u", processId);
    }

    // Append -INDEX if applicable.
    if (args.mHotkeySupport) {
        ADD_TO_PATH(L"-%d", gRecordingCount);
    }

    // Append extension.
    ADD_TO_PATH(L"%s", ext);

    #undef ADD_TO_PATH
}

template<typename FrameMetricsT>
void WriteCsvHeader(FILE* fp);

template<typename FrameMetricsT>
void WriteCsvRow(FILE* fp, PMTraceSession const& pmSession, ProcessInfo const& processInfo, PresentEvent const& p, FrameMetricsT const& metrics);

template<>
void WriteCsvHeader<FrameMetrics1>(FILE* fp)
{
    auto const& args = GetCommandLineArgs();

    fwprintf(fp, L"Application"
                 L",ProcessID"
                 L",SwapChainAddress"
                 L",Runtime"
                 L",SyncInterval"
                 L",PresentFlags"
                 L",Dropped");
    fwprintf(fp, L",TimeInSeconds"
                 L",msInPresentAPI"
                 L",msBetweenPresents");
    if (args.mTrackDisplay) {
        fwprintf(fp, L",AllowsTearing"
                     L",PresentMode"
                     L",msUntilRenderComplete"
                     L",msUntilDisplayed"
                     L",msBetweenDisplayChange"
                     L",msFlipDelay");
    }
    if (args.mTrackGPU) {
        fwprintf(fp, L",msUntilRenderStart"
                     L",msGPUActive");
    }
    if (args.mTrackGPUVideo) {
        fwprintf(fp, L",msGPUVideoActive");
    }
    if (args.mTrackInput) {
        fwprintf(fp, L",msSinceInput");
    }
    if (args.mTimeUnit == TimeUnit::QPC || args.mTimeUnit == TimeUnit::QPCMilliSeconds) {
        fwprintf(fp, L",QPCTime");
    }
    if (args.mWriteDisplayTime) {
        fwprintf(fp, L",msDisplayTime");
    }
    if (args.mWriteFrameId) {
        fwprintf(fp, L",FrameId");
    }
    fwprintf(fp, L"\n");

    if (args.mCSVOutput == CSVOutput::Stdout) {
        fflush(fp);
    }
}

template<>
void WriteCsvRow<FrameMetrics1>(
    FILE* fp,
    PMTraceSession const& pmSession,
    ProcessInfo const& processInfo,
    PresentEvent const& p,
    FrameMetrics1 const& metrics)
{
    auto const& args = GetCommandLineArgs();

    fwprintf(fp, L"%s,%d,0x%016llX,%hs,%d,%d,%hs", processInfo.mModuleName.c_str(),
                                                   p.ProcessId,
                                                   p.SwapChainAddress,
                                                   RuntimeToString(p.Runtime),
                                                   p.SyncInterval,
                                                   p.PresentFlags,
                                                   FinalStateToDroppedString(p.FinalState));
    switch (args.mTimeUnit) {
    case TimeUnit::DateTime: {
        SYSTEMTIME st = {};
        uint64_t ns = 0;
        pmSession.TimestampToLocalSystemTime(p.PresentStartTime, &st, &ns);
        fwprintf(fp, L",%u-%u-%u %u:%02u:%02u.%09llu", st.wYear,
                                                       st.wMonth,
                                                       st.wDay,
                                                       st.wHour,
                                                       st.wMinute,
                                                       st.wSecond,
                                                       ns);
    }   break;
    default:
        fwprintf(fp, L",%.*lf", DBL_DIG - 1, 0.001 * pmSession.TimestampToMilliSeconds(p.PresentStartTime));
        break;
    }
    fwprintf(fp, L",%.*lf,%.*lf", DBL_DIG - 1, metrics.msInPresentApi,
                                  DBL_DIG - 1, metrics.msBetweenPresents);
    if (args.mTrackDisplay) {
        fwprintf(fp, L",%d,%hs,%.*lf,%.*lf,%.*lf,%.*lf", p.SupportsTearing,
                                                         PresentModeToString(p.PresentMode),
                                                         DBL_DIG - 1, metrics.msUntilRenderComplete,
                                                         DBL_DIG - 1, metrics.msUntilDisplayed,
                                                         DBL_DIG - 1, metrics.msBetweenDisplayChange,
                                                         DBL_DIG - 1, metrics.msFlipDelay);
    }
    if (args.mTrackGPU) {
        fwprintf(fp, L",%.*lf,%.*lf", DBL_DIG - 1, metrics.msUntilRenderStart,
                                      DBL_DIG - 1, metrics.msGPUDuration);
    }
    if (args.mTrackGPUVideo) {
        fwprintf(fp, L",%.*lf", DBL_DIG - 1, metrics.msVideoDuration);
    }
    if (args.mTrackInput) {
        fwprintf(fp, L",%.*lf", DBL_DIG - 1, metrics.msSinceInput);
    }
    switch (args.mTimeUnit) {
    case TimeUnit::QPC:
        fwprintf(fp, L",%llu", p.PresentStartTime);
        break;
    case TimeUnit::QPCMilliSeconds:
        fwprintf(fp, L",%.*lf", DBL_DIG - 1, 0.001 * pmSession.TimestampDeltaToMilliSeconds(p.PresentStartTime));
        break;
    }
    if (args.mWriteDisplayTime) {
        if (metrics.qpcScreenTime == 0) {
            fwprintf(fp, L",NA");
        }
        else {
            fwprintf(fp, L",%.*lf", DBL_DIG - 1, 0.001 * pmSession.TimestampToMilliSeconds(metrics.qpcScreenTime));
        }
    }
    if (args.mWriteFrameId) {
        fwprintf(fp, L",%u", p.FrameId);
    }
    fwprintf(fp, L"\n");

    if (args.mCSVOutput == CSVOutput::Stdout) {
        fflush(fp);
    }
}

template<>
void WriteCsvHeader<FrameMetrics>(FILE* fp)
{
    auto const& args = GetCommandLineArgs();

    fwprintf(fp, L"Application"
                 L",ProcessID"
                 L",SwapChainAddress"
                 L",PresentRuntime"
                 L",SyncInterval"
                 L",PresentFlags");
    if (args.mTrackDisplay) {
        fwprintf(fp, L",AllowsTearing"
                     L",PresentMode");
    }
    if (args.mTrackFrameType) {
        fwprintf(fp, L",FrameType");
    }
    if (args.mTrackHybridPresent) {
        fwprintf(fp, L",HybridPresent");
    }
    if (args.mUseV2Metrics == false) {
        switch (args.mTimeUnit) {
        case TimeUnit::MilliSeconds:    fwprintf(fp, L",TimeInMs"); break;
        case TimeUnit::QPC:             fwprintf(fp, L",TimeInQPC"); break;
        case TimeUnit::DateTime:        fwprintf(fp, L",TimeInDateTime"); break;
        default:                        fwprintf(fp, L",TimeInSeconds"); break;
        }

        fwprintf(fp, L",MsBetweenSimulationStart"
            L",MsBetweenPresents");

        if (args.mTrackDisplay) {
            fwprintf(fp, L",MsBetweenDisplayChange");
        }

        fwprintf(fp, L",MsInPresentAPI"
            L",MsRenderPresentLatency");

        if (args.mTrackDisplay) {
            fwprintf(fp, L",MsUntilDisplayed");
            if (args.mTrackPcLatency) {
                fwprintf(fp, L",MsPCLatency");
            }
        }
    }

    if (args.mUseV2Metrics) {
        switch (args.mTimeUnit) {
        case TimeUnit::MilliSeconds:    fwprintf(fp, L",CPUStartTime"); break;
        case TimeUnit::QPC:             fwprintf(fp, L",CPUStartQPC"); break;
        case TimeUnit::QPCMilliSeconds: fwprintf(fp, L",CPUStartQPCTime"); break;
        case TimeUnit::DateTime:        fwprintf(fp, L",CPUStartDateTime"); break;
        default:                        fwprintf(fp, L",CPUStartTime"); break;
        }
        fwprintf(fp, L",FrameTime"
                     L",CPUBusy"
                     L",CPUWait");
        if (args.mTrackGPU) {
            fwprintf(fp, L",GPULatency"
                         L",GPUTime"
                         L",GPUBusy"
                         L",GPUWait");
        }
        if (args.mTrackGPUVideo) {
            fwprintf(fp, L",VideoBusy");
        }
        if (args.mTrackDisplay) {
                fwprintf(fp, 
                    L",DisplayLatency"
                         L",DisplayedTime"
                         L",AnimationError"
                         L",AnimationTime"
                         L",FlipDelay");
        }
        if (args.mTrackInput) {
            fwprintf(fp, L",AllInputToPhotonLatency");
            fwprintf(fp, L",ClickToPhotonLatency");
        }
        if (args.mTrackAppTiming) {
            fwprintf(fp, L",InstrumentedLatency");
        }
    } else {
        switch (args.mTimeUnit) {
        case TimeUnit::MilliSeconds:    fwprintf(fp, L",CPUStartTimeInMs"); break;
        case TimeUnit::QPC:             fwprintf(fp, L",CPUStartQPC"); break;
        case TimeUnit::QPCMilliSeconds: fwprintf(fp, L",CPUStartQPCTimeInMs"); break;
        case TimeUnit::DateTime:        fwprintf(fp, L",CPUStartDateTime"); break;
        default:                        fwprintf(fp, L",CPUStartTimeInSeconds"); break;
        }
        fwprintf(fp, L",MsBetweenAppStart"
            L",MsCPUBusy"
            L",MsCPUWait");
        if (args.mTrackGPU) {
            fwprintf(fp, L",MsGPULatency"
                L",MsGPUTime"
                L",MsGPUBusy"
                L",MsGPUWait");
        }
        if (args.mTrackGPUVideo) {
            fwprintf(fp, L",MsVideoBusy");
        }
        if (args.mTrackDisplay) {
            fwprintf(fp, L",MsAnimationError"
                L",AnimationTime"
                L",FlipDelay");
        }
        if (args.mTrackInput) {
            fwprintf(fp, L",MsAllInputToPhotonLatency");
            fwprintf(fp, L",MsClickToPhotonLatency");
        }
        if (args.mTrackAppTiming) {
            fwprintf(fp, L",MsInstrumentedLatency");
        }
    }
    if (args.mWriteDisplayTime) {
        fwprintf(fp, L",DisplayTimeAbs");
    }
    if (args.mWriteFrameId) {
        fwprintf(fp, L",FrameId");
        if (args.mTrackAppTiming) {
            fwprintf(fp, L",AppFrameId");
        }
        if (args.mTrackPcLatency) {
            fwprintf(fp, L",PCLFrameId");
        }
    }
    fwprintf(fp, L"\n");

    if (args.mCSVOutput == CSVOutput::Stdout) {
        fflush(fp);
    }
}

template<>
void WriteCsvRow<FrameMetrics>(
    FILE* fp,
    PMTraceSession const& pmSession,
    ProcessInfo const& processInfo,
    PresentEvent const& p,
    FrameMetrics const& metrics)
{
    auto const& args = GetCommandLineArgs();

    fwprintf(fp, L"%s,%d,0x%llX,%hs,%d,%d", processInfo.mModuleName.c_str(),
                                            p.ProcessId,
                                            p.SwapChainAddress,
                                            RuntimeToString(p.Runtime),
                                            p.SyncInterval,
                                            p.PresentFlags);
    if (args.mTrackDisplay) {
        fwprintf(fp, L",%d,%hs", p.SupportsTearing,
                                 PresentModeToString(p.PresentMode));
    }
    if (args.mTrackFrameType) {
        fwprintf(fp, L",%hs", FrameTypeToString(metrics.mFrameType));
    }
    if (args.mTrackHybridPresent) {
        fwprintf(fp, L",%d", p.IsHybridPresent);
    }

    if (args.mUseV2Metrics == false) {
        // Time in Seconds
        switch (args.mTimeUnit) {
        case TimeUnit::DateTime: {
            SYSTEMTIME st = {};
            uint64_t ns = 0;
            pmSession.TimestampToLocalSystemTime(metrics.mTimeInSeconds, &st, &ns);
            fwprintf(fp, L",%u-%u-%u %u:%02u:%02u.%09llu", st.wYear,
                st.wMonth,
                st.wDay,
                st.wHour,
                st.wMinute,
                st.wSecond,
                ns);
        }   break;
        case TimeUnit::MilliSeconds:
        case TimeUnit::QPCMilliSeconds:
            fwprintf(fp, L",%.4lf", pmSession.TimestampToMilliSeconds(metrics.mTimeInSeconds));
            break;
        case TimeUnit::QPC:
            fwprintf(fp, L",%llu", metrics.mTimeInSeconds);
            break;
        default:
            fwprintf(fp, L",%.*lf", DBL_DIG - 1, 0.001 * pmSession.TimestampToMilliSeconds(metrics.mTimeInSeconds));
            break;
        }

        // MsBetweenSimulationStart
        if (metrics.mMsBetweenSimStarts == 0.0) {
            fwprintf(fp, L",NA");
        }
        else {
            fwprintf(fp, L",%.4lf", metrics.mMsBetweenSimStarts);
        }

        // MsBetweenPresents
        fwprintf(fp, L",%.*lf", DBL_DIG - 1, metrics.mMsBetweenPresents);

        // MsBetweenDisplayChange -> Transition from DisplayedTime
        if (args.mTrackDisplay) {
            if (metrics.mMsBetweenDisplayChange == 0.0) {
                fwprintf(fp, L",NA");
            }
            else {
                fwprintf(fp, L",%.*lf", DBL_DIG - 1, metrics.mMsBetweenDisplayChange);
            }
        }

        // MsInPresentAPI
        fwprintf(fp, L",%.*lf", DBL_DIG - 1, metrics.mMsInPresentApi);

        // MsRenderPresentLatency
        fwprintf(fp, L",%.*lf", DBL_DIG - 1, metrics.mMsUntilRenderComplete);

        // MsUntilDisplayed
        if (args.mTrackDisplay) {
            if (metrics.mMsUntilDisplayed == 0.0) {
                fwprintf(fp, L",NA");
            } else {
                fwprintf(fp, L",%.4lf", metrics.mMsUntilDisplayed);
            }
        }
        if (args.mTrackPcLatency) {
            if (metrics.mMsPcLatency == 0.0) {
                fwprintf(fp, L",NA");
            }
            else {
                fwprintf(fp, L",%.4lf", metrics.mMsPcLatency);
            }
        }
    }

    // CPUStartTime
    switch (args.mTimeUnit) {
    case TimeUnit::MilliSeconds:
        fwprintf(fp, L",%.4lf", pmSession.TimestampToMilliSeconds(metrics.mCPUStart));
        break;
    case TimeUnit::QPC:
        fwprintf(fp, L",%llu", metrics.mCPUStart);
        break;
    case TimeUnit::QPCMilliSeconds:
        fwprintf(fp, L",%.4lf", pmSession.TimestampDeltaToMilliSeconds(metrics.mCPUStart));
        break;
    case TimeUnit::DateTime: {
        SYSTEMTIME st = {};
        uint64_t ns = 0;
        pmSession.TimestampToLocalSystemTime(metrics.mCPUStart, &st, &ns);
        fwprintf(fp, L",%u-%u-%u %u:%02u:%02u.%09llu", st.wYear,
                                                       st.wMonth,
                                                       st.wDay,
                                                       st.wHour,
                                                       st.wMinute,
                                                       st.wSecond,
                                                       ns);
    }
    break;
    default:
        fwprintf(fp, L",%.4lf", 0.001 * pmSession.TimestampToMilliSeconds(metrics.mCPUStart));
    }

    // MsBetweenAppStart, MsCPUBusy, MsCPUWait
    fwprintf(fp, L",%.4lf,%.4lf,%.4lf", metrics.mMsCPUBusy + metrics.mMsCPUWait,
        metrics.mMsCPUBusy,
        metrics.mMsCPUWait);

    if (args.mTrackGPU) {
        fwprintf(fp, L",%.4lf,%.4lf,%.4lf,%.4lf", metrics.mMsGPULatency,
                                                  metrics.mMsGPUBusy + metrics.mMsGPUWait,
                                                  metrics.mMsGPUBusy,
                                                  metrics.mMsGPUWait);
    }
    if (args.mTrackGPUVideo) {
        fwprintf(fp, L",%.4lf", metrics.mMsVideoBusy);
    }
    if (args.mTrackDisplay) {
        if (args.mUseV2Metrics) {
            if (metrics.mMsDisplayedTime == 0.0) {
                fwprintf(fp, L",NA,NA");
            } else {
                fwprintf(fp, L",%.4lf,%.4lf", metrics.mMsDisplayLatency,
                    metrics.mMsDisplayedTime);
            }
        }
        if (metrics.mMsAnimationError.has_value()) {
            fwprintf(fp, L",%.4lf", metrics.mMsAnimationError.value());
        }
        else {
            fwprintf(fp, L",NA");
        }
        if (metrics.mAnimationTime.has_value()) {
            fwprintf(fp, L",%.4lf", metrics.mAnimationTime.value());
        }
        else {
            fwprintf(fp, L",NA");
        }
        if (metrics.mMsFlipDelay == 0.0) {
            fwprintf(fp, L",NA");
        } else {
            fwprintf(fp, L",%.4lf", metrics.mMsFlipDelay);
        }
    }
    if (args.mTrackInput) {
        if (metrics.mMsAllInputPhotonLatency == 0.0) {
            fwprintf(fp, L",NA");
        }
        else {
            fwprintf(fp, L",%.4lf", metrics.mMsAllInputPhotonLatency);
        }
        if (metrics.mMsClickToPhotonLatency == 0.0) {
            fwprintf(fp, L",NA");
        } else {
            fwprintf(fp, L",%.4lf", metrics.mMsClickToPhotonLatency);
        }
    }
    if (args.mTrackAppTiming) {
        if (metrics.mMsInstrumentedLatency == 0.0) {
            fwprintf(fp, L",NA");
        }
        else {
            fwprintf(fp, L",%.4lf", metrics.mMsInstrumentedLatency);
        }
    }
    if (args.mWriteDisplayTime) {
        if (metrics.mScreenTime == 0) {
            fwprintf(fp, L",NA");
        }
        else {
            fwprintf(fp, L",%.4lf", pmSession.TimestampToMilliSeconds(metrics.mScreenTime));
        }
    }
    if (args.mWriteFrameId) {
        fwprintf(fp, L",%u", p.FrameId);
        if (args.mTrackAppTiming) {
            fwprintf(fp, L",%u", p.AppFrameId);
        }
        if (args.mTrackPcLatency) {
            fwprintf(fp, L",%u", p.PclFrameId);
        }
    }
    fwprintf(fp, L"\n");

    if (args.mCSVOutput == CSVOutput::Stdout) {
        fflush(fp);
    }
}

template<typename FrameMetricsT>
void UpdateCsvT(
    PMTraceSession const& pmSession,
    ProcessInfo* processInfo,
    PresentEvent const& p,
    FrameMetricsT const& metrics)
{
    auto const& args = GetCommandLineArgs();

    // Early return if not outputing to CSV.
    if (args.mCSVOutput == CSVOutput::None) {
        return;
    }

    // Don't output dropped frames (if requested).
    auto presented = p.FinalState == PresentResult::Presented;
    if (args.mExcludeDropped && !presented) {
        return;
    }

    // Get/create file
    FILE** fp = args.mMultiCsv
        ? &processInfo->mOutputCsv
        : &gGlobalOutputCsv;

    if (*fp == nullptr) {
        if (args.mCSVOutput == CSVOutput::File) {
            wchar_t path[MAX_PATH];
            GenerateFilename(path, processInfo->mModuleName, p.ProcessId);
            if (_wfopen_s(fp, path, L"w,ccs=UTF-8")) {
                return;
            }
        } else {
            *fp = stdout;
        }

        WriteCsvHeader<FrameMetricsT>(*fp);
    }

    // Output in CSV format
    WriteCsvRow(*fp, pmSession, *processInfo, p, metrics);
}

void UpdateCsv(PMTraceSession const& pmSession, ProcessInfo* processInfo, PresentEvent const& p, FrameMetrics1 const& metrics)
{
    UpdateCsvT(pmSession, processInfo, p, metrics);
}

void UpdateCsv(PMTraceSession const& pmSession, ProcessInfo* processInfo, PresentEvent const& p, FrameMetrics const& metrics)
{
    UpdateCsvT(pmSession, processInfo, p, metrics);
}

static void CloseCsv(FILE** fp)
{
    if (*fp != nullptr) {
        if (*fp != stdout) {
            fclose(*fp);
        }
        *fp = nullptr;
    }
}

void CloseMultiCsv(ProcessInfo* processInfo)
{
    CloseCsv(&processInfo->mOutputCsv);
}

void CloseGlobalCsv()
{
    CloseCsv(&gGlobalOutputCsv);
}

