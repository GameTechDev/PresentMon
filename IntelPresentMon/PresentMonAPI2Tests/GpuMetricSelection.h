// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "CppUnitTest.h"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <format>
#include <vector>

namespace pmon::tests
{
    struct GpuMetricChoice
    {
        PM_METRIC metric;
        const char* symbol;
        bool introspectionAvailable;
    };

    inline constexpr std::array GpuMetricPriority{
        GpuMetricChoice{ PM_METRIC_GPU_POWER, "PM_METRIC_GPU_POWER", false },
        GpuMetricChoice{ PM_METRIC_GPU_FAN_SPEED, "PM_METRIC_GPU_FAN_SPEED", false },
        GpuMetricChoice{ PM_METRIC_GPU_TEMPERATURE, "PM_METRIC_GPU_TEMPERATURE", false },
        GpuMetricChoice{ PM_METRIC_GPU_VOLTAGE, "PM_METRIC_GPU_VOLTAGE", false },
        GpuMetricChoice{ PM_METRIC_GPU_FREQUENCY, "PM_METRIC_GPU_FREQUENCY", false },
        GpuMetricChoice{ PM_METRIC_GPU_MEM_USED, "PM_METRIC_GPU_MEM_USED", false },
    };

    inline bool IsMetricAvailableForDevice(
        const pmapi::intro::Root& intro,
        PM_METRIC metricId,
        uint32_t deviceId,
        uint32_t arrayIndex = 0)
    {
        const auto metric = intro.FindMetric(metricId);
        for (auto info : metric.GetDeviceMetricInfo()) {
            if (info.GetDevice().GetId() != deviceId) {
                continue;
            }
            return info.IsAvailable() && info.GetArraySize() > arrayIndex;
        }
        return false;
    }

    inline std::vector<GpuMetricChoice> SelectGpuMetrics(
        const pmapi::intro::Root& intro,
        uint32_t deviceId,
        size_t count)
    {
        std::vector<GpuMetricChoice> selected;
        selected.reserve((std::min)(count, GpuMetricPriority.size()));

        for (const auto& candidate : GpuMetricPriority) {
            if (selected.size() == count) {
                return selected;
            }
            if (IsMetricAvailableForDevice(intro, candidate.metric, deviceId)) {
                selected.push_back(GpuMetricChoice{
                    .metric = candidate.metric,
                    .symbol = candidate.symbol,
                    .introspectionAvailable = true,
                    });
            }
        }

        for (const auto& candidate : GpuMetricPriority) {
            if (selected.size() == count) {
                break;
            }
            bool alreadySelected = false;
            for (const auto& choice : selected) {
                if (choice.metric == candidate.metric) {
                    alreadySelected = true;
                    break;
                }
            }
            if (!alreadySelected) {
                selected.push_back(candidate);
            }
        }

        return selected;
    }

    inline GpuMetricChoice SelectGpuMetric(
        const pmapi::intro::Root& intro,
        uint32_t deviceId)
    {
        return SelectGpuMetrics(intro, deviceId, 1).front();
    }

    inline void LogGpuMetricChoices(
        const char* label,
        uint32_t deviceId,
        const std::vector<GpuMetricChoice>& choices)
    {
        using namespace Microsoft::VisualStudio::CppUnitTestFramework;
        Logger::WriteMessage(std::format(
            "{} selected GPU metrics for device {}:\n",
            label,
            deviceId).c_str());
        for (const auto& choice : choices) {
            Logger::WriteMessage(std::format(
                "  {} ({})\n",
                choice.symbol,
                choice.introspectionAvailable ? "available" : "not available").c_str());
        }
    }

    inline std::vector<GpuMetricChoice> SelectAndLogGpuMetrics(
        const pmapi::intro::Root& intro,
        uint32_t deviceId,
        size_t count,
        const char* label)
    {
        auto choices = SelectGpuMetrics(intro, deviceId, count);
        LogGpuMetricChoices(label, deviceId, choices);
        return choices;
    }

    inline GpuMetricChoice SelectAndLogGpuMetric(
        const pmapi::intro::Root& intro,
        uint32_t deviceId,
        const char* label)
    {
        auto choices = SelectAndLogGpuMetrics(intro, deviceId, 1, label);
        return choices.front();
    }
}
