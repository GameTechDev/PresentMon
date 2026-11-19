// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace pb2
{
	enum class FrameMarker
	{
		SleepStart,
		SleepEnd,
		SimulationStart,
		SimulationEnd,
		RenderSubmitStart,
		RenderSubmitEnd,
		PresentStart,
		PresentEnd,
	};

	class IFrameMarkerSink
	{
	public:
		virtual ~IFrameMarkerSink() = default;
		// Called once per frame before Simulation markers (XeLL sleep lives here).
		virtual void BeginFrame(uint32_t frameId) { (void)frameId; }
		virtual void Mark(FrameMarker marker, uint32_t frameId) = 0;
		virtual void InputSample(uint32_t frameId, bool mouse, bool keyboard)
		{
			(void)frameId;
			(void)mouse;
			(void)keyboard;
		}
	};

	class MarkerHub final : public IFrameMarkerSink
	{
	public:
		void Add(std::unique_ptr<IFrameMarkerSink> sink)
		{
			if (sink) {
				sinks_.push_back(std::move(sink));
			}
		}
		void AddRaw(IFrameMarkerSink* sink)
		{
			if (sink) {
				rawSinks_.push_back(sink);
			}
		}

		void BeginFrame(uint32_t frameId) override
		{
			for (auto& s : sinks_) {
				s->BeginFrame(frameId);
			}
			for (auto* s : rawSinks_) {
				s->BeginFrame(frameId);
			}
		}

		void Mark(FrameMarker marker, uint32_t frameId) override
		{
			for (auto& s : sinks_) {
				s->Mark(marker, frameId);
			}
			for (auto* s : rawSinks_) {
				s->Mark(marker, frameId);
			}
		}

		void InputSample(uint32_t frameId, bool mouse, bool keyboard) override
		{
			for (auto& s : sinks_) {
				s->InputSample(frameId, mouse, keyboard);
			}
			for (auto* s : rawSinks_) {
				s->InputSample(frameId, mouse, keyboard);
			}
		}

	private:
		std::vector<std::unique_ptr<IFrameMarkerSink>> sinks_;
		std::vector<IFrameMarkerSink*> rawSinks_;
	};

	std::unique_ptr<IFrameMarkerSink> CreateProviderSink(bool emitFrameType);
	std::unique_ptr<IFrameMarkerSink> CreatePclStatsSink();
}
