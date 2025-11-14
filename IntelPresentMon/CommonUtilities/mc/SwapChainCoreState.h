// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <vector>
#include "MetricsTypes.h"

namespace pmon::util::metrics {

template<typename PresentStorageT>
struct SwapChainCoreState {

    // Pending and Historical Presents
    
    // Pending presents waiting for the next displayed present.
    std::vector<PresentStorageT> pendingPresents;

    // The most recent present that has been processed (e.g., output into CSV and/or used for frame
    // statistics).
    std::optional<PresentStorageT> lastPresent;

    // The most recent app present that has been processed (e.g., output into CSV and/or used for frame
    // statistics).
    std::optional<PresentStorageT> lastAppPresent;

    // Timing State

    // QPC of the last simulation start time regardless of whether it was displayed or not
    uint64_t lastSimStartTime = 0;

    // The simulation start time of the most recent displayed frame
    uint64_t lastDisplayedSimStartTime = 0;

    // The screen time of the most recent displayed frame (any type)
    uint64_t lastDisplayedScreenTime = 0;

    // The screen time of the most recent displayed application frame
    uint64_t lastDisplayedAppScreenTime = 0;

    // QPC of the first received simulation start time from the application provider
    uint64_t firstAppSimStartTime = 0;

    // Dropped Frame Input Tracking

    // QPC of last received all-input that did not make it to the screen (dropped Present)
    uint64_t lastReceivedNotDisplayedAllInputTime = 0;

    // QPC of last received mouse-click input that did not make it to the screen (dropped Present)
    uint64_t lastReceivedNotDisplayedMouseClickTime = 0;

    // QPC of the last received app provider input that did not make it to the screen (dropped Present)
    uint64_t lastReceivedNotDisplayedAppProviderInputTime = 0;

    // QPC of last received PC Latency simulation start that did not make it to the screen (dropped Present)
    uint64_t lastReceivedNotDisplayedPclSimStart = 0;

    // QPC of last received PC Latency input time that did not make it to the screen (dropped Present)
    uint64_t lastReceivedNotDisplayedPclInputTime = 0;

    // Animation Error Configuration
    AnimationErrorSource animationErrorSource = AnimationErrorSource::CpuStart;

    // PC Latency Accumulation

    // Accumulated PC latency input to frame start time due to dropped Present() calls
    double accumulatedInput2FrameStartTime = 0.0;

    // NVIDIA Specific Tracking
    uint64_t lastDisplayedFlipDelay = 0;
};

}