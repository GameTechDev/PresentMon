// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <deque>
#include <functional>
#include <algorithm>
#include <Core/source/gfx/base/Geometry.h>
#include <Core/source/gfx/layout/Enums.h>


namespace p2c::gfx::lay
{
	struct DataPoint
	{
		std::optional<float> value;
		double time;
		bool operator<(const DataPoint& rhs) const { return time < rhs.time; }
	};

	// ExtremeQueue is a container to store min/max values in window
	// Allow fast updating of min/max across entire window (worst case log(n), typically faster)
	template<class Comp1, class Comp2>
	class ExtremeQueue
	{
	public:
		void Push(float value)
		{
			// if queue is empty, value is most extreme by default
			if (candidates.empty()) {
				candidates.push_front(value);
			}
			// value is most extreme, reset queue (max: gt(front))
			else if (Comp2{}(value, candidates.front())) {
				candidates.clear();
				candidates.push_front(value);
			}
			// value is least extreme, append (max: le(back) => not gt)
			else if (!Comp2{}(value, candidates.back())) {
				candidates.push_back(value);
			}
			// value is in between, remove all less extreme [keep all where](max: ge => not lt)
			else {
				auto stale = std::partition_point(
					candidates.begin(), candidates.end(),
					[value](float el) {return !Comp1{}(el, value);}
				);
				candidates.erase(stale, candidates.end());
				candidates.push_back(value);
			}
		}
		void Pop(float value)
		{
			if (Empty()) {
				pmlog_warn("Trying to pop from empty extremeq");
				return;
			}
			if (candidates.front() == value) {
				candidates.pop_front();
			}
		}
		std::optional<float> GetCurrent() const
		{
			if (!Empty()) {
				return candidates.front();
			}
			return std::nullopt;
		}
		bool Empty() const
		{
			return candidates.empty();
		}
	private:
		std::deque<float> candidates;
	};

	using MaxQueue = ExtremeQueue<std::less<float>, std::greater<float>>;
	using MinQueue = ExtremeQueue<std::greater<float>, std::less<float>>;

	// GraphData is a container for data to be displayed in a GraphElement
	// conceptually it is a circular buffer
	// time of entries must be added in increasing order
	class GraphData
	{
	public:
		GraphData(double timeWindow);
		DataPoint& operator[](size_t i);
		const DataPoint& operator[](size_t i) const;
		// newest sample
		DataPoint& Front();
		// newest sample
		const DataPoint& Front() const;
		// oldest sample
		DataPoint& Back();
		// oldest sample
		const DataPoint& Back() const;
		void Push(const DataPoint& data);
		size_t Size() const;
		void Trim(double now);
		void Resize(double window);
		std::optional<float> Min() const;
		std::optional<float> Max() const;
		double GetWindowSize() const;
	private:
		// data
		double timeWindow;
		std::deque<DataPoint> data;
		MinQueue min;
		MaxQueue max;
	}; // TODO: only track min/max when auto range adjustment is active (might be tricky)

	// GraphLinePack combines graph data (which may be shared among widgets)
	// With style and annotation, such as color, axis affinity, and label text
	struct GraphLinePack
	{
		std::shared_ptr<GraphData> data;
		// colors populated via style during SetPosition
		gfx::Color lineColor;
		gfx::Color fillColor;
		// affinity populated via style during SetPosition, but this is TOO LATE for use in ctor
		// we rework data passing from OverlaySpec, possibly removing the style connection
		gfx::lay::AxisAffinity axisAffinity = gfx::lay::AxisAffinity::Left;
		std::wstring label;
		std::wstring units;
		bool dataUnavailable = false;
	};
}