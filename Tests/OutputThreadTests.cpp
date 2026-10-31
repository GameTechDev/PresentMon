// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>
#include <stdexcept>

// Exercise the console's event ordering directly without collecting ETW or starting its thread.
#include "../PresentMon/OutputThread.cpp"
#include "../PresentData/ETW/Microsoft_Windows_Kernel_Process.h"
#include "../PresentData/ETW/NT_Process.h"

namespace {
CommandLineArgs testArgs{};
unsigned exitRequests = 0;
}

CommandLineArgs const& GetCommandLineArgs() { return testArgs; }
void ExitMainThread() { ++exitRequests; }
bool BeginConsoleUpdate() { return false; }
void EndConsoleUpdate() {}
void ConsolePrintLn(wchar_t const*, ...) {}
void UpdateConsole(uint32_t, ProcessInfo const&) {}

namespace {
class CsvDirectory {
public:
    std::filesystem::path path;
    CsvDirectory()
    {
        wchar_t temporary[MAX_PATH], unique[MAX_PATH];
        if (!GetTempPathW(MAX_PATH, temporary) || !GetTempFileNameW(temporary, L"pmo", 0, unique)) {
            throw std::runtime_error("Could not create test output directory");
        }
        path = unique;
        std::filesystem::remove(path);
        std::filesystem::create_directory(path);
    }
    ~CsvDirectory()
    {
        // Release streams before removing only the files created in this unique test directory.
        gProcesses.clear();
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            std::filesystem::remove(entry.path(), error);
        }
        std::filesystem::remove(path, error);
    }
};

class OutputThreadTests : public testing::Test {
protected:
    PMTraceSession session;

    void SetUp() override
    {
        Reset(0);
        session.mTimestampFrequency.QuadPart = 1000;
    }

    void TearDown() override
    {
        gProcesses.clear();
        gTargetProcessCount = 0;
        CloseGlobalCsv();
    }

    void Reset(unsigned mode)
    {
        gProcesses.clear();
        gTargetProcessCount = 0;
        exitRequests = 0;
        testArgs = {};
        testArgs.mEtlFileName = L"synthetic.etl";
        testArgs.mMultiCsv = true;
        testArgs.mCSVOutput = CSVOutput::File;
        testArgs.mTimeUnit = TimeUnit::QPC;
        testArgs.mTrackDisplay = true;
        testArgs.mUseV1Metrics = mode == 1;
        testArgs.mUseV2Metrics = mode == 2;
    }

    static ProcessEvent Event(uint32_t pid, uint64_t time, bool start, const wchar_t* name = L"app.exe")
    {
        ProcessEvent event;
        event.ProcessId = pid;
        event.QpcTime = time;
        event.IsStartEvent = start;
        event.ImageFileName = name;
        return event;
    }

    static ProcessEvent Rundown(uint32_t pid, uint64_t time, bool start = true, const wchar_t* name = L"app.exe")
    {
        auto event = Event(pid, time, start, name);
        event.IsRundown = true;
        return event;
    }

    static std::shared_ptr<PresentEvent> Present(uint64_t time, uint32_t pid = 11, bool displayed = true,
        uint64_t swapChain = 55)
    {
        auto present = std::make_shared<PresentEvent>();
        present->ProcessId = pid;
        present->SwapChainAddress = swapChain;
        present->PresentStartTime = time;
        present->TimeInPresent = 1;
        present->GPUStartTime = time + 1;
        present->ReadyTime = time + 2;
        present->GPUDuration = 1;
        present->Runtime = Runtime::DXGI;
        present->FinalState = displayed ? PresentResult::Presented : PresentResult::Discarded;
        if (displayed) {
            present->Displayed.emplace_back(FrameType::Application, time + 3);
        }
        return present;
    }

    static ProcessInfo* Lookup(const std::shared_ptr<PresentEvent>& present)
    {
        ProcessInfo* process = nullptr;
        SwapChainData* chain = nullptr;
        uint64_t time = 0;
        if (GetPresentProcessInfo(present, &process, &chain, &time)) {
            return nullptr;
        }
        return process;
    }

    static std::shared_ptr<FILE> Capture(ProcessInfo* process)
    {
        FILE* file = nullptr;
        if (tmpfile_s(&file) != 0) {
            ADD_FAILURE() << "Could not create temporary CSV";
            return {};
        }
        auto owner = std::shared_ptr<FILE>(file, [](FILE* stream) { fclose(stream); });
        process->mOutputCsv->mFile = owner;
        return owner;
    }

    static std::vector<std::wstring> Rows(FILE* file)
    {
        fflush(file);
        rewind(file);
        std::vector<std::wstring> rows;
        wchar_t line[8192];
        while (fgetws(line, (int)std::size(line), file)) {
            rows.emplace_back(line);
        }
        return rows;
    }

    static std::vector<std::wstring> FileRows(std::wstring const& path)
    {
        FILE* file = nullptr;
        if (_wfopen_s(&file, path.c_str(), L"r,ccs=UTF-8") != 0) {
            ADD_FAILURE() << "Could not read test CSV";
            return {};
        }
        auto rows = Rows(file);
        fclose(file);
        if (!rows.empty() && !rows.front().empty() && rows.front().front() == L'\xFEFF') {
            rows.front().erase(0, 1);
        }
        return rows;
    }

    static void WriteRow(PMTraceSession const& session, ProcessInfo* info, uint32_t pid, uint64_t time)
    {
        pmon::util::metrics::FrameData frame;
        frame.processId = pid;
        frame.presentStartTime = time;
        frame.finalState = PresentResult::Presented;
        pmon::util::metrics::FrameMetrics metrics;
        UpdateCsv(session, info, frame, metrics);
    }

    void Process(const std::vector<std::shared_ptr<PresentEvent>>& presents,
        std::vector<ProcessEvent> events = {})
    {
        std::vector<uint64_t> toggles;
        ProcessEvents(session, presents, &events, &toggles, true);
        EXPECT_TRUE(events.empty());
    }

    std::vector<std::wstring> TrailingRows(unsigned mode, bool stop)
    {
        Reset(mode);
        ProcessProcessEvent(Event(11, 1, true));
        auto process = Lookup(Present(10000));
        auto file = Capture(process);
        if (!file) return {};
        std::vector<ProcessEvent> events;
        if (stop) {
            events.push_back(Event(11, 11500, false));
        }
        Process({Present(10000), Present(11000), Present(12000), Present(13000)}, events);
        return Rows(file.get());
    }

