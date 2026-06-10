// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "GraphElement.h"
#include "TextElement.h"
#include "LinePlotElement.h"
#include "HistogramPlotElement.h"
#include "GraphData.h"
#include <cmath>
#include <format>
#include <Core/source/infra/Logging.h>
#include <CommonUtilities/Exception.h>
#include <Core/source/gfx/layout/style/StyleProcessor.h>
#include <ranges>


namespace p2c::gfx::lay
{
	using namespace std::string_literals;
	namespace rn = std::ranges;
	namespace vi = rn::views;
	using namespace ::pmon::util;

	// invisible element used so that we can target individual metrics (lines) with styles
	class GraphElement::VirtualLineElement : public Element
	{
	public:
		VirtualLineElement(GraphElement* pParent, size_t index)
			:
			Element{ { "$metric"s, std::format("$metric-{}", index) } },
			pParent{ pParent },
			index{ index }
		{}
	protected:
		// functions
		LayoutConstraints QueryLayoutConstraints_(std::optional<float> width, sty::StyleProcessor& sp, Graphics& gfx) const override
		{
			return LayoutConstraints{
				.min = 0.f,
				.max = 0.f,
				.basis = 0.f,
				.flexGrow = 0.f,
			};
		}
		void SetDimension_(float dimension, FlexDirection dir, sty::StyleProcessor& sp, Graphics& gfx) override {}
		void SetPosition_(const Vec2& pos, const Dimensions& dimensions, sty::StyleProcessor& sp, Graphics& gfx) override
		{
			// TODO: consider injecting these via spec instead of style
			// might eliminate virtual elements altogether
			pParent->packs[index]->lineColor = sp.Resolve<lay::sty::at::graphLineColor>();
			pParent->packs[index]->fillColor = sp.Resolve<lay::sty::at::graphFillColor>();
		}
		void Draw_(Graphics& gfx) const override {}
		// data
		GraphElement* pParent = nullptr;
		size_t index;
	};

	GraphElement::GraphElement(GraphType type_, std::vector<std::shared_ptr<GraphLinePack>> packs_, std::vector<std::string> classes_)
		:
		FlexElement{ {}, [&classes_, type_] {
			classes_.push_back("$graph");
			classes_.push_back(type_ == GraphType::Histogram ? "$hist" : "$line");
			return std::move(classes_);
		}() },
		showCurrentValues{ type_ != GraphType::Histogram },
		packs{ std::move(packs_) }
	{
		// graph labels (title) and virtual line elements
		if (showCurrentValues) {
			metricValueReadouts.resize(packs.size());
			previousMetricValues.resize(packs.size());
		}
		for (size_t i = 0; i < packs.size(); i++) {
			const auto& pack = *packs[i];
			if (pack.axisAffinity == AxisAffinity::Left) {
				auto titleFlex = FlexElement::Make({
					FlexElement::Make({}, {"$label-swatch", std::format("$metric-{}", i)}),
					TextElement::Make(pack.label, {"$label"})
				}, { "$label-wrap-left" });
				if (showCurrentValues) {
					// value
					metricValueReadouts[i] = TextElement::Make(L"0000.", { "$label-value", std::format("$metric-{}", i) });
					titleFlex->AddChild(metricValueReadouts[i]);
					// units
					titleFlex->AddChild(TextElement::Make(pack.units, { "$label-units", std::format("$metric-{}", i) }));
				}
				AddChild(std::move(titleFlex));
			}
			else {
				auto titleFlex = FlexElement::Make({}, { "$label-wrap-right" });
				if (showCurrentValues) {
					// units
					metricValueReadouts[i] = TextElement::Make(L"0000.", { "$label-value", std::format("$metric-{}", i) });
					titleFlex->AddChild(metricValueReadouts[i]);
					// units
					titleFlex->AddChild(TextElement::Make(pack.units, { "$label-units", std::format("$metric-{}", i) }));
				}
				titleFlex->AddChild(TextElement::Make(pack.label, { "$label" }));
				titleFlex->AddChild(FlexElement::Make({}, { "$label-swatch", std::format("$metric-{}", i) }));
				AddChild(std::move(titleFlex));
			}
			AddChild(std::make_shared<VirtualLineElement>(this, i));
		}

		// body 
		const auto MakePlotElement = [&]() -> std::shared_ptr<PlotElement> {
			switch (type_)
			{
			case GraphType::Histogram:
				if (packs.size() > 1) pmlog_warn("Histogram with multiple data packs");
				return std::make_shared<HistogramPlotElement>(packs.front(), std::vector<std::string>{"$body-plot"});
			case GraphType::Line: return std::make_shared<LinePlotElement>(packs, std::vector<std::string>{"$body-plot"});
			default: pmlog_error("Bad graph type"); throw Except<Exception>();
			}
		};
		AddChild( FlexElement::Make(
			{
				// left gutter
				FlexElement::Make(
					[&]() -> std::vector<std::shared_ptr<Element>> {
						return {
							// top label
							pLeftTop = TextElement::Make(L"9999", {"$body-left-top", "$axis", "$y-axis"}),
							// bottom label
							pLeftBottom = TextElement::Make(L"9999", {"$body-left-bottom", "$axis", "$y-axis"}),
						};
					}(),
					{"$body-left", "$vert-axis"}
				),
				// plot element
				pPlot = MakePlotElement(),

				// right gutter displays 2nd axis
				FlexElement::Make(
					[&]() -> std::vector<std::shared_ptr<Element>> {
						return {
							// top label
							pRightTop = TextElement::Make(L"9999", {"$body-right-top", "$axis", "$y-axis"}),
							// bottom label
							pRightBottom = TextElement::Make(L"9999", {"$body-right-bottom", "$axis", "$y-axis"}),
						};
					}(),
					{"$body-right", "$vert-axis"}
				),				
			},
			{"$body"}
		));
		// bottom (x-axis) labels
		AddChild(FlexElement::Make(
			{
				FlexElement::Make({}, {"$footer-left"}),
				FlexElement::Make(
					{
						// left label
						// must preset with 0 because justification-left doesn't work like we want
						pBottomLeft = TextElement::Make(L"0", {"$footer-center-left", "$axis", "$x-axis"}),
						// right label
						pBottomRight = TextElement::Make(L"9999", {"$footer-center-right", "$axis", "$x-axis"}),
					},
					{"$footer-center"}
				),
				FlexElement::Make({}, {"$footer-right"}),
			},
			{"$footer"}
		));
	}

