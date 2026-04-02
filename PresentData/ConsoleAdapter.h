// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include "PresentMonTraceConsumer.hpp"

namespace pmon::util::metrics {
    // Zero-cost wrapper around Console's shared_ptr<PresentEvent>
    // Provides same interface as PresentSnapshot via inline getters
    class ConsoleAdapter {
        const PresentEvent* ptr_;

    public:
        // Constructors
        explicit ConsoleAdapter(const std::shared_ptr<PresentEvent>& p) : ptr_(p.get()) {}
        explicit ConsoleAdapter(const PresentEvent* p) : ptr_(p) {}

        // Inline getters - compile to direct field access (zero overhead)
        uint64_t getPresentStartTime() const { return ptr_->PresentStartTime; }
        uint64_t getReadyTime() const { return ptr_->ReadyTime; }
        uint64_t getTimeInPresent() const { return ptr_->TimeInPresent; }
        uint64_t getGPUStartTime() const { return ptr_->GPUStartTime; }
        uint64_t getGPUDuration() const { return ptr_->GPUDuration; }
        uint64_t getGPUVideoDuration() const { return ptr_->GPUVideoDuration; }

        // Propagated data
        uint64_t getAppPropagatedPresentStartTime() const { return ptr_->AppPropagatedPresentStartTime; }
        uint64_t getAppPropagatedTimeInPresent() const { return ptr_->AppPropagatedTimeInPresent; }
        uint64_t getAppPropagatedGPUStartTime() const { return ptr_->AppPropagatedGPUStartTime; }
        uint64_t getAppPropagatedReadyTime() const { return ptr_->AppPropagatedReadyTime; }
        uint64_t getAppPropagatedGPUDuration() const { return ptr_->AppPropagatedGPUDuration; }
        uint64_t getAppPropagatedGPUVideoDuration() const { return ptr_->AppPropagatedGPUVideoDuration; }

        // Instrumented data
        uint64_t getAppSimStartTime() const { return ptr_->AppSimStartTime; }
        uint64_t getAppSleepStartTime() const { return ptr_->AppSleepStartTime; }
        uint64_t getAppSleepEndTime() const { return ptr_->AppSleepEndTime; }
        uint64_t getAppRenderSubmitStartTime() const { return ptr_->AppRenderSubmitStartTime; }
        std::pair<uint64_t, InputDeviceType> getAppInputSample() const { return ptr_->AppInputSample; }

        // PC Latency
        uint64_t getPclSimStartTime() const { return ptr_->PclSimStartTime; }
        uint64_t getPclInputPingTime() const { return ptr_->PclInputPingTime; }

        // Input tracking
        uint64_t getInputTime() const { return ptr_->InputTime; }
        uint64_t getMouseClickTime() const { return ptr_->MouseClickTime; }

        // Display data - normalized access
        size_t getDisplayedCount() const { return ptr_->Displayed.size(); }
        FrameType getDisplayedFrameType(size_t idx) const { return ptr_->Displayed[idx].first; }
        uint64_t getDisplayedScreenTime(size_t idx) const { return ptr_->Displayed[idx].second; }

        // Vendor-specific
        uint64_t getFlipDelay() const { return ptr_->FlipDelay; }

        // Metadata
        PresentResult getFinalState() const { return ptr_->FinalState; }
        uint32_t getProcessId() const { return ptr_->ProcessId; }
        uint64_t getSwapChainAddress() const { return ptr_->SwapChainAddress; }

        // Predicates
        bool hasAppPropagatedData() const { return ptr_->AppPropagatedPresentStartTime != 0; }
        bool hasPclSimStartTime() const { return ptr_->PclSimStartTime != 0; }
        bool hasPclInputPingTime() const { return ptr_->PclInputPingTime != 0; }
    };
}