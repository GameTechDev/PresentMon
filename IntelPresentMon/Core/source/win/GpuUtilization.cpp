// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "GpuUtilization.h"
#include <CommonUtilities/rng/PairToRange.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/Memory.h>
#include <CommonUtilities/win/Utilities.h>
#include <Core/source/infra/Logging.h>
#include <chrono>
#include <vector>
#include <regex>
#include <ranges>
#include <format>
#include <thread>
#include <unordered_map>
#include <string_view>
#include <memory>
#include <type_traits>

#include <pdh.h>
#include <pdhmsg.h>

namespace rn = std::ranges;
namespace vi = rn::views;
using namespace pmon::util;

namespace p2c::win
{
    PM_DEFINE_EX(PdhException);

    namespace
    {
        std::string TrimFormattedMessage_(std::string message)
        {
            while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
                message.pop_back();
            }
            return message;
        }

        std::string TryFormatPdhStatusFromModule_(PDH_STATUS status) noexcept
        {
            try {
                const auto hPdh = GetModuleHandleA("pdh.dll");
                if (!hPdh) {
                    return {};
                }

                UniqueLocalPtr<CHAR> messageLocal;
                if (!FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                    hPdh, static_cast<DWORD>(status), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    reinterpret_cast<LPSTR>(static_cast<char**>(OutPtr(messageLocal))), 0, nullptr))
                {
                    return {};
                }

                return TrimFormattedMessage_(messageLocal.get());
            }
            catch (...) {
                return {};
            }
        }

        std::string FormatPdhStatus_(PDH_STATUS status) noexcept
        {
            try {
                if (auto message = TryFormatPdhStatusFromModule_(status); !message.empty()) {
                    return message;
                }

                const auto message = ::pmon::util::win::GetErrorDescription(static_cast<HRESULT>(status));
                if (message != "COULD NOT FORMAT") {
                    return message;
                }

                return std::format("PDH status 0x{:08X}", static_cast<uint32_t>(status));
            }
            catch (...) {
                return "COULD NOT FORMAT";
            }
        }

        std::string MakePdhFailureLogMessage_(
            std::string_view operation,
            PDH_STATUS status,
            const std::string& statusDescription)
        {
            return std::format(
                "{} failed: status=0x{:08X} ({})",
                operation,
                static_cast<uint32_t>(status),
                statusDescription);
        }

        struct PdhQueryDeleter_
        {
            void operator()(std::remove_pointer_t<HQUERY>* hQuery) const noexcept
            {
                if (hQuery) {
                    PdhCloseQuery(hQuery);
                }
            }
        };

        using PdhQueryPtr_ = std::unique_ptr<std::remove_pointer_t<HQUERY>, PdhQueryDeleter_>;
    }

    std::optional<uint32_t> GetTopGpuProcess(const std::vector<::pmon::util::win::Process>& candidates)
	{
        // query size of counter and instance buffers
        DWORD counterListSize = 0;
        DWORD instanceListSize = 0;
        if (auto status = PdhEnumObjectItemsA(
            nullptr, nullptr, "GPU Engine", nullptr,
            &counterListSize, nullptr, &instanceListSize,
            PERF_DETAIL_WIZARD, 0); status != PDH_MORE_DATA)
        {
            const auto statusDescription = FormatPdhStatus_(status);
            const char* object = "GPU Engine";
            const char* phase = "query sizes";
            pmlog_error(MakePdhFailureLogMessage_("PdhEnumObjectItemsA", status, statusDescription))
                .hr(static_cast<uint32_t>(status))
                .pmwatch(status)
                .pmwatch(statusDescription)
                .pmwatch(object)
                .pmwatch(phase)
                .pmwatch(candidates.size());
            throw Except<PdhException>("failed PdhEnumObjectItems()");
        }

        // enumerate counters and instances for GPU Engine
        std::vector<char> counterList(counterListSize);
        std::vector<char> instanceList(instanceListSize);
        if (auto status = PdhEnumObjectItemsA(
            nullptr, nullptr, "GPU Engine",
            counterList.data(), &counterListSize,
            instanceList.data(), &instanceListSize,
            PERF_DETAIL_WIZARD, 0); status != ERROR_SUCCESS)
        {
            const auto statusDescription = FormatPdhStatus_(status);
            const char* object = "GPU Engine";
            const char* phase = "enumerate items";
            pmlog_error(MakePdhFailureLogMessage_("PdhEnumObjectItemsA", status, statusDescription))
                .hr(static_cast<uint32_t>(status))
                .pmwatch(status)
                .pmwatch(statusDescription)
                .pmwatch(object)
                .pmwatch(phase)
                .pmwatch(counterListSize)
                .pmwatch(instanceListSize)
                .pmwatch(candidates.size());
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
        HQUERY hRawQuery = nullptr;
        if (auto status = PdhOpenQueryA(NULL, 0, &hRawQuery); status != ERROR_SUCCESS)
        {
            const auto statusDescription = FormatPdhStatus_(status);
            pmlog_error(MakePdhFailureLogMessage_("PdhOpenQueryA", status, statusDescription))
                .hr(static_cast<uint32_t>(status))
                .pmwatch(status)
                .pmwatch(statusDescription)
                .pmwatch(candidates.size())
                .pmwatch(instanceMap3D.size());
            throw Except<PdhException>("Failed opening pdh query");
        }
        PdhQueryPtr_ hQuery{ hRawQuery };

        // matching instances with filter candidates and add counters to query
        struct CountedCounter
        {
            HCOUNTER handle;
            std::string path;
        };
        struct CountedProcess
        {
            uint32_t pid;
            std::vector<CountedCounter> counters;
            double totalValue;
        };
        std::vector<CountedProcess> countedProcesses;
        for (auto& proc : candidates) {
            std::vector<CountedCounter> counterHandles;
            for (auto&&[instPid, inst] : instanceMap3D.equal_range(proc.pid) | rng::PairToRange) {
                const auto counterPath = std::format("\\GPU Engine({})\\Running time", inst);
                counterHandles.push_back({ HCOUNTER{}, counterPath });
                auto& counter = counterHandles.back();
                if (auto status = PdhAddCounterA(hQuery.get(), counterPath.c_str(), 0, &counter.handle);
                    status != ERROR_SUCCESS)
                {
                    const auto statusDescription = FormatPdhStatus_(status);
                    pmlog_error(MakePdhFailureLogMessage_("PdhAddCounterA", status, statusDescription))
                        .hr(static_cast<uint32_t>(status))
                        .pmwatch(status)
                        .pmwatch(statusDescription)
                        .pmwatch(proc.pid)
                        .pmwatch(inst)
                        .pmwatch(counterPath);
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

        const auto GetCounterCount = [&] {
            size_t count = 0;
            for (const auto& proc : countedProcesses) {
                count += proc.counters.size();
            }
            return count;
        };

        // prime the counter with an initial polling call
        if (auto status = PdhCollectQueryData(hQuery.get()); status != ERROR_SUCCESS)
        {
            const auto statusDescription = FormatPdhStatus_(status);
            const char* phase = "prime";
            const auto counterCount = GetCounterCount();
            pmlog_error(MakePdhFailureLogMessage_("PdhCollectQueryData", status, statusDescription))
                .hr(static_cast<uint32_t>(status))
                .pmwatch(status)
                .pmwatch(statusDescription)
                .pmwatch(phase)
                .pmwatch(countedProcesses.size())
                .pmwatch(counterCount);
            throw Except<PdhException>("Failed collecting query data");
        }

        // lambda to poll counters and gather results
        const auto RunPoll = [&](double factor) {
            if (auto status = PdhCollectQueryData(hQuery.get()); status != ERROR_SUCCESS)
            {
                const auto statusDescription = FormatPdhStatus_(status);
                const char* phase = "poll";
                const auto counterCount = GetCounterCount();
                pmlog_error(MakePdhFailureLogMessage_("PdhCollectQueryData", status, statusDescription))
                    .hr(static_cast<uint32_t>(status))
                    .pmwatch(status)
                    .pmwatch(statusDescription)
                    .pmwatch(phase)
                    .pmwatch(factor)
                    .pmwatch(countedProcesses.size())
                    .pmwatch(counterCount);
                throw Except<PdhException>("Failed collecting query data");
            }
            for (auto& proc : countedProcesses) {
                for (auto& counter : proc.counters) {
                    PDH_FMT_COUNTERVALUE counterValue;
                    if (auto status = PdhGetFormattedCounterValue(counter.handle, PDH_FMT_DOUBLE, nullptr, &counterValue);
                        status != ERROR_SUCCESS)
                    {
                        const auto statusDescription = FormatPdhStatus_(status);
                        pmlog_error(MakePdhFailureLogMessage_("PdhGetFormattedCounterValue", status, statusDescription))
                            .hr(static_cast<uint32_t>(status))
                            .pmwatch(status)
                            .pmwatch(statusDescription)
                            .pmwatch(proc.pid)
                            .pmwatch(factor)
                            .pmwatch(counter.path);
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
