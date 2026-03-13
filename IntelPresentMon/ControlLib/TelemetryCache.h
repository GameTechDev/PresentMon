// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace pmon::tele
{
    template<class T>
    struct EndpointCache
    {
        T output{};
        int64_t requestQpc = 0;

        bool HasValue(int64_t candidateRequestQpc) const noexcept
        {
            return requestQpc != 0 && candidateRequestQpc != 0 &&
                requestQpc == candidateRequestQpc;
        }

        void Store(int64_t newRequestQpc, const T& newOutput) noexcept
        {
            output = newOutput;
            requestQpc = newRequestQpc;
        }

        void Clear() noexcept
        {
            requestQpc = 0;
        }
    };
}