    std::vector<std::wstring> BufferedRows(unsigned mode, bool prune)
    {
        Reset(mode);
        if (prune) testArgs.mEtlFileName = nullptr;
        ProcessProcessEvent(Event(11, 1, true));
        auto process = Lookup(Present(10000));
        auto file = Capture(process);
        if (!file) return {};
        Process({Present(10000), Present(11000)});
        for (uint64_t time = 12000; time <= 17000; time += 1000) {
            Process({Present(time, 11, false)});
            EXPECT_EQ(1u, process->mSwapChain.count(55));
        }
        Process({Present(18000), Present(19000), Present(20000)});
        return Rows(file.get());
    }
};

TEST_F(OutputThreadTests, TerminationPreservesTrailingRowsInEveryMetricsMode)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        SCOPED_TRACE(mode);
        auto expected = TrailingRows(mode, false);
        ASSERT_FALSE(expected.empty());
        EXPECT_EQ(expected, TrailingRows(mode, true));
    }
}

TEST_F(OutputThreadTests, BufferedPresentsKeepActiveSwapChainAlive)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        SCOPED_TRACE(mode);
        auto expected = BufferedRows(mode, false);
        ASSERT_FALSE(expected.empty());
        EXPECT_EQ(expected, BufferedRows(mode, true));
    }
}

TEST_F(OutputThreadTests, ReusedPidHasSeparateHistoryAndAcceptsLateOldPresents)
{
    ProcessProcessEvent(Event(11, 1, true, L"old.exe"));
    auto old = Lookup(Present(10000));
    auto oldFile = Capture(old);
    Process({Present(10000), Present(11000)});

    ProcessProcessEvent(Event(11, 14000, false));
    ProcessProcessEvent(Event(11, 15000, true, L"new.exe"));
    auto current = Lookup(Present(16000));
    ASSERT_NE(old, current);
    EXPECT_EQ(L"new.exe", current->mModuleName);
    EXPECT_FALSE(current->mSwapChain.at(55).mUnifiedSwapChain.swapChain.lastPresent.has_value());
    auto currentFile = Capture(current);

    Process({Present(16000), Present(17000), Present(12000), Present(13000), Present(18000)});
    EXPECT_EQ(old, Lookup(Present(13000)));
    EXPECT_EQ(current, Lookup(Present(18000)));
    auto oldRows = Rows(oldFile.get());
    auto newRows = Rows(currentFile.get());
    ASSERT_FALSE(oldRows.empty());
    ASSERT_FALSE(newRows.empty());
    for (const auto& row : oldRows) EXPECT_EQ(0u, row.find(L"old.exe,"));
    for (const auto& row : newRows) EXPECT_EQ(0u, row.find(L"new.exe,"));
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, StopDoesNotBlockOtherProcessStarts)
{
    ProcessProcessEvent(Event(11, 1, true));
    auto file = Capture(Lookup(Present(10000)));
    Process({Present(10000)});
    Process({Present(13000)}, {Event(11, 11000, false), Event(22, 12000, true, L"other.exe")});
    auto other = Lookup(Present(13000, 22));
    ASSERT_NE(nullptr, other);
    EXPECT_EQ(L"other.exe", other->mModuleName);
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, FilteredOldLifetimeDoesNotHideReusedTargetPid)
{
    testArgs.mTargetProcessNames = {L"target"};
    ProcessProcessEvent(Event(11, 1, true, L"ignored.exe"));
    EXPECT_EQ(nullptr, Lookup(Present(10000)));
    // Seed only, so this does not need a CSV stream.
    Process({Present(13000)}, {Event(11, 11000, false), Event(11, 12000, true, L"target.exe")});
    auto target = Lookup(Present(13000));
    ASSERT_NE(nullptr, target);
    EXPECT_EQ(L"target.exe", target->mModuleName);
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, DuplicateStopsOnlyTerminateTargetOnce)
{
    testArgs.mTerminateOnProcExit = true;
    ProcessProcessEvent(Event(11, 1, true));
    ProcessProcessEvent(Event(11, 10000, false));
    ProcessProcessEvent(Event(11, 11000, false));
    EXPECT_EQ(0u, gTargetProcessCount);
    EXPECT_EQ(1u, exitRequests);
    ASSERT_NE(nullptr, Lookup(Present(12000)));
    EXPECT_EQ(0u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, ReplacingTargetLifetimeDoesNotRequestPrematureExit)
{
    testArgs.mTerminateOnProcExit = true;
    ProcessProcessEvent(Event(11, 1, true));
    ProcessProcessEvent(Event(11, 10000, true));
    EXPECT_EQ(1u, gTargetProcessCount);
    EXPECT_EQ(0u, exitRequests);
}

TEST_F(OutputThreadTests, RepeatedNameAndPidShareEvenAnUnopenedCsv)
{
    ProcessProcessEvent(Event(11, 1, true));
    auto old = Lookup(Present(10000));
    ProcessProcessEvent(Event(11, 11000, false));
    ProcessProcessEvent(Event(11, 12000, true));
    auto current = Lookup(Present(13000));
    ASSERT_NE(old, current);
    EXPECT_EQ(old->mOutputCsv, current->mOutputCsv);
    auto file = Capture(current);
    EXPECT_EQ(file, old->mOutputCsv->mFile);
    Process({Present(9000), Present(10000), Present(13000), Present(14000), Present(15000)});
    EXPECT_FALSE(Rows(file.get()).empty());
}

TEST_F(OutputThreadTests, PruningRemovesOnlyInactiveSwapChains)
{
    testArgs.mEtlFileName = nullptr;
    ProcessProcessEvent(Event(11, 1, true));
    auto process = Lookup(Present(10000));
    Lookup(Present(10000, 11, true, 66));
    Lookup(Present(16000));
    PruneOldSwapChainData(session, 16000);
    EXPECT_EQ(1u, process->mSwapChain.count(55));
    EXPECT_EQ(0u, process->mSwapChain.count(66));
}

TEST_F(OutputThreadTests, OutOfOrderPresentsDoNotMoveActivityBackwards)
{
    testArgs.mEtlFileName = nullptr;
    ProcessProcessEvent(Event(11, 1, true));
    auto process = Lookup(Present(16000));
    Lookup(Present(11000));
    PruneOldSwapChainData(session, 19000);
    EXPECT_EQ(1u, process->mSwapChain.count(55));
    PruneOldSwapChainData(session, 20001);
    EXPECT_TRUE(process->mSwapChain.empty());
}

TEST_F(OutputThreadTests, RetiredLifetimeCleanupDoesNotCloseReusedCsv)
{
    testArgs.mEtlFileName = nullptr;
    ProcessProcessEvent(Event(11, 1, true));
    auto old = Lookup(Present(10000));
    auto file = Capture(old);
    ProcessProcessEvent(Event(11, 11000, false));
    ProcessProcessEvent(Event(11, 12000, true));
    auto current = Lookup(Present(16000));
    PruneOldSwapChainData(session, 16000);
    EXPECT_EQ(1u, gProcesses.size());
    EXPECT_EQ(file, current->mOutputCsv->mFile);
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, PruningBeforeFourSecondsDoesNotUnderflow)
{
    testArgs.mEtlFileName = nullptr;
    ProcessProcessEvent(Event(11, 1, true));
    auto process = Lookup(Present(100));
    PruneOldSwapChainData(session, 200);
    EXPECT_EQ(1u, process->mSwapChain.count(55));
}

TEST_F(OutputThreadTests, MissingRealtimeStartDetectsReusedPidAndPreservesLateFrames)
{
    testArgs.mEtlFileName = nullptr;
    LARGE_INTEGER frequency{}, now{};
    QueryPerformanceFrequency(&frequency);
    session.mTimestampFrequency = frequency;
    ProcessInfo live;
    auto pid = GetCurrentProcessId();
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    live.mHandle = NULL;
    ASSERT_GT(live.mCreationTimestamp, (uint64_t)frequency.QuadPart * 2);
    auto born = live.mCreationTimestamp;
    auto oldFrame = born - frequency.QuadPart;
    ProcessProcessEvent(Event(pid, born - 2 * frequency.QuadPart, true, L"retired.exe"));
    auto old = Lookup(Present(oldFrame, pid));
    ASSERT_NE(nullptr, old);
    ProcessProcessEvent(Event(pid, born - frequency.QuadPart / 2, false));
    ASSERT_EQ(0u, gTargetProcessCount);
    QueryPerformanceCounter(&now);
    auto current = Lookup(Present((uint64_t)now.QuadPart, pid));
    ASSERT_NE(nullptr, current);
    EXPECT_NE(old, current);
    EXPECT_EQ(live.mModuleName, current->mModuleName);
    EXPECT_EQ(live.mCreationTime, current->mCreationTime);
    EXPECT_EQ(1u, gTargetProcessCount);
    EXPECT_EQ(old, Lookup(Present(oldFrame, pid)));
    EXPECT_EQ(oldFrame, old->mLastReceivedPresentTime);
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, MissingRealtimeStartReevaluatesFiltersForReusedPid)
{
    testArgs.mEtlFileName = nullptr;
    ProcessInfo live;
    auto pid = GetCurrentProcessId();
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto target = live.mModuleName;
    CanonicalizeProcessName(&target);
    testArgs.mTargetProcessNames = {target};
    auto period = util::GetTimestampFrequencyUint64();
    ASSERT_GT(live.mCreationTimestamp, period * 2);
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp - 2 * period, true, L"ignored.exe"));
    EXPECT_EQ(nullptr, Lookup(Present(live.mCreationTimestamp - period, pid)));
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp - period / 2, false));
    auto current = Lookup(Present((uint64_t)util::GetCurrentTimestamp(), pid));
    ASSERT_NE(nullptr, current);
    EXPECT_EQ(live.mModuleName, current->mModuleName);
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, MissingRealtimeStartDetectsReuseWhenOldLifetimeNeverPresented)
{
    for (bool filterByName : {false, true}) {
        SCOPED_TRACE(filterByName);
        Reset(0);
        testArgs.mEtlFileName = nullptr;
        testArgs.mTerminateOnProcExit = true;
        ProcessInfo live;
        auto pid = GetCurrentProcessId();
        QueryProcessName(pid, &live);
        ASSERT_NE(nullptr, live.mHandle);
        CloseHandle(live.mHandle);
        auto period = util::GetTimestampFrequencyUint64();
        ASSERT_GT(live.mCreationTimestamp, 2 * period);
        if (filterByName) {
            auto target = live.mModuleName;
            CanonicalizeProcessName(&target);
            testArgs.mTargetProcessNames = {target};
        }
        ProcessProcessEvent(Event(pid, live.mCreationTimestamp - 2 * period, true, L"ignored.exe"));
        auto old = &gProcesses.begin()->second;
        ASSERT_EQ(0u, old->mFirstReceivedPresentTime);
        auto current = Lookup(Present((uint64_t)util::GetCurrentTimestamp(), pid));
        ASSERT_NE(nullptr, current);
        EXPECT_NE(old, current);
        EXPECT_EQ(live.mModuleName, current->mModuleName);
        EXPECT_EQ(live.mCreationTime, current->mCreationTime);
        EXPECT_TRUE(old->mTerminationTime.has_value());
        EXPECT_EQ(1u, gTargetProcessCount);
        EXPECT_EQ(0u, exitRequests);
        EXPECT_EQ(current, Lookup(Present((uint64_t)util::GetCurrentTimestamp(), pid)));
        EXPECT_EQ(2u, gProcesses.size());
        EXPECT_EQ(1u, gTargetProcessCount);
    }
}

