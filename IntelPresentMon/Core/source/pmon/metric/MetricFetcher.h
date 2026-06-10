// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <optional>

namespace p2c::pmon::met
{
    class MetricFetcher
    {
    public:
        MetricFetcher() = default;
        virtual ~MetricFetcher();
        virtual std::wstring ReadStringValue();
        virtual std::optional<float> ReadValue() = 0;

        MetricFetcher(const MetricFetcher&) = delete;
        MetricFetcher & operator=(const MetricFetcher&) = delete;
        MetricFetcher(MetricFetcher&&) = delete;
        MetricFetcher & operator=(MetricFetcher&&) = delete;
    };

    class UnavailableMetricFetcher : public MetricFetcher
    {
    public:
        std::optional<float> ReadValue() override;
        std::wstring ReadStringValue() override;
    };
}