// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <windows.h>

struct PresentMonCsv
{
    enum Header {
        // Headers common to all versions
        Header_Application,
        Header_ProcessID,
        Header_SwapChainAddress,
        Header_PresentRuntime,
        Header_SyncInterval,
        Header_PresentFlags,
        Header_AllowsTearing,
        Header_PresentMode,

        // Current headers
        Header_MsBetweenSimulationStart,
        Header_MsPCLatency,
        Header_CPUStartTimeInMs,
        Header_CPUStartTimeInSeconds,
        Header_MsBetweenAppStart,
        Header_MsCPUBusy,
        Header_MsCPUWait,
        Header_MsGPULatency,
        Header_MsGPUTime,
        Header_MsGPUBusy,
        Header_MsGPUWait,
        Header_MsVideoBusy,
        Header_MsAnimationError,
        Header_MsClickToPhotonLatency,
        Header_MsAllInputToPhotonLatency,
        Header_MsRenderPresentLatency,
        Header_TimeInQPC,
        Header_TimeInMs,
        Header_TimeInDateTime,
        Header_MsFlipDelay,

        // Current Internal Intel Metrics
        Header_MsInstrumentedLatency,
        Header_MsInstrumentedRenderLatency,
        Header_MsInstrumentedSleep,
        Header_MsInstrumentedGPULatency,
        Header_MsReadyTimeToDisplayLatency,
        Header_MsReprojectedLatency,

        // Headers for --v2_metrics only
        Header_CPUStartTime,
        Header_FrameTime,
        Header_CPUBusy,
        Header_CPUWait,
        Header_GPULatency,
        Header_GPUTime,
        Header_GPUBusy,
        Header_GPUWait,
        Header_VideoBusy,
        Header_DisplayLatency,
        Header_DisplayedTime,
        Header_AnimationError,
        Header_AnimationTime,
        Header_ClickToPhotonLatency,
        Header_AllInputToPhotonLatency,

        // Headers of internal Intel Metrics --v2_metrics only
        Header_InstrumentedLatency,
        Header_InstrumentedRenderLatency,
        Header_InstrumentedSleep,
        Header_InstrumentedGPULatency,
        Header_ReadyTimeToDisplayLatency,

        // Headers shared between current and --v2_metrics
        Header_FrameType,
        Header_CPUStartQPC,
        Header_CPUStartQPCTime,
        Header_CPUStartDateTime,

        // Headers shared between current, --v1_metrics and --v2_metrics
        Header_msBetweenPresents,
        Header_msInPresentAPI,
        Header_msBetweenDisplayChange,
        Header_msUntilRenderComplete,
        Header_msUntilDisplayed,

        // Headers shared between current and --v1_metrics
        Header_TimeInSeconds,

        // --v1_metrics
        Header_Runtime,
        Header_Dropped,
        Header_msUntilRenderStart,
        Header_msGPUActive,
        Header_msGPUVideoActive,
        Header_msSinceInput,
        Header_QPCTime,
        Header_msFlipDelay,

        // Deprecated
        Header_WasBatched,
        Header_DwmNotified,

        // Special values:
        KnownHeaderCount,
        UnknownHeader,
    };

