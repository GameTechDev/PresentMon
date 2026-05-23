// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <functional>
#include "PresentMonTests.h"

template<typename T, typename U> T Convert(U, LARGE_INTEGER const& freq);
template<> uint64_t Convert(char const* u, LARGE_INTEGER const&)      { return strtoull(u, nullptr, 10); }
template<> double   Convert(char const* u, LARGE_INTEGER const&)      { return strtod(u, nullptr); }
template<> uint64_t Convert(uint64_t    u, LARGE_INTEGER const&)      { return u; }
template<> double   Convert(uint64_t    u, LARGE_INTEGER const& freq) { return (double) u / freq.QuadPart; }
template<> double   Convert(double      u, LARGE_INTEGER const&)      { return u; }

namespace {

void TerminateAfterTimedTest(uint32_t timed, DWORD timeoutMilliseconds)
{
    wchar_t timedArg[128];
    _snwprintf_s(timedArg, _TRUNCATE, L"%u", timed);

    PresentMon pm;
    pm.Add(L"--stop_existing_session --terminate_after_timed --timed");
    pm.Add(timedArg);
    pm.PMSTART();
    pm.PMEXITED(timeoutMilliseconds, 0);

    // We don't check the CSV... it's ok if buffers drain adn we get more data
}

void TerminateExistingTest(wchar_t const* sessionName)
{
    PresentMon pm;
    pm.Add(L"--stop_existing_session --no_csv");
    if (sessionName != nullptr) {
        pm.Add(L" --session_name");
        pm.Add(sessionName);
    }
    pm.PMSTART();
    EXPECT_TRUE(pm.IsRunning(1000));

    PresentMon pm2;
    pm2.Add(L"--terminate_existing_session");
    if (sessionName != nullptr) {
        pm2.Add(L" --session_name");
        pm2.Add(sessionName);
    }
    pm2.PMSTART();
    pm2.PMEXITED(1000);
    pm.PMEXITED(1000);
}

template<typename T>
void QpcTimeTest(bool qpc)
{
    LARGE_INTEGER freq = {};
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER qpcMin = {};
    LARGE_INTEGER qpcMax = {};
    std::wstring csvPath(outDir_ + (qpc ? L"qpc_time" : L"qpc_time_ms") + L".csv");

    PresentMon pm;
    pm.Add(L"--stop_existing_session --terminate_after_timed --timed 1 --no_track_gpu --no_track_input --no_track_display --v1_metrics");
    if (qpc) {
        pm.Add(L"--qpc_time");
    } else {
        pm.Add(L"--qpc_time_ms");
    }
    pm.AddCsvPath(csvPath);

    QueryPerformanceCounter(&qpcMin);
    pm.PMSTART();
    pm.PMEXITED(2000, 0);
    QueryPerformanceCounter(&qpcMax);
    if (::testing::Test::HasFailure()) {
        return;
    }

    auto qmin = Convert<T>((uint64_t) qpcMin.QuadPart, freq);
    auto qmax = Convert<T>((uint64_t) qpcMax.QuadPart, freq);

    PresentMonCsv csv;
    if (!csv.CSVOPEN(csvPath)) {
        printf("    PresentMon didn't create a CSV file, likely because there were no presents to capture.\n"
               "    Re-run the test with a graphics application running.\n");
        return;
    }

    auto idxProcessID     = csv.GetColumnIndex("ProcessID");
    auto idxTimeInSeconds = csv.GetColumnIndex("TimeInSeconds");
    auto idxQPCTime       = csv.GetColumnIndex("QPCTime");
    if (idxProcessID     == SIZE_MAX) AddTestFailure(__FILE__, __LINE__, "    Output missing required column: ProcessID");
    if (idxTimeInSeconds == SIZE_MAX) AddTestFailure(__FILE__, __LINE__, "    Output missing required column: TimeInSeconds");
    if (idxQPCTime       == SIZE_MAX) AddTestFailure(__FILE__, __LINE__, "    Output missing required column: QPCTime");
    if (::testing::Test::HasFailure()) {
        return;
    }

    // TimeInSeconds is only ordered per-process, so we track each process separately
    std::unordered_map<uint32_t, std::pair<double, T>> firstMeasurement;

    while (!::testing::Test::HasFailure() && csv.ReadRow()) {
        auto pid = strtoul(csv.cols_[idxProcessID], nullptr, 10);
        auto t = Convert<double>(csv.cols_[idxTimeInSeconds], freq);
        auto q = Convert<T>     (csv.cols_[idxQPCTime],       freq);

        EXPECT_LE(qmin, q);
        EXPECT_LE(q, qmax);

        auto const& first = firstMeasurement.emplace(std::make_pair(pid, std::make_pair(t, q))).first->second;
        t -= first.first;
        q -= first.second;

        auto tq = Convert<double>(q, freq);
        EXPECT_LE(fabs(t - tq), 0.0000010001) // TimeInSeconds format is %.6lf
            << "    t=" << t << std::endl
            << "    tq=" << tq << std::endl
            << "    fabs(t-tq)=" << fabs(t - tq) << std::endl;

        if (::testing::Test::HasFailure()) {
            printf("line %zu\n", csv.line_);
        }
    }
    csv.Close();

    if (csv.line_ <= 1u) {
        AddTestFailure(__FILE__, __LINE__, "    PresentMon didn't capture any presents during the test.\n"
                                           "    Re-run the test with a graphics application running.");
    }

    if (::testing::Test::HasFailure()) {
        printf("%ls\n", csvPath.c_str());
    }
}

}

