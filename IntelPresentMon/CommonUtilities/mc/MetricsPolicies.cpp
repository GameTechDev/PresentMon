// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"
#include "MetricsPolicies.h"

// Include full definitions
#include "../../../PresentMon/PresentMon.hpp"
#include "../..//PresentMonMiddleware/ConcreteMiddleware.h"

namespace pmon::util::metrics {

void ConsoleMetricsPolicy::OnMetricsComputed(
    const FrameMetrics& metrics,
    const ComputedMetrics::StateDeltas& deltas,
    SwapChainData& chain,
    size_t displayIndex,
    bool isAppIndex)
{
    if (deltas.newAccumulatedInput2FrameStart) {
        chain.core.accumulatedInput2FrameStartTime = *deltas.newAccumulatedInput2FrameStart;
    }
    
    if (deltas.newLastReceivedPclSimStart) {
        chain.core.lastReceivedNotDisplayedPclSimStart = *deltas.newLastReceivedPclSimStart;
    }
    
    if (deltas.lastReceivedNotDisplayedAllInputTime) {
        chain.core.lastReceivedNotDisplayedAllInputTime = *deltas.lastReceivedNotDisplayedAllInputTime;
    }
    
    if (deltas.lastReceivedNotDisplayedMouseClickTime) {
        chain.core.lastReceivedNotDisplayedMouseClickTime = *deltas.lastReceivedNotDisplayedMouseClickTime;
    }
    
    // Console-specific: Update averages (UNCHANGED - not in core)
    if (isRecording_) {
        chain.mAvgCPUDuration = UpdateExponentialMovingAverage(
            chain.mAvgCPUDuration, metrics.msCPUBusy);
        chain.mAvgGPUDuration = UpdateExponentialMovingAverage(
            chain.mAvgGPUDuration, metrics.msGPUBusy);
        // ... etc
    }
    
    // Write to CSV (unchanged logic)
    if (isRecording_) {
        WriteCsvRow(processInfo_, metrics);
    }
}

void ConsoleMetricsPolicy::OnFrameComplete(
    const PresentSnapshot& present,
    SwapChainData& chain)
{
    // BEFORE: Update chain state directly
    // chain.mLastPresent = present.toSharedPtr();
    // chain.mLastSimStartTime = present.simStartTime;
    
    // AFTER: Update through .core member
    // Note: Might need to reconstruct shared_ptr from snapshot, or keep original
    // Alternatively, policy might track the original shared_ptr separately
    chain.core.lastSimStartTime = GetSimStartTime(present);
    
    // Update last displayed times if this frame was displayed
    if (IsDisplayed(present)) {
        chain.core.lastDisplayedSimStartTime = GetSimStartTime(present);
        chain.core.lastDisplayedScreenTime = present.screenTime;
    }
}

void MiddlewareMetricsPolicy::OnMetricsComputed(
    const FrameMetrics& metrics,
    const ComputedMetrics::StateDeltas& deltas,
    pmon::mid::fpsSwapChainData& chain,
    size_t displayIndex,
    bool isAppIndex)
{
    // BEFORE: Apply deltas to chain fields directly
    // if (deltas.newAccumulatedInput2FrameStart) {
    //     chain.mAccumulatedInput2FrameStartTime = *deltas.newAccumulatedInput2FrameStart;
    // }
    
    // AFTER: Apply deltas through .core member (IDENTICAL to Console!)
    if (deltas.newAccumulatedInput2FrameStart) {
        chain.core.accumulatedInput2FrameStartTime = *deltas.newAccumulatedInput2FrameStart;
    }
    
    if (deltas.newLastReceivedPclSimStart) {
        chain.core.lastReceivedNotDisplayedPclSimStart = *deltas.newLastReceivedPclSimStart;
    }
    
    if (deltas.lastReceivedNotDisplayedAllInputTime) {
        chain.core.lastReceivedNotDisplayedAllInputTime = *deltas.lastReceivedNotDisplayedAllInputTime;
    }
    
    // Middleware-specific: Push to telemetry vectors (UNCHANGED - not in core)
    if (isAppIndex) {
        chain.mCPUBusy.push_back(metrics.msCPUBusy);
        chain.mGPULatency.push_back(metrics.msGPULatency);
        chain.mDisplayLatency.push_back(metrics.msDisplayLatency);
    }
    
    // Update InputToFsManager (unchanged logic)
    pclManager_.UpdateMetrics(processId_, metrics);
}

void MiddlewareMetricsPolicy::OnFrameComplete(
    const PresentSnapshot& present,
    pmon::mid::fpsSwapChainData& chain)
{
    // AFTER: Update through .core member (same pattern as Console)
    chain.core.lastSimStartTime = present.simStartTime;
    
    if (IsDisplayed(present)) {
        chain.core.lastDisplayedSimStartTime = present.simStartTime;
        chain.core.lastDisplayedScreenTime = present.screenTime;
    }
    
    // Middleware-specific: Update optimization counters (UNCHANGED)
    chain.display_count++;
}

} // namespace pmon::metrics