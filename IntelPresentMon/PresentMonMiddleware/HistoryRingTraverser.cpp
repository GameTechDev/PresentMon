#include "HistoryRingTraverser.h"

namespace pmon::mid
{
    HistoryRingTraverser::HistoryRingTraverser() = default;
    HistoryRingTraverser::~HistoryRingTraverser() = default;

    size_t HistoryRingTraverser::GetCursor() const
    {
        return cursor_;
    }
}
