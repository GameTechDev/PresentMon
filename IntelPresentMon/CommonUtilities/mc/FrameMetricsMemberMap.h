// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "MetricsTypes.h"
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::util::metrics
{
    template<PM_METRIC MetricId>struct FrameMetricMember{};
    template<>struct FrameMetricMember<PM_METRIC_ALLOWS_TEARING>{static constexpr auto member=&FrameMetrics::allowsTearing;};
    template<>struct FrameMetricMember<PM_METRIC_PRESENT_RUNTIME>{static constexpr auto member=&FrameMetrics::runtime;};
    template<>struct FrameMetricMember<PM_METRIC_PRESENT_MODE>{static constexpr auto member=&FrameMetrics::presentMode;};
    template<>struct FrameMetricMember<PM_METRIC_PRESENT_FLAGS>{static constexpr auto member=&FrameMetrics::presentFlags;};
    template<>struct FrameMetricMember<PM_METRIC_SYNC_INTERVAL>{static constexpr auto member=&FrameMetrics::syncInterval;};
    template<>struct FrameMetricMember<PM_METRIC_SWAP_CHAIN_ADDRESS>{static constexpr auto member=&FrameMetrics::swapChainAddress;};
    template<>struct FrameMetricMember<PM_METRIC_PRESENT_START_QPC>{static constexpr auto member=&FrameMetrics::presentStartQpc;};
    template<>struct FrameMetricMember<PM_METRIC_PRESENT_START_TIME>{static constexpr auto member=&FrameMetrics::presentStartMs;};
    template<>struct FrameMetricMember<PM_METRIC_CPU_START_QPC>{static constexpr auto member=&FrameMetrics::cpuStartQpc;};
    template<>struct FrameMetricMember<PM_METRIC_CPU_START_TIME>{static constexpr auto member=&FrameMetrics::cpuStartMs;};
    template<>struct FrameMetricMember<PM_METRIC_BETWEEN_PRESENTS>{static constexpr auto member=&FrameMetrics::msBetweenPresents;};
    template<>struct FrameMetricMember<PM_METRIC_IN_PRESENT_API>{static constexpr auto member=&FrameMetrics::msInPresentApi;};
    template<>struct FrameMetricMember<PM_METRIC_RENDER_PRESENT_LATENCY>{static constexpr auto member=&FrameMetrics::msUntilRenderComplete;};
    template<>struct FrameMetricMember<PM_METRIC_BETWEEN_DISPLAY_CHANGE>{static constexpr auto member=&FrameMetrics::msBetweenDisplayChange;};
    template<>struct FrameMetricMember<PM_METRIC_UNTIL_DISPLAYED>{static constexpr auto member=&FrameMetrics::msUntilDisplayed;};
    template<>struct FrameMetricMember<PM_METRIC_DISPLAYED_TIME>{static constexpr auto member=&FrameMetrics::msDisplayedTime;};
    template<>struct FrameMetricMember<PM_METRIC_DISPLAY_LATENCY>{static constexpr auto member=&FrameMetrics::msDisplayLatency;};
    template<>struct FrameMetricMember<PM_METRIC_BETWEEN_SIMULATION_START>{static constexpr auto member=&FrameMetrics::msBetweenSimStarts;};
    template<>struct FrameMetricMember<PM_METRIC_PC_LATENCY>{static constexpr auto member=&FrameMetrics::msPcLatency;};
    template<>struct FrameMetricMember<PM_METRIC_CPU_BUSY>{static constexpr auto member=&FrameMetrics::msCPUBusy;};
    template<>struct FrameMetricMember<PM_METRIC_CPU_WAIT>{static constexpr auto member=&FrameMetrics::msCPUWait;};
    template<>struct FrameMetricMember<PM_METRIC_CPU_FRAME_TIME>{static constexpr auto member=&FrameMetrics::msCPUTime;};
    template<>struct FrameMetricMember<PM_METRIC_BETWEEN_APP_START>{static constexpr auto member=&FrameMetrics::msCPUTime;};
    template<>struct FrameMetricMember<PM_METRIC_GPU_LATENCY>{static constexpr auto member=&FrameMetrics::msGPULatency;};
    template<>struct FrameMetricMember<PM_METRIC_GPU_TIME>{static constexpr auto member=&FrameMetrics::msGPUTime;};
    template<>struct FrameMetricMember<PM_METRIC_GPU_BUSY>{static constexpr auto member=&FrameMetrics::msGPUBusy;};
    template<>struct FrameMetricMember<PM_METRIC_GPU_WAIT>{static constexpr auto member=&FrameMetrics::msGPUWait;};
    template<>struct FrameMetricMember<PM_METRIC_DROPPED_FRAMES>{static constexpr auto member=&FrameMetrics::isDroppedFrame;};
    template<>struct FrameMetricMember<PM_METRIC_ANIMATION_ERROR>{static constexpr auto member=&FrameMetrics::msAnimationError;};
    template<>struct FrameMetricMember<PM_METRIC_ANIMATION_TIME>{static constexpr auto member=&FrameMetrics::msAnimationTime;};
    template<>struct FrameMetricMember<PM_METRIC_CLICK_TO_PHOTON_LATENCY>{static constexpr auto member=&FrameMetrics::msClickToPhotonLatency;};
    template<>struct FrameMetricMember<PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY>{static constexpr auto member=&FrameMetrics::msAllInputPhotonLatency;};
    template<>struct FrameMetricMember<PM_METRIC_INSTRUMENTED_LATENCY>{static constexpr auto member=&FrameMetrics::msInstrumentedLatency;};
    template<>struct FrameMetricMember<PM_METRIC_FLIP_DELAY>{static constexpr auto member=&FrameMetrics::msFlipDelay;};
    template<>struct FrameMetricMember<PM_METRIC_FRAME_TYPE>{static constexpr auto member=&FrameMetrics::frameType;};

    template<PM_METRIC MetricId>
    inline constexpr bool HasFrameMetricMember = requires{ FrameMetricMember<MetricId>::member; };
}
