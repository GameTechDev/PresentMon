// Copyright (C) 2019-2021,2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMon.hpp"

#include <fcntl.h>
#include <io.h>

static COORD gWritePosition{};
static bool gStderrIsConsole = false;
static bool gStdoutIsConsole = false;

static bool InitConsole(DWORD stdHandle, FILE* fp)
{
    HANDLE console = GetStdHandle(stdHandle);
    if (console == INVALID_HANDLE_VALUE) {
        return false;
    }
    if (console == NULL) {
        return false;
    }

    DWORD mode;
    if (GetConsoleMode(console, &mode)) {
        _setmode(_fileno(fp), _O_U16TEXT);
        return true;
    }

    DWORD type = GetFileType(console);
    if ((type & 0x3) == FILE_TYPE_DISK) {
        LARGE_INTEGER size{};
        GetFileSizeEx(console, &size);

        if (size.QuadPart == 0) {
            uint16_t bom = 0xfeff;
            DWORD w;
            WriteFile(console, &bom, 2, &w, nullptr);
        }

        _setmode(_fileno(fp), _O_U16TEXT);
    }

    return false;
}

bool StdOutIsConsole()
{
    return gStdoutIsConsole;
}

void InitializeConsole()
{
    gStderrIsConsole = InitConsole(STD_ERROR_HANDLE, stderr);
    gStdoutIsConsole = InitConsole(STD_OUTPUT_HANDLE, stdout);
}

void FinalizeConsole()
{
    if (BeginConsoleUpdate()) {
        EndConsoleUpdate();
    }
}

static uint32_t VPrint(wchar_t* buffer, uint32_t bufferCount, wchar_t const* format, va_list val)
{
    assert(bufferCount > 0);

    int r = _vsnwprintf_s(buffer, bufferCount, _TRUNCATE, format, val);

    uint32_t numChars;
    if (r >= 0) {
        numChars = (uint32_t) r;
    } else {
        numChars = bufferCount - 1;

        if (numChars > 0) {
            uint32_t i = numChars;
            auto formatLen = wcslen(format);
            if (formatLen > 0 && format[formatLen - 1] == L'\n') {
                buffer[--i] = L'\n';
            }

            for (uint32_t j = 0; j < std::min(i, 3u); ++j) {
                buffer[--i] = L'.';
            }
        }
    }

    return numChars;
}

static void VConsolePrint(wchar_t const* format, va_list val, bool newLine)
{
    wchar_t buffer[256];
    uint32_t numChars = VPrint(buffer, _countof(buffer), format, val);

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO info = {};
    GetConsoleScreenBufferInfo(console, &info);

    COORD endPosition;
    endPosition.Y = gWritePosition.Y + (SHORT) ((numChars + gWritePosition.X) / info.dwSize.X);
    endPosition.X = (numChars + gWritePosition.X) % info.dwSize.X;

    SHORT finalY = endPosition.Y + (newLine ? 1 : 0);
    if (finalY >= info.dwSize.Y) {
        SHORT deltaY = finalY - info.dwSize.Y + 1;

        SMALL_RECT rect;
        rect.Left   = 0;
        rect.Top    = deltaY;
        rect.Right  = info.dwSize.X - 1;
        rect.Bottom = info.dwSize.Y - deltaY - 1;

        COORD dstPos;
        dstPos.X = 0;
        dstPos.Y = 0;

        CHAR_INFO fill;
        fill.Char.UnicodeChar = L' ';
        fill.Attributes = info.wAttributes;

        ScrollConsoleScreenBufferW(console, &rect, nullptr, dstPos, &fill);

        info.dwCursorPosition.Y -= deltaY;
        gWritePosition.Y        -= deltaY;
        endPosition.Y           -= deltaY;

        SetConsoleCursorPosition(console, info.dwCursorPosition);
    }

    if (info.dwCursorPosition.Y >= info.srWindow.Top &&
        info.dwCursorPosition.Y <= info.srWindow.Bottom &&
        finalY > info.srWindow.Bottom) {
        SHORT deltaY = finalY - info.srWindow.Bottom;

        SMALL_RECT rect;
        rect.Left   = 0;
        rect.Top    = deltaY;
        rect.Right  = 0;
        rect.Bottom = deltaY;

        SetConsoleWindowInfo(console, FALSE, &rect);
    }

    DWORD numCharsWritten = 0;
    WriteConsoleOutputCharacterW(console, buffer, numChars, gWritePosition, &numCharsWritten);

    if (newLine) {
        numChars = info.dwSize.X - endPosition.X;
        FillConsoleOutputCharacterW(console, L' ', numChars, endPosition, &numCharsWritten);

        gWritePosition.X = 0;
        gWritePosition.Y = endPosition.Y + 1;
    } else {
        gWritePosition = endPosition;
    }
}

