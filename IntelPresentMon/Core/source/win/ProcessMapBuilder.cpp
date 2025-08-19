// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ProcessMapBuilder.h"
#include <Core/source/infra/Logging.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/win/Handle.h>
#include <CommonUtilities/win/HrError.h>
#include <unordered_set>
#include <ranges>
#include <cwctype>


namespace p2c::win
{
    namespace rn = std::ranges;
    namespace vi = rn::views;
    using namespace ::pmon::util;
    namespace cwin = ::pmon::util::win;

    ProcessMapBuilder::ProcessMapBuilder()
    {
        // enumerate all processes
        PopulateProcessMap_();
    }

    ProcessMapBuilder::ProcMap ProcessMapBuilder::Extract()
    {
        return std::move(map);
    }

    const ProcessMapBuilder::ProcMap& ProcessMapBuilder::Peek() const
    {
        return map;
    }

    void ProcessMapBuilder::FillWindowHandles()
    {
        // associate main windows with processes
        EnumWindows(&EnumWindowsCallback_, (LPARAM)this);
    }

    void ProcessMapBuilder::FilterHavingWindow()
    {
        // prune all processes without a main window
        std::erase_if(map, [](const ProcMap::value_type& v) { return !v.second.hWnd; });
    }

    void ProcessMapBuilder::FilterHavingAncestor(DWORD pidRoot)
    {
        // building a map of parentId => childId
        std::unordered_multimap<DWORD, DWORD> parentMap;
        for (auto& e : map) {
            if (e.second.parentId) {
                parentMap.emplace(e.second.parentId, e.second.pid);
            }
        }
        std::unordered_set<DWORD> whiteList;
        std::function<void(DWORD)> visit;
        visit = [&](DWORD pidRoot) {
            // add root to whitelist
            whiteList.emplace(pidRoot);
            // visit all children
            const auto r = parentMap.equal_range(pidRoot);
            for (auto i = r.first; i != r.second; i++) {
                visit(i->second);
            }
        };
        // recursive building of ancestry tree (set)
        visit(pidRoot);
        // prune all processes not in ancestry set
        std::erase_if(map, [&](const ProcMap::value_type& v) {
            return !whiteList.contains(v.second.pid);
        });
    }

    ProcessMapBuilder::NameMap ProcessMapBuilder::AsNameMap(bool lowercase) const
    {
        NameMap nameMap;
        for (const auto& e : map) {
            if (lowercase) {
                nameMap.emplace(std::make_pair(
                    e.second.name
                        | vi::transform([](auto c){return(wchar_t)std::towlower(c);})
                        | rn::to<std::basic_string>(),
                    e.second
                ));
            }
            else {
                nameMap.emplace(std::make_pair(e.second.name, e.second));
            }
        }
        return nameMap;
    }

    std::wstring ProcessMapBuilder::ToString() const
    {
        return MapToString(map);
    }

    std::wstring ProcessMapBuilder::MapToString(const ProcMap& map)
    {
        std::wstring buffer;
        for (auto& e : map) {
            auto& p = e.second;
            buffer.append(std::format(
                L"[pid] {:5} | [pai] {:5} | [hwd] {:7} | [nam] {}\n",
                (int)p.pid, (int)p.parentId, reinterpret_cast<uintptr_t>(p.hWnd), p.name
            ));
        }
        return buffer;
    }

    void ProcessMapBuilder::PopulateProcessMap_()
    {
        PROCESSENTRY32 process_info{};
        process_info.dwSize = sizeof(process_info);

        auto processes_snapshot = (cwin::Handle)CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (processes_snapshot == INVALID_HANDLE_VALUE)
        {
            pmlog_error("Failed to get process snapshot").hr().raise<cwin::HrError>();
        }

        Process32FirstW(processes_snapshot, &process_info);

        for (BOOL done = Process32FirstW(processes_snapshot, &process_info);
            done; done = Process32NextW(processes_snapshot, &process_info))
        {
            map.emplace(process_info.th32ProcessID, ::pmon::util::win::Process{
                .pid =      process_info.th32ProcessID,
                .parentId = process_info.th32ParentProcessID,
                .name =     process_info.szExeFile,
            });
        }
    }

    bool ProcessMapBuilder::WindowIsMain_(HWND hWnd)
    {
        return GetWindow(hWnd, GW_OWNER) == nullptr && IsWindowVisible(hWnd);
    }

    BOOL CALLBACK ProcessMapBuilder::EnumWindowsCallback_(HWND hWnd, LPARAM lParam)
    {
        auto& builder = *reinterpret_cast<ProcessMapBuilder*>(lParam);

        DWORD pid = 0;
        GetWindowThreadProcessId(hWnd, &pid);

        if (auto i = builder.map.find(pid); i != builder.map.end())
        {
            if (i->second.hWnd == nullptr && WindowIsMain_(hWnd))
            {
                i->second.hWnd = hWnd;
            }
        }

        return TRUE;
    }
}