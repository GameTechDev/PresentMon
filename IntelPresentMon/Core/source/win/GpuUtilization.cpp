// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "GpuUtilization.h"
#include <CommonUtilities/rng/PairToRange.h>
#include <CommonUtilities/Exception.h>
#include <chrono>
#include <vector>
#include <regex>
#include <ranges>
#include <format>
#include <thread>
#include <unordered_map>

#include <pdh.h>
#include <pdhmsg.h>

namespace rn = std::ranges;
namespace vi = rn::views;
using namespace pmon::util;

namespace p2c::win
{
    PM_DEFINE_EX(PdhException);

    std::optional<uint32_t> GetTopGpuProcess(const std::vector<::pmon::util::win::Process>& candidates)
	{
        // query size of counter and instance buffers
        DWORD counterListSize = 0;
        DWORD instanceListSize = 0;
        if (auto status = PdhEnumObjectItemsA(
            nullptr, nullptr, "GPU Engine", nullptr,
            &counterListSize, nullptr, &instanceListSize,
            PERF_DETAIL_WIZARD, 0); status != PDH_MORE_DATA) {
            throw Except<PdhException>("failed PdhEnumObjectItems()");
        }

        // enumerate counters and instances for GPU Engine
        std::vector<char> counterList(counterListSize);
        std::vector<char> instanceList(instanceListSize);
        if (auto status = PdhEnumObjectItemsA(
            nullptr, nullptr, "GPU Engine",
            counterList.data(), &counterListSize,
            instanceList.data(), &instanceListSize,
            PERF_DETAIL_WIZARD, 0)) {
            throw Except<PdhException>("failed PdhEnumObjectItems()");
        }

        // lambda to parse out null terminated sequence of strings from a buffer into vectors of string
        const auto ParsePDHList = [](const std::vector<char>& listData) {
            return listData |
                vi::split('\0') |
                vi::transform([](auto&& rng) { return rng | rn::to<std::basic_string>(); }) |
                vi::filter([](auto&& str) { return !str.empty(); }) |
                rn::to<std::vector>();
        };

        // parse counters and instances as vectors of strings from the raw buffers
        auto counters = ParsePDHList(counterList);
        auto instances = ParsePDHList(instanceList);

        // create a map of pid => instance string
        std::unordered_multimap<uint32_t, std::string> instanceMap3D;
        {
            const std::regex re(R"(^pid_(\d+)_.*?_engtype_3D$)");
            for (auto& i : instances) {
                std::smatch sm;
                if (std::regex_search(i, sm, re)) {
                    const uint32_t pid = std::stoi(sm[1]);
                    instanceMap3D.emplace(pid, sm[0]);
                }
            }
        }

        // open pdh query
        HQUERY hQuery = nullptr;
        if (PdhOpenQueryA(NULL, 0, &hQuery)) {
            throw Except<PdhException>("Failed opening pdh query");
        }

        // matching instances with filter candidates and add counters to query
        struct CountedProcess
        {
            uint32_t pid;
            std::vector<HCOUNTER> counters;
            double totalValue;
        };
        std::vector<CountedProcess> countedProcesses;
        for (auto& proc : candidates) {
            std::vector<HCOUNTER> counterHandles;
            for (auto&&[instPid, inst] : instanceMap3D.equal_range(proc.pid) | rng::PairToRange) {
                counterHandles.emplace_back();
                const auto counterPath = std::format("\\GPU Engine({})\\Running time", inst);
                if (PdhAddCounterA(hQuery, counterPath.c_str(), 0, &counterHandles.back())) {
                    throw Except<PdhException>("Failed adding pdh counter");
                }
            }
            if (!counterHandles.empty()) {
                countedProcesses.emplace_back(proc.pid, std::move(counterHandles), 0.);
            }
        }

        // if there are no counted processes, return empty result
        if (countedProcesses.empty()) {
            return {};
        }

        // prime the counter with an initial polling call
        if (PdhCollectQueryData(hQuery)) {
            throw Except<PdhException>("Failed collecting query data");
        }

        // lambda to poll counters and gather results
        const auto RunPoll = [&](double factor) {
            if (PdhCollectQueryData(hQuery)) {
                throw Except<PdhException>("Failed collecting query data");
            }
            for (auto& proc : countedProcesses) {
                for (auto& hCounter : proc.counters) {
                    PDH_FMT_COUNTERVALUE counterValue;
                    if (PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, nullptr, &counterValue)) {
                        throw Except<PdhException>("Failed formatting counter value");
                    }
                    proc.totalValue += counterValue.doubleValue * factor;
                }
            }
        };

        // poll first data point and gather, to be subtracted from second
        RunPoll(-1.);

        // 100ms delay between counter samples
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);

        // poll second datapoint, gathering by adding to first (first subtracts from this)
        RunPoll(1.);

        // remove any candidates with zero utilization
        std::erase_if(countedProcesses, [](const CountedProcess& p) { return p.totalValue == 0.; });

        // if container empty, return empty optional
        if (countedProcesses.empty()) {
            return {};
        }

        // find max gpu utilization process and return its pid
        return rn::max_element(countedProcesses, {}, &CountedProcess::totalValue)->pid;
	}
}