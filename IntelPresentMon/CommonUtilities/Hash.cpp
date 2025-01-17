// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Hash.h"
#include "win/WinAPI.h"


namespace pmon::util::hash
{
#if defined(_WIN64)
    // 64-bit version
    size_t HashCombine(size_t lhs, size_t rhs) noexcept
    {
        constexpr size_t kHashConstant = 0x517c'c1b7'2722'0a95ull;
        return lhs ^ (rhs + kHashConstant + (lhs << 6) + (lhs >> 2));
    }
    size_t HashGuid(const _GUID& guid) noexcept
    {
        return HashCombine(reinterpret_cast<const uint64_t&>(guid.Data1),
            reinterpret_cast<const size_t&>(guid.Data4));
    }
#else
    // 32-bit version
    size_t HashCombine(size_t lhs, size_t rhs) noexcept
    {
        constexpr size_t kHashConstant = 0x9e37'79b9u;
        return lhs ^ (rhs + kHashConstant + (lhs << 6) + (lhs >> 2));
    }
    size_t HashGuid(const _GUID& guid) noexcept
    {
        return HashCombine(
            HashCombine(reinterpret_cast<const size_t&>(guid.Data1), reinterpret_cast<const size_t&>(guid.Data2)),
            HashCombine(reinterpret_cast<const size_t&>(guid.Data4), reinterpret_cast<const size_t&>(guid.Data4[4]))
        );
    }
#endif

}