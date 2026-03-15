// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TelemetryDeviceFingerprint.h"
#include <cctype>
#include <format>

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

    std::string TelemetryDeviceFingerprint::LuidAsString() const
    {
        if (luid.empty()) {
            return "<EMPTY>";
        }

        std::string text{};
        for (size_t i = 0; i < luid.size(); ++i) {
            text += std::format("{}{:02X}", i != 0 ? " " : "", luid[i]);
        }

        return text;
    }

    void MergeTelemetryDeviceFingerprint(
        TelemetryDeviceFingerprint& dst,
        const TelemetryDeviceFingerprint& src)
    {
        if (dst.vendor == PM_DEVICE_VENDOR_UNKNOWN) {
            dst.vendor = src.vendor;
        }

        if (dst.deviceName.empty()) {
            dst.deviceName = src.deviceName;
        }

        if (!dst.isIntegratedAdapter) {
            dst.isIntegratedAdapter = src.isIntegratedAdapter;
        }

        if (dst.luid.empty()) {
            dst.luid = src.luid;
        }

        if (!dst.pciDeviceId.has_value()) {
            dst.pciDeviceId = src.pciDeviceId;
        }

        if (!dst.pciSubSystemId.has_value()) {
            dst.pciSubSystemId = src.pciSubSystemId;
        }

        if (!dst.pciBusId.has_value()) {
            dst.pciBusId = src.pciBusId;
        }
    }
}