	GraphElement::~GraphElement() {}

	std::shared_ptr<Element> GraphElement::Make(GraphType type_, std::vector<std::shared_ptr<GraphLinePack>> packs_, std::vector<std::string> classes_)
	{
		return std::make_shared<GraphElement>(type_, std::move(packs_), std::move(classes_));
	}

	void GraphElement::SetValueRangeLeft(float min, float max)
	{
		pPlot->SetValueRangeLeft(min, max);
		// value range is x-axis for histogram, otherwise (line plot) y-axis
		if (dynamic_cast<HistogramPlotElement*>(pPlot.get()))
		{
			SetBottomAxisLabels_(float(min), float(max), packs.front()->units);
		}
		else // line plot
		{
			SetAxisLabelsLeft_(min, max);
		}
	}

	void GraphElement::SetValueRangeRight(float min, float max)
	{
		pPlot->SetValueRangeRight(min, max);
		SetAxisLabelsRight_(min, max);
	}

	void GraphElement::SetCountRange(int min, int max)
	{
		// count range only applicable to histogram plots
		if (auto pHisto = dynamic_cast<HistogramPlotElement*>(pPlot.get()))
		{
			pHisto->SetCountRange(min, max);
			SetAxisLabelsLeft_(float(min), float(max));
		}
	}

	void GraphElement::SetTimeWindow(float dt)
	{
		pPlot->SetTimeWindow(dt);
		// time range not applicable to historgram plot (for axes)
		if (!dynamic_cast<HistogramPlotElement*>(pPlot.get()))
		{
			SetBottomAxisLabels_(dt, 0.f);
		}
	}

	void GraphElement::SetAxisLabelsLeft_(float bottom, float top)
	{
		if (pLeftTop && pLeftBottom)
		{
			pLeftTop->SetText(std::format(L"{:.0f}", top));
			pLeftBottom->SetText(std::format(L"{:.0f}", bottom));
		}
	}

	void GraphElement::SetAxisLabelsRight_(float bottom, float top)
	{
		if (pRightTop && pRightBottom)
		{
			pRightTop->SetText(std::format(L"{:.0f}", top));
			pRightBottom->SetText(std::format(L"{:.0f}", bottom));
		}
	}

