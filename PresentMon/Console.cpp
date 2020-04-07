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

static HANDLE gConsoleHandle;
static char gConsoleWriteBuffer[8 * 1024];
static uint32_t gConsoleWriteBufferIndex;
static uint32_t gConsolePrevWriteBufferSize;
static SHORT gConsoleTop;
static SHORT gConsoleWidth;
static SHORT gConsoleBufferHeight;
static bool gConsoleFirstCommit;

bool InitializeConsole()
{
    gConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (gConsoleHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    CONSOLE_SCREEN_BUFFER_INFO info = {};
    if (GetConsoleScreenBufferInfo(gConsoleHandle, &info) == 0) {
        return false;
    }

    gConsoleTop = info.dwCursorPosition.Y;
    gConsoleWidth = info.srWindow.Right - info.srWindow.Left + 1;
    gConsoleBufferHeight = info.dwSize.Y;
    gConsoleWriteBufferIndex = 0;
    gConsolePrevWriteBufferSize = 0;
    gConsoleFirstCommit = true;

    return true;
}

static void vConsolePrint(char const* format, va_list args)
{
    auto s = gConsoleWriteBuffer + gConsoleWriteBufferIndex;
    auto n = sizeof(gConsoleWriteBuffer) - gConsoleWriteBufferIndex;

    int r = vsnprintf(s, n, format, args);
    if (r > 0) {
        gConsoleWriteBufferIndex = min((uint32_t) (n - 1), gConsoleWriteBufferIndex + r);
    }
}

void ConsolePrint(char const* format, ...)
{
    va_list args;
    va_start(args, format);
    vConsolePrint(format, args);
    va_end(args);
}

void ConsolePrintLn(char const* format, ...)
{
    va_list args;
    va_start(args, format);
    vConsolePrint(format, args);
    va_end(args);

    auto x = gConsoleWriteBufferIndex % gConsoleWidth;
    auto s = gConsoleWidth - x;
    memset(gConsoleWriteBuffer + gConsoleWriteBufferIndex, ' ', s);
    gConsoleWriteBufferIndex += s;
}

void CommitConsole()
{
    auto sizeWritten = gConsoleWriteBufferIndex;
    auto linesWritten = (SHORT) (sizeWritten / gConsoleWidth);

    // Reset gConsoleTop on the first commit so we don't overwrite any warning
    // messages.
    auto size = sizeWritten;
    if (gConsoleFirstCommit) {
        gConsoleFirstCommit = false;

        CONSOLE_SCREEN_BUFFER_INFO info = {};
        GetConsoleScreenBufferInfo(gConsoleHandle, &info);
        gConsoleTop = info.dwCursorPosition.Y;
    } else {
        // Write some extra empty lines to make sure we clear anything from
        // last time.
        if (size < gConsolePrevWriteBufferSize) {
            memset(gConsoleWriteBuffer + size, ' ', gConsolePrevWriteBufferSize - size);
            size = gConsolePrevWriteBufferSize;
        }
    }

    // If we're at the end of the console buffer, issue some new lines to make
    // some space.
    auto maxCursorY = gConsoleBufferHeight - linesWritten;
    if (gConsoleTop > maxCursorY) {
        COORD bottom = { 0, gConsoleBufferHeight - 1 };
        SetConsoleCursorPosition(gConsoleHandle, bottom);
        printf("\n");
        for (--gConsoleTop; gConsoleTop > maxCursorY; --gConsoleTop) {
            printf("\n");
        }
    }

    // Write to the console.
    DWORD dwCharsWritten = 0;
    COORD cursor = { 0, gConsoleTop };
    WriteConsoleOutputCharacterA(gConsoleHandle, gConsoleWriteBuffer, (DWORD) size, cursor, &dwCharsWritten);

    // Put the cursor at the end of the written text.
    cursor.Y += linesWritten;
    SetConsoleCursorPosition(gConsoleHandle, cursor);

    // Update console info in case it was resized.
    CONSOLE_SCREEN_BUFFER_INFO info = {};
    GetConsoleScreenBufferInfo(gConsoleHandle, &info);
    gConsoleWidth = info.srWindow.Right - info.srWindow.Left + 1;
    gConsoleBufferHeight = info.dwSize.Y;
    gConsoleWriteBufferIndex = 0;
    gConsolePrevWriteBufferSize = sizeWritten;
}

void UpdateConsole(uint32_t processId, ProcessInfo const& processInfo)
{
    auto const& args = GetCommandLineArgs();

    // Don't display non-target or empty processes
    if (!processInfo.mTargetProcess ||
        processInfo.mModuleName.empty() ||
        processInfo.mSwapChain.empty()) {
        return;
    }

    auto empty = true;

    for (auto const& pair : processInfo.mSwapChain) {
        auto address = pair.first;
        auto const& chain = pair.second;

        // Only show swapchain data if there at least two presents in the
        // history.
        if (chain.mPresentHistoryCount < 2) {
            continue;
        }

        if (empty) {
            empty = false;
            ConsolePrintLn("%s[%d]:", processInfo.mModuleName.c_str(), processId);
        }

        auto const& present0 = *chain.mPresentHistory[(chain.mNextPresentIndex - chain.mPresentHistoryCount) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto const& presentN = *chain.mPresentHistory[(chain.mNextPresentIndex - 1) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto cpuAvg = QpcDeltaToSeconds(presentN.QpcTime - present0.QpcTime) / (chain.mPresentHistoryCount - 1);

        ConsolePrint("    %016llX (%s): SyncInterval=%d Flags=%d %.2lf ms/frame (%.1lf fps",
            address,
            RuntimeToString(presentN.Runtime),
            presentN.SyncInterval,
            presentN.PresentFlags,
            1000.0 * cpuAvg,
            1.0 / cpuAvg);

        size_t displayCount = 0;
        uint64_t latencySum = 0;
        uint64_t display0ScreenTime = 0;
        PresentEvent* displayN = nullptr;
        if (args.mVerbosity > Verbosity::Simple) {
            for (uint32_t i = 0; i < chain.mPresentHistoryCount; ++i) {
                auto const& p = chain.mPresentHistory[(chain.mNextPresentIndex - chain.mPresentHistoryCount + i) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
                if (p->FinalState == PresentResult::Presented) {
                    if (displayCount == 0) {
                        display0ScreenTime = p->ScreenTime;
                    }
                    displayN = p.get();
                    latencySum += p->ScreenTime - p->QpcTime;
                    displayCount += 1;
                }
            }
        }

        if (displayCount >= 2) {
            ConsolePrint(", %.1lf fps displayed", (double) (displayCount - 1) / QpcDeltaToSeconds(displayN->ScreenTime - display0ScreenTime));
        }

        if (displayCount >= 1) {
            ConsolePrint(", %.2lf ms latency", 1000.0 * QpcDeltaToSeconds(latencySum) / displayCount);
        }

        ConsolePrint(")");

        if (displayCount > 0) {
            ConsolePrint(" %s", PresentModeToString(displayN->PresentMode));
        }

        ConsolePrintLn("");
    }

    if (!empty) {
        ConsolePrintLn("");
    }
}