    static constexpr char const* GetHeaderString(Header h)
    {
        switch (h) {
        case Header_Application:                return "Application";
        case Header_ProcessID:                  return "ProcessID";
        case Header_SwapChainAddress:           return "SwapChainAddress";
        case Header_PresentRuntime:             return "PresentRuntime";
        case Header_SyncInterval:               return "SyncInterval";
        case Header_PresentFlags:               return "PresentFlags";
        case Header_AllowsTearing:              return "AllowsTearing";
        case Header_PresentMode:                return "PresentMode";
        case Header_FrameType:                  return "FrameType";
        case Header_CPUStartTimeInMs:           return "CPUStartTimeInMs";
        case Header_CPUStartTimeInSeconds:      return "CPUStartTimeInSeconds";
        case Header_CPUStartTime:               return "CPUStartTime";
        case Header_CPUStartQPC:                return "CPUStartQPC";
        case Header_CPUStartQPCTime:            return "CPUStartQPCTime";
        case Header_CPUStartDateTime:           return "CPUStartDateTime";
        case Header_FrameTime:                  return "FrameTime";
        case Header_CPUBusy:                    return "CPUBusy";
        case Header_CPUWait:                    return "CPUWait";
        case Header_GPULatency:                 return "GPULatency";
        case Header_GPUTime:                    return "GPUTime";
        case Header_GPUBusy:                    return "GPUBusy";
        case Header_GPUWait:                    return "GPUWait";
        case Header_VideoBusy:                  return "VideoBusy";
        case Header_DisplayLatency:             return "DisplayLatency";
        case Header_DisplayedTime:              return "DisplayedTime";
        case Header_AnimationError:             return "AnimationError";
        case Header_AnimationTime:              return "AnimationTime";
        case Header_ClickToPhotonLatency:       return "ClickToPhotonLatency";
        case Header_AllInputToPhotonLatency:    return "AllInputToPhotonLatency";
        case Header_Runtime:                    return "Runtime";
        case Header_Dropped:                    return "Dropped";
        case Header_TimeInSeconds:              return "TimeInSeconds";
        case Header_TimeInQPC:                  return "TimeInQPC";
        case Header_TimeInMs:                   return "TimeInMs";
        case Header_TimeInDateTime:             return "TimeInDateTime";
        case Header_msBetweenPresents:          return "msBetweenPresents";
        case Header_msInPresentAPI:             return "msInPresentAPI";
        case Header_msBetweenDisplayChange:     return "msBetweenDisplayChange";
        case Header_msUntilRenderComplete:      return "msUntilRenderComplete";
        case Header_msUntilDisplayed:           return "msUntilDisplayed";
        case Header_msUntilRenderStart:         return "msUntilRenderStart";
        case Header_msGPUActive:                return "msGPUActive";
        case Header_msGPUVideoActive:           return "msGPUVideoActive";
        case Header_msSinceInput:               return "msSinceInput";
        case Header_QPCTime:                    return "QPCTime";

        case Header_WasBatched:                 return "WasBatched";
        case Header_DwmNotified:                return "DwmNotified";

        case Header_InstrumentedLatency:        return "InstrumentedLatency";
        case Header_MsRenderPresentLatency:     return "MsRenderPresentLatency";
        case Header_MsBetweenAppStart:          return "MsBetweenAppStart";
        case Header_MsCPUBusy:                  return "MsCPUBusy";
        case Header_MsCPUWait:                  return "MsCPUWait";
        case Header_MsGPULatency:               return "MsGPULatency";
        case Header_MsGPUTime:                  return "MsGPUTime";
        case Header_MsGPUBusy:                  return "MsGPUBusy";
        case Header_MsGPUWait:                  return "MsGPUWait";
        case Header_MsVideoBusy:                return "MsVideoBusy";
        case Header_MsAnimationError:           return "MsAnimationError";
        case Header_MsClickToPhotonLatency:     return "MsClickToPhotonLatency";
        case Header_MsAllInputToPhotonLatency:  return "MsAllInputToPhotonLatency";
        case Header_MsInstrumentedLatency:      return "MsInstrumentedLatency";
        case Header_MsBetweenSimulationStart:   return "MsBetweenSimulationStart";
        case Header_MsPCLatency:                return "MsPCLatency";
        case Header_MsFlipDelay:                return "MsFlipDelay";
        case Header_msFlipDelay:                return "msFlipDelay";
        }
        return "<unknown>";
    }

    std::wstring path_;
    size_t line_ = 0;
    FILE* fp_ = nullptr;

    // headerColumnIndex_[h] is the file column index where h was found, or SIZE_MAX if
    // h wasn't found in the file.
    size_t headerColumnIndex_[KnownHeaderCount];

    char row_[1024];
    std::vector<char const*> cols_;
    std::vector<wchar_t const*> params_;

    bool Open(char const* file, int line, std::wstring const& path);
    void Close();
    bool ReadRow();

    size_t GetColumnIndex(char const* header) const;
};

#define CSVOPEN(_P) Open(__FILE__, __LINE__, _P)

struct PresentMon : PROCESS_INFORMATION {
    static std::wstring exePath_;
    std::wstring cmdline_;
    bool csvArgSet_;

    PresentMon();
    ~PresentMon();

    void AddEtlPath(std::wstring const& etlPath);
    void AddCsvPath(std::wstring const& csvPath);
    void Add(wchar_t const* args);
    void Start(char const* file, int line);

    // Returns true if the process is still running for timeoutMilliseconds
    bool IsRunning(DWORD timeoutMilliseconds=0) const;

    // Expect the process to exit with expectedExitCode within
    // timeoutMilliseconds (or kill it otherwise).
    void ExpectExited(char const* file, int line, DWORD timeoutMilliseconds=INFINITE, DWORD expectedExitCode=0);
};

#define PMSTART() Start(__FILE__, __LINE__)
#define PMEXITED(...) ExpectExited(__FILE__, __LINE__, __VA_ARGS__)

// PresentMonTests.cpp
extern std::wstring outDir_;
extern bool reportAllCsvDiffs_;
extern bool warnOnMissingCsv_;
extern std::wstring diffPath_;

bool EnsureDirectoryCreated(std::wstring path);
std::string Convert(std::wstring const& s);
std::wstring Convert(std::string const& s);

// PresentMon.cpp
void AddTestFailure(char const* file, int line, char const* fmt, ...);

// GoldEtlCsvTests.cpp
void AddGoldEtlCsvTests(std::wstring const& dir, size_t relIdx);
