#include "MetricCapabilities.h"
#include "metadata/MetricList.h"
#include <sstream>
#include <type_traits>

namespace pmon::ipc
{
    namespace
    {
        const char* GetMetricSymbol_(PM_METRIC metricId) noexcept
        {
            switch (metricId) {
#define X_(id_, ...) case id_: return #id_;
                METRIC_LIST(X_)
#undef X_
            default:
                return "PM_METRIC_UNKNOWN";
            }
        }

        const char* GetAvailabilitySymbol_(PM_METRIC_AVAILABILITY availability) noexcept
        {
            switch (availability) {
            case PM_METRIC_AVAILABILITY_AVAILABLE:
                return "AVAILABLE";
            case PM_METRIC_AVAILABILITY_UNAVAILABLE:
                return "UNAVAILABLE";
            case PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE:
                return "NOT_EXPORTED_BY_SOURCE";
            case PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE:
                return "NOT_SUPPORTED_BY_DEVICE";
            case PM_METRIC_AVAILABILITY_NOT_IMPLEMENTED_BY_PRESENTMON:
                return "NOT_IMPLEMENTED_BY_PRESENTMON";
            default:
                return "UNKNOWN";
            }
        }
    }

    void MetricCapabilities::Set(PM_METRIC metricId, size_t arraySize, PM_METRIC_AVAILABILITY availability)
    {
        if (availability == PM_METRIC_AVAILABILITY_AVAILABLE && arraySize == 0) {
            return;
        }

        caps_[metricId] = MetricCapabilityEntry{ .arraySize = arraySize, .availability = availability };
    }

    void MetricCapabilities::Merge(const MetricCapabilities& capsToMerge)
    {
        for (const auto& kv : capsToMerge.caps_) {
            const auto metricId = kv.first;
            const auto& entry = kv.second;
            if (entry.availability == PM_METRIC_AVAILABILITY_AVAILABLE) {
                Set(metricId, entry.arraySize, entry.availability);
            }
            else if (!caps_.contains(metricId)) {
                Set(metricId, entry.arraySize, entry.availability);
            }
            else if (caps_[metricId].availability != PM_METRIC_AVAILABILITY_AVAILABLE) {
                caps_[metricId] = entry;
            }
        }
    }

    size_t MetricCapabilities::Check(PM_METRIC metricId) const noexcept
    {
        auto it = caps_.find(metricId);
        if (it == caps_.end()) {
            return 0;
        }
        if (it->second.availability != PM_METRIC_AVAILABILITY_AVAILABLE) {
            return 0;
        }
        return it->second.arraySize;
    }

    std::optional<MetricCapabilityEntry> MetricCapabilities::Lookup(PM_METRIC metricId) const noexcept
    {
        auto it = caps_.find(metricId);
        if (it == caps_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::string MetricCapabilities::ToString(size_t indentSpaces) const
    {
        std::ostringstream oss;
        const std::string indent(indentSpaces, ' ');
        bool first = true;
        for (const auto& kv : caps_) {
            if (!first) {
                oss << "\r\n";
                oss << indent;
            }
            first = false;
            oss << "metric=" << GetMetricSymbol_(kv.first)
                << " metricId=" << static_cast<std::underlying_type_t<PM_METRIC>>(kv.first)
                << " arraySize=" << kv.second.arraySize
                << " availability=" << GetAvailabilitySymbol_(kv.second.availability);
        }
        return oss.str();
    }
}
