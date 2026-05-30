// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MakeOverlaySpec.h"
#include "../Core/source/gfx/layout/style/RawAttributeHelpers.h"
#include "../Core/source/gfx/layout/style/Stylesheet.h"
#include "../Core/source/gfx/prim/Enums.h"
#include "../Core/source/kernel/OverlaySpec.h"
#include "../Core/source/gfx/base/Geometry.h"
#include "kact/PushSpecification.h"
#include "../CommonUtilities/str/String.h"

namespace kproc
{
    namespace vi = std::views;
    using namespace p2c;
    using namespace gfx;
    using namespace lay;
    using ::pmon::util::str::ToNarrow;
    using ::pmon::util::str::ToWide;

    std::unique_ptr<p2c::kern::OverlaySpec> MakeOverlaySpec(const kact::push_spec_impl::Params& spec)
    {
        // Alias for brevity.
        auto& pref = spec.preferences;

        // Build the core OverlaySpec with fields converted from the new input.
        auto pSpec = std::make_unique<kern::OverlaySpec>(kern::OverlaySpec{
            .pid = *spec.pid, // Assuming pid is set based on behavior of kact::PushSpecification
            .capturePath = ToWide(pref.capturePath),
            .graphDataWindowSize = double(pref.timeRange),
            .averagingWindowSize = double(pref.metricsWindow),
            .metricsOffset = double(pref.metricsOffset),
            .etwFlushPeriod = double(pref.etwFlushPeriod),
            .manualEtwFlush = pref.manualEtwFlush,
            .overlayPosition = pref.overlayPosition,
            .overlayWidth = int(pref.overlayWidth),
            .upscale = pref.upscale,
            .upscaleFactor = pref.upscaleFactor,
            .metricPollRate = pref.metricPollRate,
            .overlayDrawRate = pref.overlayDrawRate,
            .telemetrySamplingPeriodMs = pref.telemetrySamplingPeriodMs,
            .hideDuringCapture = pref.hideDuringCapture,
            .hideAlways = pref.hideAlways,
            .independentKernelWindow = pref.independentWindow,
            .generateStats = pref.generateStats,
            .enableFlashInjection = pref.enableFlashInjection,
        });
        if (pref.adapterId && *pref.adapterId > 0) {
            pSpec->frameQueryAdapterId = uint32_t(*pref.adapterId);
        }

        // style sheets
        {
            using namespace gfx::lay::sty;
            std::vector<std::shared_ptr<Stylesheet>> sheets{ Stylesheet::MakeDefaultInherit() };

            const auto gutterSize = 30.;
            const auto gutterPadding = 4.;

            const double graphPadding = pref.graphPadding;
            const double graphBorder = pref.graphBorder;
            const double graphMargin = pref.graphMargin;

            const double overlayPadding = pref.overlayPadding;
            const double overlayBorder = pref.overlayBorder;
            const double overlayMargin = pref.overlayMargin;
            const auto overlayBorderColor = at::make::Color(pref.overlayBorderColor);
            const auto overlayBackgroundColor = at::make::Color(pref.overlayBackgroundColor);

            sheets.push_back(Stylesheet::Make({ {}, {"doc"} }));
            sheets.back()->InsertRaw<at::backgroundColor>(overlayBackgroundColor);
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
            sheets.back()->InsertRaw<at::textFont>(ToWide(pref.graphFont.name));
            sheets.back()->InsertRaw<at::graphLineColor>(at::make::Color(Color::FromBytes(100, 255, 255, 220)));
            sheets.back()->InsertRaw<at::graphFillColor>(at::make::Color(Color::FromBytes(57, 210, 210, 25)));
            // TODO: clarify, consider which plot attributes apply to which sub-elements
            sheets.back()->InsertRaw<at::graphTimeWindow>((double)pref.timeRange);

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
            sheets.back()->InsertRaw<at::textSize>((double)pref.graphFont.axisSize);

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
            sheets.back()->InsertRaw<at::width>(48.);

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
            for (auto&& [i,widget] : spec.widgets | vi::enumerate) {
                if (auto pGraph = std::get_if<kact::push_spec_impl::Graph>(&widget)) {
                    const auto tag = std::format("g{}", i);
                    auto& graph = *pGraph;

                    {
                        // create the metric specs for each line etc. in widget
                        std::vector<kern::GraphMetricSpec> graphMetricSpecs;
                        for (auto& widgetMetric : graph.metrics) {
                            auto& qualifiedMetric = widgetMetric.metric;
                            graphMetricSpecs.push_back(kern::GraphMetricSpec{
                                .metric = {
                                    .metricId = qualifiedMetric.metricId,
                                    .statId = qualifiedMetric.statId,
                                    .arrayIndex = qualifiedMetric.arrayIndex,
                                    .deviceId = qualifiedMetric.deviceId,
                                    .unitId = qualifiedMetric.desiredUnitId,
                                },
                                .axisAffinity = widgetMetric.axisAffinity,
                            });
                        }
                        pSpec->widgets.push_back(kern::GraphSpec{
                            .metrics = std::move(graphMetricSpecs),
                            .type = lay::EnumRegistry<GraphType>::ToEnum(ToWide(graph.graphType.name)),
                            .tag = tag,
                         });
                    }

                    {
                        using namespace gfx::lay::sty;
                        auto& sheets = pSpec->sheets;

                        const auto gridColor = at::make::Color(graph.gridColor);
                        const auto backgroundColor = at::make::Color(graph.backgroundColor);
                        const auto borderColor = at::make::Color(graph.borderColor);
                        const auto textColor = at::make::Color(graph.textColor);
                        const auto dividerColor = at::make::Color(graph.dividerColor);


                        sheets.push_back(Stylesheet::Make({ {}, { "$graph", tag } }));
                        // TODO: consider renaming range to rangeLeft
                        sheets.back()->InsertRaw<at::graphMinValueLeft>((double)graph.graphType.range[0]);
                        sheets.back()->InsertRaw<at::graphMaxValueLeft>((double)graph.graphType.range[1]);
                        sheets.back()->InsertRaw<at::graphMinValueRight>((double)graph.graphType.rangeRight[0]);
                        sheets.back()->InsertRaw<at::graphMaxValueRight>((double)graph.graphType.rangeRight[1]);
                        sheets.back()->InsertRaw<at::graphMinCount>((double)graph.graphType.countRange[0]);
                        sheets.back()->InsertRaw<at::graphMaxCount>((double)graph.graphType.countRange[1]);
                        sheets.back()->InsertRaw<at::backgroundColor>(backgroundColor);
                        sheets.back()->InsertRaw<at::textColor>(textColor);
                        // since border is 0px, these settings do nothing
                        // sheets.back()->InsertRaw<at::borderColorLeft>(borderColor);
                        // sheets.back()->InsertRaw<at::borderColorTop>(borderColor);
                        // sheets.back()->InsertRaw<at::borderColorRight>(borderColor);
                        // sheets.back()->InsertRaw<at::borderColorBottom>(borderColor);
                        if (graph.graphType.autoLeft) {
                            sheets.back()->InsertRaw<at::graphAutoscaleLeft>(true);
                        }
                        if (graph.graphType.autoRight) {
                            sheets.back()->InsertRaw<at::graphAutoscaleRight>(true);
                        }
                        if (graph.graphType.autoCount) {
                            sheets.back()->InsertRaw<at::graphAutoscaleCount>(true);
                        }

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)graph.textSize);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label-units"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)graph.textSize);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label-value"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)graph.textSize);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$label-value"} }));
                        sheets.back()->InsertRaw<at::textSize>((double)graph.textSize);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$body"} }));
                        sheets.back()->InsertRaw<at::height>((double)graph.height);

                        sheets.push_back(Stylesheet::Make({ { "$graph", tag }, {"$body-plot"} }));
                        sheets.back()->InsertRaw<at::borderColorLeft>(gridColor);
                        sheets.back()->InsertRaw<at::borderColorTop>(gridColor);
                        sheets.back()->InsertRaw<at::borderColorRight>(gridColor);
                        sheets.back()->InsertRaw<at::borderColorBottom>(gridColor);
                        sheets.back()->InsertRaw<at::graphGridColor>(gridColor);
                        sheets.back()->InsertRaw<at::graphVerticalDivs>((double)graph.vDivs);
                        sheets.back()->InsertRaw<at::graphHorizontalDivs>((double)graph.hDivs);
                        sheets.back()->InsertRaw<at::graphBinCount>((double)graph.graphType.binCount);

                        sheets.push_back(Stylesheet::Make({ { "$graph", "$line", tag }, {"$body-plot"} }));
                        sheets.back()->InsertRaw<at::borderColorRight>(dividerColor);

                        sheets.push_back(Stylesheet::Make({ { "$graph", "$line", tag }, {"$body-right", "$value"} }));
                        // TODO: investigate--why is this hardcoded using index 0?
                        sheets.back()->InsertRaw<at::textColor>(at::make::Color(graph.metrics[0].lineColor));


                        bool hasRightAxis = false;
                        for (auto&& [iwm, widgetMetric] : graph.metrics | vi::enumerate) {
                            hasRightAxis = hasRightAxis || widgetMetric.axisAffinity == AxisAffinity::Right;
                            const auto lineColor = at::make::Color(widgetMetric.lineColor);
                            const auto fillColor = at::make::Color(widgetMetric.fillColor);
                            
                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$metric", std::format("$metric-{}", iwm)}}));
                            sheets.back()->InsertRaw<at::graphLineColor>(lineColor);
                            sheets.back()->InsertRaw<at::graphFillColor>(fillColor);
                            sheets.back()->InsertRaw<at::backgroundColor>(lineColor);

                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$label-swatch", std::format("$metric-{}", iwm)} }));
                            sheets.back()->InsertRaw<at::graphLineColor>(lineColor);
                            sheets.back()->InsertRaw<at::graphFillColor>(fillColor);
                            sheets.back()->InsertRaw<at::backgroundColor>(lineColor);

                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$label-value", std::format("$metric-{}", iwm)}}));
                            sheets.back()->InsertRaw<at::textColor>(lineColor);

                            sheets.push_back(Stylesheet::Make({ { "$graph", tag }, { "$label-units", std::format("$metric-{}", iwm)} }));
                            sheets.back()->InsertRaw<at::textColor>(
                                // darken line color for units
                                at::make::Color(widgetMetric.lineColor * 0.7f)
                            );
                        }

                        // multi-line right axis hiding
                        if (!hasRightAxis) {
                            sheets.push_back(Stylesheet::Make({ {"$graph", tag }, {"$body-right", "$vert-axis"} }));
                            sheets.back()->InsertRaw<at::display>(at::make::Enum(gfx::lay::Display::Invisible));
                        }
                    }
                }
                else if (auto pReadout = std::get_if<kact::push_spec_impl::Readout>(&widget)) {
                    using namespace gfx::lay::sty;
                    auto& readout = *pReadout;
                    auto& sheets = pSpec->sheets;
                    const auto tag = std::format("r{}", i);
                    if (pReadout->metrics.size() > 1) {
                        pmlog_warn("Too many metricIds for readout widget");
                    }
                    auto& qualifiedMetric = readout.metrics.back().metric;
                    pSpec->widgets.push_back(kern::ReadoutSpec{
                        .metric = {
                            .metricId = qualifiedMetric.metricId,
                            .statId = qualifiedMetric.statId,
                            .arrayIndex = qualifiedMetric.arrayIndex,
                            .deviceId = qualifiedMetric.deviceId,
                            .unitId = qualifiedMetric.desiredUnitId,
                        },
                        .tag = tag,
                    });

                    const auto fontSize = double(readout.fontSize);
                    auto backgroundColor = at::make::Color(readout.backgroundColor);
                    auto fontColor = at::make::Color(readout.fontColor);

                    sheets.push_back(Stylesheet::Make({ {}, { "$readout", tag } }));
                    sheets.back()->InsertRaw<at::backgroundColor>(backgroundColor);

                    if (!readout.showLabel) {
                        sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$label" } }));
                        sheets.back()->InsertRaw<at::display>(at::make::Enum(Display::None));
                    }

                    sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$text-large" } }));
                    sheets.back()->InsertRaw<at::textColor>(fontColor);
                    sheets.back()->InsertRaw<at::textSize>(fontSize);

                    sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$text-large" } }));
                    sheets.back()->InsertRaw<at::textColor>(fontColor);
                    sheets.back()->InsertRaw<at::textSize>(fontSize);

                    sheets.push_back(Stylesheet::Make({ { "$readout", tag }, { "$numeric-units" } }));
                    sheets.back()->InsertRaw<at::textColor>(fontColor);
                    sheets.back()->InsertRaw<at::textSize>(fontSize);
                    sheets.back()->InsertRaw<at::paddingLeft>(fontSize / 5.);
                }
                else {
                    pmlog_warn(std::format("Unknown widget type; variant index [{}]", widget.index()));
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
