// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "Config.h"
#include "markers/FrameMarkers.h"
#include <memory>
#include <cstdint>

namespace pb2
{
	struct FrameInfo
	{
		uint32_t frameId = 0;
		float sceneTimeSec = 0.f;
		uint32_t clears = 1;
		uint32_t psoCompiles = 0;
	};

	struct PresentResult
	{
		uint32_t framesPresented = 1;
		bool frameGenerationEnabled = false;
	};

	class IRenderBackend
	{
	public:
		virtual ~IRenderBackend() = default;
		// Returns false while rendering must pause, such as during an exclusive-fullscreen alt-tab.
		virtual bool PrepareFrame(bool windowActive) { return windowActive; }
		// Wait until a frame slot is free (frames-in-flight throttle).
		virtual void WaitForFrameSlot() = 0;
		virtual void Render(const FrameInfo& info) = 0;
		virtual PresentResult Present(const FrameInfo& info) = 0;
		// Optional XeLL/other sink owned by the backend (e.g. XeFG). May be null.
		virtual IFrameMarkerSink* GetMarkerSink() { return nullptr; }
	};

	std::unique_ptr<IRenderBackend> CreateBackend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height);
}