	void GraphElement::SetBottomAxisLabels_(float left, float right, std::optional<std::wstring> units)
	{
		if (pBottomLeft && pBottomRight) {
			if (!units) {
				pBottomLeft->SetText(std::format(L"{:.0f}", left));
				pBottomRight->SetText(std::format(L"{:.0f}", right));
			}
			else {
				pBottomLeft->SetText(std::format(L"{:.0f}{}", left, *units));
				pBottomRight->SetText(std::format(L"{:.0f}{}", right, *units));
			}
		}
	}

	double GraphElement::GetCurrentTime_() const
	{
		auto t = std::numeric_limits<double>::max();
		for (auto& pack : packs) {
			auto& data = *pack->data;
			if (data.Size()) {
				t = std::min(t, data.Front().time);
			}
		}
		return t == std::numeric_limits<double>::max() ? 0. : t;
	}

	void GraphElement::Draw_(Graphics& gfx) const
	{
		// Display current value for each metric in the title/legend
		for (size_t i = 0; i < metricValueReadouts.size(); i++) {
			const auto& data = *packs[i]->data;
			if (data.Size() > 0) {
				auto& previousValue = previousMetricValues[i];
				auto& readout = *metricValueReadouts[i];
				if (const auto vopt = data.Front().value; (!previousValue || vopt != *previousValue))
				{
					if (!vopt) {
						readout.SetText(L"NA");
					}
					else {
						const auto v = *vopt;
						const auto digitsBeforeDecimal = std::max(int(log10(std::abs(v))), 0);
						const int maxFractionalDigits = 2;
						auto text = std::format(L"{:.{}f}", v, std::max(maxFractionalDigits - digitsBeforeDecimal, 0));
						readout.SetText(std::move(text));
					}
					previousValue = vopt;
				}
			}
			else if (packs[i]->dataUnavailable) {
				auto& readout = *metricValueReadouts[i];
				readout.SetText(L"NA");
			}
		}

		std::optional<double> currentTime;

		// auto-scaling line values / histogram bin ranges
		if (isAutoScalingLeft || isAutoScalingRight) {
			std::optional<float> newLeftMin;
			std::optional<float> newLeftMax;
			std::optional<float> newRightMin;
			std::optional<float> newRightMax;
			// finding largest extent over all packs for left and right axes separately
			for (const auto& p : packs) {
				if (p->axisAffinity == AxisAffinity::Left && isAutoScalingLeft) {
					if (auto min = p->data->Min()) {
						min = std::floor(*min);
						newLeftMin = std::min(newLeftMin.value_or(*min), *min);
					}
					if (auto max = p->data->Max()) {
						max = std::ceil(*max);
						newLeftMax = std::max(newLeftMax.value_or(*max), *max);
					}
				}
				else if (p->axisAffinity == AxisAffinity::Right && isAutoScalingRight) {
					if (auto min = p->data->Min()) {
						min = std::floor(*min);
						newRightMin = std::min(newRightMin.value_or(*min), *min);
					}
					if (auto max = p->data->Max()) {
						max = std::ceil(*max);
						newRightMax = std::max(newRightMax.value_or(*max), *max);
					}
				}
			}
			// fallback to previous value if nothing set, or 0/100 default if no previous
			newLeftMin = newLeftMin.value_or(autoLeftMin.value_or(0.f));
			newLeftMax = newLeftMax.value_or(autoLeftMax.value_or(100.f));
			newRightMin = newRightMin.value_or(autoRightMin.value_or(0.f));
			newRightMax = newRightMax.value_or(autoRightMax.value_or(100.f));
			// adjust max to be at least 1 greater than min
			if (*newLeftMax < *newLeftMin + 1.f) {
				newLeftMax = *newLeftMin + 1.f;
			}
			if (*newRightMax < *newRightMin + 1.f) {
				newRightMax = *newRightMin + 1.f;
			}
			// revert changes if shrinking within the cooldown period
			currentTime = GetCurrentTime_();
			const auto inCooldownLeftMin = *currentTime - autoScalingTimestampLeftMin <= autoScalingShrinkCooldown;
			const auto inCooldownLeftMax = *currentTime - autoScalingTimestampLeftMax <= autoScalingShrinkCooldown;
			const auto inCooldownRightMin = *currentTime - autoScalingTimestampRightMin <= autoScalingShrinkCooldown;
			const auto inCooldownRightMax = *currentTime - autoScalingTimestampRightMax <= autoScalingShrinkCooldown;
			if (newLeftMin > autoLeftMin) {
				newLeftMin = inCooldownLeftMin ? autoLeftMin.value_or(*newLeftMin) : *newLeftMin;
			}
			if (newLeftMax < autoLeftMax) {
				newLeftMax = inCooldownLeftMax ? autoLeftMax.value_or(*newLeftMax) : *newLeftMax;
			}
			if (newRightMin > autoRightMin) {
				newRightMin = inCooldownRightMin ? autoRightMin.value_or(*newRightMin) : *newRightMin;
			}
			if (newRightMax < autoRightMax) {
				newRightMax = inCooldownRightMax ? autoRightMax.value_or(*newRightMax) : *newRightMax;
			}
			// take action if any changes since last pass, per axis
			if (isAutoScalingLeft) {
				bool scaling = false;
				if (newLeftMin != autoLeftMin) {
					scaling = true;
					autoScalingTimestampLeftMin = *currentTime;
				}
				if (newLeftMax != autoLeftMax) {
					scaling = true;
					autoScalingTimestampLeftMax = *currentTime;
				}
				if (scaling) {
					// TODO: figure out how to not const cast this
					const_cast<GraphElement*>(this)->SetValueRangeLeft(*newLeftMin, *newLeftMax);
					autoLeftMin = *newLeftMin;
					autoLeftMax = *newLeftMax;
				}
			}
			if (isAutoScalingRight) {
				bool scaling = false;
				if (newRightMin != autoRightMin) {
					scaling = true;
					autoScalingTimestampRightMin = *currentTime;
				}
				if (newRightMax != autoRightMax) {
					scaling = true;
					autoScalingTimestampRightMax = *currentTime;
				}
				if (scaling) {
					// TODO: figure out how to not const cast this
					const_cast<GraphElement*>(this)->SetValueRangeRight(*newRightMin, *newRightMax);
					autoRightMin = *newRightMin;
					autoRightMax = *newRightMax;
				}
			}
		}

		// auto histogram counting
		if (isAutoScalingCount) {
			if (const auto pHist = dynamic_cast<HistogramPlotElement*>(pPlot.get())) {
				bool scaling = false;
				const auto max = pHist->GetMaxCount();
				currentTime = currentTime.value_or(GetCurrentTime_());
				if (max > autoCountMax) {
					autoCountMax = max;
					scaling = true;
					autoScalingTimestampCount = *currentTime;
				}
				else if (max < autoCountMax) {
					const auto timeSinceAutoScale = *currentTime - autoScalingTimestampCount;
					if (timeSinceAutoScale > autoScalingShrinkCooldown) {
						autoCountMax = max;
						scaling = true;
						autoScalingTimestampCount = *currentTime;
					}
				}
				if (scaling) {
					// TODO: figure out how to not const cast this
					const_cast<GraphElement*>(this)->SetCountRange(0, int(*autoCountMax));
				}
			}
		}

		FlexElement::Draw_(gfx);
	}

	void GraphElement::SetPosition_(const Vec2& pos, const Dimensions& dimensions, sty::StyleProcessor& sp, Graphics& gfx)
	{
		SetValueRangeLeft(sp.Resolve<sty::at::graphMinValueLeft>(), sp.Resolve<sty::at::graphMaxValueLeft>());
		SetValueRangeRight(sp.Resolve<sty::at::graphMinValueRight>(), sp.Resolve<sty::at::graphMaxValueRight>());
		isAutoScalingLeft = sp.Resolve<sty::at::graphAutoscaleLeft>();
		isAutoScalingRight = sp.Resolve<sty::at::graphAutoscaleRight>();
		isAutoScalingCount = sp.Resolve<sty::at::graphAutoscaleCount>();
		SetCountRange(sp.Resolve<sty::at::graphMinCount>(), sp.Resolve<sty::at::graphMaxCount>());
		SetTimeWindow(sp.Resolve<sty::at::graphTimeWindow>());
		FlexElement::SetPosition_(pos, dimensions, sp, gfx);
	}
}