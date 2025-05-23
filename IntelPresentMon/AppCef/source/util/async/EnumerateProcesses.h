// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/ProcessMapBuilder.h>
#include <Core/source/kernel/Kernel.h>
#include <CommonUtilities/win/Handle.h>
#include "../AsyncEndpoint.h"
#include "../CefValues.h"
#include <ranges>

namespace p2c::client::util::async
{
    class EnumerateProcesses : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "enumerateProcesses"; }
        EnumerateProcesses() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}
        // {} => {processes: [{name: string, pid: uint}]}
        Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor&) const override
        {
            namespace vi = std::views;
            // enumerate processes on system
            win::ProcessMapBuilder builder;
            builder.FillWindowHandles();
            builder.FilterHavingWindow();
            auto pmap = builder.Extract();
            // get window titles for each proc
            std::vector<kern::Process> list;
            list.reserve(pmap.size());
            for (auto& entry : pmap) {
                using Win32Handle = ::pmon::util::win::Handle;
                kern::Process proc = std::move(entry.second);
                if (proc.hWnd) {
                    proc.windowName = win::GetWindowTitle(proc.hWnd);
                }
                list.push_back(std::move(proc));
            }
            // convert to cef objects
            auto procListCef = MakeCefList(list.size());
            for (auto&&[i, proc] : list | vi::enumerate) {
                procListCef->SetValue(i, MakeCefObject(
                    CefProp{ "name", std::move(proc.name) },
                    CefProp{ "pid", proc.pid },
                    CefProp{ "windowName", std::move(proc.windowName) }
                ));
            }
            return Result{ true, MakeCefObject(CefProp{ "processes", CefValueDecay(std::move(procListCef)) }) };
        }
    };
}