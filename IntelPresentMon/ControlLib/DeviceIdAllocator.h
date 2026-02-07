// Copyright (C) 2024 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstdint>

namespace pwr
{
    class DeviceIdAllocator
    {
    public:
        explicit DeviceIdAllocator(uint32_t startId = 1) noexcept
            : nextId_{ startId } {}

        uint32_t Next() noexcept
        {
            return nextId_.fetch_add(1, std::memory_order_relaxed);
        }

    private:
        std::atomic<uint32_t> nextId_;
    };
}
