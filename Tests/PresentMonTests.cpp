// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include <generated/version.h>
#include "PresentMonTests.h"

#include <src/gtest-all.cc>

bool EnsureDirectoryCreated(std::wstring path)
{
    for (auto i = path.find(L'\\');; i = path.find(L'\\', i + 1)) {
        std::wstring dir;
        if (i == std::wstring::npos) {
            dir = path;
        } else {
            dir = path.substr(0, i);
        }

        auto attr = GetFileAttributes(dir.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            if (!CreateDirectory(dir.c_str(), NULL)) {
                fprintf(stderr, "error: failed to create directory: %ls\n", dir.c_str());
                return false;
            }
        } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            fprintf(stderr, "error: existing path is not a directory: %ls\n", dir.c_str());
            return false;
        }

        if (i == std::wstring::npos) {
            break;
        }

        path[i] = L'\\';
    }

    return true;
}

namespace {

void DeleteDirectory(std::wstring const& dir)
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
            DeleteDirectory(dir + ff.cFileName + L'\\');
        } else {
            DeleteFile((dir + ff.cFileName).c_str());
        }
    } while (FindNextFile(h, &ff) != 0);

    RemoveDirectory(dir.substr(0, dir.size() - 1).c_str());

    FindClose(h);
}

bool CheckPath(
    char const* commandLineArg,
    std::wstring* str,
    wchar_t const* path,
    bool directory,
    bool* exists)
{
    // If path not specified in command line, use default value
    if (path == nullptr) {
        path = str->c_str();
    }

    // Get full path
    wchar_t fullPath[MAX_PATH];
    auto r = GetFullPathName(path, _countof(fullPath), fullPath, nullptr);
    if (r == 0 || r > _countof(fullPath)) {
        fprintf(stderr, "error: could not get full path for: %ls\n", path);
        fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
        return false;
    }

    // Make sure file exists and is the right type (file vs directory).
    auto attr = GetFileAttributes(fullPath);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (exists == nullptr) { // must exist
            fprintf(stderr, "error: path does not exist: %ls\n", fullPath);
            fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
            return false;
        }
        *exists = false;
    } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != (directory ? FILE_ATTRIBUTE_DIRECTORY : 0u)) {
        fprintf(stderr, "error: path is not a %s: %ls\n", directory ? "directory" : "file", fullPath);
        fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
        return false;
    }

    // Update the string with the full path and end directories with separator.
    str->assign(fullPath);
    if (directory) {
        *str += L'\\';
    }

    return true;
}

}

std::wstring PresentMon::exePath_;
std::wstring outDir_;
bool reportAllCsvDiffs_ = false;
bool warnOnMissingCsv_ = true;
std::wstring diffPath_;

std::string Convert(std::wstring const& src)
{
    std::string dst(WideCharToMultiByte(CP_UTF8, 0, src.c_str(), (int) src.size(), nullptr, 0, nullptr, nullptr), 0);
    WideCharToMultiByte(CP_UTF8, 0, src.c_str(), (int) src.size(), &dst[0], (int) dst.size(), nullptr, nullptr);
    return dst;
}

std::wstring Convert(std::string const& src)
{
    std::wstring dst(MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int) src.size(), nullptr, 0), 0);
    MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int) src.size(), &dst[0], (int) dst.size());
    return dst;
}