TEST_F(OutputThreadTests, RealtimeStartWithinClockToleranceKeepsItsLifetime)
{
    testArgs.mEtlFileName = nullptr;
    ProcessInfo live;
    auto pid = GetCurrentProcessId();
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto tolerance = util::GetTimestampFrequencyUint64() / 50;
    ASSERT_GT(live.mCreationTimestamp, tolerance);
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp - tolerance / 2, true, live.mModuleName.c_str()));
    auto original = &gProcesses.begin()->second;
    auto current = Lookup(Present((uint64_t)util::GetCurrentTimestamp(), pid));
    ASSERT_NE(nullptr, current);
    EXPECT_EQ(original, current);
    EXPECT_FALSE(current->mTerminationTime.has_value());
    EXPECT_EQ(live.mCreationTime, current->mCreationTime);
    EXPECT_EQ(1u, gProcesses.size());
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, PositiveCreationTimeErrorPreservesBufferedHistoryInEveryMetricsMode)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        for (unsigned scenario = 0; scenario < 3; ++scenario) {
            SCOPED_TRACE(mode);
            SCOPED_TRACE(scenario);
            Reset(mode);
            bool hasStart = scenario != 1;
            bool hasInitialIdentity = scenario != 2;
            testArgs.mEtlFileName = nullptr;
            auto frequency = util::GetTimestampFrequencyUint64();
            session.mTimestampFrequency.QuadPart = frequency;
            ProcessInfo live;
            auto pid = GetCurrentProcessId();
            QueryProcessName(pid, &live);
            ASSERT_NE(nullptr, live.mHandle);
            CloseHandle(live.mHandle);
            ASSERT_GT(live.mCreationTimestamp, frequency);
            // Model a creation estimate 10 ms after the actual start and a first
            // frame 5 ms after that start, within the supported 20 ms tolerance.
            auto start = live.mCreationTimestamp - frequency / 100;
            auto firstTime = live.mCreationTimestamp - frequency / 200;
            if (hasStart) ProcessProcessEvent(Event(pid, start, true, live.mModuleName.c_str()));
            // Seed history without an OS identity, as when an earlier query was unavailable.
            if (!hasInitialIdentity) testArgs.mEtlFileName = L"synthetic.etl";
            auto original = Lookup(Present(firstTime, pid));
            ASSERT_NE(nullptr, original);
            auto file = Capture(original);
            ASSERT_TRUE(file);
            Process({Present(firstTime, pid)});
            auto priorQpc = original->mSwapChain.at(55).mUnifiedSwapChain.GetLastPresentQpcOr(0);
            ASSERT_NE(0u, priorQpc);
            testArgs.mEtlFileName = nullptr;
            auto nextTime = firstTime + frequency / 1000;
            auto current = Lookup(Present(nextTime, pid));
            ASSERT_NE(nullptr, current);
            EXPECT_EQ(original, current);
            EXPECT_FALSE(original->mTerminationTime.has_value());
            EXPECT_EQ(priorQpc, current->mSwapChain.at(55).mUnifiedSwapChain.GetLastPresentQpcOr(0));
            EXPECT_EQ(original, Lookup(Present(firstTime - frequency / 1000, pid, true, 66)));
            if (!hasStart) ProcessProcessEvent(Event(pid, start, true, live.mModuleName.c_str()));
            EXPECT_EQ(original, Lookup(Present(nextTime, pid)));
            EXPECT_EQ(file, original->mOutputCsv->mFile);
            EXPECT_EQ(1u, gProcesses.size());
            EXPECT_EQ(1u, gTargetProcessCount);
            Process({Present(nextTime, pid), Present(nextTime + frequency / 1000, pid),
                Present(nextTime + frequency / 500, pid)});
            EXPECT_FALSE(Rows(file.get()).empty());
        }
    }
}

