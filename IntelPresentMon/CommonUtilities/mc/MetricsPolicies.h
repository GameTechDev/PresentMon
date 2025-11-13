// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "MetricsTypes.h"
#include "MetricsCalculator.h"

struct SwapChainData;        // Defined in PresentData/PresentMonTraceConsumer.hpp
struct fpsSwapChainData;     // Defined in IntelPresentMon/Interprocess/source/SyncData.h
struct ProcessInfo;          // Defined in PresentData/PresentMonTraceConsumer.hpp
class InputToFsManager;      // Defined in IntelPresentMon/Interprocess/

namespace pmon::util::metrics {
    // Policy interface - works with forward-declared types
    template<typename SwapChainT>
    class MetricsOutputPolicy {
    public:
        virtual ~MetricsOutputPolicy() = default;

        virtual void OnMetricsComputed(
            const FrameMetrics& metrics,
            const ComputedMetrics::StateDeltas& deltas,
            SwapChainT& chain,              // ← Forward-declared type is fine here
            size_t displayIndex,
            bool isAppIndex) = 0;

        virtual void OnFrameComplete(
            const PresentSnapshot& present,
            SwapChainT& chain) = 0;         // ← Forward-declared type is fine here
    };

    // Console policy - forward-declared types in declaration
    class ConsoleMetricsPolicy : public MetricsOutputPolicy<SwapChainData> {
        ProcessInfo* processInfo_;       // ← Pointer to forward-declared type is fine
        bool isRecording_;
        bool computeAvg_;

    public:
        ConsoleMetricsPolicy(
            ProcessInfo* processInfo,
            bool isRecording,
            bool computeAvg);

        void OnMetricsComputed(
            const FrameMetrics& metrics,
            const ComputedMetrics::StateDeltas& deltas,
            SwapChainData& chain,
            size_t displayIndex,
            bool isAppIndex) override;

        void OnFrameComplete(
            const PresentSnapshot& present,
            SwapChainData& chain) override;
    };

    // Middleware policy - forward-declared types in declaration
    class MiddlewareMetricsPolicy : public MetricsOutputPolicy<fpsSwapChainData> {
        InputToFsManager& pclManager_;   // ← Reference to forward-declared type is fine
        uint32_t processId_;

    public:
        MiddlewareMetricsPolicy(
            InputToFsManager& pclManager,
            uint32_t processId);

        void OnMetricsComputed(
            const FrameMetrics& metrics,
            const ComputedMetrics::StateDeltas& deltas,
            fpsSwapChainData& chain,
            size_t displayIndex,
            bool isAppIndex) override;

        void OnFrameComplete(
            const PresentSnapshot& present,
            fpsSwapChainData& chain) override;
    };
}