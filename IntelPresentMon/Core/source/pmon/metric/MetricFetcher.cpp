// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricFetcher.h"
#include <format>

namespace p2c::pmon::met
{
    MetricFetcher::~MetricFetcher() = default;
    std::wstring MetricFetcher::ReadStringValue()
    {
        if (const auto val = ReadValue()) {
            const auto digitsBeforeDecimal = std::max(int(log10(std::abs(*val))), 0);
            const int maxFractionalDigits = 2;
            return std::format(L"{:.{}f}", *val, std::max(maxFractionalDigits - digitsBeforeDecimal, 0));
        }
        return L"NA";
    }

    std::optional<float> UnavailableMetricFetcher::ReadValue()
    {
        return {};
    }

    std::wstring UnavailableMetricFetcher::ReadStringValue()
    {
        return L"NA";
    }
}