TEST_F(OutputThreadTests, CreationTimeToleranceDoesNotCrossAKnownStartEvent)
{
    testArgs.mEtlFileName = nullptr;
    ProcessInfo live;
    auto pid = GetCurrentProcessId();
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto period = util::GetTimestampFrequencyUint64();
    ASSERT_GT(live.mCreationTimestamp, period);
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp, true, live.mModuleName.c_str()));
    auto current = &gProcesses.begin()->second;
    auto historical = Lookup(Present(live.mCreationTimestamp - period / 200, pid));
    ASSERT_NE(nullptr, historical);
    EXPECT_NE(current, historical);
    EXPECT_EQ(L"<unknown>", historical->mModuleName);
    EXPECT_EQ(live.mCreationTimestamp, historical->mTerminationTime.value_or(0));
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, RealtimeLookupDoesNotAssignCurrentNameToAnOlderUnknownLifetime)
{
    testArgs.mEtlFileName = nullptr;
    ProcessInfo live;
    auto pid = GetCurrentProcessId();
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto period = util::GetTimestampFrequencyUint64();
    ASSERT_GT(live.mCreationTimestamp, period);
    auto oldTime = live.mCreationTimestamp - period;
    auto old = Lookup(Present(oldTime, pid));
    ASSERT_NE(nullptr, old);
    EXPECT_EQ(L"<unknown>", old->mModuleName);
    EXPECT_EQ(nullptr, old->mHandle);
    EXPECT_TRUE(old->mTerminationTime.has_value());
    EXPECT_EQ(0u, gTargetProcessCount);
    auto current = Lookup(Present((uint64_t)util::GetCurrentTimestamp(), pid));
    ASSERT_NE(nullptr, current);
    EXPECT_NE(old, current);
    EXPECT_EQ(live.mModuleName, current->mModuleName);
    EXPECT_EQ(old, Lookup(Present(oldTime, pid)));
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, MissingRealtimeStartHonorsTerminationWhenReplacementIsExcluded)
{
    testArgs.mEtlFileName = nullptr;
    testArgs.mTerminateOnProcExit = true;
    testArgs.mTargetProcessNames = {L"retired"};
    ProcessInfo live;
    auto pid = GetCurrentProcessId();
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto period = util::GetTimestampFrequencyUint64();
    ASSERT_GT(live.mCreationTimestamp, 2 * period);
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp - 2 * period, true, L"retired.exe"));
    ASSERT_NE(nullptr, Lookup(Present(live.mCreationTimestamp - period, pid)));
    EXPECT_EQ(nullptr, Lookup(Present((uint64_t)util::GetCurrentTimestamp(), pid)));
    EXPECT_EQ(0u, gTargetProcessCount);
    EXPECT_EQ(1u, exitRequests);
}

TEST_F(OutputThreadTests, DelayedStartReconcilesProvisionalHistoryInEveryMetricsMode)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        SCOPED_TRACE(mode);
        auto expected = TrailingRows(mode, false);
        Reset(mode);
        auto provisional = Lookup(Present(10000));
        auto file = Capture(provisional);
        ASSERT_TRUE(file);
        Process({Present(10000), Present(11000)});
        auto previous = provisional->mSwapChain.at(55).mUnifiedSwapChain.GetLastPresentQpc();
        ProcessProcessEvent(Event(11, 9000, true));
        auto reconciled = Lookup(Present(12000));
        EXPECT_EQ(provisional, reconciled);
        EXPECT_EQ(previous, reconciled->mSwapChain.at(55).mUnifiedSwapChain.GetLastPresentQpc());
        EXPECT_EQ(file, reconciled->mOutputCsv->mFile);
        EXPECT_FALSE(reconciled->mTerminationTime.has_value());
        EXPECT_EQ(1u, gTargetProcessCount);
        EXPECT_EQ(1u, gProcesses.size());
        EXPECT_EQ(9000u, gProcesses.begin()->first.second);
        Process({Present(12000), Present(13000)});
        auto actual = Rows(file.get());
        // The provisional name was unknown when any already-published rows were emitted.
        for (auto& row : expected) row.erase(0, row.find(L','));
        for (auto& row : actual) row.erase(0, row.find(L','));
        EXPECT_EQ(expected, actual);
    }
}

TEST_F(OutputThreadTests, DelayedLifetimeBatchKeepsRowsWithTheCurrentProcessInEveryMetricsMode)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        SCOPED_TRACE(mode);
        auto expected = TrailingRows(mode, false);
        Reset(mode);
        CsvDirectory directory;
        auto output = (directory.path / L"capture.csv").wstring();
        testArgs.mOutputCsvFileName = output.c_str();
        testArgs.mTerminateOnProcExit = true;
        auto provisional = Lookup(Present(10000));
        auto file = Capture(provisional);
        ASSERT_TRUE(file);
        Process({Present(10000), Present(11000)});
        Process({Present(12000), Present(13000)}, {Event(11, 1000, true, L"old.exe"),
            Event(11, 2000, false), Event(11, 3000, true)});
        auto current = Lookup(Present(14000));
        ASSERT_NE(nullptr, current);
        EXPECT_EQ(provisional, current);
        auto old = FindProcess(11, 1500);
        ASSERT_NE(gProcesses.end(), old);
        EXPECT_TRUE(old->second.mSwapChain.empty());
        EXPECT_EQ(2000u, old->second.mTerminationTime.value_or(0));
        EXPECT_EQ(10000u, current->mFirstReceivedPresentTime);
        EXPECT_EQ(1u, gTargetProcessCount);
        EXPECT_EQ(0u, exitRequests);
        auto actual = Rows(file.get());
        // Ignore the unresolved name on rows already published before the start arrived.
        for (auto& row : expected) row.erase(0, row.find(L','));
        for (auto& row : actual) row.erase(0, row.find(L','));
        EXPECT_EQ(expected, actual);
    }
}

