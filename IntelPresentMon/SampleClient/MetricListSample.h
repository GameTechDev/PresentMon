#pragma once
#include <fstream>
#include <format>
#include <ranges>
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../Core/source/pmon/RawFrameDataMetricList.h"

int MetricListSample(pmapi::Session& session)
{
    namespace rn = std::ranges;
    namespace vi = rn::views;

    std::ofstream out{ "metrics.md" };

    // header
    out << "| Metric | CSV Column | Description | Compatible Query Types |\n"
        << "| - | - | - |:-:|\n";

    // get list of metrics used in CSV (as vector of query element definitions)
    const auto csvElements = p2c::pmon::GetRawFrameDataMetricList(0, false);

    // Loop through ALL PresentMon metrics
    for (auto metric : session.GetIntrospectionRoot()->GetMetrics()) {
        // get CSV column name if this metric is used in CSV capture file
        std::string csvColumnName;
        if (rn::contains(csvElements, metric.GetId(), &p2c::pmon::RawFrameQueryElementDefinition::metricId)) {
            csvColumnName = metric.Introspect().GetName()
                | vi::filter([](char c) { return c != ' '; })
                | rn::to<std::basic_string>();
        }
        // name and description
        out << std::format("|{}|{}|{}|",
            metric.Introspect().GetName(),
            csvColumnName,
            metric.Introspect().GetDescription()
        );
        // query compatibility codes
        switch (metric.GetType()) {
        case PM_METRIC_TYPE_DYNAMIC: out << "D|\n"; break;
        case PM_METRIC_TYPE_DYNAMIC_FRAME: out << "DF|\n"; break;
        case PM_METRIC_TYPE_FRAME_EVENT: out << "F|\n"; break;
        case PM_METRIC_TYPE_STATIC: out << "DS|\n"; break;
        default: out << "|\n";
        }
    }

    // footer
    out << "*Query Type Codes: **D** = Dynamic Query, **F** = Frame Event Query, **S** = Static Query\n";

    return 0;
}