int wmain(
    int argc,
    wchar_t** argv)
{
    // Set defaults
    std::wstring goldDir(L"../../Tests/Gold");
    
    // Check for additional test directory from environment variable
    // This allows developers to specify their own test directories without
    // modifying the source code. Set PRESENTMON_ADDITIONAL_TEST_DIR environment
    // variable or use a .runsettings.user file (see template).
    std::wstring additionalTestDir;
    wchar_t* envTestDir = nullptr;
    size_t envTestDirLen = 0;
    if (_wdupenv_s(&envTestDir, &envTestDirLen, L"PRESENTMON_ADDITIONAL_TEST_DIR") == 0 && envTestDir != nullptr) {
        additionalTestDir = envTestDir;
        free(envTestDir);
    }

    {
        // If exe == <dir>/PresentMonTests-<ver>-<platform>.exe use
        // <dir>/PresentMon-<ver>-<platform>.exe as the default PresentMon
        // path.  Otherwise use same the same <dir> but reconstruct
        // <ver>/<platform> from version header and compiler macros.
        wchar_t path[MAX_PATH];
        GetModuleFileName(NULL, path, _countof(path));
        PresentMon::exePath_.assign(path);
        size_t i = PresentMon::exePath_.size();
        for (; i > 0; --i) {
            if (PresentMon::exePath_[i] == '/' || PresentMon::exePath_[i] == '\\') {
                i += 1;
                break;
            }
        }
        if (PresentMon::exePath_.compare(i, 15, L"PresentMonTests") == 0) {
            PresentMon::exePath_.erase(i + 10, 5);
        } else {
            PresentMon::exePath_.erase(i);
            PresentMon::exePath_ += L"PresentMon-";
            if (isdigit(*PRESENT_MON_VERSION)) {
                PresentMon::exePath_ += L"dev";
            } else {
                PresentMon::exePath_ += Convert(PRESENT_MON_VERSION);
            }
            #ifdef _WIN64
            #ifdef _M_ARM64
            PresentMon::exePath_ += L"-ARM64.exe";
            #else
            PresentMon::exePath_ += L"-x64.exe";
            #endif
            #else
            #ifdef _M_ARM
            PresentMon::exePath_ += L"-ARM.exe";
            #else
            PresentMon::exePath_ += L"-x86.exe";
            #endif
            #endif
        }
    }

    {
        // Default output directory = <temp>/PresentMonTestOutput/
        wchar_t path[MAX_PATH];
        GetTempPath(_countof(path), path);
        outDir_.assign(path);
        outDir_ += L"PresentMonTestOutput";
    }

    // If there is a help argument print help information before GoogleTest
    // does.
    auto help = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"--help") == 0 ||
            _wcsicmp(argv[i], L"-h") == 0 ||
            _wcsicmp(argv[i], L"-?") == 0 ||
            _wcsicmp(argv[i], L"/?") == 0) {
            printf(
                "PresentMonTests.exe [options]\n"
                "options:\n"
                "    --presentmon=path    Path to the PresentMon exe path to test (default=%ls).\n"
                "    --golddir=path       Path to directory of test ETLs and gold CSVs (default=%ls).\n"
                "    --opttestdir=path    Optional additional directory of test ETLs and gold CSVs.\n"
                "    --outdir=path        Path to directory for test outputs (default=%%temp%%/PresentMonTestOutput).\n"
                "    --nodelete           Keep the output directory after tests.\n"
                "    --nowarnmissing      Don't warn if a found ETL is missing a gold CSV.\n"
                "    --allcsvdiffs        Report all CSV differences, not just the first.\n"
                "    --diff=path          Start an extra process to compare each differing CSV.\n"
                "\n",
                PresentMon::exePath_.c_str(),
                goldDir.c_str());
            help = true;
            argv[i] = (wchar_t*) L"--help"; // gtest only recognises this one
            break;
        }
    }

    // Note: InitGoogleTest() will remove the arguments it recognizes.
    testing::InitGoogleTest(&argc, argv);
    if (help) {
        return 0;
    }

    // Parse remaining command line arguments for custom commands.
    wchar_t* presentMonPathArg = nullptr;
    wchar_t* goldDirArg = nullptr;
    wchar_t* optTestDirArg = nullptr;
    wchar_t* outDirArg = nullptr;
    bool deleteOutDir = true;
    for (int i = 1; i < argc; ++i) {
        if (_wcsnicmp(argv[i], L"--presentmon=", 13) == 0) {
            presentMonPathArg = argv[i] + 13;
            continue;
        }

        if (_wcsnicmp(argv[i], L"--golddir=", 10) == 0) {
            goldDirArg = argv[i] + 10;
            continue;
        }

        if (_wcsnicmp(argv[i], L"--opttestdir=", 10) == 0) {
            optTestDirArg = argv[i] + 13;
            continue;
        }

        if (_wcsnicmp(argv[i], L"--outdir=", 9) == 0) {
            outDirArg = argv[i] + 9;
            continue;
        }

        if (_wcsicmp(argv[i], L"--nodelete") == 0) {
            deleteOutDir = false;
            continue;
        }

        if (_wcsicmp(argv[i], L"--nowarnmissing") == 0) {
            warnOnMissingCsv_ = false;
            continue;
        }

        if (_wcsicmp(argv[i], L"--allcsvdiffs") == 0) {
            reportAllCsvDiffs_ = true;
            continue;
        }

        if (_wcsnicmp(argv[i], L"--diff=", 7) == 0) {
            diffPath_ = argv[i] + 7;
            continue;
        }

        fprintf(stderr, "error: unrecognized command line argument: %ls.\n", argv[i]);
        fprintf(stderr, "       Use --help command line argument for usage.\n");
        return 1;
    }

    // Check command line arguments...
    bool goldDirExists = true;
    bool outDirExists = true;
    std::wstring optTestDir;
    bool optTestDirExists = false;
    bool hasOptTestDir = optTestDirArg != nullptr;
    
    if (hasOptTestDir) {
        optTestDir = optTestDirArg;
    }
    
    if (!CheckPath("--presentmon", &PresentMon::exePath_, presentMonPathArg, false, nullptr) ||
        !CheckPath("--golddir", &goldDir, goldDirArg, true, &goldDirExists) ||
        !CheckPath("--outdir", &outDir_, outDirArg, true, &outDirExists) ||
        (hasOptTestDir && !CheckPath("--opttestdir", &optTestDir, optTestDirArg, true, &optTestDirExists))) {
        return 1;
    }

    if (goldDirExists) {
        AddGoldEtlCsvTests(goldDir, goldDir.size());
    } else {
        fprintf(stderr, "warning: gold directory does not exist: %ls\n", goldDir.c_str());
        fprintf(stderr, "         Continuing, but no GoldEtlCsvTests.* will run.  Specify a new path\n");
        fprintf(stderr, "         using the --golddir command line argument.\n");
    }

    // Add tests from optional test directory if specified (command line)
    if (hasOptTestDir) {
        printf("INFO: Using optional test directory from command line: %ls\n", optTestDir.c_str());
        if (optTestDirExists) {
            printf("INFO: Adding tests from test directory: %ls\n", optTestDir.c_str());
            AddGoldEtlCsvTests(optTestDir, optTestDir.size());
        } else {
            fprintf(stderr, "warning: optional test directory does not exist: %ls\n", optTestDir.c_str());
            fprintf(stderr, "         Continuing, but no tests from this directory will run.\n");
        }
    }
    
    // Add tests from additional test directory (environment variable)
    if (!additionalTestDir.empty()) {
        printf("INFO: Using additional test directory from environment: %ls\n", additionalTestDir.c_str());
        // Ensure directory ends with backslash
        if (additionalTestDir.back() != L'\\') {
            additionalTestDir += L'\\';
        }
        
        auto attr = GetFileAttributes(additionalTestDir.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            printf("INFO: Adding tests from additional directory: %ls\n", additionalTestDir.c_str());
            AddGoldEtlCsvTests(additionalTestDir, additionalTestDir.size());
        } else {
            fprintf(stderr, "warning: additional test directory does not exist: %ls\n", additionalTestDir.c_str());
        }
    }
    
    if (!hasOptTestDir && additionalTestDir.empty()) {
        printf("INFO: No optional test directories specified.\n");
    }

    if (outDirExists) {
        if (deleteOutDir) {
            fprintf(stderr, "warning: output directory already exists: %ls\n", outDir_.c_str());
            fprintf(stderr, "         Continuing, but directory won't be deleted afterwards.  Use the\n");
            fprintf(stderr, "         --nodelete argument, or delete directory before running, to remove this\n");
            fprintf(stderr, "         warning.\n");
            deleteOutDir = false;
        }
    } else {
        EnsureDirectoryCreated(outDir_);
    }

    // Run all the tests
    int result = RUN_ALL_TESTS();

    // If there were any failures, disable deleting of the output directory.
    if (deleteOutDir && ::testing::UnitTest::GetInstance()->failed_test_count() > 0) {
        fprintf(stderr, "warning: not deleting output directory since there were errors\n");
        fprintf(stderr, "         %ls\n", outDir_.c_str());
        deleteOutDir = false;
    }

    // If we created the output directory and the user told us to, delete the
    // output directory.
    if (deleteOutDir) {
        DeleteDirectory(outDir_);
    }

    return result;
}