TEST_F(OutputThreadTests, QueuedLifetimeBoundariesPreserveChronologicalTargetCounts)
{
    testArgs.mCSVOutput = CSVOutput::None;
    testArgs.mTargetProcessNames = {L"app"};
    testArgs.mTerminateOnProcExit = true;
    Process({Present(10000)});
    auto provisional = &gProcesses.begin()->second;
    std::vector<ProcessEvent> events{Event(11, 1000, true), Event(11, 2000, false),
        Event(11, 3000, true, L"other.exe")};
    std::vector<uint64_t> toggles;
    ProcessEvents(session, {Present(500, 22)}, &events, &toggles, true);
    EXPECT_EQ(3u, events.size());
    EXPECT_EQ(provisional, &gProcesses.at({11, 0}));
    EXPECT_EQ(0u, gTargetProcessCount);
    ProcessEvents(session, {Present(1500, 22)}, &events, &toggles, true);
    ASSERT_EQ(2u, events.size());
    auto old = FindProcess(11, 1500);
    ASSERT_NE(gProcesses.end(), old);
    EXPECT_FALSE(old->second.mTerminationTime.has_value());
    EXPECT_EQ(provisional, &gProcesses.at({11, 2001}));
    EXPECT_FALSE(provisional->mHasStartEvent);
    EXPECT_EQ(L"<unknown>", provisional->mModuleName);
    EXPECT_EQ(1u, gTargetProcessCount);
    EXPECT_EQ(0u, exitRequests);
    ProcessEvents(session, {Present(2500, 22)}, &events, &toggles, true);
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ(2000u, old->second.mTerminationTime.value_or(0));
    EXPECT_EQ(0u, gTargetProcessCount);
    EXPECT_EQ(1u, exitRequests);
    EXPECT_FALSE(provisional->mHasStartEvent);
    ProcessEvents(session, {Present(12000)}, &events, &toggles, true);
    EXPECT_TRUE(events.empty());
    EXPECT_EQ(provisional, &gProcesses.at({11, 3000}));
    EXPECT_EQ(L"other.exe", provisional->mModuleName);
    EXPECT_EQ(0u, gTargetProcessCount);
    EXPECT_EQ(1u, exitRequests);
}

TEST_F(OutputThreadTests, QueuedOldLifetimeKeepsHistoryUntilTheNewStartArrives)
{
    testArgs.mCSVOutput = CSVOutput::None;
    testArgs.mTerminateOnProcExit = true;
    auto provisional = Lookup(Present(10000));
    Process({Present(10000), Present(11000)});
    Process({Present(12000)}, {Event(11, 1000, true, L"old.exe"), Event(11, 2000, false)});
    EXPECT_EQ(provisional, Lookup(Present(13000)));
    EXPECT_FALSE(provisional->mTerminationTime.has_value());
    EXPECT_FALSE(provisional->mHasStartEvent);
    EXPECT_EQ(1u, gTargetProcessCount);
    EXPECT_EQ(0u, exitRequests);
    Process({Present(14000)}, {Event(11, 3000, true)});
    EXPECT_EQ(provisional, Lookup(Present(15000)));
    EXPECT_EQ(10000u, provisional->mFirstReceivedPresentTime);
    EXPECT_EQ(1u, gTargetProcessCount);
    EXPECT_EQ(0u, exitRequests);
}

TEST_F(OutputThreadTests, QueuedNewStartRetiresTheOldLifetimeWhenItsStopIsMissing)
{
    for (bool replacementTarget : {false, true}) {
        SCOPED_TRACE(replacementTarget);
        Reset(0);
        testArgs.mCSVOutput = CSVOutput::None;
        testArgs.mTargetProcessNames = {L"app"};
        testArgs.mTerminateOnProcExit = true;
        Process({Present(10000)});
        auto provisional = &gProcesses.begin()->second;
        Process({Present(12000)}, {Event(11, 1000, true),
            Event(11, 3000, true, replacementTarget ? L"app.exe" : L"other.exe")});
        auto old = FindProcess(11, 1500);
        ASSERT_NE(gProcesses.end(), old);
        EXPECT_EQ(3000u, old->second.mTerminationTime.value_or(0));
        EXPECT_TRUE(old->second.mSwapChain.empty());
        EXPECT_EQ(provisional, &FindProcess(11, 12000)->second);
        EXPECT_FALSE(provisional->mTerminationTime.has_value());
        EXPECT_EQ(replacementTarget ? 1u : 0u, gTargetProcessCount);
        EXPECT_EQ(replacementTarget ? 0u : 1u, exitRequests);
    }
}

TEST_F(OutputThreadTests, QueuedLifetimeLookaheadSkipsInventoriesAndOtherPids)
{
    testArgs.mCSVOutput = CSVOutput::None;
    testArgs.mTargetPid = 11;
    testArgs.mTerminateOnProcExit = true;
    auto provisional = Lookup(Present(10000));
    Process({Present(10000)});
    Process({Present(12000)}, {Event(11, 1000, true, L"old.exe"), Event(22, 1200, true),
        Rundown(11, 1500, true, L"old.exe"), Event(11, 2000, false), Event(11, 3000, true)});
    EXPECT_EQ(provisional, Lookup(Present(13000)));
    EXPECT_FALSE(provisional->mTerminationTime.has_value());
    EXPECT_TRUE(provisional->mHasStartEvent);
    EXPECT_EQ(1u, gTargetProcessCount);
    EXPECT_EQ(0u, exitRequests);
}

TEST_F(OutputThreadTests, RundownPreservesBufferedRowsInEveryMetricsMode)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        for (bool start : {false, true}) {
            SCOPED_TRACE(mode);
            SCOPED_TRACE(start);
            auto expected = TrailingRows(mode, false);
            Reset(mode);
            testArgs.mTerminateOnProcExit = true;
            ProcessProcessEvent(Event(11, 1, true));
            auto original = Lookup(Present(10000));
            auto file = Capture(original);
            ASSERT_TRUE(file);
            Process({Present(10000), Present(11000)});
            Process({Present(12000), Present(13000)}, {Rundown(11, 11500, start)});
            EXPECT_EQ(original, Lookup(Present(14000)));
            EXPECT_EQ(1u, gProcesses.size());
            EXPECT_EQ(1u, gTargetProcessCount);
            EXPECT_EQ(0u, exitRequests);
            EXPECT_EQ(expected, Rows(file.get()));
        }
    }
}

