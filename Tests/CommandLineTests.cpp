/*
Copyright 2020 Intel Corporation

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
    pm.Add(L"-stop_existing_session -terminate_after_timed -timed");
    pm.Add(timedArg);
    pm.PMSTART();
    pm.PMEXITED(timeoutMilliseconds, 0);

    // We don't check the CSV... it's ok if buffers drain adn we get more data
}

void TerminateExistingTest(wchar_t const* sessionName)
{
    PresentMon pm;
    pm.Add(L"-stop_existing_session -no_csv");
    if (sessionName != nullptr) {
        pm.Add(L" -session_name");
        pm.Add(sessionName);
    }
    pm.PMSTART();
    EXPECT_TRUE(pm.IsRunning(1000));

    PresentMon pm2;
    pm2.Add(L"-terminate_existing");
    if (sessionName != nullptr) {
        pm2.Add(L" -session_name");
        pm2.Add(sessionName);
    }
    pm2.PMSTART();
    pm2.PMEXITED(1000);
    pm.PMEXITED(1000);
}

template<typename T>
void QpcTimeTest(wchar_t const* qpcTimeArg)
{
    LARGE_INTEGER freq = {};
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER qpcMin = {};
    LARGE_INTEGER qpcMax = {};
    std::wstring csvPath(outDir_ + (qpcTimeArg + 1) + L".csv");

    // TODO: Seems to work, but how can we make sure to capture presents during
    // this 1 second? Do we need to also launch a presenting process?
    PresentMon pm;
    pm.Add(L"-stop_existing_session -terminate_after_timed -timed 1 -simple");
    pm.Add(qpcTimeArg);
    pm.AddCsvPath(csvPath);

    QueryPerformanceCounter(&qpcMin);
    pm.PMSTART();
    pm.PMEXITED(2000, 0);
    QueryPerformanceCounter(&qpcMax);
    if (::testing::Test::HasFailure()) {
        return;
    }

    PresentMonCsv csv;
    if (!csv.CSVOPEN(csvPath)) {
        return;
    }

    auto idxTimeInSeconds = csv.GetColumnIndex("TimeInSeconds");
    auto idxQPCTime       = csv.GetColumnIndex("QPCTime");
    EXPECT_NE(idxTimeInSeconds, SIZE_MAX);
    EXPECT_NE(idxQPCTime,       SIZE_MAX);

    auto t0 = 0.0;
    auto q0 = (T) 0;
    auto qmin = Convert<T>((uint64_t) qpcMin.QuadPart, freq);
    auto qmax = Convert<T>((uint64_t) qpcMax.QuadPart, freq);
    while (!::testing::Test::HasFailure() && csv.ReadRow()) {
        auto t = Convert<double>(csv.cols_[idxTimeInSeconds], freq);
        auto q = Convert<T>     (csv.cols_[idxQPCTime],       freq);
        if (csv.line_ == 2) {
            t0 = t;
            q0 = q;
        }

        EXPECT_LE(qmin, q);
        EXPECT_LE(q, qmax);

        t -= t0;
        q -= q0;

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
    pm.Add(L"-terminate_existing -session_name session_name_that_hopefully_isnt_in_use");
    pm.PMSTART();
    pm.PMEXITED(1000, 7); // session name not found -> exit code = 7
}

TEST(CommandLineTests, QPCTime)
{
    QpcTimeTest<uint64_t>(L"-qpc_time");
}

TEST(CommandLineTests, QPCTimeInSeconds)
{
    QpcTimeTest<double>(L"-qpc_time_s");
}
