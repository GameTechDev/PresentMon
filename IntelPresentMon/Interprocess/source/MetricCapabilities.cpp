#include "MetricCapabilities.h"

namespace pmon::ipc
{
    void MetricCapabilities::Set(PM_METRIC metricId, size_t arraySize)
    {
        if (arraySize == 0) {
            // Zero-sized capabilities are effectively "not available";
            // ignore rather than storing.
            return;
        }

        auto it = caps_.find(metricId);
        if (it == caps_.end()) {
            caps_.emplace(metricId, arraySize);
        }
        else {
            it->second = arraySize;
        }
    }

    void MetricCapabilities::Merge(const MetricCapabilities& capsToMerge)
    {
        // Union of capabilities; for overlapping metrics overwrite
        for (const auto& kv : capsToMerge.caps_) {
            const auto metricId = kv.first;
            const auto arraySize = kv.second;
            Set(metricId, arraySize);
        }
    }

    size_t MetricCapabilities::Check(PM_METRIC metricId) const noexcept
    {
        auto it = caps_.find(metricId);
        if (it == caps_.end()) {
            return 0; // not present / not available
        }
        return it->second;
    }
}