void ConsolePrint(wchar_t const* format, ...)
{
    va_list val;
    va_start(val, format);
    VConsolePrint(format, val, false);
    va_end(val);
}

void ConsolePrintLn(wchar_t const* format, ...)
{
    va_list val;
    va_start(val, format);
    VConsolePrint(format, val, true);
}

bool BeginConsoleUpdate()
{
    if (!gStdoutIsConsole) {
        return false;
    }

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO info = {};
    GetConsoleScreenBufferInfo(console, &info);

    gWritePosition = info.dwCursorPosition;

    return true;
}

void EndConsoleUpdate()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO info = {};
    GetConsoleScreenBufferInfo(console, &info);

    COORD dstPos;
    dstPos.X = 0;
    dstPos.Y = 0;

    COORD dstSize;
    dstSize.X = info.dwSize.X;
    dstSize.Y = 4;  // 2 should be enough to catch any presentmon stats line, but 4 is more robust
                    // to printing with concurrent resizing of the console.

    CHAR_INFO* buffer = new CHAR_INFO [dstSize.X * dstSize.Y];

    COORD srcPos;
    srcPos.X = 0;
    srcPos.Y = gWritePosition.Y;

    SMALL_RECT rect;
    for ( ; srcPos.Y < info.dwSize.Y; srcPos.Y += dstSize.Y) {
        rect.Left   = 0;
        rect.Top    = srcPos.Y;
        rect.Right  = dstSize.X - 1;
        rect.Bottom = srcPos.Y + dstSize.Y - 1;
        ReadConsoleOutputW(console, buffer, dstSize, dstPos, &rect);

        uint32_t numChars = (uint32_t) (rect.Bottom - rect.Top + 1) * (rect.Right - rect.Left + 1);

        bool clear = false;
        for (uint32_t i = 0; i < numChars; ++i) {
            if (buffer[i].Char.UnicodeChar != L' ') {
                clear = true;
                break;
            }
        }

        if (clear) {
            DWORD numCharsWritten;
            FillConsoleOutputCharacterW(console, L' ', numChars, srcPos, &numCharsWritten);
            assert(numChars == numCharsWritten);
        } else {
            break;
        }
    }

    delete[] buffer;
}

