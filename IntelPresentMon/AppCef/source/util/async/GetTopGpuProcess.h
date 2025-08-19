// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../CefValues.h"
#include <unordered_set>
#include <ranges>
#include <Core/source/win/GpuUtilization.h>
#include <Core/source/win/ProcessMapBuilder.h>
#include <CommonUtilities/str/String.h>

namespace p2c::client::util::async
{
    class GetTopGpuProcess : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "getTopGpuProcess"; }
        GetTopGpuProcess() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}
        // {blacklist: uint[]} => {top: Process|null}
        Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor&) const override
        {
            namespace rn = std::ranges;
            namespace vi = rn::views;

            // get a list of possible target processes currently running
            win::ProcessMapBuilder builder;
            builder.FillWindowHandles();
            builder.FilterHavingWindow();
            const auto procMap = builder.Extract();
            std::vector<::pmon::util::win::Process> procList;
            for (auto&&[pid, proc] : procMap) {
                procList.push_back(static_cast<const ::pmon::util::win::Process&>(proc));
            }
            // check if a blacklist was provided
            if (pArgObj->GetType() != CefValueType::VTYPE_NULL) {
                // convert blacklist to vector
                auto blacklist = CefValueTraverser(pArgObj)["blacklist"].ToVector<std::string>();
                // build hash table set from vector
                std::unordered_set<std::string> blacklistSet(blacklist.begin(), blacklist.end());
                // remove candidate target processes that match blacklist (lower case compare)
                std::erase_if(procList, [&](const ::pmon::util::win::Process& proc) {
                    auto narrowName = ::pmon::util::str::ToNarrow(proc.name);
                    for (auto& c : narrowName) {
                        c = std::tolower(c);
                    }
                    return blacklistSet.contains(narrowName);
                });
            }
            // get top utilization among candidates
            if (const auto top = win::GetTopGpuProcess(procList)) {
                auto& proc = procMap.at(*top);
                std::wstring windowName;
                if (proc.hWnd) {
                    windowName = win::GetWindowTitle(proc.hWnd);
                }
                auto returnObj = MakeCefObject(
                    CefProp{ "name", proc.name },
                    CefProp{ "pid", proc.pid },
                    CefProp{ "windowName", std::move(windowName) }
                );
                return Result{ true, MakeCefObject(CefProp{ "top", std::move(returnObj) })};
            }
            else {
                return Result{ true, MakeCefObject(CefProp{ "top", CefValueNull() })};
            }
        }
    };
}