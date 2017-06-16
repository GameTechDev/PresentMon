/*
Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <deque>
#include <stdint.h>

#include "PresentMonTraceConsumer.hpp"

struct SwapChainData {
    Runtime mRuntime = Runtime::Other;
    uint64_t mLastUpdateTicks = 0;
    uint32_t mLastSyncInterval = UINT32_MAX;
    uint32_t mLastFlags = UINT32_MAX;
    std::deque<PresentEvent> mPresentHistory;
    std::deque<PresentEvent> mDisplayedPresentHistory;
    PresentMode mLastPresentMode = PresentMode::Unknown;
    uint32_t mLastPlane = 0;
    bool mHasBeenBatched = false;
    
    void PruneDeque(std::deque<PresentEvent> &presentHistory, uint64_t perfFreq, uint32_t msTimeDiff, uint32_t maxHistLen);
    void AddPresentToSwapChain(PresentEvent& p);
    void UpdateSwapChainInfo(PresentEvent&p, uint64_t now, uint64_t perfFreq);
    double ComputeDisplayedFps(uint64_t qpcFreq);
    double ComputeFps(uint64_t qpcFreq);
    double ComputeLatency(uint64_t qpcFreq);
    double ComputeCpuFrameTime(uint64_t qpcFreq);
    bool IsStale(uint64_t now) const;
private:
    double ComputeFps(const std::deque<PresentEvent>& presentHistory, uint64_t qpcFreq);
};
