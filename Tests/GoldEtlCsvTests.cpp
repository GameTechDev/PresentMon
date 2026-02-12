// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMonTests.h"

namespace {

struct TestArgs {
    std::wstring etl_;
    std::wstring goldCsv_;
    std::wstring testCsv_;
};

class Tests : public ::testing::Test, TestArgs {
public:
    explicit Tests(TestArgs const& args)
    {
        TestArgs::operator=(args);
    }

    void TestBody() override
    {
        // Open the gold CSV
        PresentMonCsv goldCsv;
        if (!goldCsv.CSVOPEN(goldCsv_)) {
            return;
        }

        // Make sure output directory exists.
        for (auto i = testCsv_.find_last_of(L"/\\"); i == std::wstring::npos || !EnsureDirectoryCreated(testCsv_.substr(0, i)); ) {
            AddTestFailure(__FILE__, __LINE__, "Output directory does not exist!");
            goldCsv.Close();
            return;
        }

        // Generate command line, querying gold CSV to try and match expected
        // data.
        PresentMon pm;
        pm.Add(L"--stop_existing_session");
        pm.AddEtlPath(etl_);
        pm.AddCsvPath(testCsv_);
        for (auto param : goldCsv.params_) {
            pm.Add(param);
        }
        pm.PMSTART();
        pm.PMEXITED();

        // Open test CSV file and check it has the same columns as gold
        PresentMonCsv testCsv;
        if (!testCsv.CSVOPEN(testCsv_)) {
            goldCsv.Close();
            return;
        }

        // Compare gold/test CSV data rows
        for (;;) {
            auto goldDone = !goldCsv.ReadRow();
            auto testDone = !testCsv.ReadRow();
            if (goldDone || testDone) {
                if (!goldDone || !testDone) {
                    AddTestFailure(__FILE__, __LINE__, "GOLD and TEST CSV had different number of rows");
                    printf("GOLD = %ls\n", goldCsv_.c_str());
                    printf("TEST = %ls\n", testCsv_.c_str());
                }
                break;
            }

            auto rowOk = true;
            for (size_t h = 0; h < PresentMonCsv::KnownHeaderCount; ++h) {
                if (testCsv.headerColumnIndex_[h] != SIZE_MAX && goldCsv.headerColumnIndex_[h] != SIZE_MAX) {
                    // Need to protect against missing columns on each line as
                    // the file may be corrupted.
                    auto testColIdx = testCsv.headerColumnIndex_[h];
                    auto goldColIdx = goldCsv.headerColumnIndex_[h];
                    char const* a = testColIdx < testCsv.cols_.size() ? testCsv.cols_[testColIdx] : "<missing>";
                    char const* b = goldColIdx < goldCsv.cols_.size() ? goldCsv.cols_[goldColIdx] : "<missing>";
                    if (_stricmp(a, b) == 0) {
                        continue;
                    }

                    // Different versions of PresentMon may output different decimal precision.  Also, 
                    // floating point may be inconsistently rounded by printf() on different platforms.
                    // Therefore, we do a rounding check by ensuring the difference between the two
                    // numbers is less than 1 in the final printed digit.

                    double testNumber = 0.0;
                    double goldNumber = 0.0;
                    int testSucceededCount = sscanf_s(a, "%lf", &testNumber);
                    int goldSucceededCount = sscanf_s(b, "%lf", &goldNumber);
                    if (testSucceededCount == 1 && goldSucceededCount == 1) {
                        const char* testDecimalAddr = strchr(a, '.');
                        const char* goldDecimalAddr = strchr(b, '.');
                        size_t testDecimalNumbersCount = testDecimalAddr == nullptr ? 0 : ((a + strlen(a)) - testDecimalAddr - 1);
                        size_t goldDecimalNumbersCount = goldDecimalAddr == nullptr ? 0 : ((b + strlen(b)) - goldDecimalAddr - 1);
                        size_t decimals = std::min(testDecimalNumbersCount, goldDecimalNumbersCount);
                        // Cap precision used for tolerance to avoid ultra-long double-to-string artifacts
                        // (e.g., 100.06490000000001 vs 100.06489999999999)
                        decimals = std::min(decimals, size_t(9));
                        double threshold = pow(0.1, decimals);
                        double difference = testNumber - goldNumber;

                        if (difference > -threshold && difference < threshold) {
                            continue;
                        }
                    }

                    if (rowOk) {
                        rowOk = false;
                        printf("GOLD = %ls\n", goldCsv_.c_str());
                        printf("TEST = %ls\n", testCsv_.c_str());
                        AddTestFailure(__FILE__, __LINE__, "Difference on line: %zu", testCsv.line_);
                        printf("    COLUMN                    TEST VALUE                            GOLD VALUE\n");
                    }

                    auto r = printf("    %s", testCsv.GetHeaderString((PresentMonCsv::Header) h));
                    printf("%*s", r < 29 ? 29 - r : 0, "");
                    r = printf(" %s", a);
                    printf("%*s", r < 38 ? 38 - r : 0, "");
                    printf(" %s\n", b);
                }
            }
            if (!reportAllCsvDiffs_ && !rowOk) {
                break;
            }
        }

        goldCsv.Close();
        testCsv.Close();

        if (::testing::Test::HasFailure() && !diffPath_.empty()) {
            std::wstring cmd;
            cmd += diffPath_;
            cmd += L" \"";
            cmd += goldCsv_;
            cmd += L"\" \"";
            cmd += testCsv_;
            cmd += L"\"";

            STARTUPINFO si = {};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESTDHANDLES;

            PROCESS_INFORMATION pi = {};
            if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi) == 0) {
                printf("error: failed to execute diff request: %ls\n", cmd.c_str());
            } else {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
        }
    }
};

}

