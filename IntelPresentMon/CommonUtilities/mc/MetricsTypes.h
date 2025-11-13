// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <vector>
#include <memory>

// Forward declarations for external types
enum class FrameType;       // From PresentData
enum class PresentResult;   // From PresentData
struct PmNsmPresentEvent;   // From PresentMonUtils

namespace pmon::util::metrics {

    // Immutable snapshot - safe for both ownership models
    struct PresentSnapshot {
        // Timing Data
        uint64_t presentStartTime;
        uint64_t readyTime;
        uint64_t timeInPresent;
        uint64_t gpuStartTime;
        uint64_t gpuDuration;
        uint64_t gpuVideoDuration;

        // Propagated Timing (for frame generation)
        uint64_t appPropagatedPresentStartTime;
        uint64_t appPropagatedTimeInPresent;
        uint64_t appPropagatedGPUStartTime;
        uint64_t appPropagatedReadyTime;
        uint64_t appPropagatedGPUDuration;
        uint64_t appPropagatedGPUVideoDuration;

        // Instrumented Timestamps
        uint64_t appSimStartTime;
        uint64_t appSleepStartTime;
        uint64_t appSleepEndTime;
        uint64_t appRenderSubmitStartTime;
        std::pair<uint64_t, uint32_t> appInputSample;  // time, frame_id

        // PC Latency Timestamps
        uint64_t pclSimStartTime;
        uint64_t pclInputPingTime;

        // Input Device Timestamps
        uint64_t inputTime;           // All input devices
        uint64_t mouseClickTime;      // Mouse click specific

        // Display Data (normalized from both formats)
        struct DisplayEntry {
            FrameType frameType;
            uint64_t screenTime;
        };
        std::vector<DisplayEntry> displayed;

        // Vendor-Specific
        uint64_t flipDelay;  // NVIDIA

        // Metadata
        PresentResult finalState;
        uint32_t processId;
        uint64_t swapChainAddress;

        // Factory Methods
        // Console uses ConsoleAdapter directly, so no conversion needed
        static PresentSnapshot FromCircularBuffer(const PmNsmPresentEvent& p);
    };

    struct FrameMetrics {
        // Core Timing (always computed)
        uint64_t timeInSeconds;              // QPC timestamp
        double msBetweenPresents;
        double msInPresentApi;
        double msUntilRenderComplete;

        // CPU Metrics (app frames only)
        uint64_t cpuStartQpc;
        double msCPUBusy;
        double msCPUWait;

        // GPU Metrics (app frames only)
        double msGPULatency;
        double msGPUBusy;
        double msVideoBusy;
        double msGPUWait;

        // Display Metrics (displayed frames only)
        double msDisplayLatency;
        double msDisplayedTime;
        double msUntilDisplayed;
        double msBetweenDisplayChange;
        uint64_t screenTimeQpc;

        // Input Latency (optional, app+displayed only)
        std::optional<double> msClickToPhotonLatency;
        std::optional<double> msAllInputPhotonLatency;
        std::optional<double> msInstrumentedInputTime;
        std::optional<double> msPcLatency;

        // Animation (optional, app+displayed only)
        std::optional<double> msAnimationError;
        std::optional<double> msAnimationTime;

        // Instrumented Metrics (optional)
        std::optional<double> msInstrumentedLatency;
        std::optional<double> msInstrumentedRenderLatency;
        std::optional<double> msInstrumentedSleep;
        std::optional<double> msInstrumentedGpuLatency;
        std::optional<double> msReadyTimeToDisplayLatency;
        std::optional<double> msBetweenSimStarts;

        // Vendor-specific (optional)
        std::optional<double> msFlipDelay;  // NVIDIA

        // Frame Classification
        FrameType frameType;
    };
}