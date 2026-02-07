// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include "PresentMonPowerTelemetry.h"
#include "PowerTelemetryProvider.h"
#include "NvapiWrapper.h"
#include "NvmlWrapper.h"

namespace pwr::nv
{
    class NvidiaPowerTelemetryProvider : public PowerTelemetryProvider
    {
    public:
        NvidiaPowerTelemetryProvider();
        const std::vector<std::shared_ptr<PowerTelemetryAdapter>>& GetAdapters() noexcept override;
        uint32_t GetAdapterCount() const noexcept override;

    private:
        NvapiWrapper nvapi;
        NvmlWrapper nvml;
        std::vector<std::shared_ptr<PowerTelemetryAdapter>> adapterPtrs;
    };
}
