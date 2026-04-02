// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT
#pragma once

enum class PresentMode {
    Unknown = 0,
    Hardware_Legacy_Flip = 1,
    Hardware_Legacy_Copy_To_Front_Buffer = 2,
    Hardware_Independent_Flip = 3,
    Composed_Flip = 4,
    Composed_Copy_GPU_GDI = 5,
    Composed_Copy_CPU_GDI = 6,
    Hardware_Composed_Independent_Flip = 8,
};

enum class PresentResult {
    Unknown = 0,
    Presented = 1,
    Discarded = 2,
};

enum class Runtime {
    Other = 0,
    DXGI = 1,
    D3D9 = 2,
};

enum class InputDeviceType {
    None = 0,
    Unknown = 1,
    Mouse = 2,
    Keyboard = 3,
};

enum class FrameType {
    NotSet = 0,
    Unspecified = 1,
    Application = 2,
    Repeated = 3,
    Intel_XEFG = 50,
    AMD_AFMF = 100,
};