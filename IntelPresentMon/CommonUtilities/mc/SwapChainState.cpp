// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "SwapChainState.h"
#include <PresentData/PresentMonTraceConsumer.hpp>

namespace pmon::util::metrics {

    void SwapChainCoreState::UpdateAfterPresent(const FrameData& present)
    {
        // Legacy V1 present-level update path. This intentionally preserves the old
        // Displayed.Back()-based app attribution. Frame-generation paths must use
        // UpdateAfterReadyDisplayRow(), where app attribution is resolved per display row.
        const auto finalState = present.finalState;
        const size_t displayCnt = present.displayed.Size();

        if (finalState == PresentResult::Presented) {
            if (displayCnt > 0) {
                const size_t lastIdx = displayCnt - 1;
                const auto   lastType = present.displayed[lastIdx].first;
                const bool   lastIsAppFrm =
                    (lastType == FrameType::NotSet || lastType == FrameType::Application);

                if (lastIsAppFrm) {
                    const uint64_t lastScreenTime = present.displayed[lastIdx].second;

                    if (animationErrorSource == AnimationErrorSource::AppProvider) {
                        if (present.appSimStartTime != 0) {
                            lastDisplayedSimStartTime = present.appSimStartTime;
                            if (firstAppSimStartTime == 0) {
                                firstAppSimStartTime = present.appSimStartTime;
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                    }
                    else if (animationErrorSource == AnimationErrorSource::PCLatency) {
                        if (present.appSimStartTime != 0) {
                            animationErrorSource = AnimationErrorSource::AppProvider;
                            firstAppSimStartTime = present.appSimStartTime;
                            lastDisplayedSimStartTime = present.appSimStartTime;
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                        else if (present.pclSimStartTime != 0) {
                            lastDisplayedSimStartTime = present.pclSimStartTime;
                            if (firstAppSimStartTime == 0) {
                                firstAppSimStartTime = present.pclSimStartTime;
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                    }
                    else { // AnimationErrorSource::CpuStart
                        // Check for provider sim start and possibly change source.
                        if (present.appSimStartTime != 0) {
                            animationErrorSource = AnimationErrorSource::AppProvider;
                            firstAppSimStartTime = present.appSimStartTime;
                            lastDisplayedSimStartTime = present.appSimStartTime;
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                        else if (present.pclSimStartTime != 0) {
                            animationErrorSource = AnimationErrorSource::PCLatency;
                            firstAppSimStartTime = present.pclSimStartTime;
                            lastDisplayedSimStartTime = present.pclSimStartTime;
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                        else {
                            // Fall back to CPU start from last app present, if any.
                            if (lastAppPresent.has_value()) {
                                const FrameData& lastApp = lastAppPresent.value();
                                lastDisplayedSimStartTime =
                                    lastApp.presentStartTime + lastApp.timeInPresent;
                            }
                            lastDisplayedAppScreenTime = lastScreenTime;
                        }
                    }
                }
            }

            // Always track "last displayed" screen time + flip delay when presented.
            if (displayCnt > 0) {
                const size_t lastIdx = displayCnt - 1;
                lastDisplayedScreenTime = present.displayed[lastIdx].second;
                lastDisplayedFlipDelay = present.flipDelay;
            }
            else {
                lastDisplayedScreenTime = 0;
                lastDisplayedFlipDelay = 0;
            }
        }

        // Last app present selection (same logic as UpdateChain)
        if (displayCnt > 0) {
            const size_t lastIdx = displayCnt - 1;
            const auto   lastType = present.displayed[lastIdx].first;
            if (lastType == FrameType::NotSet || lastType == FrameType::Application) {
                lastAppPresent = present;
            }
        }
        else {
            // If not displayed at all, treat this as the last app present.
            lastAppPresent = present;
        }

        // Last simulation start time: PCL wins over App if both exist.
        if (present.pclSimStartTime != 0) {
            lastSimStartTime = present.pclSimStartTime;
        }
        else if (present.appSimStartTime != 0) {
            lastSimStartTime = present.appSimStartTime;
        }

        // Always advance lastPresent
        lastPresent = present;
    }

    void SwapChainCoreState::UpdateAfterReadyDisplayRow(const ReadyDisplayRow& row)
    {
        const auto& present = row.present;

        if (row.isDisplayed) {
            lastDisplayedScreenTime = row.screenTime;
            lastDisplayedFlipDelay = present.flipDelay;

            if (row.isAppFrame) {
                lastDisplayedAppScreenTime = row.screenTime;
                lastAppPresent = present;

                if (row.animation.hasResolvedSimStart) {
                    animationErrorSource = row.animation.source;
                    lastDisplayedSimStartTime = row.animation.resolvedSimStartTime;
                    if (row.animation.firstSimStartTime != 0) {
                        firstAppSimStartTime = row.animation.firstSimStartTime;
                    }
                    else if (firstAppSimStartTime == 0) {
                        firstAppSimStartTime = row.animation.resolvedSimStartTime;
                    }
                }
            }
        }
        else {
            // Not displayed: advance present/input history but leave last displayed
            // screen time and flip delay unchanged. The screen is still showing
            // whatever it last showed; a dropped present does not change that.
            lastAppPresent = present;
        }

        if (present.pclSimStartTime != 0) {
            lastSimStartTime = present.pclSimStartTime;
        }
        else if (present.appSimStartTime != 0) {
            lastSimStartTime = present.appSimStartTime;
        }

        lastPresent = present;
    }

} // namespace pmon::util::metrics
