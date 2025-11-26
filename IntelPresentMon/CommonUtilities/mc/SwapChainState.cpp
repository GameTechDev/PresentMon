// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "SwapChainState.h"
#include "../PresentData/PresentMonTraceConsumer.hpp"

namespace pmon::util::metrics {

    void SwapChainCoreState::UpdateAfterPresent(const FrameData& present)
    {
        const auto finalState = present.getFinalState();
        const size_t displayCnt = present.getDisplayedCount();

        if (finalState == PresentResult::Presented) {
            if (displayCnt > 0) {
                const size_t lastIdx = displayCnt - 1;
                const auto   lastType = present.getDisplayedFrameType(lastIdx);
                const bool   lastIsAppFrm =
                    (lastType == FrameType::NotSet || lastType == FrameType::Application);

                if (lastIsAppFrm) {
                    const uint64_t lastScreenTime = present.getDisplayedScreenTime(lastIdx);

                    // This block is the port of the big "if (p->Displayed.back().first ...)" from UpdateChain
                    if (animationErrorSource == AnimationErrorSource::AppProvider) {
                        if (present.getAppSimStartTime() != 0) {
                            lastDisplayedSimStartTime = present.getAppSimStartTime();
                            if (firstAppSimStartTime == 0) {
                                firstAppSimStartTime = present.getAppSimStartTime();
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                    }
                    else if (animationErrorSource == AnimationErrorSource::PCLatency) {
                        if (present.getPclSimStartTime() != 0) {
                            lastDisplayedSimStartTime = present.getPclSimStartTime();
                            if (firstAppSimStartTime == 0) {
                                firstAppSimStartTime = present.getPclSimStartTime();
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                    }
                    else { // AnimationErrorSource::CpuStart
                        // Check for app or PCL sim start and possibly change source.
                        if (present.getAppSimStartTime() != 0) {
                            animationErrorSource = AnimationErrorSource::AppProvider;
                            lastDisplayedSimStartTime = present.getAppSimStartTime();
                            if (firstAppSimStartTime == 0) {
                                firstAppSimStartTime = present.getAppSimStartTime();
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                        else if (present.getPclSimStartTime() != 0) {
                            animationErrorSource = AnimationErrorSource::PCLatency;
                            lastDisplayedSimStartTime = present.getPclSimStartTime();
                            if (firstAppSimStartTime == 0) {
                                firstAppSimStartTime = present.getPclSimStartTime();
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                        else {
                            // Fall back to CPU start from last app present, if any.
                            if (lastAppPresent.has_value()) {
                                const FrameData& lastApp = lastAppPresent.value();
                                lastDisplayedSimStartTime =
                                    lastApp.getPresentStartTime() + lastApp.getTimeInPresent();
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                    }
                }
            }

            // Always track "last displayed" screen time + flip delay when presented.
            if (displayCnt > 0) {
                const size_t lastIdx = displayCnt - 1;
                lastDisplayedScreenTime = present.getDisplayedScreenTime(lastIdx);
                lastDisplayedFlipDelay = present.getFlipDelay();
            }
            else {
                lastDisplayedScreenTime = 0;
                lastDisplayedFlipDelay = 0;
            }
        }

        // Last app present selection (same logic as UpdateChain)
        if (displayCnt > 0) {
            const size_t lastIdx = displayCnt - 1;
            const auto   lastType = present.getDisplayedFrameType(lastIdx);
            if (lastType == FrameType::NotSet || lastType == FrameType::Application) {
                lastAppPresent = present;
            }
        }
        else {
            // If not displayed at all, treat this as the last app present.
            lastAppPresent = present;
        }

        // Last simulation start time: PCL wins over App if both exist.
        if (present.getPclSimStartTime() != 0) {
            lastSimStartTime = present.getPclSimStartTime();
        }
        else if (present.getAppSimStartTime() != 0) {
            lastSimStartTime = present.getAppSimStartTime();
        }

        // Always advance lastPresent
        lastPresent = present;
    }

} // namespace pmon::util::metrics
