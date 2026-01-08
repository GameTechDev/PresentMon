// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"
#include "MetricsCalculatorInternal.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"
#include "../Math.h"

namespace pmon::util::metrics
{
    namespace
    {
        // ---- CPU/GPU metrics ----
        double ComputeMsCpuBusy(
            const QpcConverter& qpc,
            const SwapChainCoreState& swapChain,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            const auto cpuStart = CalculateCPUStart(swapChain, present);
            if (cpuStart == 0) {
                return 0.0;
            }

            if (present.appPropagatedPresentStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.appPropagatedPresentStartTime);
            }
            if (present.presentStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.presentStartTime);
            }
            return 0.0;
        }


        double ComputeMsCpuWait(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            if (present.appPropagatedTimeInPresent != 0) {
                return qpc.DurationMilliSeconds(present.appPropagatedTimeInPresent);
            }
            if (present.timeInPresent != 0) {
                return qpc.DurationMilliSeconds(present.timeInPresent);
            }
            return 0.0;
        }


        double ComputeMsGpuLatency(
            const QpcConverter& qpc,
            const SwapChainCoreState& swapChain,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            const auto cpuStart = CalculateCPUStart(swapChain, present);
            if (cpuStart == 0) {
                return 0.0;
            }

            if (present.appPropagatedGPUStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.appPropagatedGPUStartTime);
            }
            if (present.gpuStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.gpuStartTime);
            }
            return 0.0;
        }


        double ComputeMsGpuBusy(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            //out.msGPUBusy = std::nullopt;
            double msGPUBusy = 0.0;
            if (isAppPresent) {
                if (present.appPropagatedGPUDuration != 0) {
                    msGPUBusy = qpc.DurationMilliSeconds(present.appPropagatedGPUDuration);
                }
                else if (present.gpuDuration != 0) {
                    msGPUBusy = qpc.DurationMilliSeconds(present.gpuDuration);
                }
            }
            return msGPUBusy;
        }

        double ComputeMsVideoBusy(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            if (present.appPropagatedGPUVideoDuration != 0) {
                return qpc.DurationMilliSeconds(present.appPropagatedGPUVideoDuration);
            }
            if (present.gpuVideoDuration != 0) {
                return qpc.DurationMilliSeconds(present.gpuVideoDuration);
            }
            return 0.0;
        }

        double ComputeMsGpuDuration(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            //msGPUDuration = std::nullopt;
            double msGPUDuration = 0.0;
            if (isAppPresent) {
                if (present.appPropagatedGPUStartTime != 0 || present.appPropagatedReadyTime != 0) {
                    msGPUDuration = qpc.DeltaUnsignedMilliSeconds(present.appPropagatedGPUStartTime, present.appPropagatedReadyTime);
                }
                else if (present.gpuStartTime != 0 || present.readyTime != 0) {
                    msGPUDuration = qpc.DeltaUnsignedMilliSeconds(present.gpuStartTime, present.readyTime);
                }
            }
            return msGPUDuration;
        }

        double ComputeMsGpuWait(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            return std::max(0.0,
                ComputeMsGpuDuration(qpc, present, isAppPresent) -
                ComputeMsGpuBusy(qpc, present, isAppPresent));
        }
    }

    void CalculateCpuGpuMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& chainState,
        const FrameData& present,
        bool isAppFrame,
        FrameMetrics& metrics)
    {
        metrics.msCPUBusy = ComputeMsCpuBusy(qpc, chainState, present, isAppFrame);
        metrics.msCPUWait = ComputeMsCpuWait(qpc, present, isAppFrame);
        metrics.msGPULatency = ComputeMsGpuLatency(qpc, chainState, present, isAppFrame);

        metrics.msGPUBusy = ComputeMsGpuBusy(qpc, present, isAppFrame);
        metrics.msVideoBusy = ComputeMsVideoBusy(qpc, present, isAppFrame);
        metrics.msGPUWait = ComputeMsGpuWait(qpc, present, isAppFrame);
    }
}