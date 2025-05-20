// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#pragma once

namespace NvidiaDisplayDriver_Events {

    enum class Keyword : uint64_t {
        None = 0x00,
    };

    struct __declspec(uuid("{AE4F8626-8265-40D1-A70B-11B64240E8E9}")) GUID_STRUCT;
    static const auto GUID = __uuidof(GUID_STRUCT);

    // Event descriptors:
#define EVENT_DESCRIPTOR_DECL(name_, id_, version_, channel_, level_, opcode_, task_, keyword_) struct name_ { \
    static uint16_t const Id      = id_; \
    static uint8_t  const Version = version_; \
    static uint8_t  const Channel = channel_; \
    static uint8_t  const Level   = level_; \
    static uint8_t  const Opcode  = opcode_; \
    static uint16_t const Task    = task_; \
    static Keyword  const Keyword = (Keyword) keyword_; \
};

    EVENT_DESCRIPTOR_DECL(FlipRequest, 0x0001, 0x00, 0x13, 0x04, 0x0a, 0x0001, 0x1000000000000000)
#undef EVENT_DESCRIPTOR_DECL
}