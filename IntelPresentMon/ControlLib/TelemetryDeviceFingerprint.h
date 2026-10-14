// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../PresentMonAPI2/PresentMonAPI.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pmon::tel
{
    struct TelemetryDeviceFingerprint
    {
        PM_DEVICE_TYPE deviceType = PM_DEVICE_TYPE_INDEPENDENT;
        PM_DEVICE_VENDOR vendor = PM_DEVICE_VENDOR_UNKNOWN;
        std::string deviceName;
        bool isIntegratedAdapter = false;

        // Does not currently participate in matching: LUIDs reported by different providers are
        // not guaranteed to be comparable. Matching by LUID would require extra data to tell
        // which providers' LUIDs are compatible. When merging, the first non-empty LUID wins.
        std::vector<uint8_t> luid;
        // Correlation fields aligned with NVIDIA matching.
        std::optional<uint32_t> pciDeviceId;
        std::optional<uint32_t> pciSubSystemId;
        std::optional<uint32_t> pciBusId;

        std::string LuidAsString() const;
    };

    bool TelemetryDeviceFingerprintMatches(
        const TelemetryDeviceFingerprint& lhs,
        const TelemetryDeviceFingerprint& rhs) noexcept;

    void MergeTelemetryDeviceFingerprint(
        TelemetryDeviceFingerprint& dst,
        const TelemetryDeviceFingerprint& src);
}
