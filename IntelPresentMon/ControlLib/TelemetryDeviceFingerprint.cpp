// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TelemetryDeviceFingerprint.h"
#include <array>
#include <cctype>
#include <format>
#include <unordered_set>

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

    FingerprintMatchStrength GetTelemetryDeviceFingerprintMatchStrength(
        const TelemetryDeviceFingerprint& lhs,
        const TelemetryDeviceFingerprint& rhs) noexcept
    {
        if (lhs.deviceType != rhs.deviceType) {
            return FingerprintMatchStrength::None;
        }

        // There is only one logical system device exposed through IPC. Any
        // provider reporting PM_DEVICE_TYPE_SYSTEM should correlate to it.
        if (lhs.deviceType == PM_DEVICE_TYPE_SYSTEM) {
            return FingerprintMatchStrength::Name;
        }

        if (lhs.vendor != rhs.vendor) {
            return FingerprintMatchStrength::None;
        }

        // Contradicting identifiers must not be hidden by a match on a less
        // specific field.
        if ((lhs.pciDeviceId && rhs.pciDeviceId && lhs.pciDeviceId != rhs.pciDeviceId) ||
            (lhs.pciSubSystemId && rhs.pciSubSystemId && lhs.pciSubSystemId != rhs.pciSubSystemId)) {
            return FingerprintMatchStrength::None;
        }

        // A validated PCI bus number is the strongest available physical-instance
        // discriminator. Device/subsystem ids and names identify only a model and
        // are shared by identical cards, so they are fallbacks.
        if (lhs.pciBusId.has_value() && rhs.pciBusId.has_value()) {
            return *lhs.pciBusId == *rhs.pciBusId ?
                FingerprintMatchStrength::PciBus :
                FingerprintMatchStrength::None;
        }

        if (OptionalEqual_(lhs.pciDeviceId, rhs.pciDeviceId) &&
            OptionalEqual_(lhs.pciSubSystemId, rhs.pciSubSystemId)) {
            return FingerprintMatchStrength::PciIds;
        }

        if (!lhs.deviceName.empty() && !rhs.deviceName.empty()) {
            const auto lhsName = NormalizeName_(lhs.deviceName);
            const auto rhsName = NormalizeName_(rhs.deviceName);
            if (!lhsName.empty() && lhsName == rhsName) {
                return FingerprintMatchStrength::Name;
            }
        }

        return FingerprintMatchStrength::None;
    }

    bool TelemetryDeviceFingerprintMatches(
        const TelemetryDeviceFingerprint& lhs,
        const TelemetryDeviceFingerprint& rhs) noexcept
    {
        return GetTelemetryDeviceFingerprintMatchStrength(lhs, rhs) !=
            FingerprintMatchStrength::None;
    }

    std::unordered_map<uint32_t, uint32_t> ResolveFingerprintMergeTargets(
        std::span<const FingerprintMergeCandidate> candidates)
    {
        std::unordered_set<uint32_t> unresolvedSources;
        std::unordered_set<uint32_t> unresolvedTargets;
        for (const auto& candidate : candidates) {
            if (candidate.strength != FingerprintMatchStrength::None) {
                unresolvedSources.insert(candidate.sourceId);
                unresolvedTargets.insert(candidate.targetId);
            }
        }

        std::unordered_map<uint32_t, uint32_t> targets;
        constexpr std::array strengths{
            FingerprintMatchStrength::PciBus,
            FingerprintMatchStrength::PciIds,
            FingerprintMatchStrength::Name,
        };

        for (const auto minimumStrength : strengths) {
            while (true) {
                std::unordered_map<uint32_t, size_t> optionsBySource;
                std::unordered_map<uint32_t, size_t> optionsByTarget;
                for (const auto& candidate : candidates) {
                    if ((uint8_t)candidate.strength < (uint8_t)minimumStrength ||
                        !unresolvedSources.contains(candidate.sourceId) ||
                        !unresolvedTargets.contains(candidate.targetId)) {
                        continue;
                    }
                    ++optionsBySource[candidate.sourceId];
                    ++optionsByTarget[candidate.targetId];
                }

                std::vector<std::pair<uint32_t, uint32_t>> accepted;
                for (const auto& candidate : candidates) {
                    if ((uint8_t)candidate.strength < (uint8_t)minimumStrength ||
                        !unresolvedSources.contains(candidate.sourceId) ||
                        !unresolvedTargets.contains(candidate.targetId) ||
                        optionsBySource[candidate.sourceId] != 1 ||
                        optionsByTarget[candidate.targetId] != 1) {
                        continue;
                    }
                    accepted.emplace_back(candidate.sourceId, candidate.targetId);
                }
                if (accepted.empty()) {
                    break;
                }

                for (const auto& match : accepted) {
                    targets.emplace(match.first, match.second);
                    unresolvedSources.erase(match.first);
                    unresolvedTargets.erase(match.second);
                }
            }
        }

        return targets;
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
