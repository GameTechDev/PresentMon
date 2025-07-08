// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMonTests.h"

#include <initializer_list>
#include <unordered_set>

void AddTestFailure(char const* file, int line, char const* fmt, ...)
{
    char buffer[512];

    va_list val;
    va_start(val, fmt);
    vsnprintf(buffer, _countof(buffer), fmt, val);
    va_end(val);

    GTEST_MESSAGE_AT_(file, line, buffer, ::testing::TestPartResult::kNonFatalFailure);
}

namespace {

void CheckAll(size_t const* columnIndex, bool* ok, std::initializer_list<PresentMonCsv::Header> const& headers)
{
    *ok = true;
    for (auto const& h : headers) {
        if (columnIndex[h] == SIZE_MAX) {
            *ok = false;
            return;
        }
    }
}

size_t CheckOne(size_t const* columnIndex, bool* ok, std::initializer_list<PresentMonCsv::Header> const& headers)
{
    *ok = true;
    size_t i = 0;
    for (auto const& h : headers) {
        if (columnIndex[h] != SIZE_MAX) {
            for (auto const& h2 : headers) {
                if (h2 != h && columnIndex[h2] != SIZE_MAX) {
                    *ok = false;
                    break;
                }
            }
            return i;
        }
        ++i;
    }
    *ok = false;
    return SIZE_MAX;
}

bool CheckAllIfAny(size_t const* columnIndex, bool* ok, std::initializer_list<PresentMonCsv::Header> const& headers)
{
    *ok = true;
    for (auto const& h : headers) {
        if (columnIndex[h] != SIZE_MAX) {
            CheckAll(columnIndex, ok, headers);
            return *ok;
        }
    }
    return false;
}

bool CaseInsensitiveCompare(const std::string& str1, const std::string& str2) {
    if (str1.size() != str2.size()) {
        return false;
    }
    return std::equal(str1.begin(), str1.end(), str2.begin(),
        [](unsigned char c1, unsigned char c2) {
            return std::tolower(c1) == std::tolower(c2);
        });
}

PresentMonCsv::Header FindHeader(char const* header)
{
    for (uint32_t i = 0; i < PresentMonCsv::KnownHeaderCount; ++i) {
        auto h = (PresentMonCsv::Header) i;
        // do a case insensitive compare to see if the header starts with "ms"

        std::string headerName = PresentMonCsv::GetHeaderString(h);
        if (std::tolower(headerName[0]) == 'm' && std::tolower(headerName[1]) == 's') {
            if (CaseInsensitiveCompare(header, headerName)) {
                return h;
            }
        } else {
            if (strcmp(header, PresentMonCsv::GetHeaderString(h)) == 0) {
                return h;
            }
        }
    }
    return PresentMonCsv::UnknownHeader;
}

}

