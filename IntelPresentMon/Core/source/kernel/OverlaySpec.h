// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/WinAPI.h>
#include <vector>
#include <Core/source/gfx/layout/Enums.h>
#include <Core/source/gfx/base/Geometry.h>
#include <Core/source/gfx/layout/style/Stylesheet.h>
#include <variant>


namespace p2c::kern
{
    enum class WidgetType
    {
        Graph,
        Readout,
    };

    struct QualifiedMetric
    {
        int32_t metricId;
        int32_t statId;
        uint32_t arrayIndex;
        uint32_t deviceId;
        int32_t unitId;
        bool operator==(const QualifiedMetric& rhs) const
        {
            return metricId == rhs.metricId &&
                statId == rhs.statId &&
                arrayIndex == rhs.arrayIndex &&
                deviceId == rhs.deviceId;
            // TODO: cosider what to do about unitId here
        }
        std::string Dump() const
        {
            return std::format("QMet> metId:{} staId:{} arrIdx:{} devId:{} untId:{}",
                metricId, statId, arrayIndex, deviceId, unitId);
        }
    };

    struct GraphMetricSpec
    {
        QualifiedMetric metric;
        gfx::lay::AxisAffinity axisAffinity;
    };

    struct GraphSpec
    {
        std::vector<GraphMetricSpec> metrics;
        gfx::lay::GraphType type;
        std::string tag;
    };

    struct ReadoutSpec
    {
        QualifiedMetric metric;
        std::string tag;
    };

    struct OverlaySpec
    {
        // types
        enum class OverlayPosition
        {
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight,
            Center,
        };
        // members
        DWORD pid;
        std::wstring capturePath;
        std::wstring captureName = L"pmcap";
        std::optional<std::wstring> captureFullPathOverride;
        double graphDataWindowSize;
        double averagingWindowSize;
        double metricsOffset;
        double etwFlushPeriod;
        bool manualEtwFlush;
        OverlayPosition overlayPosition;
        std::vector<std::variant<GraphSpec, ReadoutSpec>> widgets;
        int overlayWidth;
        bool upscale;
        float upscaleFactor;
        uint64_t metricPollRate = 10;
        uint64_t overlayDrawRate = 10;
        uint32_t telemetrySamplingPeriodMs;
        bool hideDuringCapture;
        bool hideAlways;
        bool independentKernelWindow;
        bool generateStats;
        bool enableFlashInjection;
        std::vector<std::shared_ptr<gfx::lay::sty::Stylesheet>> sheets;
    };
}