void UpdateConsole(uint32_t processId, ProcessInfo const& processInfo)
{
    auto const& args = GetCommandLineArgs();

    // Don't display non-target or empty processes
    if (!processInfo.mIsTargetProcess ||
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

        auto const& present0 = *chain.mPresentHistory[(chain.mNextPresentIndex - chain.mPresentHistoryCount) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto const& presentN = *chain.mPresentHistory[(chain.mNextPresentIndex - 1) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto cpuAvg = TimestampDeltaToSeconds(presentN.PresentStartTime - present0.PresentStartTime) / (chain.mPresentHistoryCount - 1);
        auto gpuAvg = 0.0;
        auto dspAvg = 0.0;
        auto latAvg = 0.0;

        PresentEvent* displayN = nullptr;
        if (args.mTrackDisplay) {
            uint64_t display0ScreenTime = 0;
            uint64_t gpuSum = 0;
            uint64_t latSum = 0;
            uint32_t displayCount = 0;
            for (uint32_t i = 0; i < chain.mPresentHistoryCount; ++i) {
                auto const& p = chain.mPresentHistory[(chain.mNextPresentIndex - chain.mPresentHistoryCount + i) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];

                gpuSum += p->GPUDuration;

                if (p->FinalState == PresentResult::Presented) {
                    if (displayCount == 0) {
                        display0ScreenTime = p->ScreenTime;
                    }
                    displayN = p.get();
                    latSum += p->ScreenTime - p->PresentStartTime;
                    displayCount += 1;
                }
            }

            gpuAvg = TimestampDeltaToSeconds(gpuSum) / (chain.mPresentHistoryCount - 1);

            if (displayCount >= 2) {
                dspAvg = TimestampDeltaToSeconds(displayN->ScreenTime - display0ScreenTime) / (displayCount - 1);
            }

            if (displayCount >= 1) {
                latAvg = TimestampDeltaToSeconds(latSum) / displayCount;
            }
        }

        if (empty) {
            empty = false;
            ConsolePrintLn(L"%s[%d]:", processInfo.mModuleName.c_str(), processId);
        }

        ConsolePrint(L"    %016llX (%hs): SyncInterval=%d Flags=%d CPU%hs%hs=%.2lf",
            address,
            RuntimeToString(presentN.Runtime),
            presentN.SyncInterval,
            presentN.PresentFlags,
            gpuAvg > 0.0 ? "/GPU" : "",
            dspAvg > 0.0 ? "/Display" : "",
            1000.0 * cpuAvg);

        if (gpuAvg > 0.0) ConsolePrint(L"/%.2lf", 1000.0 * gpuAvg);
        if (dspAvg > 0.0) ConsolePrint(L"/%.2lf", 1000.0 * dspAvg);

        ConsolePrint(L"ms (%.1lf", 1.0 / cpuAvg);
        if (gpuAvg > 0.0) ConsolePrint(L"/%.1lf", 1.0 / gpuAvg);
        if (dspAvg > 0.0) ConsolePrint(L"/%.1lf", 1.0 / dspAvg);
        ConsolePrint(L" fps)");

        if (latAvg > 0.0) {
            ConsolePrint(L" latency=%.2lfms", 1000.0 * latAvg);
        }

        if (displayN != nullptr) {
            ConsolePrint(L" %hs", PresentModeToString(displayN->PresentMode));
        }

        ConsolePrintLn(L"");
    }

    if (!empty) {
        ConsolePrintLn(L"");
    }
}

static int PrintColor(WORD color, wchar_t const* format, va_list val)
{
    #ifndef NDEBUG
    {
        wchar_t buffer[64];
        VPrint(buffer, _countof(buffer), format, val);
        OutputDebugStringW(buffer);
    }
    #endif

    wchar_t* pformat = (wchar_t*) format;

    HANDLE console = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info = {};
    auto setColor = console != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(console, &info) != 0;
    if (setColor) {
        auto bg = info.wAttributes & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
        if (bg == 0) {
            color |= FOREGROUND_INTENSITY;
        }
        SetConsoleTextAttribute(console, WORD(bg | color));

        auto formatLen = wcslen(format);
        if (formatLen > 0 && format[formatLen - 1] == L'\n') {
            auto size = sizeof(wchar_t) * formatLen;
            pformat = (wchar_t*) malloc(size);
            memcpy(pformat, format, size);
            pformat[formatLen - 1] = '\0';
        }
    }

    int c = vfwprintf(stderr, pformat, val);

    if (setColor) {
        SetConsoleTextAttribute(console, info.wAttributes);

        if (pformat != format) {
            c += fwprintf(stderr, L"\n");
            free(pformat);
        }
    }

    return c;
}

int PrintWarning(wchar_t const* format, ...)
{
    va_list val;
    va_start(val, format);
    int c = PrintColor(FOREGROUND_RED | FOREGROUND_GREEN, format, val);
    va_end(val);
    return c;
}

int PrintError(wchar_t const* format, ...)
{
    va_list val;
    va_start(val, format);
    int c = PrintColor(FOREGROUND_RED, format, val);
    va_end(val);
    return c;
}
