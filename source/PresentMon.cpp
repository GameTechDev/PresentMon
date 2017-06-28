/*
Copyright 2017 Intel Corporation

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

#include <algorithm>
#include <shlwapi.h>

#include "TraceSession.hpp"
#include "PresentMon.hpp"

template <typename Map, typename F>
static void map_erase_if(Map& m, F pred)
{
    typename Map::iterator i = m.begin();
    while ((i = std::find_if(i, m.end(), pred)) != m.end()) {
        m.erase(i++);
    }
}

static void SetConsoleText(const char *text)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    enum { MAX_BUFFER = 16384 };
    char buffer[16384];
    int bufferSize = 0;
    auto write = [&](char ch) {
        if (bufferSize < MAX_BUFFER) {
            buffer[bufferSize++] = ch;
        }
    };

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    int oldBufferSize = int(csbi.dwSize.X * csbi.dwSize.Y);
    if (oldBufferSize > MAX_BUFFER) {
        oldBufferSize = MAX_BUFFER;
    }

    int x = 0;
    while (*text) {
        int repeat = 1;
        char ch = *text;
        if (ch == '\t') {
            ch = ' ';
            repeat = 4;
        }
        else if (ch == '\n') {
            ch = ' ';
            repeat = csbi.dwSize.X - x;
        }
        for (int i = 0; i < repeat; ++i) {
            write(ch);
            if (++x >= csbi.dwSize.X) {
                x = 0;
            }
        }
        text++;
    }

    for (int i = bufferSize; i < oldBufferSize; ++i)
    {
        write(' ');
    }

    COORD origin = { 0,0 };
    DWORD dwCharsWritten;
    WriteConsoleOutputCharacterA(
        hConsole,
        buffer,
        bufferSize,
        origin,
        &dwCharsWritten);

    SetConsoleCursorPosition(hConsole, origin);
}

static void UpdateProcessInfo_Realtime(ProcessInfo& info, uint64_t now, uint32_t thisPid)
{
    if (now - info.mLastRefreshTicks > 1000) {
        info.mLastRefreshTicks = now;
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, thisPid);
        if (h) {
            info.mProcessExists = true;

            char path[MAX_PATH] = "<error>";
            char* name = path;
            DWORD numChars = sizeof(path);
            if (QueryFullProcessImageNameA(h, 0, path, &numChars) == TRUE) {
                name = PathFindFileNameA(path);
            }
            if (info.mModuleName.compare(name) != 0) {
                info.mModuleName.assign(name);
                info.mChainMap.clear();
            }

            DWORD dwExitCode = 0;
            if (GetExitCodeProcess(h, &dwExitCode) == TRUE && dwExitCode != STILL_ACTIVE) {
                info.mProcessExists = false;
            }

            CloseHandle(h);
        } else {
            info.mChainMap.clear();
            info.mProcessExists = false;
        }
    }

    // remove chains without recent updates
    map_erase_if(info.mChainMap, [now](const std::pair<const uint64_t, SwapChainData>& entry) {
        return entry.second.IsStale(now);
    });
}

const char* PresentModeToString(PresentMode mode)
{
    switch (mode) {
    case PresentMode::Hardware_Legacy_Flip: return "Hardware: Legacy Flip";
    case PresentMode::Hardware_Legacy_Copy_To_Front_Buffer: return "Hardware: Legacy Copy to front buffer";
    case PresentMode::Hardware_Direct_Flip: return "Hardware: Direct Flip";
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

void AddPresent(PresentMonData& pm, PresentEvent& p, uint64_t now, uint64_t perfFreq)
{
    auto& proc = pm.mProcessMap[p.ProcessId];
    if (!proc.mLastRefreshTicks && !pm.mArgs->mEtlFileName) {
        UpdateProcessInfo_Realtime(proc, now, p.ProcessId);
    }

    if (pm.mArgs->mTargetProcessName != nullptr && _stricmp(pm.mArgs->mTargetProcessName, proc.mModuleName.c_str()) != 0) {
        // process name does not match
        return;
    }
    if (pm.mArgs->mTargetPid && p.ProcessId != pm.mArgs->mTargetPid) {
        return;
    }

    if (pm.mArgs->mTerminateOnProcExit && !proc.mTerminationProcess) {
        proc.mTerminationProcess = true;
        ++pm.mTerminationProcessCount;
    }

    auto& chain = proc.mChainMap[p.SwapChainAddress];
    chain.AddPresentToSwapChain(p);

    if (pm.mOutputFile && (p.FinalState == PresentResult::Presented || !pm.mArgs->mExcludeDropped)) {
        auto len = chain.mPresentHistory.size();
        auto displayedLen = chain.mDisplayedPresentHistory.size();
        if (len > 1) {
            auto& curr = chain.mPresentHistory[len - 1];
            auto& prev = chain.mPresentHistory[len - 2];
            double deltaMilliseconds = 1000 * double(curr.QpcTime - prev.QpcTime) / perfFreq;
            double deltaReady = curr.ReadyTime == 0 ? 0.0 : (1000 * double(curr.ReadyTime - curr.QpcTime) / perfFreq);
            double deltaDisplayed = curr.FinalState == PresentResult::Presented ? (1000 * double(curr.ScreenTime - curr.QpcTime) / perfFreq) : 0.0;
            double timeTakenMilliseconds = 1000 * double(curr.TimeTaken) / perfFreq;

            double timeSincePreviousDisplayed = 0.0;
            if (curr.FinalState == PresentResult::Presented && displayedLen > 1) {
                assert(chain.mDisplayedPresentHistory[displayedLen - 1].QpcTime == curr.QpcTime);
                auto& prevDisplayed = chain.mDisplayedPresentHistory[displayedLen - 2];
                timeSincePreviousDisplayed = 1000 * double(curr.ScreenTime - prevDisplayed.ScreenTime) / perfFreq;
            }

            double timeInSeconds = (double)(int64_t)(p.QpcTime - pm.mStartupQpcTime) / perfFreq;
            fprintf(pm.mOutputFile, "%s,%d,0x%016llX,%s,%d,%d",
                    proc.mModuleName.c_str(), p.ProcessId, p.SwapChainAddress, RuntimeToString(p.Runtime), curr.SyncInterval, curr.PresentFlags);
            if (pm.mArgs->mVerbosity > Verbosity::Simple)
            {
                fprintf(pm.mOutputFile, ",%d,%s", curr.SupportsTearing, PresentModeToString(curr.PresentMode));
            }
            if (pm.mArgs->mVerbosity >= Verbosity::Verbose)
            {
                fprintf(pm.mOutputFile, ",%d,%d", curr.WasBatched, curr.DwmNotified);
            }
            fprintf(pm.mOutputFile, ",%s,%.6lf,%.3lf", FinalStateToDroppedString(curr.FinalState), timeInSeconds, deltaMilliseconds);
            if (pm.mArgs->mVerbosity > Verbosity::Simple)
            {
                fprintf(pm.mOutputFile, ",%.3lf", timeSincePreviousDisplayed);
            }
            fprintf(pm.mOutputFile, ",%.3lf", timeTakenMilliseconds);
            if (pm.mArgs->mVerbosity > Verbosity::Simple)
            {
                fprintf(pm.mOutputFile, ",%.3lf,%.3lf", deltaReady, deltaDisplayed);
            }
            fprintf(pm.mOutputFile, "\n");
        }
    }

    chain.UpdateSwapChainInfo(p, now, perfFreq);
}

void PresentMon_Init(const CommandLineArgs& args, PresentMonData& pm)
{
    pm.mArgs = &args;

    if (!args.mEtlFileName)
    {
        QueryPerformanceCounter((PLARGE_INTEGER)&pm.mStartupQpcTime);
    }
    else
    {
        // Reading events from ETL file so current QPC value is irrelevant. Update this 
        // later from first event in the file.
        pm.mStartupQpcTime = 0;
    }

    if (args.mOutputFile) {
        // Figure out what file name to use:
        //    FILENAME.EXT                     If FILENAME.EXT specified on command line
        //    FILENAME-RECORD#.EXT             If FILENAME.EXT specified on command line and -hotkey used
        //    PresentMon-PROCESSNAME-TIME.csv  If targetting a process by name
        //    PresentMon-TIME.csv              Otherwise
        if (args.mOutputFileName) {
            if (!args.mHotkeySupport) {
                _snprintf_s(pm.mOutputFilePath, _TRUNCATE, "%s", args.mOutputFileName);
            } else {
                char drive[_MAX_DRIVE] = {};
                char dir[_MAX_DIR] = {};
                char name[_MAX_FNAME] = {};
                char ext[_MAX_EXT] = {};
                _splitpath_s(args.mOutputFileName, drive, dir, name, ext);
                _snprintf_s(pm.mOutputFilePath, _TRUNCATE, "%s%s%s-%d%s", drive, dir, name, args.mRecordingCount, ext);
            }
        } else {
            struct tm tm;
            time_t time_now = time(NULL);
            localtime_s(&tm, &time_now);

            if (args.mTargetProcessName == nullptr) {
                _snprintf_s(pm.mOutputFilePath, _TRUNCATE, "PresentMon-%4d-%02d-%02dT%02d%02d%02d.csv", // ISO 8601
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
            } else {
                _snprintf_s(pm.mOutputFilePath, _TRUNCATE, "PresentMon-%s-%4d-%02d-%02dT%02d%02d%02d.csv", // ISO 8601
                    args.mTargetProcessName,
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
            }
        }

        // Open output file and print CSV header
        fopen_s(&pm.mOutputFile, pm.mOutputFilePath, "w");
        if (pm.mOutputFile) {
            fprintf(pm.mOutputFile, "Application,ProcessID,SwapChainAddress,Runtime,SyncInterval,PresentFlags");
            if (pm.mArgs->mVerbosity > Verbosity::Simple)
            {
                fprintf(pm.mOutputFile, ",AllowsTearing,PresentMode");
            }
            if (pm.mArgs->mVerbosity >= Verbosity::Verbose)
            {
                fprintf(pm.mOutputFile, ",WasBatched,DwmNotified");
            }
            fprintf(pm.mOutputFile, ",Dropped,TimeInSeconds,MsBetweenPresents");
            if (pm.mArgs->mVerbosity > Verbosity::Simple)
            {
                fprintf(pm.mOutputFile, ",MsBetweenDisplayChange");
            }
            fprintf(pm.mOutputFile, ",MsInPresentAPI");
            if (pm.mArgs->mVerbosity > Verbosity::Simple)
            {
                fprintf(pm.mOutputFile, ",MsUntilRenderComplete,MsUntilDisplayed");
            }
            fprintf(pm.mOutputFile, "\n");
        }
    }
}

void PresentMon_Update(PresentMonData& pm, std::vector<std::shared_ptr<PresentEvent>>& presents, uint64_t perfFreq)
{
    std::string display;
    uint64_t now = GetTickCount64();

    // store the new presents into processes
    for (auto& p : presents)
    {
        AddPresent(pm, *p, now, perfFreq);
    }

    // update all processes
    for (auto& proc : pm.mProcessMap)
    {
        if (!pm.mArgs->mEtlFileName) {
            UpdateProcessInfo_Realtime(proc.second, now, proc.first);
        }

        if (proc.second.mTerminationProcess && !proc.second.mProcessExists) {
            --pm.mTerminationProcessCount;
            if (pm.mTerminationProcessCount == 0) {
                PostStopRecording();
                PostQuitProcess();
            }
            proc.second.mTerminationProcess = false;
        }

        if (pm.mArgs->mSimpleConsole ||
            proc.second.mModuleName.empty() ||
            proc.second.mChainMap.empty())
        {
            // don't display empty processes
            continue;
        }

        char str[256] = {};
        _snprintf_s(str, _TRUNCATE, "%s[%d]:\n", proc.second.mModuleName.c_str(),proc.first);
        display += str;
        for (auto& chain : proc.second.mChainMap)
        {
            double fps = chain.second.ComputeFps(perfFreq);

            _snprintf_s(str, _TRUNCATE, "\t%016llX (%s): SyncInterval %d | Flags %d | %.2lf ms/frame (%.1lf fps, ",
                chain.first,
                RuntimeToString(chain.second.mRuntime),
                chain.second.mLastSyncInterval,
                chain.second.mLastFlags,
                1000.0/fps,
                fps);
            display += str;

            if (pm.mArgs->mVerbosity > Verbosity::Simple) {
                _snprintf_s(str, _TRUNCATE, "%.1lf displayed fps, ", chain.second.ComputeDisplayedFps(perfFreq));
                display += str;
            }

            _snprintf_s(str, _TRUNCATE, "%.2lf ms CPU", chain.second.ComputeCpuFrameTime(perfFreq) * 1000.0);
            display += str;

            if (pm.mArgs->mVerbosity > Verbosity::Simple) {
                _snprintf_s(str, _TRUNCATE, ", %.2lf ms latency) (%s",
                    1000.0 * chain.second.ComputeLatency(perfFreq),
                    PresentModeToString(chain.second.mLastPresentMode));
                display += str;

                if (chain.second.mLastPresentMode == PresentMode::Hardware_Composed_Independent_Flip) {
                    _snprintf_s(str, _TRUNCATE, ": Plane %d", chain.second.mLastPlane);
                    display += str;
                }

                if ((chain.second.mLastPresentMode == PresentMode::Hardware_Composed_Independent_Flip ||
                     chain.second.mLastPresentMode == PresentMode::Hardware_Independent_Flip) &&
                    pm.mArgs->mVerbosity >= Verbosity::Verbose &&
                    chain.second.mDwmNotified) {
                    _snprintf_s(str, _TRUNCATE, ", DWM notified");
                    display += str;
                }

                if (pm.mArgs->mVerbosity >= Verbosity::Verbose &&
                    chain.second.mHasBeenBatched) {
                    _snprintf_s(str, _TRUNCATE, ", batched");
                    display += str;
                }
            }

            _snprintf_s(str, _TRUNCATE, ")%s\n",
                (now - chain.second.mLastUpdateTicks) > 1000 ? " [STALE]" : "");
            display += str;
        }
    }

    // refresh the console
    if (pm.mArgs->mSimpleConsole == false) {
        SetConsoleText(display.c_str());
    }
}

void PresentMon_Shutdown(PresentMonData& pm, bool log_corrupted)
{
    if (pm.mOutputFile)
    {
        if (log_corrupted) {
            fclose(pm.mOutputFile);
            fopen_s(&pm.mOutputFile, pm.mOutputFilePath, "w");
            if (pm.mOutputFile) {
                fprintf(pm.mOutputFile, "Error: Some ETW packets were lost. Collected data is unreliable.\n");
            }
        }
        if (pm.mOutputFile) {
            fclose(pm.mOutputFile);
            pm.mOutputFile = nullptr;
        }
    }
    pm.mProcessMap.clear();

    if (pm.mArgs->mSimpleConsole == false) {
        SetConsoleText("");
    }
}

static bool g_EtwProcessingThreadProcessing = false;
static void EtwProcessingThread(TraceSession *session)
{
    assert(g_EtwProcessingThreadProcessing == true);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    auto status = ProcessTrace(&session->traceHandle_, 1, NULL, NULL);
    (void) status; // check: _status == ERROR_SUCCESS;

    // Notify EtwConsumingThread that processing is complete
    g_EtwProcessingThreadProcessing = false;
}

void EtwConsumingThread(const CommandLineArgs& args)
{
    Sleep(args.mDelay * 1000);
    if (EtwThreadsShouldQuit()) {
        return;
    }

    PresentMonData data;
    PMTraceConsumer pmConsumer(args.mVerbosity == Verbosity::Simple);

    TraceSession session;

    session.AddProvider(DXGI_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0, 0, (EventHandlerFn) &HandleDXGIEvent, &pmConsumer);
    session.AddProvider(D3D9_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0, 0, (EventHandlerFn) &HandleD3D9Event, &pmConsumer);
    if (args.mVerbosity != Verbosity::Simple) {
        session.AddProvider(DXGKRNL_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 1,      0, (EventHandlerFn) &HandleDXGKEvent,   &pmConsumer);
        session.AddProvider(WIN32K_PROVIDER_GUID,  TRACE_LEVEL_INFORMATION, 0x1000, 0, (EventHandlerFn) &HandleWin32kEvent, &pmConsumer);
        session.AddProvider(DWM_PROVIDER_GUID,     TRACE_LEVEL_VERBOSE,     0,      0, (EventHandlerFn) &HandleDWMEvent,    &pmConsumer);
    }
    if (args.mEtlFileName != nullptr) {
        session.AddProvider(NT_PROCESS_EVENT_GUID, TRACE_LEVEL_NONE, 0, 0, (EventHandlerFn) &HandleNTProcessEvent, &pmConsumer);
    }

    if (!(args.mEtlFileName == nullptr
        ? session.InitializeRealtime("PresentMon", &EtwThreadsShouldQuit)
        : session.InitializeEtlFile(args.mEtlFileName, &EtwThreadsShouldQuit))) {
        return;
    }

    if (args.mSimpleConsole) {
        printf("Started recording.\n");
    }
    if (args.mScrollLockIndicator) {
        EnableScrollLock(true);
    }

    {
        // Launch the ETW producer thread
        g_EtwProcessingThreadProcessing = true;
        std::thread etwProcessingThread(EtwProcessingThread, &session);

        // Consume / Update based on the ETW output
        {

            PresentMon_Init(args, data);
            auto timerRunning = args.mTimer > 0;
            auto timerEnd = GetTickCount64() + args.mTimer * 1000;

            std::vector<std::shared_ptr<PresentEvent>> presents;
            std::vector<NTProcessEvent> ntProcessEvents;

            bool log_corrupted = false;
            for (;;) {
#if _DEBUG
                if (args.mSimpleConsole) {
                    printf(".");
                }
#endif

                presents.clear();
                ntProcessEvents.clear();

                // If we are reading events from ETL file set start time to match time stamp of first event
                if (data.mArgs->mEtlFileName && data.mStartupQpcTime == 0)
                {
                    data.mStartupQpcTime = session.startTime_;
                }

                if (args.mEtlFileName) {
                    {
                        auto lock = scoped_lock(pmConsumer.mNTProcessEventMutex);
                        ntProcessEvents.swap(pmConsumer.mNTProcessEvents);
                    }

                    for (auto ntProcessEvent : ntProcessEvents) {
                        if (!ntProcessEvent.ImageFileName.empty()) {
                            ProcessInfo processInfo;
                            processInfo.mModuleName = ntProcessEvent.ImageFileName;

                            data.mProcessMap[ntProcessEvent.ProcessId] = processInfo;
                        }
                    }
                }

                pmConsumer.DequeuePresents(presents);
                if (args.mScrollLockToggle && (GetKeyState(VK_SCROLL) & 1) == 0) {
                    presents.clear();
                }

                auto doneProcessingEvents = g_EtwProcessingThreadProcessing ? false : true;
                PresentMon_Update(data, presents, session.frequency_);

                if (args.mEtlFileName) {
                    for (auto ntProcessEvent : ntProcessEvents) {
                        if (ntProcessEvent.ImageFileName.empty()) {
                            data.mProcessMap.erase(ntProcessEvent.ProcessId);
                        }
                    }
                }

                uint32_t eventsLost = 0;
                uint32_t buffersLost = 0;
                if (session.CheckLostReports(&eventsLost, &buffersLost)) {
                    printf("Lost %u events, %u buffers.", eventsLost, buffersLost);
                    // FIXME: How do we set a threshold here?
                    if (eventsLost > 100) {
                        log_corrupted = true;
                        PostStopRecording();
                        PostQuitProcess();
                    }
                }

                if (timerRunning) {
                    if (GetTickCount64() >= timerEnd) {
                        PostStopRecording();
                        if (args.mTerminateAfterTimer) {
                            PostQuitProcess();
                        }
                        timerRunning = false;
                    }
                }

                if (doneProcessingEvents) {
                    assert(EtwThreadsShouldQuit() || args.mEtlFileName);
                    if (!EtwThreadsShouldQuit()) {
                        PostStopRecording();
                        PostQuitProcess();
                    }
                    break;
                }

                Sleep(100);
            }

            PresentMon_Shutdown(data, log_corrupted);
        }

        assert(etwProcessingThread.joinable());
        assert(!g_EtwProcessingThreadProcessing);
        etwProcessingThread.join();
    }

    session.Finalize();

    if (args.mScrollLockIndicator) {
        EnableScrollLock(false);
    }
    if (args.mSimpleConsole) {
        printf("Stopping recording.\n");
    }
}

