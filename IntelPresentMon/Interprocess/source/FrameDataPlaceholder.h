#pragma once
#include "../../../PresentData/PresentMonTraceConsumer.hpp"
#include <utility>
#include <boost/container/static_vector.hpp>

namespace pmon::ipc
{
    // temporary; replace with structure dictated by new PresentMon calculation library
    struct FrameData {
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

        boost::container::static_vector<std::pair<FrameType, uint64_t>, 10> displayed;

        // PC Latency data
        uint64_t pclSimStartTime = 0;
        uint64_t pclInputPingTime = 0;
        uint64_t flipDelay = 0;
        uint32_t flipToken = 0;

        // Metadata
        PresentResult finalState;
        uint32_t processId = 0;
        uint32_t threadId = 0;
        uint64_t swapChainAddress = 0;
        uint32_t frameId = 0;
        uint32_t appFrameId = 0;
    };
}