TEST_F(OutputThreadTests, RundownNamesAProvisionalLifetimeWithoutMovingItsBoundary)
{
    testArgs.mTargetProcessNames = {L"app"};
    testArgs.mCSVOutput = CSVOutput::None;
    ProcessProcessEvent(Rundown(11, 10000));
    auto original = Lookup(Present(9000));
    ASSERT_NE(nullptr, original);
    EXPECT_FALSE(original->mHasStartEvent);
    EXPECT_EQ(0u, gProcesses.begin()->first.second);
    ProcessProcessEvent(Rundown(11, 11000, false, L"<unknown>"));
    EXPECT_EQ(L"app.exe", original->mModuleName);
    EXPECT_EQ(1u, gTargetProcessCount);
    ProcessProcessEvent(Event(11, 8000, true));
    EXPECT_EQ(original, Lookup(Present(12000)));
    EXPECT_TRUE(original->mHasStartEvent);
    EXPECT_EQ(8000u, gProcesses.begin()->first.second);
    EXPECT_EQ(1u, gProcesses.size());
}

TEST_F(OutputThreadTests, RundownAfterAnExitPreservesBothPidLifetimes)
{
    testArgs.mCSVOutput = CSVOutput::None;
    testArgs.mTerminateOnProcExit = true;
    ProcessProcessEvent(Event(11, 1000, true, L"old.exe"));
    auto old = Lookup(Present(10000));
    ProcessProcessEvent(Event(11, 11000, false));
    ProcessProcessEvent(Rundown(11, 15000, false));
    auto current = Lookup(Present(14000));
    ASSERT_NE(nullptr, current);
    EXPECT_NE(old, current);
    EXPECT_EQ(old, Lookup(Present(10500)));
    EXPECT_EQ(L"old.exe", old->mModuleName);
    EXPECT_EQ(L"app.exe", current->mModuleName);
    EXPECT_FALSE(current->mHasStartEvent);
    EXPECT_FALSE(current->mTerminationTime.has_value());
    EXPECT_EQ(1u, gTargetProcessCount);
    EXPECT_EQ(1u, exitRequests);
    ProcessProcessEvent(Event(11, 11000, false));
    EXPECT_EQ(1u, gTargetProcessCount);
    ProcessProcessEvent(Event(11, 12000, true));
    EXPECT_EQ(current, Lookup(Present(16000)));
    EXPECT_EQ(2u, gProcesses.size());
}

TEST_F(OutputThreadTests, HistoricalRealtimeRundownDoesNotKeepATargetActive)
{
    testArgs.mEtlFileName = nullptr;
    testArgs.mCSVOutput = CSVOutput::None;
    testArgs.mTargetProcessNames = {L"app"};
    testArgs.mTerminateOnProcExit = true;
    auto pid = GetCurrentProcessId();
    ProcessInfo live;
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto period = util::GetTimestampFrequencyUint64();
    ASSERT_GT(live.mCreationTimestamp, period);
    auto time = live.mCreationTimestamp - period;
    ProcessProcessEvent(Rundown(pid, time));
    auto historical = FindProcess(pid, time);
    ASSERT_NE(gProcesses.end(), historical);
    EXPECT_TRUE(historical->second.mTerminationTime.has_value());
    EXPECT_EQ(nullptr, historical->second.mHandle);
    EXPECT_EQ(L"app.exe", historical->second.mModuleName);
    EXPECT_EQ(&historical->second, Lookup(Present(time + period / 100, pid)));
    EXPECT_EQ(0u, gTargetProcessCount);
    EXPECT_EQ(0u, exitRequests);
    ProcessProcessEvent(Event(22, live.mCreationTimestamp + period, true));
    ProcessProcessEvent(Event(22, live.mCreationTimestamp + 2 * period, false));
    EXPECT_EQ(0u, gTargetProcessCount);
    EXPECT_EQ(1u, exitRequests);
}

TEST_F(OutputThreadTests, ProcessDecoderDistinguishesRundownFromLifetimeBoundaries)
{
    for (bool kernelProvider : {false, true}) {
        auto types = kernelProvider ? std::vector<uint8_t>{1, 2, 15} : std::vector<uint8_t>{1, 2, 3, 4};
        for (auto type : types) {
            SCOPED_TRACE(kernelProvider);
            SCOPED_TRACE(type);
            PMTraceConsumer consumer;
            consumer.mTrackPcLatency = true;
            consumer.mLatestPingTimestampByProcessId[42] = 5000;
            EVENT_RECORD record{};
            record.EventHeader.ProviderId = kernelProvider ? Microsoft_Windows_Kernel_Process::GUID : NT_Process::GUID;
            record.EventHeader.TimeStamp.QuadPart = 10000;
            if (kernelProvider) record.EventHeader.EventDescriptor.Id = type;
            else record.EventHeader.EventDescriptor.Opcode = type;

            // Supply a minimal schema so the real decoder runs without installed provider metadata.
            std::wstring names[] = {kernelProvider ? L"ProcessID" : L"ProcessId",
                kernelProvider ? L"ImageName" : L"ImageFileName"};
            size_t offset = offsetof(TRACE_EVENT_INFO, EventPropertyInfoArray) + 2 * sizeof(EVENT_PROPERTY_INFO);
            std::vector<uint8_t> metadata(offset + (names[0].size() + names[1].size() + 2) * sizeof(wchar_t));
            auto info = reinterpret_cast<TRACE_EVENT_INFO*>(metadata.data());
            info->PropertyCount = info->TopLevelPropertyCount = 2;
            for (size_t index = 0; index < 2; ++index) {
                auto& property = info->EventPropertyInfoArray[index];
                property.NameOffset = (ULONG)offset;
                property.count = 1;
                property.length = index == 0 ? 4 : 0;
                property.nonStructType.InType = (USHORT)(index == 0 ? TDH_INTYPE_UINT32 :
                    kernelProvider ? TDH_INTYPE_UNICODESTRING : TDH_INTYPE_ANSISTRING);
                auto bytes = (names[index].size() + 1) * sizeof(wchar_t);
                std::memcpy(metadata.data() + offset, names[index].c_str(), bytes);
                offset += bytes;
            }
            consumer.mMetadata.metadata_.emplace(
                EventMetadataKey{record.EventHeader.ProviderId, record.EventHeader.EventDescriptor}, std::move(metadata));
            const uint32_t pid = 42;
            const wchar_t unicodeName[] = L"C:\\games\\app.exe";
            const char ansiName[] = "app.exe";
            auto nameBytes = kernelProvider ? sizeof(unicodeName) : sizeof(ansiName);
            std::vector<uint8_t> payload(sizeof(pid) + nameBytes);
            std::memcpy(payload.data(), &pid, sizeof(pid));
            if (kernelProvider) std::memcpy(payload.data() + sizeof(pid), unicodeName, nameBytes);
            else std::memcpy(payload.data() + sizeof(pid), ansiName, nameBytes);
            record.UserData = payload.data();
            record.UserDataLength = (USHORT)payload.size();
            consumer.HandleProcessEvent(&record);
            std::vector<ProcessEvent> events;
            consumer.DequeueProcessEvents(events);
            ASSERT_EQ(1u, events.size());
            bool rundown = kernelProvider ? type == 15 : type == 3 || type == 4;
            bool start = kernelProvider ? type != 2 : type == 1 || type == 3;
            EXPECT_EQ(42u, events[0].ProcessId);
            EXPECT_EQ(10000u, events[0].QpcTime);
            EXPECT_EQ(start, events[0].IsStartEvent);
            EXPECT_EQ(rundown, events[0].IsRundown);
            if (start || rundown) EXPECT_EQ(L"app.exe", events[0].ImageFileName);
            EXPECT_EQ(start || rundown, consumer.mLatestPingTimestampByProcessId.contains(42));
        }
    }
}