bool PresentMonCsv::Open(char const* file, int line, std::wstring const& path)
{
    // Load the CSV
    for (uint32_t i = 0; i < KnownHeaderCount; ++i) {
        headerColumnIndex_[i] = SIZE_MAX;
    }

    path_ = path;
    line_ = 0;

    if (_wfopen_s(&fp_, path.c_str(), L"r")) {
        AddTestFailure(file, line, "Failed to open file: %ls", path.c_str());
        return false;
    }

    // Remove UTF-8 marker if there is one.
    if (fread(row_, 3, 1, fp_) != 1 ||
        row_[0] != -17 || // 0xef
        row_[1] != -69 || // 0xbb
        row_[2] != -65) { // 0xbf
        fseek(fp_, 0, SEEK_SET);
    }

    // Read the header and ensure required columns are present
    ReadRow();

    for (size_t i = 0, n = cols_.size(); i < n; ++i) {
        auto h = FindHeader(cols_[i]);
        switch (h) {
        case KnownHeaderCount:
        case UnknownHeader:
            AddTestFailure(Convert(path_).c_str(), (int) line_, "Unrecognised column: %s", cols_[i]);
            break;
        default:
            if (headerColumnIndex_[(size_t) h] != SIZE_MAX) {
                AddTestFailure(Convert(path_).c_str(), (int) line_, "Duplicate column: %s", cols_[i]);
            } else {
                headerColumnIndex_[(size_t) h] = i;
            }
            break;
        }
    }

    bool columnsOK = true;

    auto v1 = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_Runtime,
                                                              Header_Dropped,
                                                              Header_TimeInSeconds,
                                                              Header_msBetweenPresents,
                                                              Header_msInPresentAPI });

    auto v2 = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_FrameTime,
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
                                                              Header_AllInputToPhotonLatency });
    if (v1) {
        CheckAll(headerColumnIndex_, &columnsOK, { Header_Application,
                                                   Header_ProcessID,
                                                   Header_SwapChainAddress,
                                                   Header_SyncInterval,
                                                   Header_PresentFlags });

        auto qpc_time        = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_QPCTime, });
        auto track_display   = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_AllowsTearing,
                                                                               Header_PresentMode,
                                                                               Header_msBetweenDisplayChange,
                                                                               Header_msUntilRenderComplete,
                                                                               Header_msUntilDisplayed });
        auto track_gpu       = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_msUntilRenderStart,
                                                                               Header_msGPUActive });
        auto track_gpu_video = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_msGPUVideoActive });
        auto track_input     = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_msSinceInput });

                              params_.emplace_back(L"--v1_metrics");
        if (qpc_time)         params_.emplace_back(L"--qpc_time");
        if (!track_display)   params_.emplace_back(L"--no_track_display");
        if (!track_gpu)       params_.emplace_back(L"--no_track_gpu");
        if (track_gpu_video)  params_.emplace_back(L"--track_gpu_video");
        if (!track_input)     params_.emplace_back(L"--no_track_input");
    } else if (v2){
        CheckAll(headerColumnIndex_, &columnsOK, { Header_Application,
                                                   Header_ProcessID,
                                                   Header_SwapChainAddress,
                                                   Header_PresentRuntime,
                                                   Header_SyncInterval,
                                                   Header_PresentFlags,
                                                   Header_FrameTime,
                                                   Header_CPUBusy,
                                                   Header_CPUWait });

        size_t time           = CheckOne(headerColumnIndex_, &columnsOK,      { Header_CPUStartTime,
                                                                                Header_CPUStartQPC,
                                                                                Header_CPUStartQPCTime,
                                                                                Header_CPUStartDateTime });
        auto track_display    = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_AllowsTearing,
                                                                                Header_PresentMode,
                                                                                Header_DisplayLatency,
                                                                                Header_DisplayedTime,
                                                                                Header_AnimationError,
                                                                                Header_AnimationTime });
        auto track_gpu        = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_GPULatency,
                                                                                Header_GPUTime,
                                                                                Header_GPUBusy,
                                                                                Header_GPUWait });
        auto track_gpu_video  = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_VideoBusy });
        auto track_input      = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_ClickToPhotonLatency });
        auto track_frame_type = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_FrameType });
        auto track_app_timing = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_InstrumentedLatency });
        params_.emplace_back(L"--v2_metrics");
        switch (time) {
        case 1: params_.emplace_back(L"--qpc_time");    break;
        case 2: params_.emplace_back(L"--qpc_time_ms"); break;
        case 3: params_.emplace_back(L"--date_time");   break;
        }
        if (!track_display)   params_.emplace_back(L"--no_track_display");
        if (!track_gpu)       params_.emplace_back(L"--no_track_gpu");
        if (track_gpu_video)  params_.emplace_back(L"--track_gpu_video");
        if (!track_input)     params_.emplace_back(L"--no_track_input");
        if (track_frame_type) params_.emplace_back(L"--track_frame_type");
        if (track_app_timing) params_.emplace_back(L"--track_app_timing");
    }
    else {
        CheckAll(headerColumnIndex_, &columnsOK, { Header_Application,
                                                   Header_ProcessID,
                                                   Header_SwapChainAddress,
                                                   Header_PresentRuntime,
                                                   Header_SyncInterval,
                                                   Header_PresentFlags,
                                                   Header_MsBetweenSimulationStart,
                                                   Header_msBetweenPresents,
                                                   Header_msInPresentAPI,
                                                   Header_MsPCLatency, 
                                                   Header_MsBetweenAppStart,
                                                   Header_MsCPUBusy,
                                                   Header_MsCPUWait });

        size_t time           = CheckOne(headerColumnIndex_, &columnsOK,      { Header_TimeInSeconds,
                                                                                Header_TimeInQPC,
                                                                                Header_TimeInMs,
                                                                                Header_TimeInDateTime });
        auto track_display    = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_AllowsTearing,
                                                                                Header_PresentMode,
                                                                                Header_msBetweenDisplayChange,
                                                                                Header_msUntilDisplayed,
                                                                                Header_MsRenderPresentLatency,
                                                                                Header_MsAnimationError,
                                                                                Header_AnimationTime });
        auto track_gpu        = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_MsGPULatency,
                                                                                Header_MsGPUTime,
                                                                                Header_MsGPUBusy,
                                                                                Header_MsGPUWait });
        auto track_gpu_video  = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_MsVideoBusy });
        auto track_input      = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_MsClickToPhotonLatency,
                                                                                Header_MsAllInputToPhotonLatency });
        auto track_frame_type = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_FrameType });
        auto track_app_timing = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_MsInstrumentedLatency});
        auto track_pc_latency = CheckAllIfAny(headerColumnIndex_, &columnsOK, { Header_MsPCLatency });

        switch (time) {
        case 1: params_.emplace_back(L"--qpc_time");    break;
        case 2: params_.emplace_back(L"--qpc_time_ms"); break;
        case 3: params_.emplace_back(L"--date_time");   break;
        }
        if (!track_display)   params_.emplace_back(L"--no_track_display");
        if (!track_gpu)       params_.emplace_back(L"--no_track_gpu");
        if (track_gpu_video)  params_.emplace_back(L"--track_gpu_video");
        if (!track_input)     params_.emplace_back(L"--no_track_input");
        if (track_frame_type) params_.emplace_back(L"--track_frame_type");
        if (track_app_timing) params_.emplace_back(L"--track_app_timing");
        if (track_pc_latency) params_.emplace_back(L"--track_pc_latency");
    }

    if (!columnsOK) {
        AddTestFailure(Convert(path_).c_str(), (int) line_, "Missing required columns.");
    }

    return true;
}

