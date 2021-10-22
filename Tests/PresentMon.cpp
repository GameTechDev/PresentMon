// Copyright (C) 2020-2021 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMonTests.h"

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

size_t FindHeader(
    char const* header,
    uint32_t* requiredCount,
    uint32_t* trackDisplayCount,
    uint32_t* trackDebugCount)
{
    size_t idx = 0;
    for (size_t i = 0; i < _countof(PresentMonCsv::REQUIRED_HEADER); ++i, ++idx) {
        if (strcmp(header, PresentMonCsv::REQUIRED_HEADER[i]) == 0) {
            *requiredCount += 1;
            return idx;
        }
    }
    for (size_t i = 0; i < _countof(PresentMonCsv::TRACK_DISPLAY_HEADER); ++i, ++idx) {
        if (strcmp(header, PresentMonCsv::TRACK_DISPLAY_HEADER[i]) == 0) {
            *trackDisplayCount += 1;
            return idx;
        }
    }
    for (size_t i = 0; i < _countof(PresentMonCsv::TRACK_DEBUG_HEADER); ++i, ++idx) {
        if (strcmp(header, PresentMonCsv::TRACK_DEBUG_HEADER[i]) == 0) {
            *trackDebugCount += 1;
            return idx;
        }
    }
    for (size_t i = 0; i < _countof(PresentMonCsv::OPT_HEADER); ++i, ++idx) {
        if (strcmp(header, PresentMonCsv::OPT_HEADER[i]) == 0) {
            return idx;
        }
    }
    return SIZE_MAX;
}

}

PresentMonCsv::PresentMonCsv()
    : line_(0)
    , fp_(nullptr)
    , trackDisplay_(false)
    , trackDebug_(false)
{
}

bool PresentMonCsv::Open(char const* file, int line, std::wstring const& path)
{
    memset(headerColumnIndex_, 0xff, sizeof(headerColumnIndex_));
    cols_.clear();
    path_ = path;
    line_ = 0;

    if (_wfopen_s(&fp_, path.c_str(), L"rb")) {
        AddTestFailure(file, line, "Failed to open file: %ls", path.c_str());
        return false;
    }

    // Remove UTF-8 marker if there is one.
    long int startOfs = 0;
    if (fread(row_, 3, 1, fp_) == 1 &&
        row_[0] == -17 &&
        row_[1] == -69 &&
        row_[2] == -65) {
        startOfs = 3;
    }
    fseek(fp_, startOfs, SEEK_SET);

    // Read the header and ensure required columns are present
    ReadRow();

    uint32_t requiredCount = 0;
    uint32_t trackDisplayCount = 0;
    uint32_t trackDebugCount = 0;
    for (uint32_t i = 0, n = (uint32_t) cols_.size(); i < n; ++i) {
        auto idx = FindHeader(cols_[i], &requiredCount, &trackDisplayCount, &trackDebugCount);
        if (idx == SIZE_MAX) {
            AddTestFailure(Convert(path_).c_str(), (int) line_, "Unrecognised column: %s", cols_[i]);
        } else if (headerColumnIndex_[idx] != SIZE_MAX) {
            AddTestFailure(Convert(path_).c_str(), (int) line_, "Duplicate column: %s", cols_[i]);
        } else {
            headerColumnIndex_[idx] = i;
        }
    }

    trackDisplay_ = trackDisplayCount > 0;
    trackDebug_ = trackDebugCount > 0;

    if (requiredCount != _countof(REQUIRED_HEADER) ||
        (trackDisplay_ && trackDisplayCount != _countof(TRACK_DISPLAY_HEADER)) ||
        (trackDebug_   && trackDebugCount   != _countof(TRACK_DEBUG_HEADER))) {
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

    return true;
}

size_t PresentMonCsv::GetColumnIndex(char const* header) const
{
    uint32_t na = 0;
    auto headerIdx = FindHeader(header, &na, &na, &na);
    return headerIdx == SIZE_MAX
        ? SIZE_MAX
        : headerColumnIndex_[headerIdx];
}

PresentMon::PresentMon()
    : cmdline_()
    , csvArgSet_(false)
{
    cmdline_ += L'\"';
    cmdline_ += exePath_;
    cmdline_ += L"\" -no_top";
}

PresentMon::~PresentMon()
{
    if (::testing::Test::HasFailure()) {
        printf("%ls\n", cmdline_.c_str());
    }
}

void PresentMon::AddEtlPath(std::wstring const& etlPath)
{
    cmdline_ += L" -etl_file \"";
    cmdline_ += etlPath;
    cmdline_ += L'\"';
}

void PresentMon::AddCsvPath(std::wstring const& csvPath)
{
    EXPECT_FALSE(csvArgSet_);
    cmdline_ += L" -output_file \"";
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
        cmdline_ += L" -no_csv";
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