TEST(CommandLineTests, TerminateAfterTimed_0s)
{
    TerminateAfterTimedTest(0, 2000);
}

TEST(CommandLineTests, TerminateAfterTimed_1s)
{
    TerminateAfterTimedTest(1, 2000);
}

TEST(CommandLineTests, TerminateExisting_Default)
{
    TerminateExistingTest(nullptr);
}

TEST(CommandLineTests, TerminateExisting_Named)
{
    TerminateExistingTest(L"sessionname");
}

TEST(CommandLineTests, TerminateExisting_NotFound)
{
    PresentMon pm;
    pm.Add(L"--terminate_existing_session --session_name session_name_that_hopefully_isnt_in_use");
    pm.PMSTART();
    pm.PMEXITED(1000, 7); // session name not found -> exit code = 7
}

TEST(CommandLineTests, QPCTime)               { QpcTimeTest<uint64_t>(true); }
TEST(CommandLineTests, QPCTimeInMilliSeconds) { QpcTimeTest<double>(false); }

void InputTest(uint32_t v)
{
    std::wstring csvPath(outDir_ + L"input.csv");

    // TODO: This test requires a target application to be presenting, and the
    // user to provide input (move the mouse at minimum) during the capture.
    // Otherwise the test will fail.  Is there a better way than just hoping
    // this happens and failing if not?

    PresentMon pm;
    pm.Add(L"--stop_existing_session --terminate_after_timed --timed 3");
    if (v == 1) {
        pm.Add(L"--v1_metrics");
    }
    pm.AddCsvPath(csvPath);

    pm.PMSTART();
    pm.PMEXITED(5000, 0);
    if (::testing::Test::HasFailure()) {
        return;
    }

    PresentMonCsv csv;
    if (!csv.CSVOPEN(csvPath)) {
        printf("    PresentMon didn't create a CSV file, likely because there were no presents to capture.\n"
               "    Re-run the test with a graphics application running.\n");
        return;
    }

    char const* inputHeader = v == 1 ? "msSinceInput" : "AllInputToPhotonLatency";

    auto idxInputHeader = csv.GetColumnIndex(inputHeader);
    if (idxInputHeader == SIZE_MAX) {
       FAIL() << "    Output missing required column: " << inputHeader;
    }

    uint32_t nonZeroInputRowCount = 0;
    while (!::testing::Test::HasFailure() && csv.ReadRow()) {
        auto inputValue = strtod(csv.cols_[idxInputHeader], nullptr);
        if (inputValue != 0) {
            nonZeroInputRowCount += 1;
        }
    }
    csv.Close();

    ASSERT_GT(csv.line_, 1u)
        << "    PresentMon didn't capture any presents during the test.\n"
           "    Re-run the test with a graphics application running.";

    EXPECT_GT(nonZeroInputRowCount, 0u)
        << "    PresentMon didn't capture any inputs during the test.\n"
           "    Re-run the test and make sure to interact with the kb or mouse.";

    if (::testing::Test::HasFailure()) {
        printf("%ls\n", csvPath.c_str());
    }
}

TEST(CommandLineTests, Input_v1) { InputTest(1); }
TEST(CommandLineTests, Input_v2) { InputTest(2); }

TEST(CommandLineTests, HotkeyWithoutCsv)
{
    PresentMon pm;
    pm.Add(L"--stop_existing_session --no_csv --hotkey F11");
    pm.PMSTART();
    EXPECT_TRUE(pm.IsRunning(1000));

    // Simulate F11 key press
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_F11;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_F11;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, input, sizeof(INPUT));

    // Wait for hotkey to be processed
    Sleep(1000);

    // Check the output
    std::string output = pm.GetOutput();
    EXPECT_NE(output.find("Started recording."), std::string::npos);

    // Terminate the process
    pm.PMEXITED(1000, 0);
}
