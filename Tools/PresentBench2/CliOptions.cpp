// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CliOptions.h"
#include <stdexcept>

namespace clio
{
	pb2::BenchConfig Options::ToConfig() const
	{
		pb2::BenchConfig cfg;
		if ((bool)dx9) {
			cfg.api = pb2::GraphicsApi::Dx9;
		}
		else if ((bool)dx11) {
			cfg.api = pb2::GraphicsApi::Dx11;
		}
		else if ((bool)vk) {
			cfg.api = pb2::GraphicsApi::Vulkan;
		}
		else {
			cfg.api = pb2::GraphicsApi::Dx12;
		}

		if ((bool)borderless) {
			cfg.mode = pb2::WindowMode::Borderless;
		}
		else if ((bool)fullscreen) {
			cfg.mode = pb2::WindowMode::Fullscreen;
		}
		else {
			cfg.mode = pb2::WindowMode::Window;
		}

		cfg.monitor = *monitor;
		cfg.adapter = *adapter;
		cfg.enableDebugLayer = (bool)enableDebugLayer;
		cfg.logPath = *log;
		cfg.frameSleepMs = *frameSleep;
		cfg.coarseFrameWait = (bool)coarseFrameWait;
		if (*frameCount > 0) {
			cfg.frameCount = *frameCount;
		}
		cfg.framesInFlight = *framesInFlight;
		cfg.swap.width = *width;
		cfg.swap.height = *height;
		cfg.swap.bufferCount = *bufferCount;
		cfg.swap.swapEffect = *swapEffect;
		if (cfg.api == pb2::GraphicsApi::Dx9 && cfg.swap.bufferCount > 4) {
			throw std::invalid_argument(
				"DX9 supports --buffer-count values from 2 through 4");
		}
		if (cfg.api == pb2::GraphicsApi::Dx12 &&
			cfg.swap.swapEffect != pb2::SwapEffect::FlipDiscard &&
			cfg.swap.swapEffect != pb2::SwapEffect::FlipSequential) {
			throw std::invalid_argument(
				"DX12 requires --swap-effect FlipDiscard or FlipSequential");
		}
		cfg.swap.vkMode = *vkMode;
		cfg.present.syncInterval = *syncInterval;
		if (cfg.api == pb2::GraphicsApi::Dx9 && cfg.present.syncInterval > 1) {
			throw std::invalid_argument("DX9 supports --sync-interval 0 or 1");
		}
		cfg.present.flags = *flags;
		cfg.workload.clearsPerFrame = *clearsPerFrame;
		cfg.workload.cpuBusyMs = *cpuBusyMs;
		cfg.workload.psoCompilesPerFrame = *psoCompiles;
		cfg.workload.injectMouseClick = (bool)injectMouse;
		cfg.workload.injectKeyboard = (bool)injectKeyboard;
		cfg.workload.emitProviderMarkers = !(bool)noEmitProviderMarkers;
		cfg.workload.emitPclMarkers = (bool)emitPclMarkers;
		cfg.workload.useXessFg = (bool)useXessFg;
		cfg.workload.frameGenerationInterpolatedFrames = *fgInterpolatedFrames;
		cfg.describeCapabilities = (bool)describeCapabilities;
		cfg.enableStdioControl = (bool)enableStdioControl;
		if (cfg.api == pb2::GraphicsApi::Vulkan &&
			cfg.enableDebugLayer && cfg.enableStdioControl) {
			throw std::invalid_argument(
				"Vulkan validation cannot be combined with --enable-stdio-control "
				"because validation output can corrupt the stdout protocol");
		}
		cfg.resultJsonPath = *resultJson;
		return cfg;
	}
}
