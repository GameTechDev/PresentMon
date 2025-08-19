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
                ar(metricId, arrayIndex, deviceId, statId, desiredUnitId);
            }
        };

        struct WidgetMetric
        {
            Metric metric;
            Color lineColor;
            Color fillColor;
            ::p2c::gfx::lay::AxisAffinity axisAffinity;

            template<class A> void serialize(A& ar) {
                ar(metric, lineColor, fillColor, axisAffinity);
            }
        };

        // Graph widget type.
        struct Graph {
            std::vector<WidgetMetric> metrics;

            uint32_t height;
            uint32_t vDivs;
            uint32_t hDivs;
            bool showBottomAxis;
            struct GraphType {
                std::string name;
                std::array<int, 2> range;
                std::array<int, 2> rangeRight;
                uint32_t binCount;
                std::array<int, 2> countRange;
                bool autoLeft;
                bool autoRight;
                bool autoCount;

                template<class A> void serialize(A& ar) {
                    ar(name, range, rangeRight, binCount, countRange, autoLeft, autoRight, autoCount);
                }
            } graphType;
            Color gridColor;
            Color dividerColor;
            Color backgroundColor;
            Color borderColor;
            Color textColor;
            float textSize;

            template<class A> void serialize(A& ar) {
                ar(metrics, height, vDivs, hDivs, showBottomAxis, graphType,
                    gridColor, dividerColor, backgroundColor, borderColor, textColor, textSize);
            }
        };

        struct Readout {
            std::vector<WidgetMetric> metrics;

            bool showLabel;
            float fontSize;
            Color fontColor;
            Color backgroundColor;

            template<class A> void serialize(A& ar) {
                ar(metrics, showLabel, fontSize, fontColor, backgroundColor);
            }
        };

        using Widget = std::variant<Graph, Readout>;

        struct Params {
            std::optional<uint32_t> pid;

            struct Preferences {
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

                struct GraphFont {
                    std::string name;
                    float axisSize;
                    template<class A> void serialize(A& ar) {
                        ar(name, axisSize);
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

                template<class A> void serialize(A& ar) {
                    ar(capturePath, captureDelay, enableCaptureDelay,
                        captureDuration, enableCaptureDuration, hideDuringCapture, hideAlways,
                        independentWindow, metricPollRate, overlayDrawRate, telemetrySamplingPeriodMs,
                        etwFlushPeriod, manualEtwFlush, metricsOffset, metricsWindow, overlayPosition,
                        timeRange, overlayMargin, overlayBorder, overlayPadding, graphMargin,
                        graphBorder, graphPadding, overlayBorderColor, overlayBackgroundColor,
                        graphFont, overlayWidth, upscale, generateStats, enableTargetBlocklist,
                        enableAutotargetting, upscaleFactor, adapterId, enableFlashInjection,
                        flashInjectionSize, flashInjectionEnableTargetOverride, flashInjectionTargetOverride,
                        flashInjectionColor, flashInjectionBackgroundEnable, flashInjectionBackgroundColor,
                        flashInjectionRightShift);
                }
            } preferences;

            // Widgets stored as a variant: either Graph or Readout.
            std::vector<Widget> widgets;

            template<class A> void serialize(A& ar) {
                ar(pid, preferences, widgets);
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
        archive(s.r, s.g, s.b, s.a);
    }
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE