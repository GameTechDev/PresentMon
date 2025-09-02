// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
// must include PresentMonAPI.h before including this file
#include <optional>
#include <cstdint>
#include <vector>
#include <ranges>

namespace p2c::pmon
{
    struct RawFrameQueryElementDefinition
    {
        PM_METRIC metricId;
        uint32_t deviceId;
        std::optional<uint32_t> index;
    };

    inline std::vector<RawFrameQueryElementDefinition> GetRawFrameDataMetricList(uint32_t activeDeviceId, bool enableTimestamp)
    {
        namespace rn = std::ranges;
        using Element = RawFrameQueryElementDefinition;

        std::vector<Element> queryElements{
            Element{.metricId = PM_METRIC_SWAP_CHAIN_ADDRESS, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PRESENT_RUNTIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_SYNC_INTERVAL, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PRESENT_FLAGS, .deviceId = 0 },
            Element{.metricId = PM_METRIC_ALLOWS_TEARING, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PRESENT_MODE, .deviceId = 0 },
            Element{.metricId = PM_METRIC_FRAME_TYPE, .deviceId = 0 },

            Element{.metricId = PM_METRIC_PRESENT_START_TIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_BETWEEN_SIMULATION_START, .deviceId = 0 },
            Element{.metricId = PM_METRIC_BETWEEN_PRESENTS, .deviceId = 0 },
            Element{.metricId = PM_METRIC_BETWEEN_DISPLAY_CHANGE, .deviceId = 0 },
            Element{.metricId = PM_METRIC_IN_PRESENT_API, .deviceId = 0 },
            Element{.metricId = PM_METRIC_RENDER_PRESENT_LATENCY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_UNTIL_DISPLAYED, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PC_LATENCY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_CPU_START_TIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_BETWEEN_APP_START, .deviceId = 0 },
            Element{.metricId = PM_METRIC_CPU_BUSY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_CPU_WAIT, .deviceId = 0 },
            Element{.metricId = PM_METRIC_GPU_LATENCY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_GPU_TIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_GPU_BUSY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_GPU_WAIT, .deviceId = 0 },
            Element{.metricId = PM_METRIC_ANIMATION_ERROR, .deviceId = 0 },
            Element{.metricId = PM_METRIC_FLIP_DELAY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_ANIMATION_TIME, .deviceId = 0 },
            Element{.metricId = PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_CLICK_TO_PHOTON_LATENCY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_INSTRUMENTED_LATENCY, .deviceId = 0 },
            Element{.metricId = PM_METRIC_PC_LATENCY, .deviceId = 0 },

            Element{.metricId = PM_METRIC_GPU_POWER, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_VOLTAGE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_FREQUENCY, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_TEMPERATURE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_UTILIZATION, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEDIA_UTILIZATION, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_POWER, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_VOLTAGE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_FREQUENCY, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_TEMPERATURE, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_SIZE, .deviceId = activeDeviceId }, // special case filling static
            Element{.metricId = PM_METRIC_GPU_MEM_USED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_MAX_BANDWIDTH, .deviceId = activeDeviceId }, // special case filling static
            Element{.metricId = PM_METRIC_GPU_MEM_READ_BANDWIDTH, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_WRITE_BANDWIDTH, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 0 },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 1 },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 2 },
            Element{.metricId = PM_METRIC_GPU_FAN_SPEED, .deviceId = activeDeviceId, .index = 3 },
            Element{.metricId = PM_METRIC_GPU_POWER_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_TEMPERATURE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_CURRENT_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_VOLTAGE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_UTILIZATION_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_POWER_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_CURRENT_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_VOLTAGE_LIMITED, .deviceId = activeDeviceId },
            Element{.metricId = PM_METRIC_GPU_MEM_UTILIZATION_LIMITED, .deviceId = activeDeviceId },

            Element{.metricId = PM_METRIC_CPU_UTILIZATION },
            Element{.metricId = PM_METRIC_CPU_POWER },
            Element{.metricId = PM_METRIC_CPU_TEMPERATURE },
            Element{.metricId = PM_METRIC_CPU_FREQUENCY },
        };

        if (enableTimestamp) {
            queryElements.insert(queryElements.begin(), Element{ .metricId = PM_METRIC_CPU_START_QPC, .deviceId = 0 });
        }

        return queryElements;
    }
}
