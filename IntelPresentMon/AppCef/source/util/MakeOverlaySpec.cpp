// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MakeOverlaySpec.h"
#include <Core/source/gfx/layout/style/RawAttributeHelpers.h>
#include "CefValues.h"

namespace p2c::client::util
{
    using namespace gfx;
    using namespace lay;

    namespace
    {
        Color ColorFromV8(const CefRefPtr<CefValue>& rgba)
        {
            return Color::FromBytes(
                Traverse(rgba)["r"],
                Traverse(rgba)["g"],
                Traverse(rgba)["b"]
            ).WithAlpha(Traverse(rgba)["a"]);
        }
    }

    std::unique_ptr<kern::OverlaySpec> MakeOverlaySpec(CefRefPtr<CefValue> vSpec)
    {
        auto traversedSpec = Traverse(vSpec);
        auto traversedPref = traversedSpec["preferences"];

        auto pSpec = std::make_unique<kern::OverlaySpec>(kern::OverlaySpec{
            .pid = traversedSpec["pid"],
            .capturePath = traversedPref["capturePath"].AsWString(),
            .graphDataWindowSize = traversedPref["timeRange"],
            .averagingWindowSize = traversedPref["metricsWindow"],
            .metricsOffset = traversedPref["metricsOffset"],
            .overlayPosition = (kern::OverlaySpec::OverlayPosition)traversedPref["overlayPosition"],
            .overlayWidth = traversedPref["overlayWidth"],
            .upscale = traversedPref["upscale"],
            .upscaleFactor = traversedPref["upscaleFactor"],
            .samplingPeriodMs = traversedPref["samplingPeriodMs"],
            .samplesPerFrame = traversedPref["samplesPerFrame"],
            .telemetrySamplingPeriodMs = traversedPref["telemetrySamplingPeriodMs"],
            .hideDuringCapture = traversedPref["hideDuringCapture"],
            .hideAlways = traversedPref["hideAlways"],
            .independentKernelWindow = traversedPref["independentWindow"],
            .generateStats = traversedPref["generateStats"],
        });

        // style sheets
        {
            using namespace gfx::lay::sty;
            std::vector<std::shared_ptr<Stylesheet>> sheets{ Stylesheet::MakeDefaultInherit() };

            const auto gutterSize = 30.;
            const auto gutterPadding = 4.;

            const double graphPadding = traversedPref["graphPadding"];
            const double graphBorder = traversedPref["graphBorder"];
            const double graphMargin = traversedPref["graphMargin"];

            const double overlayPadding = traversedPref["overlayPadding"];
            const double overlayBorder = traversedPref["overlayBorder"];
            const double overlayMargin = traversedPref["overlayMargin"];
            const auto overlayBorderColor = at::make::Color(ColorFromV8(traversedPref["overlayBorderColor"]));

            sheets.push_back(Stylesheet::Make({ {}, {"doc"} }));
            sheets.back()->InsertRaw<at::backgroundColor>(at::make::Color(ColorFromV8(traversedPref["overlayBackgroundColor"])));
            sheets.back()->InsertRaw<at::flexDirection>(at::make::Enum(gfx::lay::FlexDirection::Column));
            sheets.back()->InsertRaw<at::paddingLeft>(overlayPadding);
            sheets.back()->InsertRaw<at::paddingTop>(overlayPadding);
            sheets.back()->InsertRaw<at::paddingRight>(overlayPadding);
            sheets.back()->InsertRaw<at::paddingBottom>(overlayPadding);
            sheets.back()->InsertRaw<at::marginLeft>(overlayMargin);
            sheets.back()->InsertRaw<at::marginTop>(overlayMargin);
            sheets.back()->InsertRaw<at::marginRight>(overlayMargin);
            sheets.back()->InsertRaw<at::marginBottom>(overlayMargin);
            sheets.back()->InsertRaw<at::borderLeft>(overlayBorder);
            sheets.back()->InsertRaw<at::borderTop>(overlayBorder);
            sheets.back()->InsertRaw<at::borderRight>(overlayBorder);
            sheets.back()->InsertRaw<at::borderBottom>(overlayBorder);
            sheets.back()->InsertRaw<at::borderColorLeft>(overlayBorderColor);
            sheets.back()->InsertRaw<at::borderColorTop>(overlayBorderColor);
            sheets.back()->InsertRaw<at::borderColorRight>(overlayBorderColor);
            sheets.back()->InsertRaw<at::borderColorBottom>(overlayBorderColor);


            // graphs
            sheets.push_back(Stylesheet::Make({ {}, {"$graph"} }));
            // stretch alignment should make this unnecessary
            sheets.back()->InsertRaw<at::width>(1920.);
            sheets.back()->InsertRaw<at::flexDirection>(at::make::Enum(gfx::lay::FlexDirection::Column));
            sheets.back()->InsertRaw<at::flexAlignment>(at::make::Enum(gfx::lay::FlexAlignment::Stretch));
            sheets.back()->InsertRaw<at::paddingLeft>(graphPadding);
            sheets.back()->InsertRaw<at::paddingTop>(graphPadding);
            sheets.back()->InsertRaw<at::paddingRight>(graphPadding);
            sheets.back()->InsertRaw<at::paddingBottom>(graphPadding);
            sheets.back()->InsertRaw<at::marginLeft>(graphMargin);
            sheets.back()->InsertRaw<at::marginTop>(graphMargin);
            sheets.back()->InsertRaw<at::marginRight>(graphMargin);
            sheets.back()->InsertRaw<at::marginBottom>(graphMargin);
            sheets.back()->InsertRaw<at::borderLeft>(graphBorder);
            sheets.back()->InsertRaw<at::borderTop>(graphBorder);
            sheets.back()->InsertRaw<at::borderRight>(graphBorder);
            sheets.back()->InsertRaw<at::borderBottom>(graphBorder);
            sheets.back()->InsertRaw<at::textFont>(traversedPref["graphFont"]["name"].AsWString());
            sheets.back()->InsertRaw<at::graphLineColor>(at::make::Color(Color::FromBytes(100, 255, 255, 220)));
            sheets.back()->InsertRaw<at::graphFillColor>(at::make::Color(Color::FromBytes(57, 210, 210, 25)));
            // TODO: clarify, consider which plot attributes apply to which sub-elements
            sheets.back()->InsertRaw<at::graphTimeWindow>((double)traversedPref["timeRange"]);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$label-units"} }));
            sheets.back()->InsertRaw<at::marginLeft>(3.);
            sheets.back()->InsertRaw<at::marginRight>(3.);

            sheets.push_back(Stylesheet::Make({ {"$label-wrap-left"}, {"$label-value"} }));
            sheets.back()->InsertRaw<at::marginLeft>(5.);
            sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(prim::Justification::Right));

            sheets.push_back(Stylesheet::Make({ {"$label-wrap-right"}, {"$label-value"} }));
            sheets.back()->InsertRaw<at::marginLeft>(5.);
            sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(prim::Justification::Right));

