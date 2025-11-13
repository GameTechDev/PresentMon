// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace pmon::util::metrics
{
    // Simple value-based QPC math utility
    // No polymorphism, no environment dependencies - just QPC calculations
    class QpcCalculator {
        uint64_t qpcFrequency_;
        uint64_t sessionStartTimestamp_;

    public:
        QpcCalculator(uint64_t qpcFrequency, uint64_t sessionStartTimestamp)
            : qpcFrequency_(qpcFrequency)
            , sessionStartTimestamp_(sessionStartTimestamp)
        {
        }

        // Convert QPC duration to milliseconds
        double TimestampDeltaToMilliSeconds(uint64_t duration) const {
            return duration * 1000.0 / qpcFrequency_;
        }

        // Convert time between two QPC timestamps to milliseconds
        double TimestampDeltaToUnsignedMilliSeconds(uint64_t start, uint64_t end) const {
            return (end - start) * 1000.0 / qpcFrequency_;
        }

        // Get trace session start timestamp (for animation time calculations)
        uint64_t GetStartTimestamp() const {
            return sessionStartTimestamp_;
        }
    };

}