// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TelemetryDeviceFingerprint.h"
#include <cctype>

namespace pmon::tel
{
    namespace
    {
        std::string NormalizeName_(const std::string& name)
        {
            std::string normalized;
            normalized.reserve(name.size());
            for (const auto ch : name) {
                const auto c = static_cast<unsigned char>(ch);
                if (std::isalnum(c)) {
                    normalized.push_back(static_cast<char>(std::tolower(c)));
                }
            }
            return normalized;
        }

        template<typename T>
        bool OptionalEqual_(const std::optional<T>& lhs, const std::optional<T>& rhs) noexcept
        {
            return lhs.has_value() && rhs.has_value() && (*lhs == *rhs);
        }
    }

    bool TelemetryDeviceFingerprintMatches(
        const TelemetryDeviceFingerprint& lhs,
        const TelemetryDeviceFingerprint& rhs) noexcept
    {
        if (lhs.deviceType != rhs.deviceType || lhs.vendor != rhs.vendor) {
            return false;
        }

        if (OptionalEqual_(lhs.pciDeviceId, rhs.pciDeviceId) &&
            OptionalEqual_(lhs.pciSubSystemId, rhs.pciSubSystemId)) {
            return true;
        }

        if (OptionalEqual_(lhs.pciBusId, rhs.pciBusId)) {
            return true;
        }

        if (!lhs.deviceName.empty() && !rhs.deviceName.empty()) {
            return NormalizeName_(lhs.deviceName) == NormalizeName_(rhs.deviceName);
        }

        return false;
    }

    void MergeTelemetryDeviceFingerprint(
        TelemetryDeviceFingerprint& dst,
        const TelemetryDeviceFingerprint& src)
    {
        if (dst.vendor == PM_DEVICE_VENDOR_UNKNOWN && src.vendor != PM_DEVICE_VENDOR_UNKNOWN) {
            dst.vendor = src.vendor;
        }

        if (dst.deviceName.empty() && !src.deviceName.empty()) {
            dst.deviceName = src.deviceName;
        }

        if (!dst.pciDeviceId.has_value() && src.pciDeviceId.has_value()) {
            dst.pciDeviceId = src.pciDeviceId;
        }

        if (!dst.pciSubSystemId.has_value() && src.pciSubSystemId.has_value()) {
            dst.pciSubSystemId = src.pciSubSystemId;
        }

        if (!dst.pciBusId.has_value() && src.pciBusId.has_value()) {
            dst.pciBusId = src.pciBusId;
        }
    }
}
