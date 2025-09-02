// Copyright (C) 2024 Intel Corporation
// SPDX-License-Identifier: MIT
//
// This file is manually created from the streamline definitions of PCL Stats found here:
// https://github.com/NVIDIAGameWorks/Streamline/blob/main/include/sl_pcl.h

#pragma once

namespace Nvidia_PCL {

    struct __declspec(uuid("{0D216F06-82A6-4D49-BC4F-8F38AE56EFAB}")) GUID_STRUCT;
    static const auto GUID = __uuidof(GUID_STRUCT);

    enum class PCLMarker : uint32_t {
        SimulationStart = 0,
        SimulationEnd = 1,
        RenderSubmitStart = 2,
        RenderSubmitEnd = 3,
        PresentStart = 4,
        PresentEnd = 5,
        PCLLatencyPing = 8,
        OutOfBandPresentStart = 11
    };
}