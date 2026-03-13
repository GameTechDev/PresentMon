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

        bool Matches(int64_t candidateRequestQpc) const noexcept
        {
            return requestQpc != 0 && candidateRequestQpc != 0 &&
                requestQpc == candidateRequestQpc;
        }

        bool Empty() const noexcept
        {
            return requestQpc == 0;
        }

        void Clear() noexcept
        {
            requestQpc = 0;
        }
    };
}
