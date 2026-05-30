#pragma once
#include <cstdint>

namespace pmon::mid
{
    struct DynamicQueryWindow
    {
        uint64_t oldest = 0;
        uint64_t newest = 0;
    };
}