            sheets.push_back(Stylesheet::Make({ {"$label-wrap-left"}, {"$label-swatch"} }));
            sheets.back()->InsertRaw<at::marginTop>(3.);
            sheets.back()->InsertRaw<at::marginRight>(3.);
            sheets.back()->InsertRaw<at::width>(7.);
            sheets.back()->InsertRaw<at::height>(7.);

            sheets.push_back(Stylesheet::Make({ {"$label-wrap-right"}, {"$label-swatch"} }));
            sheets.back()->InsertRaw<at::marginTop>(3.);
            sheets.back()->InsertRaw<at::marginLeft>(3.);
            sheets.back()->InsertRaw<at::width>(7.);
            sheets.back()->InsertRaw<at::height>(7.);

            sheets.push_back(Stylesheet::Make({ {}, {"$label-wrap-right"} }));
            sheets.back()->InsertRaw<at::flexJustification>(at::make::Enum(FlexJustification::End));

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$axis"} }));
            sheets.back()->InsertRaw<at::textSize>((double)traversedPref["graphFont"]["axisSize"]);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$body"} }));
            sheets.back()->InsertRaw<at::flexDirection>(at::make::Enum(gfx::lay::FlexDirection::Row));
            sheets.back()->InsertRaw<at::flexAlignment>(at::make::Enum(gfx::lay::FlexAlignment::Stretch));
            sheets.back()->InsertRaw<at::flexGrow>(1.);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$vert-axis"} }));
            sheets.back()->InsertRaw<at::width>(gutterSize);
            sheets.back()->InsertRaw<at::paddingRight>(gutterPadding);
            sheets.back()->InsertRaw<at::flexDirection>(at::make::Enum(gfx::lay::FlexDirection::Column));
            sheets.back()->InsertRaw<at::flexAlignment>(at::make::Enum(gfx::lay::FlexAlignment::Stretch));
            sheets.back()->InsertRaw<at::flexJustification>(at::make::Enum(gfx::lay::FlexJustification::Between));