void AddGoldEtlCsvTests(
    std::wstring const& dir,
    size_t relIdx)
{
    WIN32_FIND_DATA ff = {};
    auto h = FindFirstFile((dir + L'*').c_str(), &ff);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }
    do
    {
        if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(ff.cFileName, L".") == 0) continue;
            if (wcscmp(ff.cFileName, L"..") == 0) continue;
            AddGoldEtlCsvTests(dir + ff.cFileName + L'\\', relIdx);
        } else {
            // Confirm fileName is an ETL
            auto len = wcslen(ff.cFileName);
            if (len >= 4 && _wcsicmp(ff.cFileName + len - 4, L".etl") == 0) {
                std::wstring etl(dir + ff.cFileName);
                uint32_t csvCount = 0;

                // Add a test for each filename*.csv
                WIN32_FIND_DATA csvff = {};
                auto csvh = FindFirstFile((etl.substr(0, etl.size() - 4) + L"*.csv").c_str(), &csvff);
                if (csvh != INVALID_HANDLE_VALUE) {
                    do
                    {
                        if ((csvff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                            std::wstring fileName(csvff.cFileName);

                            TestArgs args;
                            args.etl_     = etl;
                            args.goldCsv_ = dir + fileName;
                            args.testCsv_ = outDir_ + fileName;

                            // Replace any '-' characters in the name, as they will screw up googletest
                            // filters.
                            std::string name(Convert(fileName.substr(0, fileName.size() - 4)));
                            for (auto& ch : name) {
                                if (ch == '-') {
                                    ch = '_';
                                }
                            }

                            ::testing::RegisterTest(
                                "GoldEtlCsvTests", name.c_str(), nullptr, nullptr, __FILE__, __LINE__,
                                [=]() -> ::testing::Test* { return new Tests(std::move(args)); });

                            csvCount += 1;
                        }
                    } while (FindNextFile(csvh, &csvff) != 0);

                    FindClose(csvh);
                }

                if (csvCount == 0 && warnOnMissingCsv_) {
                    fprintf(stderr, "warning: missing gold CSV for: %ls\n", etl.c_str());
                }
            }
        }
    } while (FindNextFile(h, &ff) != 0);

    FindClose(h);
}
