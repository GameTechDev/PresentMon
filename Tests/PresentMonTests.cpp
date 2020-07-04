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
#include <generated/version.h>
#include "PresentMonTests.h"

bool EnsureDirectoryCreated(std::wstring path)
{
    auto dir = path.c_str();
    for (auto i = path.find(L'\\');; i = path.find(L'\\', i + 1)) {
        if (i != std::wstring::npos) {
            path[i] = L'\0';
        }

        auto attr = GetFileAttributes(dir);
        if (attr == INVALID_FILE_ATTRIBUTES) {
            if (!CreateDirectory(dir, NULL)) {
                fprintf(stderr, "error: failed to create directory: %ls\n", dir);
                return false;
            }
        } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            fprintf(stderr, "error: existing path is not a directory: %ls\n", dir);
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
    *str = fullPath;
    if (directory) {
        *str += L'\\';
    }

    return true;
}

}

std::wstring PresentMon::exePath_;
std::wstring outDir_;

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
    PresentMon::exePath_ = L"PresentMon-";
    if (strncmp(PRESENT_MON_VERSION, "dev", 3) == 0) {
        PresentMon::exePath_ += L"dev";
    } else {
        PresentMon::exePath_ += Convert(PRESENT_MON_VERSION);
    }
    PresentMon::exePath_ += L"-x64.exe";

    std::wstring goldDir(L"../../Tests/Gold");

    {
        wchar_t path[MAX_PATH];
        GetTempPath(_countof(path), path);
        outDir_ = path;
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
                "    --outdir=path        Path to directory for test outputs (default=%%temp%%/PresentMonTestOutput).\n"
                "    --nodelete           Keep the output directory after tests.\n"
                "    --allcsvdiffs        Report all CSV differences, not just the first.\n"
                "\n",
                PresentMon::exePath_.c_str(),
                goldDir.c_str());
            help = true;
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
    wchar_t* outDirArg = nullptr;
    bool deleteOutDir = true;
    bool reportAllCsvDiffs = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsnicmp(argv[i], L"--presentmon=", 13) == 0) {
            presentMonPathArg = argv[i] + 13;
            continue;
        }

        if (_wcsnicmp(argv[i], L"--golddir=", 10) == 0) {
            goldDirArg = argv[i] + 10;
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

        if (_wcsicmp(argv[i], L"--allcsvdiffs") == 0) {
            reportAllCsvDiffs = true;
            continue;
        }

        fprintf(stderr, "error: unrecognized command line argument: %ls.\n", argv[i]);
        fprintf(stderr, "       Use --help command line argument for usage.\n");
        return 1;
    }

    // Check command line arguments...
    bool goldDirExists = true;
    bool outDirExists = true;
    if (!CheckPath("--presentmon", &PresentMon::exePath_, presentMonPathArg, false, nullptr) ||
        !CheckPath("--golddir", &goldDir, goldDirArg, true, &goldDirExists) ||
        !CheckPath("--outdir", &outDir_, outDirArg, true, &outDirExists)) {
        return 1;
    }

    if (goldDirExists) {
        AddGoldEtlCsvTests(goldDir, goldDir.size(), reportAllCsvDiffs);
    } else {
        fprintf(stderr, "warning: gold directory does not exist: %ls\n", goldDir.c_str());
        fprintf(stderr, "         Continuing, but no GoldEtlCsvTests.* will run.  Specify a new path\n");
        fprintf(stderr, "         using the --golddir command line argument.\n");
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
