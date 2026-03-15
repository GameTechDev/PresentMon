// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../PresentMonAPI2/PresentMonAPI.h"
#include <cstdint>
#include <optional>
#include <string>

namespace pmon::tel
{
    struct TelemetryDeviceFingerprint
    {
        PM_DEVICE_TYPE deviceType = PM_DEVICE_TYPE_INDEPENDENT;
        PM_DEVICE_VENDOR vendor = PM_DEVICE_VENDOR_UNKNOWN;
        std::string deviceName;

        // Correlation fields aligned with NVIDIA matching.
        std::optional<uint32_t> pciDeviceId;
        std::optional<uint32_t> pciSubSystemId;
        std::optional<uint32_t> pciBusId;
    };

    bool TelemetryDeviceFingerprintMatches(
        const TelemetryDeviceFingerprint& lhs,
        const TelemetryDeviceFingerprint& rhs) noexcept;

    void MergeTelemetryDeviceFingerprint(
        TelemetryDeviceFingerprint& dst,
        const TelemetryDeviceFingerprint& src);
}