TEST_F(OutputThreadTests, StartAfterProvisionalPresentsCreatesNewLifetime)
{
    auto old = Lookup(Present(10000));
    ProcessProcessEvent(Event(11, 12000, true));
    auto current = Lookup(Present(13000));
    EXPECT_NE(old, current);
    EXPECT_EQ(12000u, *old->mTerminationTime);
    EXPECT_EQ(old, Lookup(Present(11000)));
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, DelayedStartSharesResolvedCsvWithoutTruncatingRows)
{
    CsvDirectory directory;
    auto output = (directory.path / L"capture.csv").wstring();
    testArgs.mOutputCsvFileName = output.c_str();
    ProcessProcessEvent(Event(11, 1000, true));
    auto tracked = Lookup(Present(2000));
    WriteRow(session, tracked, 11, 2000);
    ProcessProcessEvent(Event(11, 3000, false));
    auto provisional = Lookup(Present(500));
    ASSERT_NE(nullptr, provisional);
    ASSERT_NE(tracked->mOutputCsv, provisional->mOutputCsv);
    ProcessProcessEvent(Event(11, 400, true));
    EXPECT_EQ(tracked->mOutputCsv, provisional->mOutputCsv);
    WriteRow(session, provisional, 11, 500);
    auto path = tracked->mOutputCsv->mPath;
    CloseMultiCsv(tracked);
    auto rows = FileRows(path);
    ASSERT_EQ(3u, rows.size());
    EXPECT_EQ(0u, rows[0].find(L"Application,"));
    EXPECT_EQ(0u, rows[1].find(L"app.exe,"));
    EXPECT_EQ(0u, rows[2].find(L"app.exe,"));
}

TEST_F(OutputThreadTests, RealtimeNameResolutionSharesAnExistingCsv)
{
    testArgs.mEtlFileName = nullptr;
    ProcessInfo live;
    auto pid = GetCurrentProcessId();
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto period = util::GetTimestampFrequencyUint64();
    ASSERT_GT(live.mCreationTimestamp, 3 * period);
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp - 3 * period, true, live.mModuleName.c_str()));
    auto old = &gProcesses.begin()->second;
    auto file = Capture(old);
    ASSERT_TRUE(file);
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp - 2 * period, false));
    ProcessProcessEvent(Event(pid, live.mCreationTimestamp, true, L"<unknown>"));
    auto current = Lookup(Present((uint64_t)util::GetCurrentTimestamp(), pid));
    ASSERT_NE(nullptr, current);
    EXPECT_NE(old, current);
    EXPECT_EQ(live.mModuleName, current->mModuleName);
    EXPECT_EQ(old->mOutputCsv, current->mOutputCsv);
    EXPECT_EQ(file, current->mOutputCsv->mFile);
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, HistoricalUnknownLifetimeDoesNotCountAsAnActiveTarget)
{
    for (bool stopped : {false, true}) {
        SCOPED_TRACE(stopped);
        Reset(0);
        testArgs.mTerminateOnProcExit = true;
        ProcessProcessEvent(Event(11, 10000, true));
        auto current = Lookup(Present(16000));
        if (stopped) ProcessProcessEvent(Event(11, 17000, false));
        auto historical = Lookup(Present(5000));
        ASSERT_NE(nullptr, historical);
        EXPECT_NE(current, historical);
        EXPECT_EQ(L"<unknown>", historical->mModuleName);
        EXPECT_EQ(10000u, historical->mTerminationTime.value_or(0));
        EXPECT_EQ(stopped ? 0u : 1u, gTargetProcessCount);
        ProcessProcessEvent(Event(11, 4000, true));
        EXPECT_EQ(historical, Lookup(Present(5000)));
        EXPECT_EQ(stopped ? 0u : 1u, gTargetProcessCount);
        testArgs.mEtlFileName = nullptr;
        PruneOldSwapChainData(session, 20000);
        EXPECT_EQ(1u, gProcesses.size());
        ProcessProcessEvent(Event(11, 18000, false));
        EXPECT_EQ(0u, gTargetProcessCount);
        EXPECT_EQ(1u, exitRequests);
    }
}

TEST_F(OutputThreadTests, HistoricalStartDoesNotCountAsAnActiveTarget)
{
    for (bool stopped : {false, true}) {
        SCOPED_TRACE(stopped);
        Reset(0);
        testArgs.mTerminateOnProcExit = true;
        ProcessProcessEvent(Event(11, 10000, true));
        if (stopped) ProcessProcessEvent(Event(11, 17000, false));
        ProcessProcessEvent(Event(11, 4000, true));
        auto historical = FindProcess(11, 5000);
        ASSERT_NE(gProcesses.end(), historical);
        EXPECT_EQ(10000u, historical->second.mTerminationTime.value_or(0));
        EXPECT_EQ(stopped ? 0u : 1u, gTargetProcessCount);
        ProcessProcessEvent(Event(11, 4000, true));
        ProcessProcessEvent(Event(11, 18000, false));
        EXPECT_EQ(0u, gTargetProcessCount);
        EXPECT_EQ(1u, exitRequests);
        EXPECT_EQ(&historical->second, Lookup(Present(5000)));
        EXPECT_EQ(0u, gTargetProcessCount);
    }
}

