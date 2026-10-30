// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../IntelPresentMon/CommonUtilities/cli/CliFramework.h"
#include "../../IntelPresentMon/CommonUtilities/ref/StaticReflection.h"
#include "Config.h"

namespace clio
{
	using namespace pmon::util::cli;
	using namespace pmon::util;

	struct Options : public OptionsBase<Options>
	{
	private: Group gCore_{ this, "Core", "PresentBench2 core options" }; public:
		Flag dx9{ this, "--dx9", "Use Direct3D 9" };
		Flag dx11{ this, "--dx11", "Use Direct3D 11" };
		Flag dx12{ this, "--dx12", "Use Direct3D 12 (default when no API flag is set)" };
		Flag vk{ this, "--vk", "Use Vulkan" };
		Flag borderless{ this, "--borderless", "Borderless fullscreen window" };
		Flag fullscreen{ this, "--fullscreen", "Exclusive fullscreen" };
		Option<uint32_t> monitor{ this, "--monitor", 0u, "Zero-based monitor index",
			CLI::Range(0u, 64u) };
		Option<uint32_t> adapter{ this, "--adapter", 0u, "Zero-based adapter index",
			CLI::Range(0u, 64u) };
		Flag enableDebugLayer{ this, "--enable-debug-layer", "Enable graphics API debug layer" };
		Option<std::string> log{ this, "--log", "", "Optional log file path" };
		Option<uint32_t> frameSleep{ this, "--frame-sleep", 10u,
			"Target frame interval in milliseconds", CLI::Range(0u, 60000u) };
		Flag coarseFrameWait{ this, "--coarse-frame-wait",
			"Use Sleep-based frame pacing instead of high-precision timer + 500us spin" };
		Option<uint32_t> frameCount{ this, "--frame-count", 0u,
			"Exit after N frames (0 = unlimited)", CLI::Range(0u, 100000000u) };
		Option<uint32_t> framesInFlight{ this, "--frames-in-flight", 2u,
			"GPU frames in flight (1 = serialized low-latency)", CLI::Range(1u, 4u) };
		Option<uint32_t> clearsPerFrame{ this, "--clears-per-frame", 1u,
			"Clears submitted per frame", CLI::Range(1u, 64u) };

	private: Group gSwap_{ this, "Swapchain", "Swapchain options" }; public:
		Option<uint32_t> width{ this, "--width", 1280u, "Backbuffer width (windowed)",
			CLI::Range(1u, 16384u) };
		Option<uint32_t> height{ this, "--height", 720u, "Backbuffer height (windowed)",
			CLI::Range(1u, 16384u) };
		Option<uint32_t> bufferCount{ this, "--buffer-count", 3u, "Swapchain buffer count",
			CLI::Range(2u, 8u) };
		Option<pb2::SwapEffect> swapEffect{ this, "--swap-effect", pb2::SwapEffect::FlipDiscard,
			"DXGI swap effect",
			CLI::CheckedTransformer{ ref::GenerateEnumKeyMap<pb2::SwapEffect>(), CLI::ignore_case } };
		Option<pb2::VkPresentMode> vkMode{ this, "--vk-mode", pb2::VkPresentMode::ImmediateKhr,
			"Vulkan present mode",
			CLI::CheckedTransformer{ ref::GenerateEnumKeyMap<pb2::VkPresentMode>(), CLI::ignore_case } };

	private: Group gPresent_{ this, "Present", "Present options" }; public:
		Option<uint32_t> syncInterval{ this, "--sync-interval", 0u, "Present sync interval",
			CLI::Range(0u, 4u) };
		Option<uint32_t> flags{ this, "--flags", 0u, "Present flags bitmask" };

	private: Group gWork_{ this, "Workload", "Instrumentation and workload options" }; public:
		Option<uint32_t> cpuBusyMs{ this, "--cpu-busy-ms", 0u,
			"CPU busy spin per frame in milliseconds", CLI::Range(0u, 1000u) };
		Option<uint32_t> psoCompiles{ this, "--pso-compiles", 0u,
			"DX12 PSO compiles per frame", CLI::Range(0u, 1000u) };
		Flag injectMouse{ this, "--inject-mouse", "Inject a mouse click each frame" };
		Flag injectKeyboard{ this, "--inject-keyboard", "Inject a keyboard event each frame" };
		Flag noEmitProviderMarkers{ this, "--no-emit-provider-markers",
			"Disable Intel-PresentMon provider application/frame-type markers" };
		Flag emitPclMarkers{ this, "--emit-pcl-markers",
			"Emit PCLStats TraceLogging markers" };
		Flag useXessFg{ this, "--use-xess-fg",
			"Enable XeSS frame generation (DX12 only)" };
		Option<uint32_t> fgInterpolatedFrames{ this, "--fg-interpolated-frames", 1u,
			"Interpolated frames per application frame", CLI::Range(1u, 8u) };

	private: Group gControl_{ this, "Control", "Control and output options" }; public:
		Flag describeCapabilities{ this, "--describe-capabilities",
			"Print capabilities JSON and exit" };
		Flag enableStdioControl{ this, "--enable-stdio-control",
			"Enable %command / %%{json}%% stdio control protocol" };
		Option<std::string> resultJson{ this, "--result-json", "",
			"Write machine-readable result JSON to this path" };

		MutualExclusion apiMutex_{ dx9, dx11, dx12, vk };
		MutualExclusion modeMutex_{ borderless, fullscreen };
		MutualExclusion widthBorderless_{ width, borderless };
		MutualExclusion widthFullscreen_{ width, fullscreen };
		MutualExclusion heightBorderless_{ height, borderless };
		MutualExclusion heightFullscreen_{ height, fullscreen };
		Dependency vkModeNeedsVk_{ vkMode, vk };
		MutualExclusion swapEffectVk_{ swapEffect, vk };
		MutualExclusion swapEffectDx9_{ swapEffect, dx9 };
		MutualExclusion flagsVk_{ flags, vk };
		MutualExclusion flagsDx9_{ flags, dx9 };
		MutualExclusion syncVk_{ syncInterval, vk };
		MutualExclusion psoDx9_{ psoCompiles, dx9 };
		MutualExclusion psoDx11_{ psoCompiles, dx11 };
		MutualExclusion psoVk_{ psoCompiles, vk };
		MutualExclusion xessDx9_{ useXessFg, dx9 };
		MutualExclusion xessDx11_{ useXessFg, dx11 };
		MutualExclusion xessVk_{ useXessFg, vk };
		MutualExclusion fullscreenVk_{ fullscreen, vk };
		MutualExclusion debugDx9_{ enableDebugLayer, dx9 };
		Dependency fgNeedsXess_{ fgInterpolatedFrames, useXessFg };

		static constexpr const char* description =
			"PresentBench2 - instrumented presenting workload for PresentMon validation";
		static constexpr const char* name = "PresentBench2.exe";

		pb2::BenchConfig ToConfig() const;
	};
}
