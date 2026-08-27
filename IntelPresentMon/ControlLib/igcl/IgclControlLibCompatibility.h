// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace pmon::tel::igcl
{
    struct ControlLibFileVersion
    {
        uint16_t major = 0;
        uint16_t minor = 0;
        uint16_t build = 0;
        uint16_t privatePart = 0;
    };

    bool IsPreHotfixControlLibVersion(ControlLibFileVersion version) noexcept;
    bool IsZesInitOnlyControlLibVersion(ControlLibFileVersion version) noexcept;

    // True when collected versions include both pre-hotfix (< 1.2.269.269) and 1.3+ builds.
    bool IsControlLibVersionMismatch(std::span<const ControlLibFileVersion> versions) noexcept;

    // Enumerates per-GPU ControlApiPath registry entries and compares file versions.
    // Returns true when versions are mismatched OR when the compatibility check cannot be completed.
    // Caller must not invoke ctlInit when true.
    bool AreIgclControlLibsMismatched() noexcept;
}
