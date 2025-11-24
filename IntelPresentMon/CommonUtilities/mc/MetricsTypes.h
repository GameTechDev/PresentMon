// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

// Forward declarations for external types
enum class FrameType;       // From PresentData
enum class PresentResult;   // From PresentData
enum class InputDeviceType; // From PresentData
struct PmNsmPresentEvent;   // From PresentMonUtils
struct PresentEvent;        // From PresentMonTraceConsumer

namespace pmon::util::metrics {

    // What the animation error calculation is based on
    enum class AnimationErrorSource {
        CpuStart,
        AppProvider,
        PCLatency,
    };

    // Immutable snapshot - safe for both ownership models
    struct FrameData {
        // Timing Data
        uint64_t presentStartTime;
        uint64_t readyTime;
        uint64_t timeInPresent;
        uint64_t gpuStartTime;
        uint64_t gpuDuration;
        uint64_t gpuVideoDuration;

        // Used to track the application work when Intel XeSS-FG is enabled
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
        uint64_t appRenderSubmitEndTime;
        uint64_t appPresentStartTime;
        uint64_t appPresentEndTime;
        std::pair<uint64_t, InputDeviceType> appInputSample;  // time, input type

        // Input Device Timestamps
        uint64_t inputTime;           // All input devices
        uint64_t mouseClickTime;      // Mouse click specific

        std::vector<std::pair<FrameType, uint64_t>> displayed;

        // PC Latency data
        uint64_t pclSimStartTime;
        uint64_t pclInputPingTime;
        uint64_t flipDelay;
        uint32_t FlipToken;

        // Metadata
        PresentResult finalState;
        uint32_t processId;
        uint32_t threadId;
        uint64_t swapChainAddress;
        uint32_t frameId;
        uint32_t appFrameId;

        // Setters for test setup
        void setFinalState(PresentResult state) { finalState = state; }

        // Inline getters - same interface as ConsoleAdapter (zero cost)
        uint64_t getPresentStartTime() const { return presentStartTime; }
        uint64_t getReadyTime() const { return readyTime; }
        uint64_t getTimeInPresent() const { return timeInPresent; }
        uint64_t getGPUStartTime() const { return gpuStartTime; }
        uint64_t getGPUDuration() const { return gpuDuration; }
        uint64_t getGPUVideoDuration() const { return gpuVideoDuration; }

        // Propagated data
        uint64_t getAppPropagatedPresentStartTime() const { return appPropagatedPresentStartTime; }
        uint64_t getAppPropagatedTimeInPresent() const { return appPropagatedTimeInPresent; }
        uint64_t getAppPropagatedGPUStartTime() const { return appPropagatedGPUStartTime; }
        uint64_t getAppPropagatedReadyTime() const { return appPropagatedReadyTime; }
        uint64_t getAppPropagatedGPUDuration() const { return appPropagatedGPUDuration; }
        uint64_t getAppPropagatedGPUVideoDuration() const { return appPropagatedGPUVideoDuration; }

        // Instrumented data
        uint64_t getAppSimStartTime() const { return appSimStartTime; }
        uint64_t getAppSleepStartTime() const { return appSleepStartTime; }
        uint64_t getAppSleepEndTime() const { return appSleepEndTime; }
        uint64_t getAppRenderSubmitStartTime() const { return appRenderSubmitStartTime; }
        uint64_t getAppRenderSubmitEndTime() const { return appRenderSubmitEndTime; }
        uint64_t getAppPresentStartTime() const { return appPresentStartTime; }
        uint64_t getAppPresentEndTime() const { return appPresentEndTime; }
        std::pair<uint64_t, InputDeviceType> getAppInputSample() const { return appInputSample; }

        // PC Latency
        uint64_t getPclSimStartTime() const { return pclSimStartTime; }
        uint64_t getPclInputPingTime() const { return pclInputPingTime; }

        // Input tracking
        uint64_t getInputTime() const { return inputTime; }
        uint64_t getMouseClickTime() const { return mouseClickTime; }

        // Display data - normalized access
        size_t getDisplayedCount() const { return displayed.size(); }
        FrameType getDisplayedFrameType(size_t idx) const { return displayed[idx].first; }
        uint64_t getDisplayedScreenTime(size_t idx) const { return displayed[idx].second; }

        // Vendor-specific
        uint64_t getFlipDelay() const { return flipDelay; }
        uint32_t getFlipToken() const { return FlipToken; }

        // Metadata
        PresentResult getFinalState() const { return finalState; }
        uint32_t getProcessId() const { return processId; }
        uint32_t getThreadId() const { return threadId; }
        uint64_t getSwapChainAddress() const { return swapChainAddress; }
        uint32_t getFrameId() const { return frameId; }
        uint32_t getAppFrameId() const { return appFrameId; }

        // Factory Methods
        static FrameData CopyFrameData(const PmNsmPresentEvent& p);
        static FrameData CopyFrameData(const std::shared_ptr<PresentEvent>& p);
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

        // PCLatency (optional)
        std::optional<double> msFlipDelay;  // NVIDIA

        // Frame Classification
        FrameType frameType;
    };
}