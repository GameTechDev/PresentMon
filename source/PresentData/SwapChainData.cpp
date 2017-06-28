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

#include "SwapChainData.hpp"

enum {
    MAX_HISTORY_TIME = 2000,
    CHAIN_TIMEOUT_THRESHOLD_TICKS = 10000, // 10 sec
    MAX_PRESENTS_IN_DEQUE = 60 * (MAX_HISTORY_TIME / 1000)
};

void SwapChainData::PruneDeque(std::deque<PresentEvent> &presentHistory, uint64_t perfFreq, uint32_t msTimeDiff, uint32_t maxHistLen) {
    while (!presentHistory.empty() &&
        (presentHistory.size() > maxHistLen ||
        ((double)(presentHistory.back().QpcTime - presentHistory.front().QpcTime) / perfFreq) * 1000 > msTimeDiff)) {
        presentHistory.pop_front();
    }
}

void SwapChainData::AddPresentToSwapChain(PresentEvent& p)
{
    if (p.FinalState == PresentResult::Presented)
    {
        mDisplayedPresentHistory.push_back(p);
    }
    if (!mPresentHistory.empty())
    {
        assert(mPresentHistory.back().QpcTime <= p.QpcTime);
    }
    mPresentHistory.push_back(p);
}

void SwapChainData::UpdateSwapChainInfo(PresentEvent&p, uint64_t now, uint64_t perfFreq)
{
    PruneDeque(mDisplayedPresentHistory, perfFreq, MAX_HISTORY_TIME, MAX_PRESENTS_IN_DEQUE);
    PruneDeque(mPresentHistory, perfFreq, MAX_HISTORY_TIME, MAX_PRESENTS_IN_DEQUE);

    mLastUpdateTicks = now;
    mRuntime = p.Runtime;
    mLastSyncInterval = p.SyncInterval;
    mLastFlags = p.PresentFlags;
    mLastPresentMode = p.PresentMode;
    mLastPlane = p.PlaneIndex;
    mHasBeenBatched = p.WasBatched;
    mDwmNotified = p.DwmNotified;
}

double SwapChainData::ComputeFps(const std::deque<PresentEvent>& presentHistory, uint64_t qpcFreq)
{
    if (presentHistory.size() < 2) {
        return 0.0;
    }
    auto start = presentHistory.front().QpcTime;
    auto end = presentHistory.back().QpcTime;
    auto count = presentHistory.size() - 1;

    double deltaT = double(end - start) / qpcFreq;
    return count / deltaT;
}

double SwapChainData::ComputeDisplayedFps(uint64_t qpcFreq)
{
    return ComputeFps(mDisplayedPresentHistory, qpcFreq);
}

double SwapChainData::ComputeFps(uint64_t qpcFreq)
{
    return ComputeFps(mPresentHistory, qpcFreq);
}

double SwapChainData::ComputeLatency( uint64_t qpcFreq)
{
    if (mDisplayedPresentHistory.size() < 2) {
        return 0.0;
    }

    uint64_t totalLatency = std::accumulate(mDisplayedPresentHistory.begin(), mDisplayedPresentHistory.end() - 1, 0ull,
        [](uint64_t current, PresentEvent const& e) { return current + e.ScreenTime - e.QpcTime; });
    double average = ((double)(totalLatency) / qpcFreq) / (mDisplayedPresentHistory.size() - 1);
    return average;
}

double SwapChainData::ComputeCpuFrameTime(uint64_t qpcFreq)
{
    if (mPresentHistory.size() < 2) {
        return 0.0;
    }

    uint64_t timeInPresent = std::accumulate(mPresentHistory.begin(), mPresentHistory.end() - 1, 0ull,
        [](uint64_t current, PresentEvent const& e) { return current + e.TimeTaken; });
    uint64_t totalTime = mPresentHistory.back().QpcTime - mPresentHistory.front().QpcTime;

    double timeNotInPresent = double(totalTime - timeInPresent) / qpcFreq;
    return timeNotInPresent / (mPresentHistory.size() - 1);
}

bool SwapChainData::IsStale(uint64_t now) const
{
    return now - mLastUpdateTicks > CHAIN_TIMEOUT_THRESHOLD_TICKS;
}
