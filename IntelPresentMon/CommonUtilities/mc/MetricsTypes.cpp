// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"

namespace pmon::util::metrics {

    PresentSnapshot PresentSnapshot::FromCircularBuffer(const PmNsmPresentEvent& p) {
        PresentSnapshot snap{};

        snap.presentStartTime = p.PresentStartTime;
        snap.processId  = p.ProcessId;
        snap.t
        snap.timeInPresent = p.TimeInPresent;
        snap.readyTime = p.ReadyTime;
        // ... etc ...

        // Normalize parallel arrays to vector<DisplayEntry>
        snap.displayed.reserve(p.DisplayedCount);
        for (size_t i = 0; i < p.DisplayedCount; ++i) {
            snap.displayed.push_back({
                p.Displayed_FrameType[i],
                p.Displayed_ScreenTime[i]
                });
        }

        return snap;
    }
}