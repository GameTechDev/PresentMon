#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include "../MakeOverlaySpec.h"
#include <format>
#include "../../Core/source/kernel/Kernel.h"
#include "../../Core/source/gfx/base/Geometry.h"
#include <variant>
#include <cereal/types/variant.hpp>
#include <cereal/types/array.hpp>

// cereal JSON dump + NVP macro
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/optional.hpp>

#define ACT_NAME PushSpecification
#define ACT_EXEC_CTX KernelExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS kproc::kact

namespace ACT_NS
{
    using namespace ::pmon::ipc::act;
    using ::p2c::gfx::Color;

    namespace push_spec_impl
    {
        struct Metric
        {
            PM_METRIC metricId;
            uint32_t arrayIndex;
            uint32_t deviceId;
            PM_STAT statId;
            PM_UNIT desiredUnitId;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(metricId),
                    CEREAL_NVP(arrayIndex),
                    CEREAL_NVP(deviceId),
                    CEREAL_NVP(statId),
                    CEREAL_NVP(desiredUnitId));
            }
        };

        struct WidgetMetric
        {
            Metric metric;
            Color lineColor;
            Color fillColor;
            ::p2c::gfx::lay::AxisAffinity axisAffinity;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(metric),
                    CEREAL_NVP(lineColor),
                    CEREAL_NVP(fillColor),
                    CEREAL_NVP(axisAffinity));
            }
        };

        // Graph widget type.
        struct Graph
        {
            std::vector<WidgetMetric> metrics;

            uint32_t height;
            uint32_t vDivs;
            uint32_t hDivs;
            bool showBottomAxis;

            struct GraphType
            {
                std::string name;
                std::array<int, 2> range;
                std::array<int, 2> rangeRight;
                uint32_t binCount;
                std::array<int, 2> countRange;
                bool autoLeft;
                bool autoRight;
                bool autoCount;

                template<class A> void serialize(A& ar) {
                    ar(CEREAL_NVP(name),
                        CEREAL_NVP(range),
                        CEREAL_NVP(rangeRight),
                        CEREAL_NVP(binCount),
                        CEREAL_NVP(countRange),
                        CEREAL_NVP(autoLeft),
                        CEREAL_NVP(autoRight),
                        CEREAL_NVP(autoCount));
                }
            } graphType;

            Color gridColor;
            Color dividerColor;
            Color backgroundColor;
            Color borderColor;
            Color textColor;
            float textSize;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(metrics),
                    CEREAL_NVP(height),
                    CEREAL_NVP(vDivs),
                    CEREAL_NVP(hDivs),
                    CEREAL_NVP(showBottomAxis),
                    CEREAL_NVP(graphType),
                    CEREAL_NVP(gridColor),
                    CEREAL_NVP(dividerColor),
                    CEREAL_NVP(backgroundColor),
                    CEREAL_NVP(borderColor),
                    CEREAL_NVP(textColor),
                    CEREAL_NVP(textSize));
            }
        };

        struct Readout
        {
            std::vector<WidgetMetric> metrics;

            bool showLabel;
            float fontSize;
            Color fontColor;
            Color backgroundColor;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(metrics),
                    CEREAL_NVP(showLabel),
                    CEREAL_NVP(fontSize),
                    CEREAL_NVP(fontColor),
                    CEREAL_NVP(backgroundColor));
            }
        };

        using Widget = std::variant<Graph, Readout>;

        struct Params
        {
            std::optional<uint32_t> pid;

            struct Preferences
            {
                std::string capturePath;
                uint32_t captureDelay;
                bool enableCaptureDelay;
                uint32_t captureDuration;
                bool enableCaptureDuration;
                bool hideDuringCapture;
                bool hideAlways;
                bool independentWindow;
                uint32_t metricPollRate;
                uint32_t overlayDrawRate;
                uint32_t telemetrySamplingPeriodMs;
                uint32_t etwFlushPeriod;
                bool manualEtwFlush;
                uint32_t metricsOffset;
                uint32_t metricsWindow;
                ::p2c::kern::OverlaySpec::OverlayPosition overlayPosition;
                float timeRange;
                float overlayMargin;
                float overlayBorder;
                float overlayPadding;
                float graphMargin;
                float graphBorder;
                float graphPadding;
                Color overlayBorderColor;
                Color overlayBackgroundColor;

                struct GraphFont
                {
                    std::string name;
                    float axisSize;

                    template<class A> void serialize(A& ar) {
                        ar(CEREAL_NVP(name),
                            CEREAL_NVP(axisSize));
                    }
                } graphFont;

                uint32_t overlayWidth;
                bool upscale;
                bool generateStats;
                bool enableTargetBlocklist;
                bool enableAutotargetting;
                float upscaleFactor;
                std::optional<int> adapterId; // Uncertain: may be a different type in your system.

                bool enableFlashInjection;
                bool flashInjectionEnableTargetOverride;
                std::string flashInjectionTargetOverride;
                float flashInjectionSize;
                Color flashInjectionColor;
                bool flashInjectionBackgroundEnable;
                Color flashInjectionBackgroundColor;
                float flashInjectionRightShift;
                float flashInjectionFlashDuration;
                bool flashInjectionUseRainbow;
                float flashInjectionBackgroundSize;

                template<class A> void serialize(A& ar) {
                    ar(CEREAL_NVP(capturePath),
                        CEREAL_NVP(captureDelay),
                        CEREAL_NVP(enableCaptureDelay),
                        CEREAL_NVP(captureDuration),
                        CEREAL_NVP(enableCaptureDuration),
                        CEREAL_NVP(hideDuringCapture),
                        CEREAL_NVP(hideAlways),
                        CEREAL_NVP(independentWindow),
                        CEREAL_NVP(metricPollRate),
                        CEREAL_NVP(overlayDrawRate),
                        CEREAL_NVP(telemetrySamplingPeriodMs),
                        CEREAL_NVP(etwFlushPeriod),
                        CEREAL_NVP(manualEtwFlush),
                        CEREAL_NVP(metricsOffset),
                        CEREAL_NVP(metricsWindow),
                        CEREAL_NVP(overlayPosition),
                        CEREAL_NVP(timeRange),
                        CEREAL_NVP(overlayMargin),
                        CEREAL_NVP(overlayBorder),
                        CEREAL_NVP(overlayPadding),
                        CEREAL_NVP(graphMargin),
                        CEREAL_NVP(graphBorder),
                        CEREAL_NVP(graphPadding),
                        CEREAL_NVP(overlayBorderColor),
                        CEREAL_NVP(overlayBackgroundColor),
                        CEREAL_NVP(graphFont),
                        CEREAL_NVP(overlayWidth),
                        CEREAL_NVP(upscale),
                        CEREAL_NVP(generateStats),
                        CEREAL_NVP(enableTargetBlocklist),
                        CEREAL_NVP(enableAutotargetting),
                        CEREAL_NVP(upscaleFactor),
                        CEREAL_NVP(adapterId),
                        CEREAL_NVP(enableFlashInjection),
                        CEREAL_NVP(flashInjectionEnableTargetOverride),
                        CEREAL_NVP(flashInjectionTargetOverride),
                        CEREAL_NVP(flashInjectionSize),
                        CEREAL_NVP(flashInjectionColor),
                        CEREAL_NVP(flashInjectionBackgroundEnable),
                        CEREAL_NVP(flashInjectionBackgroundColor),
                        CEREAL_NVP(flashInjectionRightShift),
                        CEREAL_NVP(flashInjectionFlashDuration),
                        CEREAL_NVP(flashInjectionUseRainbow),
                        CEREAL_NVP(flashInjectionBackgroundSize));
                }
            } preferences;

            // Widgets stored as a variant: either Graph or Readout.
            std::vector<Widget> widgets;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(pid),
                    CEREAL_NVP(preferences),
                    CEREAL_NVP(widgets));
            }
        };
    }

    class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
    {
    public:
        static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
        using Params = push_spec_impl::Params;

        struct Response {
            template<class A> void serialize(A& ar) {
                // no response fields
            }
        };

    private:
        friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;

        static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
        {
            const GfxLayer::Extension::OverlayConfig cfg{
                .BarSize = in.preferences.flashInjectionSize,
                .BarRightShift = in.preferences.flashInjectionRightShift,
                .BarColor = in.preferences.flashInjectionColor.AsArray(),
                .RenderBackground = in.preferences.flashInjectionBackgroundEnable,
                .BackgroundColor = in.preferences.flashInjectionBackgroundColor.AsArray(),
                .FlashDuration = in.preferences.flashInjectionFlashDuration,
                .UseRainbow = in.preferences.flashInjectionUseRainbow,
                .BackgroundSize = in.preferences.flashInjectionBackgroundSize,
            };

            const auto flashTgtOverride = in.preferences.flashInjectionEnableTargetOverride ?
                std::optional{ in.preferences.flashInjectionTargetOverride } : std::nullopt;

            (*ctx.ppKernel)->UpdateInjection(in.preferences.enableFlashInjection, in.pid, flashTgtOverride, cfg);

            if (!in.pid) {
                (*ctx.ppKernel)->ClearOverlay();
            }
            else {
                (*ctx.ppKernel)->PushSpec(MakeOverlaySpec(in));
            }

            // (No useful response fields; logging the request is typically what you want here.)
            using v = pmon::util::log::V;
            pmlog_verb(v::kact)("PushSpecification action")
                .serialize("pushSpecification", in);

            return {};
        }
    };

    ACTION_REG();
}

namespace cereal
{
    template<class Archive>
    void serialize(Archive& archive, p2c::gfx::Color& s)
    {
        archive(CEREAL_NVP(s.r),
            CEREAL_NVP(s.g),
            CEREAL_NVP(s.b),
            CEREAL_NVP(s.a));
    }
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
