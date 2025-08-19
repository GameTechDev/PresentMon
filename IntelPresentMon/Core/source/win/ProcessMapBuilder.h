// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "WinAPI.h"
#include <CommonUtilities/win/Process.h>
#include <unordered_map>
#include <tlhelp32.h>


namespace p2c::win
{
    // TODO: simplify this so the weird Extract() flow and the need for a wrapper struct goes away
    class ProcessMapBuilder
    {
    public:
        using ProcMap = std::unordered_map<DWORD, ::pmon::util::win::Process>;
        using NameMap = std::unordered_map<std::wstring, ::pmon::util::win::Process>;
        ProcessMapBuilder();
        ProcMap Extract();
        const ProcMap& Peek() const;
        void FillWindowHandles();
        void FilterHavingWindow();
        void FilterHavingAncestor(DWORD pidRoot);
        NameMap AsNameMap(bool lowercase = false) const;
        std::wstring ToString() const;
        static std::wstring MapToString(const ProcMap& map);
    private:
        void PopulateProcessMap_();
        static bool WindowIsMain_(HWND hWnd);
        static BOOL CALLBACK EnumWindowsCallback_(HWND hWnd, LPARAM lParam);
        ProcMap map;
    };
}