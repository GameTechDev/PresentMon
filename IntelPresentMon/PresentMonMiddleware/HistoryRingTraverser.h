#pragma once
#include <cstddef>

namespace pmon::mid
{
    class HistoryRingTraverser
    {
    public:
        HistoryRingTraverser();
        ~HistoryRingTraverser();

        size_t GetCursor() const;

    private:
        size_t cursor_ = 0;
        size_t size_ = 0;
    };
}
