// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace pb2
{
	enum class GraphicsApi
	{
		Dx9,
		Dx11,
		Dx12,
		Vulkan,
	};

	enum class WindowMode
	{
		Window,
		Borderless,
		Fullscreen,
	};

	enum class SwapEffect
	{
		Discard,
		Sequential,
		FlipSequential,
		FlipDiscard,
		Count,
	};

	enum class VkPresentMode
	{
		ImmediateKhr,
		MailboxKhr,
		FifoKhr,
		FifoRelaxedKhr,
		Count,
	};

	constexpr const char* ApiName(GraphicsApi api)
	{
		switch (api) {
		case GraphicsApi::Dx9: return "DX9";
		case GraphicsApi::Dx11: return "DX11";
		case GraphicsApi::Dx12: return "DX12";
		case GraphicsApi::Vulkan: return "VK";
		}
		return "UNKNOWN";
	}

	constexpr const char* ModeName(WindowMode mode)
	{
		switch (mode) {
		case WindowMode::Window: return "WINDOW";
		case WindowMode::Borderless: return "BORDERLESS";
		case WindowMode::Fullscreen: return "FULLSCREEN";
		}
		return "UNKNOWN";
	}

	struct PresentConfig
	{
		uint32_t syncInterval = 0;
		uint32_t flags = 0;
	};

	struct SwapChainConfig
	{
		uint32_t width = 1280;
		uint32_t height = 720;
		uint32_t bufferCount = 3;
		SwapEffect swapEffect = SwapEffect::FlipDiscard;
		VkPresentMode vkMode = VkPresentMode::ImmediateKhr;
	};

	struct WorkloadConfig
	{
		uint32_t clearsPerFrame = 1;
		uint32_t cpuBusyMs = 0;
		uint32_t psoCompilesPerFrame = 0;
		bool injectMouseClick = false;
		bool injectKeyboard = false;
		bool emitProviderMarkers = true;
		bool emitPclMarkers = false;
		bool useXessFg = false;
		uint32_t frameGenerationInterpolatedFrames = 1;
	};

	struct BenchConfig
	{
		GraphicsApi api = GraphicsApi::Dx12;
		WindowMode mode = WindowMode::Window;
		uint32_t monitor = 0;
		uint32_t adapter = 0;
		bool enableDebugLayer = false;
		uint32_t frameSleepMs = 10;
		// When true, IntervalWaiter uses Sleep; otherwise high-precision timer + 500us spin.
		bool coarseFrameWait = false;
		std::optional<uint32_t> frameCount;
		uint32_t framesInFlight = 2;
		std::string logPath;
		SwapChainConfig swap;
		PresentConfig present;
		WorkloadConfig workload;
		bool describeCapabilities = false;
		bool enableStdioControl = false;
		std::string resultJsonPath;
	};
}