            sheets.push_back(Stylesheet::Make({ {"$body-left"}, {"$y-axis"} }));
            sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(gfx::prim::Justification::Right));

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$body-plot"} }));
            sheets.back()->InsertRaw<at::borderLeft>(1.);
            sheets.back()->InsertRaw<at::borderTop>(1.);
            sheets.back()->InsertRaw<at::borderRight>(1.);
            sheets.back()->InsertRaw<at::borderBottom>(1.);
            sheets.back()->InsertRaw<at::marginLeft>(2.);
            sheets.back()->InsertRaw<at::marginTop>(2.);
            sheets.back()->InsertRaw<at::marginRight>(2.);
            sheets.back()->InsertRaw<at::marginBottom>(2.);
            sheets.back()->InsertRaw<at::flexGrow>(1.);

            // don't show right side value display for histogram (keep spacing) 
            sheets.push_back(Stylesheet::Make({ { "$graph", "$hist" }, {"$body-right"} }));
            sheets.back()->InsertRaw<at::display>(at::make::Enum(Display::Invisible));

            sheets.push_back(Stylesheet::Make({ {"$body-right"}, {"$y-axis"} }));
            sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(gfx::prim::Justification::Left));

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$footer"} }));
            sheets.back()->InsertRaw<at::marginTop>(3.);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$footer-left"} }));
            sheets.back()->InsertRaw<at::width>(gutterSize);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$footer-right"} }));
            sheets.back()->InsertRaw<at::width>(gutterSize);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$footer-center"} }));
            sheets.back()->InsertRaw<at::flexGrow>(1.);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$footer-center-left"} }));
            sheets.back()->InsertRaw<at::width>(24.);

            sheets.push_back(Stylesheet::Make({ {"$graph"}, {"$footer-center-right"} }));
            sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(gfx::prim::Justification::Right));
            sheets.back()->InsertRaw<at::flexGrow>(1.);

            // Readout container
            {
                const auto padding = 3.;
                const auto textSize = 12.;
                const auto backgroundColor = at::make::Color(gfx::Color::FromBytes(45, 50, 96, 100));
                const auto textColor = at::make::Color(gfx::Color::FromBytes(205, 211, 233));

                sheets.push_back(Stylesheet::Make({ {}, {"readout-container"} }));
                // stretch alignment should make this unnecessary
                sheets.back()->InsertRaw<at::width>(1920.);
                sheets.back()->InsertRaw<at::marginLeft>(6.);
                sheets.back()->InsertRaw<at::marginTop>(0.);
                sheets.back()->InsertRaw<at::marginRight>(12.);
                sheets.back()->InsertRaw<at::marginBottom>(0.);
                sheets.back()->InsertRaw<at::paddingLeft>(padding + 6.);
                sheets.back()->InsertRaw<at::paddingTop>(padding);
                sheets.back()->InsertRaw<at::paddingRight>(padding);
                sheets.back()->InsertRaw<at::paddingBottom>(padding);
                sheets.back()->InsertRaw<at::flexDirection>(at::make::Enum(gfx::lay::FlexDirection::Column));
                sheets.back()->InsertRaw<at::flexAlignment>(at::make::Enum(gfx::lay::FlexAlignment::Stretch));
                sheets.back()->InsertRaw<at::backgroundColor>(at::make::Color(gfx::Color::FromBytes(0, 0, 0, 0)));
                sheets.back()->InsertRaw<at::textColor>(textColor);
                sheets.back()->InsertRaw<at::textSize>(textSize);
            }

            // Readouts
            {
                const auto margin = 0.;
                const auto padding = 2.;
                const auto textSize = 12.;

                sheets.push_back(Stylesheet::Make({ {}, {"$readout"} }));
                sheets.back()->InsertRaw<at::marginLeft>(margin);
                sheets.back()->InsertRaw<at::marginTop>(margin);
                sheets.back()->InsertRaw<at::marginRight>(margin);
                sheets.back()->InsertRaw<at::marginBottom>(margin);
                sheets.back()->InsertRaw<at::paddingLeft>(padding);
                sheets.back()->InsertRaw<at::paddingTop>(padding);
                sheets.back()->InsertRaw<at::paddingRight>(padding);
                sheets.back()->InsertRaw<at::paddingBottom>(padding);

                sheets.push_back(Stylesheet::Make({ {"$readout"}, {"$label"} }));
                sheets.back()->InsertRaw<at::paddingRight>(20.);

                sheets.push_back(Stylesheet::Make({ {"$readout"}, {"$numeric-value"} }));
                sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(prim::Justification::Right));

                sheets.push_back(Stylesheet::Make({ {"$readout"}, {"$text-value"} }));
                sheets.back()->InsertRaw<at::flexGrow>(1.);
                sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(prim::Justification::Left));

                sheets.push_back(Stylesheet::Make({ {"$readout"}, {"$numeric-units"} }));
                sheets.back()->InsertRaw<at::textJustification>(at::make::Enum(prim::Justification::Left));

                // sheets.push_back(Stylesheet::Make({ {"$readout"}, {"$text-value"} }));
            }

            pSpec->sheets = std::move(sheets);
        }

        // populate widgets into spec file
        {         
            auto widgets = traversedSpec["widgets"].AsCefValue();
            const auto widgetCount = Traverse(widgets).GetArrayLength();
            for (size_t i = 0; i < widgetCount; i++)
            {
                auto widget = Traverse(widgets)[i].AsCefValue();
                const auto widgetType = kern::WidgetType(Traverse(widget)["widgetType"]);
                if (widgetType == kern::WidgetType::Graph) {
                    auto& vGraph = widget;
                    auto widgetMetrics = Traverse(vGraph)["metrics"];
                    const auto tag = std::format("g{}", i);
                    auto type = lay::EnumRegistry<GraphType>::ToEnum(Traverse(vGraph)["graphType"]["name"].AsWString());
                    // create the metric specs for each line etc. in widget
                    std::vector<kern::GraphMetricSpec> graphMetricSpecs;
                    {
                        const auto metricIds = widgetMetrics.PluckAs<size_t>("metricId");
                        const auto metricAxisAffinities = widgetMetrics.PluckAs<gfx::lay::AxisAffinity>("axisAffinity");
                        for (size_t i = 0; i < metricIds.size(); i++) {
                            graphMetricSpecs.push_back(kern::GraphMetricSpec{
                                .index = metricIds[i],
                                .axisAffinity = metricAxisAffinities[i],
                            });
                        }
                    }
                    pSpec->widgets.push_back(kern::GraphSpec{
                        .metrics = std::move(graphMetricSpecs),
                        .type = type, .tag = tag,
                    });

                    {
                        using namespace gfx::lay::sty;
                        auto& sheets = pSpec->sheets;

                        const auto gridColor = at::make::Color(ColorFromV8(Traverse(vGraph)["gridColor"]));
                        const auto backgroundColor = at::make::Color(ColorFromV8(Traverse(vGraph)["backgroundColor"]));
                        const auto borderColor = at::make::Color(ColorFromV8(Traverse(vGraph)["borderColor"]));

                        sheets.push_back(Stylesheet::Make({ {}, { "$graph", tag } }));
                        // TODO: consider renaming range to rangeLeft
                        sheets.back()->InsertRaw<at::graphMinValueLeft>((double)Traverse(vGraph)["graphType"]["range"][(size_t)0]);
                        sheets.back()->InsertRaw<at::graphMaxValueLeft>((double)Traverse(vGraph)["graphType"]["range"][1]);
                        sheets.back()->InsertRaw<at::graphMinValueRight>((double)Traverse(vGraph)["graphType"]["rangeRight"][(size_t)0]);
                        sheets.back()->InsertRaw<at::graphMaxValueRight>((double)Traverse(vGraph)["graphType"]["rangeRight"][1]);
                        sheets.back()->InsertRaw<at::graphMinCount>((double)Traverse(vGraph)["graphType"]["countRange"][(size_t)0]);
                        sheets.back()->InsertRaw<at::graphMaxCount>((double)Traverse(vGraph)["graphType"]["countRange"][1]);
                        sheets.back()->InsertRaw<at::backgroundColor>(backgroundColor);
                        sheets.push_back(Stylesheet::Make({ {}, {"$graph"} }));
                        sheets.back()->InsertRaw<at::textColor>(at::make::Color(ColorFromV8(Traverse(vGraph)["textColor"])));
                        // since border is 0px, these settings do nothing
                        // sheets.back()->InsertRaw<at::borderColorLeft>(borderColor);
                        // sheets.back()->InsertRaw<at::borderColorTop>(borderColor);
                        // sheets.back()->InsertRaw<at::borderColorRight>(borderColor);
                        // sheets.back()->InsertRaw<at::borderColorBottom>(borderColor);
                        if (Traverse(vGraph)["graphType"]["autoLeft"]) {
                            sheets.back()->InsertRaw<at::graphAutoscaleLeft>(true);
                        }
                        if (Traverse(vGraph)["graphType"]["autoRight"]) {
                            sheets.back()->InsertRaw<at::graphAutoscaleRight>(true);
                        }
                        if (Traverse(vGraph)["graphType"]["autoCount"]) {
                            sheets.back()->InsertRaw<at::graphAutoscaleCount>(true);
                        }

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)Traverse(vGraph)["textSize"]);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label-units"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)Traverse(vGraph)["textSize"]);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label-value"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)Traverse(vGraph)["textSize"]);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label-value"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)Traverse(vGraph)["textSize"]);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$body"} }));
                        sheets.back()->InsertRaw<at::height>((double)Traverse(vGraph)["height"]);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$body-plot"} }));
                        sheets.back()->InsertRaw<at::borderColorLeft>(gridColor);
                        sheets.back()->InsertRaw<at::borderColorTop>(gridColor);
                        sheets.back()->InsertRaw<at::borderColorRight>(gridColor);
                        sheets.back()->InsertRaw<at::borderColorBottom>(gridColor);
                        sheets.back()->InsertRaw<at::graphGridColor>(gridColor);
                        sheets.back()->InsertRaw<at::graphVerticalDivs>((double)Traverse(vGraph)["vDivs"]);
                        sheets.back()->InsertRaw<at::graphHorizontalDivs>((double)Traverse(vGraph)["hDivs"]);
                        sheets.back()->InsertRaw<at::graphBinCount>((double)Traverse(vGraph)["graphType"]["binCount"]);

                        sheets.push_back(Stylesheet::Make({ { "$graph", "$line", tag }, {"$body-plot"} }));
                        sheets.back()->InsertRaw<at::borderColorRight>(at::make::Color(ColorFromV8(Traverse(vGraph)["dividerColor"])));

                        sheets.push_back(Stylesheet::Make({ { "$graph", "$line", tag }, {"$body-right", "$value"}}));
                        sheets.back()->InsertRaw<at::textColor>(at::make::Color(ColorFromV8(widgetMetrics[size_t(0)]["lineColor"])));

                        const auto widgetMetricCount = widgetMetrics.GetArrayLength();
                        bool hasRightAxis = false;
                        for (size_t iWidgetMetric = 0; iWidgetMetric < widgetMetricCount; iWidgetMetric++) {
                            auto widgetMetric = widgetMetrics[iWidgetMetric];
                            const auto axisAffinity = (AxisAffinity)widgetMetric["axisAffinity"];
                            hasRightAxis = hasRightAxis || axisAffinity == AxisAffinity::Right;
                            
                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$metric", std::format("$metric-{}", iWidgetMetric)}}));
                            sheets.back()->InsertRaw<at::graphLineColor>(
                                at::make::Color(ColorFromV8(widgetMetric["lineColor"]))
                            );
                            sheets.back()->InsertRaw<at::graphFillColor>(
                                at::make::Color(ColorFromV8(widgetMetric["fillColor"]))
                            );
                            sheets.back()->InsertRaw<at::backgroundColor>(
                                at::make::Color(ColorFromV8(widgetMetric["lineColor"]))
                            );

                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$label-swatch", std::format("$metric-{}", iWidgetMetric)} }));
                            sheets.back()->InsertRaw<at::graphLineColor>(
                                at::make::Color(ColorFromV8(widgetMetric["lineColor"]))
                            );
                            sheets.back()->InsertRaw<at::graphFillColor>(
                                at::make::Color(ColorFromV8(widgetMetric["fillColor"]))
                            );
                            sheets.back()->InsertRaw<at::backgroundColor>(
                                at::make::Color(ColorFromV8(widgetMetric["lineColor"]))
                            );

                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$label-value", std::format("$metric-{}", iWidgetMetric)}}));
                            sheets.back()->InsertRaw<at::textColor>(
                                at::make::Color(ColorFromV8(widgetMetric["lineColor"]))
                            );

                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$label-units", std::format("$metric-{}", iWidgetMetric)} }));
                            sheets.back()->InsertRaw<at::textColor>(
                                // darken line color for units
                                at::make::Color(ColorFromV8(widgetMetric["lineColor"]) * 0.7f)
                            );
                        }

                        // multi-line right axis hiding
                        if (!hasRightAxis) {
                            sheets.push_back(Stylesheet::Make({ {"$graph", tag }, {"$body-right", "$vert-axis"} }));
                            sheets.back()->InsertRaw<at::display>(at::make::Enum(gfx::lay::Display::Invisible));
                        }
                    }
                }
                else if (widgetType == kern::WidgetType::Readout) {
                    using namespace gfx::lay::sty;
                    auto& vReadout = widget;
                    auto& sheets = pSpec->sheets;
                    const auto tag = std::format("r{}", i);
                    if (Traverse(vReadout)["metrics"].GetArrayLength() > 1) {
                        p2clog.warn(L"Too many metricIds for readout widget").commit();
                    }
                    pSpec->widgets.push_back(kern::ReadoutSpec{ Traverse(vReadout)["metrics"][size_t(0)]["metricId"], { tag } });

                    const double fontSize = Traverse(vReadout)["fontSize"];


                    sheets.push_back(Stylesheet::Make({ {}, { "$readout", tag } }));
                    sheets.back()->InsertRaw<at::backgroundColor>(at::make::Color(ColorFromV8(Traverse(vReadout)["backgroundColor"])));

                    if (!Traverse(vReadout)["showLabel"]) {
                        sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$label" } }));
                        sheets.back()->InsertRaw<at::display>(at::make::Enum(Display::None));
                    }

                    sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$text-large" } }));
                    sheets.back()->InsertRaw<at::textColor>(at::make::Color(ColorFromV8(Traverse(vReadout)["fontColor"])));
                    sheets.back()->InsertRaw<at::textSize>(fontSize);

                    sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$text-large" } }));
                    sheets.back()->InsertRaw<at::textColor>(at::make::Color(ColorFromV8(Traverse(vReadout)["fontColor"])));
                    sheets.back()->InsertRaw<at::textSize>(fontSize);

                    sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$numeric-units" } }));
                    sheets.back()->InsertRaw<at::textColor>(at::make::Color(ColorFromV8(Traverse(vReadout)["fontColor"])));
                    sheets.back()->InsertRaw<at::textSize>(fontSize);
                    sheets.back()->InsertRaw<at::paddingLeft>(fontSize / 5.);
                }
                else {
                    p2clog.warn(std::format(L"Unknown widget type [{}]", (int)widgetType)).commit();
                }
            }
        }

        // capture state indicator
        {
            using namespace gfx::lay::sty;
            auto& sheets = pSpec->sheets;

            const auto textColor = at::make::Color(gfx::Color::FromBytes(190, 200, 233));

            sheets.push_back(Stylesheet::Make({ {}, {"cap"} }));
            sheets.back()->InsertRaw<at::marginLeft>(10.);
            sheets.back()->InsertRaw<at::textColor>(textColor);

            sheets.push_back(Stylesheet::Make({ {"cap"}, {"label"}}));
            sheets.back()->InsertRaw<at::marginRight>(3.);
        }

        return pSpec;
    }
}
