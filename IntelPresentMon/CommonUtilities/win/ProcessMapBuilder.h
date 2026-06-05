// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "WinAPI.h"
#include "Process.h"
#include <unordered_map>
#include <tlhelp32.h>
#include <vector>


namespace pmon::util::win
{
    // TODO: simplify this so the weird Extract() flow and the need for a wrapper struct goes away
    class ProcessMapBuilder
    {
    public:
        using ProcMap = std::unordered_map<DWORD, Process>;
        using NameMap = std::unordered_map<std::wstring, Process>;
        ProcessMapBuilder();
        ProcMap Extract();
        const ProcMap& Peek() const;
        void FillWindowHandles();
        void FilterHavingWindow();
        void FilterHavingAncestor(DWORD pidRoot);
        std::vector<DWORD> GetChildTreePostOrder(DWORD pidRoot) const;
        NameMap AsNameMap(bool lowercase = false) const;
        std::wstring ToString() const;
        static std::wstring MapToString(const ProcMap& map);
    private:
        void AppendChildTreePostOrder_(DWORD pidRoot, const std::unordered_multimap<DWORD, DWORD>& parentMap, std::vector<DWORD>& order) const;
        void PopulateProcessMap_();
        static bool WindowIsMain_(HWND hWnd);
        static BOOL CALLBACK EnumWindowsCallback_(HWND hWnd, LPARAM lParam);
        ProcMap map;
    };
}
