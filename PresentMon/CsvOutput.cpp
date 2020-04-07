/*
Copyright 2017-2019 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "PresentMon.hpp"

static OutputCsv gSingleOutputCsv = {};
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
    case PresentMode::Composed_Composition_Atlas: return "Composed: Composition Atlas";
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

const char* FinalStateToDroppedString(PresentResult res)
{
    switch (res) {
    case PresentResult::Presented: return "0";
    case PresentResult::Error: return "Error";
    default: return "1";
    }
}

static void WriteCsvHeader(FILE* fp)
{
    auto const& args = GetCommandLineArgs();

    fprintf(fp, "Application,ProcessID,SwapChainAddress,Runtime,SyncInterval,PresentFlags");
    if (args.mVerbosity > Verbosity::Simple) {
        fprintf(fp, ",AllowsTearing,PresentMode");
    }
    if (args.mVerbosity >= Verbosity::Verbose) {
        fprintf(fp, ",WasBatched,DwmNotified");
    }
    fprintf(fp, ",Dropped,TimeInSeconds,MsBetweenPresents");
    if (args.mVerbosity > Verbosity::Simple) {
        fprintf(fp, ",MsBetweenDisplayChange");
    }
    fprintf(fp, ",MsInPresentAPI");
    if (args.mVerbosity > Verbosity::Simple) {
        fprintf(fp, ",MsUntilRenderComplete,MsUntilDisplayed");
    }
    if (args.mOutputQpcTime) {
        fprintf(fp, ",QPCTime");
    }
    fprintf(fp, "\n");
}

void UpdateCsv(ProcessInfo* processInfo, SwapChainData const& chain, PresentEvent const& p)
{
    auto const& args = GetCommandLineArgs();

    // Don't output dropped frames (if requested).
    auto presented = p.FinalState == PresentResult::Presented;
    if (args.mExcludeDropped && !presented) {
        return;
    }

    // Early return if not outputing to CSV.
    auto fp = GetOutputCsv(processInfo).mFile;
    if (fp == nullptr) {
        return;
    }

    // Look up the last present event in the swapchain's history.  We need at
    // least two presents to compute frame statistics.
    if (chain.mPresentHistoryCount == 0) {
        return;
    }

    auto lastPresented = chain.mPresentHistory[(chain.mNextPresentIndex - 1) % SwapChainData::PRESENT_HISTORY_MAX_COUNT].get();

    // Compute frame statistics.
    double timeInSeconds          = QpcToSeconds(p.QpcTime);
    double msBetweenPresents      = 1000.0 * QpcDeltaToSeconds(p.QpcTime - lastPresented->QpcTime);
    double msInPresentApi         = 1000.0 * QpcDeltaToSeconds(p.TimeTaken);
    double msUntilRenderComplete  = 0.0;
    double msUntilDisplayed       = 0.0;
    double msBetweenDisplayChange = 0.0;

    if (args.mVerbosity > Verbosity::Simple) {
        if (p.ReadyTime > 0) {
            msUntilRenderComplete = 1000.0 * QpcDeltaToSeconds(p.ReadyTime - p.QpcTime);
        }
        if (presented) {
            msUntilDisplayed = 1000.0 * QpcDeltaToSeconds(p.ScreenTime - p.QpcTime);

            if (chain.mLastDisplayedPresentIndex > 0) {
                auto lastDisplayed = chain.mPresentHistory[chain.mLastDisplayedPresentIndex % SwapChainData::PRESENT_HISTORY_MAX_COUNT].get();
                msBetweenDisplayChange = 1000.0 * QpcDeltaToSeconds(p.ScreenTime - lastDisplayed->ScreenTime);
            }
        }
    }

    // Output in CSV format
    fprintf(fp, "%s,%d,0x%016llX,%s,%d,%d", processInfo->mModuleName.c_str(), p.ProcessId, p.SwapChainAddress,
        RuntimeToString(p.Runtime), p.SyncInterval, p.PresentFlags);
    if (args.mVerbosity > Verbosity::Simple) {
        fprintf(fp, ",%d,%s", p.SupportsTearing, PresentModeToString(p.PresentMode));
    }
    if (args.mVerbosity >= Verbosity::Verbose) {
        fprintf(fp, ",%d,%d", p.WasBatched, p.DwmNotified);
    }
    fprintf(fp, ",%s,%.6lf,%.3lf", FinalStateToDroppedString(p.FinalState), timeInSeconds, msBetweenPresents);
    if (args.mVerbosity > Verbosity::Simple) {
        fprintf(fp, ",%.3lf", msBetweenDisplayChange);
    }
    fprintf(fp, ",%.3lf", msInPresentApi);
    if (args.mVerbosity > Verbosity::Simple) {
        fprintf(fp, ",%.3lf,%.3lf", msUntilRenderComplete, msUntilDisplayed);
    }
    if (args.mOutputQpcTime) {
        fprintf(fp, ",%llu", p.QpcTime);
    }
    fprintf(fp, "\n");
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
static void GenerateFilename(char const* processName, char* path)
{
    auto const& args = GetCommandLineArgs();

    char ext[_MAX_EXT];
    int pathLength = MAX_PATH;

#define ADD_TO_PATH(...) do { \
    if (path != nullptr) { \
        auto result = _snprintf_s(path, pathLength, _TRUNCATE, __VA_ARGS__); \
        if (result == -1) path = nullptr; else { path += result; pathLength -= result; } \
    } \
} while (0)

    // Generate base filename.
    if (args.mOutputCsvFileName) {
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char name[_MAX_FNAME];
        _splitpath_s(args.mOutputCsvFileName, drive, dir, name, ext);
        ADD_TO_PATH("%s%s%s", drive, dir, name);
    } else {
        struct tm tm;
        time_t time_now = time(NULL);
        localtime_s(&tm, &time_now);
        ADD_TO_PATH("PresentMon-%4d-%02d-%02dT%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        strcpy_s(ext, ".csv");
    }

    // Append -PROCESSNAME if applicable.
    if (processName != nullptr) {
        ADD_TO_PATH("-%s", processName);
    }

    // Append -INDEX if applicable.
    if (args.mHotkeySupport) {
        ADD_TO_PATH("-%d", gRecordingCount);
    }

    // Append extension.
    ADD_TO_PATH("%s", ext);
}

static OutputCsv CreateOutputCsv(char const* processName)
{
    auto const& args = GetCommandLineArgs();

    OutputCsv outputCsv = {};

    if (args.mOutputCsvToStdout) {
        outputCsv.mFile = stdout;
        outputCsv.mWmrFile = nullptr;       // WMR disallowed if -output_stdout
    } else {
        char path[MAX_PATH];
        GenerateFilename(processName, path);

        fopen_s(&outputCsv.mFile, path, "wb");

        if (args.mIncludeWindowsMixedReality) {
            outputCsv.mWmrFile = CreateLsrCsvFile(path);
        }
    }

    if (outputCsv.mFile != nullptr) {
        WriteCsvHeader(outputCsv.mFile);
    }

    return outputCsv;
}

OutputCsv GetOutputCsv(ProcessInfo* processInfo)
{
    auto const& args = GetCommandLineArgs();

    // TODO: If fopen_s() fails to open mFile, we'll just keep trying here
    // every time PresentMon wants to output to the file. We should detect the
    // failure and generate an error instead.

    if (args.mOutputCsvToFile && processInfo->mOutputCsv.mFile == nullptr) {
        if (args.mMultiCsv) {
            processInfo->mOutputCsv = CreateOutputCsv(processInfo->mModuleName.c_str());
        } else {
            if (gSingleOutputCsv.mFile == nullptr) {
                gSingleOutputCsv = CreateOutputCsv(nullptr);
            }

            processInfo->mOutputCsv = gSingleOutputCsv;
        }
    }

    return processInfo->mOutputCsv;
}

void CloseOutputCsv(ProcessInfo* processInfo)
{
    auto const& args = GetCommandLineArgs();

    // If processInfo is nullptr, it means we should operate on the global
    // single output CSV.
    //
    // We only actually close the FILE if we own it (we're operating on the
    // single global output CSV, or we're writing a CSV per process) and it's
    // not stdout.
    OutputCsv* csv = nullptr;
    bool closeFile = false;
    if (processInfo == nullptr) {
        csv = &gSingleOutputCsv;
        closeFile = !args.mOutputCsvToStdout;
    } else {
        csv = &processInfo->mOutputCsv;
        closeFile = !args.mOutputCsvToStdout && args.mMultiCsv;
    }

    if (closeFile) {
        if (csv->mFile != nullptr) {
            fclose(csv->mFile);
        }
        if (csv->mWmrFile != nullptr) {
            fclose(csv->mWmrFile);
        }
    }

    csv->mFile = nullptr;
    csv->mWmrFile = nullptr;
}

