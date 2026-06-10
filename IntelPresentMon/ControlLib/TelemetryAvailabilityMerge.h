// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../Interprocess/source/MetricCapabilities.h"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include <functional>
#include <vector>

namespace pmon::tel
{
    // Merge per-provider entries for one metric on one logical device.
    // Priority when none are AVAILABLE (first match wins):
    //   NOT_SUPPORTED_BY_DEVICE > NOT_EXPORTED_BY_SOURCE >
    //   NOT_IMPLEMENTED_BY_PRESENTMON > UNAVAILABLE
    ipc::MetricCapabilityEntry MergeProviderAvailabilityEntries(
        PM_DEVICE_TYPE deviceType,
        const std::vector<ipc::MetricCapabilityEntry>& providerEntries);

    ipc::MetricCapabilities MergeLogicalDeviceAvailability(
        PM_DEVICE_TYPE deviceType,
        const std::function<std::vector<ipc::MetricCapabilities>()>& getProviderCapsList);
}