void PresentMonCsv::Close()
{
    fclose(fp_);
    fp_ = nullptr;
}

bool PresentMonCsv::ReadRow()
{
    row_[0] = '\0';
    cols_.clear();

    // Read a line
    if (fgets(row_, _countof(row_), fp_) == nullptr) {
        if (ferror(fp_) != 0) {
            AddTestFailure(Convert(path_).c_str(), (int) line_, "File read error");
        }
        return false;
    }

    line_ += 1;

    // Split line into columns, skipping leading/trailing whitespace
    auto p0 = row_;
    for (; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
    for (auto p = p0; ; ++p) {
        auto ch = *p;
        if (ch == ',' || ch == '\0') {
            *p = '\0';
            cols_.push_back(p0);
            for (p0 = p + 1; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
            for (auto q = p - 1; *q == ' ' || *q == '\t' || *q == '\n' || *q == '\r'; --q) *q = '\0';
            if (ch == '\0') break;
        }
    }

    // Hard-code some per-row validation

    auto idxFrameTime = headerColumnIndex_[Header_FrameTime];
    auto idxCPUBusy   = headerColumnIndex_[Header_CPUBusy];
    auto idxCPUWait   = headerColumnIndex_[Header_CPUWait];
    if (idxFrameTime != SIZE_MAX && idxCPUBusy != SIZE_MAX && idxCPUWait != SIZE_MAX) {
        auto delta = strtod(cols_[idxFrameTime], nullptr) -
                     strtod(cols_[idxCPUBusy], nullptr) -
                     strtod(cols_[idxCPUWait], nullptr);
        if (delta <= -0.0001 || delta >= 0.0001) {
            AddTestFailure(__FILE__, __LINE__, "Invalid FrameTime: %s != %s + %s (%lf)", cols_[idxFrameTime],
                                                                                         cols_[idxCPUBusy],
                                                                                         cols_[idxCPUWait],
                                                                                         delta);
            return false;
        }
    }

    auto idxGPUTime = headerColumnIndex_[Header_GPUTime];
    auto idxGPUBusy = headerColumnIndex_[Header_GPUBusy];
    auto idxGPUWait = headerColumnIndex_[Header_GPUWait];
    if (idxGPUTime != SIZE_MAX && idxGPUBusy != SIZE_MAX && idxGPUWait != SIZE_MAX) {
        auto delta = strtod(cols_[idxGPUTime], nullptr) -
                     strtod(cols_[idxGPUBusy], nullptr) -
                     strtod(cols_[idxGPUWait], nullptr);
        auto idxVideoBusy = headerColumnIndex_[Header_VideoBusy];
        if (idxVideoBusy != SIZE_MAX) {
            delta -= strtod(cols_[idxVideoBusy], nullptr);
        }
        if (delta <= -0.0001 || delta >= 0.0001) {
            AddTestFailure(__FILE__, __LINE__, "Invalid GPUTime: %s != %s + %s + %s (%lf)", cols_[idxGPUTime],
                                                                                            cols_[idxGPUBusy],
                                                                                            cols_[idxGPUWait],
                                                                                            idxVideoBusy == SIZE_MAX ? "0" : cols_[idxVideoBusy],
                                                                                            delta);
            return false;
        }
    }

    auto idxDisplayedTime        = headerColumnIndex_[Header_DisplayedTime];
    auto idxDisplayLatency       = headerColumnIndex_[Header_DisplayLatency];
    auto idxClickToPhotonLatency = headerColumnIndex_[Header_ClickToPhotonLatency];
    if (idxDisplayedTime != SIZE_MAX && idxDisplayLatency != SIZE_MAX) {
        auto DisplayedTime  = cols_[idxDisplayedTime];
        auto DisplayLatency = cols_[idxDisplayLatency];
        if (strcmp(DisplayedTime, "NA") == 0 || strcmp(DisplayLatency, "NA") == 0) {
            if (strcmp(DisplayedTime, "NA") != 0 || strcmp(DisplayLatency, "NA") != 0) {
                AddTestFailure(__FILE__, __LINE__, "    Invalid display metrics: %s, %s", DisplayedTime, DisplayLatency);
                return false;
            }

            if (idxClickToPhotonLatency != SIZE_MAX && strcmp(cols_[idxClickToPhotonLatency], "NA") != 0) {
                AddTestFailure(__FILE__, __LINE__, "    Invalid ClickToPhotonLatency when not displayed: %s", cols_[idxClickToPhotonLatency]);
                return false;
            }
        }
    }

    return true;
}

size_t PresentMonCsv::GetColumnIndex(char const* header) const
{
    auto h = FindHeader(header);
    return h < KnownHeaderCount ? headerColumnIndex_[h] : SIZE_MAX;
}

PresentMon::PresentMon()
    : cmdline_()
    , csvArgSet_(false)
{
    cmdline_ += L'\"';
    cmdline_ += exePath_;
    cmdline_ += L"\" --no_console_stats";
}

PresentMon::~PresentMon()
{
    if (::testing::Test::HasFailure()) {
        printf("%ls\n", cmdline_.c_str());
    }
}

void PresentMon::AddEtlPath(std::wstring const& etlPath)
{
    cmdline_ += L" --etl_file \"";
    cmdline_ += etlPath;
    cmdline_ += L'\"';
}

void PresentMon::AddCsvPath(std::wstring const& csvPath)
{
    EXPECT_FALSE(csvArgSet_);
    cmdline_ += L" --output_file \"";
    cmdline_ += csvPath;
    cmdline_ += L'\"';
    csvArgSet_ = true;

    // Delete the file if it exists.  Otherwise, PresentMon may not output
    // anything and any previous content will remain.
    DeleteFile(csvPath.c_str());
}

void PresentMon::Add(wchar_t const* args)
{
    cmdline_ += L' ';
    cmdline_ += args;
}

void PresentMon::Start(char const* file, int line)
{
    if (!csvArgSet_) {
        cmdline_ += L" --no_csv";
        csvArgSet_ = true;
    }

    STARTUPINFO si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    if (CreateProcess(nullptr, &cmdline_[0], nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, (PROCESS_INFORMATION*) this) == 0) {
        AddTestFailure(file, line, "Failed to start PresentMon");
    }
}

bool PresentMon::IsRunning(DWORD timeoutMilliseconds) const
{
    return WaitForSingleObject(hProcess, timeoutMilliseconds) == WAIT_TIMEOUT;
}

void PresentMon::ExpectExited(char const* file, int line, DWORD timeoutMilliseconds, DWORD expectedExitCode)
{
    auto isRunning = IsRunning(timeoutMilliseconds);
    if (isRunning) {
        AddTestFailure(file, line, "PresentMon still running after %ums", timeoutMilliseconds);

        TerminateProcess(hProcess, 0);
        WaitForSingleObject(hProcess, INFINITE);
    } else {
        DWORD exitCode = 0;
        GetExitCodeProcess(hProcess, &exitCode);

        if (exitCode != expectedExitCode) {
            AddTestFailure(file, line, "Unexpected PresentMon exit code: %d (expecting %d)", exitCode, expectedExitCode);
        }
    }

    CloseHandle(hProcess);
    CloseHandle(hThread);
}
