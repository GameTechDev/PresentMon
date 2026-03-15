// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <format>
#include <cassert>
#include "Exceptions.h"

namespace pmon::tel
{
    class DllModule
    {
    public:
        DllModule(const DllModule& t) = delete;
        DllModule& operator=(const DllModule& t) = delete;
        DllModule(const std::vector<std::string>& dllFiles)
        {
            assert(!dllFiles.empty());

            // try list of dll candidates in order
            for (const auto& file : dllFiles) {
                if (hModule = LoadLibraryExA(file.c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32)) {
                    return;
                }
            }
            // if all failed to load, throw exception
            auto msg = std::format("Unable to locate telemetry library, last DLL tried: {}", dllFiles.back().c_str());
            throw Except<TelemetrySubsystemAbsent>(std::move(msg));
        }
        ~DllModule()
        {
            FreeLibrary(hModule);
        }
        void* GetProcAddress(const std::string& procName) const
        {
            return ::GetProcAddress(hModule, procName.c_str());
        }
    private:
        HMODULE hModule = nullptr;
    };
}
