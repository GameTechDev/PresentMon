// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include "../cnr/FixedVector.h"

// Forward declarations for external types
enum class Runtime;
enum class PresentMode;
enum class FrameType;       // From PresentData
enum class PresentResult;   // From PresentData
enum class InputDeviceType; // From PresentData
struct PmNsmPresentEvent;   // From PresentMonUtils
struct PresentEvent;        // From PresentMonTraceConsumer

namespace pmon::util::metrics {

    // Metrics pipeline mode
    enum class MetricsVersion {
        V1,
        V2,
    };

    // What the animation error calculation is based on
    enum class AnimationErrorSource {
        CpuStart,
        AppProvider,
        PCLatency,
    };

    using DisplayedEntry = std::pair<FrameType, uint64_t>;
    using DisplayedVector = pmon::util::cnr::FixedVector<DisplayedEntry, 16>;

    // Immutable snapshot - safe for both ownership models
    struct FrameData {
        Runtime runtime = {};
        PresentMode presentMode = {};

        // Timing Data
        uint64_t presentStartTime = 0;
        uint64_t readyTime = 0;
        uint64_t timeInPresent = 0;
        uint64_t gpuStartTime = 0;
        uint64_t gpuDuration = 0;
        uint64_t gpuVideoDuration = 0;

        // Used to track the application work when Intel XeSS-FG is enabled
        uint64_t appPropagatedPresentStartTime = 0;
        uint64_t appPropagatedTimeInPresent = 0;
        uint64_t appPropagatedGPUStartTime = 0;
        uint64_t appPropagatedReadyTime = 0;
        uint64_t appPropagatedGPUDuration = 0;
        uint64_t appPropagatedGPUVideoDuration = 0;

        // Instrumented Timestamps
        uint64_t appSimStartTime = 0;
        uint64_t appSimEndTime = 0;
        uint64_t appSleepStartTime = 0;
        uint64_t appSleepEndTime = 0;
        uint64_t appRenderSubmitStartTime = 0;
        uint64_t appRenderSubmitEndTime = 0;
        uint64_t appPresentStartTime = 0;
        uint64_t appPresentEndTime = 0;
        std::pair<uint64_t, InputDeviceType> appInputSample;  // time, input type

        // Input Device Timestamps
        uint64_t inputTime = 0;           // All input devices
        uint64_t mouseClickTime = 0;      // Mouse click specific

        DisplayedVector displayed;

        // PC Latency data
        uint64_t pclSimStartTime = 0;
        uint64_t pclInputPingTime = 0;
        uint64_t flipDelay = 0;
        uint32_t flipToken = 0;

        // Extra present parameters obtained through DXGI or D3D9 present
        uint64_t swapChainAddress = 0;
        int32_t syncInterval = 0;
        uint32_t presentFlags = 0;

        // Metadata
        PresentResult finalState = {};
        bool supportsTearing = 0;
        bool isHybridPresent = false;
        uint32_t processId = 0;
        uint32_t threadId = 0;
        uint32_t frameId = 0;
        uint32_t appFrameId = 0;
        uint32_t pclFrameId = 0;

        // Factory Methods
        static FrameData CopyFrameData(const PmNsmPresentEvent& p);
        static FrameData CopyFrameData(const std::shared_ptr<PresentEvent>& p);
        static FrameData CopyFrameData(const PresentEvent& p);
    };

    struct FrameMetrics {
        // Core Timing (always computed)
        uint64_t timeInSeconds = 0;
        uint64_t presentStartQpc = 0;
        double presentStartMs = 0;
        uint64_t cpuStartQpc = 0;
        double cpuStartMs = 0;
        double msBetweenPresents = 0;
        double msInPresentApi = 0;
        double msUntilRenderStart = 0;
        double msUntilRenderComplete = 0;
        double msGpuDuration = 0;
        double msVideoDuration = 0;
        double msSinceInput = 0;

        // Display Metrics (displayed frames only)
        double msDisplayLatency = 0;
        double msDisplayedTime = 0;
        double msUntilDisplayed = 0;
        double msBetweenDisplayChange = 0;
        uint64_t screenTimeQpc = 0;
        std::optional<double> msReadyTimeToDisplayLatency;
        bool isDroppedFrame = false;

        // CPU Metrics (app frames only)
        double msCPUBusy = 0;
        double msCPUWait = 0;
        double msCPUTime = 0;

        // GPU Metrics (app frames only)
        double msGPULatency = 0;
        double msGPUBusy = 0;
        double msVideoBusy = 0;
        double msGPUWait = 0;
        double msGPUTime = 0;

        // Input Latency (optional, app+displayed only)
        std::optional<double> msClickToPhotonLatency = {};
        std::optional<double> msAllInputPhotonLatency = {};
        std::optional<double> msInstrumentedInputTime;

        // Animation (optional, app+displayed only)
        std::optional<double> msAnimationError = {};
        std::optional<double> msAnimationTime = {};

        // Instrumented Metrics (optional)
        std::optional<double> msInstrumentedLatency = {};
        std::optional<double> msInstrumentedRenderLatency = {};
        std::optional<double> msInstrumentedSleep = {};
        std::optional<double> msInstrumentedGpuLatency = {};
        std::optional<double> msPcLatency = {};
        std::optional<double> msBetweenSimStarts = {};

        // PCLatency (optional)
        std::optional<double> msFlipDelay = {};  // NVIDIA

        // Frame Classification
        FrameType frameType = {};

        // Present Metadata
        uint64_t swapChainAddress = 0;
        Runtime runtime = {};
        int32_t syncInterval = 0;
        uint32_t presentFlags = 0;
        bool allowsTearing = false;
        PresentMode presentMode = {};
    };
}