TEST_F(OutputThreadTests, DelayedStartAfterStopDoesNotReactivateLifetime)
{
    auto old = Lookup(Present(10000));
    ProcessProcessEvent(Event(11, 11000, false));
    ProcessProcessEvent(Event(11, 9000, true));
    ProcessProcessEvent(Event(11, 9000, true));
    EXPECT_EQ(old, Lookup(Present(10500)));
    EXPECT_EQ(11000u, *old->mTerminationTime);
    EXPECT_EQ(0u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, DelayedStartRefreshesProvisionalTargetSelection)
{
    testArgs.mTargetProcessNames = {L"app"};
    EXPECT_EQ(nullptr, Lookup(Present(10000)));
    auto old = FindProcess(11, 10000);
    ASSERT_NE(gProcesses.end(), old);
    auto previous = &old->second;
    ProcessProcessEvent(Event(11, 9000, true));
    EXPECT_EQ(previous, Lookup(Present(11000)));
    EXPECT_EQ(1u, gTargetProcessCount);
    ProcessProcessEvent(Event(11, 9000, true));
    EXPECT_EQ(1u, gTargetProcessCount);
}

TEST_F(OutputThreadTests, UnknownStartPreservesTheResolvedRealtimeNameAndFiltering)
{
    testArgs.mEtlFileName = nullptr;
    auto pid = GetCurrentProcessId();
    ProcessInfo live;
    QueryProcessName(pid, &live);
    ASSERT_NE(nullptr, live.mHandle);
    CloseHandle(live.mHandle);
    auto canonicalName = live.mModuleName;
    CanonicalizeProcessName(&canonicalName);
    auto firstTime = live.mCreationTimestamp + util::GetTimestampFrequencyUint64() / 100;
    for (auto name : {L"", L"<unknown>"}) {
        for (bool target : {false, true}) {
            SCOPED_TRACE(name);
            SCOPED_TRACE(target);
            Reset(0);
            testArgs.mEtlFileName = nullptr;
            testArgs.mCSVOutput = CSVOutput::None;
            if (target) testArgs.mTargetProcessNames = {canonicalName};
            else testArgs.mExcludeProcessNames = {canonicalName};
            Lookup(Present(firstTime, pid));
            auto process = FindProcess(pid, firstTime);
            ASSERT_NE(gProcesses.end(), process);
            auto original = &process->second;
            ProcessProcessEvent(Event(pid, live.mCreationTimestamp, true, name));
            EXPECT_EQ(live.mModuleName, original->mModuleName);
            EXPECT_TRUE(original->mHasStartEvent);
            EXPECT_EQ(target, original->mIsTargetProcess);
            EXPECT_EQ(target ? 1u : 0u, gTargetProcessCount);
            EXPECT_EQ(target ? original : nullptr, Lookup(Present(firstTime + 1, pid)));
            EXPECT_EQ(1u, gProcesses.size());
        }
    }
}

TEST_F(OutputThreadTests, UnknownReplacementDoesNotInheritThePreviousLifetimesName)
{
    for (auto name : {L"", L"<unknown>"}) {
        SCOPED_TRACE(name);
        Reset(0);
        testArgs.mCSVOutput = CSVOutput::None;
        testArgs.mTargetProcessNames = {L"app"};
        ProcessProcessEvent(Event(11, 1000, true));
        auto old = &gProcesses.begin()->second;
        ProcessProcessEvent(Event(11, 2000, false));
        ProcessProcessEvent(Event(11, 3000, true, name));
        auto current = FindProcess(11, 4000);
        ASSERT_NE(gProcesses.end(), current);
        EXPECT_NE(old, &current->second);
        EXPECT_EQ(L"app.exe", old->mModuleName);
        EXPECT_EQ(name, current->second.mModuleName);
        EXPECT_EQ(0u, gTargetProcessCount);
        EXPECT_EQ(nullptr, Lookup(Present(4000)));
    }
}

TEST_F(OutputThreadTests, EtlCsvStreamsAreBoundedAndReopenWithoutLosingRowsOrHeaders)
{
    CsvDirectory directory;
    auto output = (directory.path / L"capture.csv").wstring();
    testArgs.mOutputCsvFileName = output.c_str();
    auto count = std::min((uint32_t)768, (uint32_t)_getmaxstdio() + 8);
    ProcessInfo* first = nullptr;
    std::wstring firstPath;
    for (uint32_t pid = 10000; pid < 10000 + count; ++pid) {
        ProcessProcessEvent(Event(pid, 1, true));
        auto info = Lookup(Present(10000, pid));
        WriteRow(session, info, pid, 10000);
        ASSERT_TRUE(info->mOutputCsv->mFile);
        if (first == nullptr) {
            first = info;
            firstPath = info->mOutputCsv->mPath;
        }
        ProcessProcessEvent(Event(pid, 11000, false));
    }
    ASSERT_NE(nullptr, first);
    EXPECT_FALSE(first->mOutputCsv->mFile);
    auto open = std::count_if(gProcesses.begin(), gProcesses.end(),
        [](auto const& entry) { return entry.second.mOutputCsv->mFile != nullptr; });
    EXPECT_LE(open, 64);
    EXPECT_EQ(0u, gTargetProcessCount);
    WriteRow(session, first, 10000, 12000);
    ASSERT_TRUE(first->mOutputCsv->mFile);
    EXPECT_EQ(firstPath, first->mOutputCsv->mPath);
    CloseMultiCsv(first);
    auto rows = FileRows(firstPath);
    ASSERT_EQ(3u, rows.size());
    EXPECT_EQ(0u, rows[0].find(L"Application,"));
    EXPECT_EQ(0u, rows[1].find(L"app.exe,"));
    EXPECT_EQ(0u, rows[2].find(L"app.exe,"));
}

TEST_F(OutputThreadTests, NewRecordingUsesNewCsvPathAndHeader)
{
    CsvDirectory directory;
    auto output = (directory.path / L"capture.csv").wstring();
    testArgs.mOutputCsvFileName = output.c_str();
    testArgs.mHotkeySupport = true;
    ProcessProcessEvent(Event(11, 1, true));
    auto info = Lookup(Present(10000));
    WriteRow(session, info, 11, 10000);
    auto oldPath = info->mOutputCsv->mPath;
    IncrementRecordingCount();
    CloseMultiCsv(info);
    WriteRow(session, info, 11, 11000);
    auto newPath = info->mOutputCsv->mPath;
    EXPECT_NE(oldPath, newPath);
    CloseMultiCsv(info);
    EXPECT_EQ(2u, FileRows(oldPath).size());
    auto rows = FileRows(newPath);
    ASSERT_EQ(2u, rows.size());
    EXPECT_EQ(0u, rows[0].find(L"Application,"));
}
}
