// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Backend.h"
#include <stdexcept>

namespace pb2
{
	std::unique_ptr<IRenderBackend> CreateDx9Backend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height);
	std::unique_ptr<IRenderBackend> CreateDx11Backend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height);
	std::unique_ptr<IRenderBackend> CreateDx12Backend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height);
	std::unique_ptr<IRenderBackend> CreateVulkanBackend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height);

	std::unique_ptr<IRenderBackend> CreateBackend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height)
	{
		switch (config.api) {
		case GraphicsApi::Dx9: return CreateDx9Backend(hwnd, config, width, height);
		case GraphicsApi::Dx11: return CreateDx11Backend(hwnd, config, width, height);
		case GraphicsApi::Dx12: return CreateDx12Backend(hwnd, config, width, height);
		case GraphicsApi::Vulkan: return CreateVulkanBackend(hwnd, config, width, height);
		}
		throw std::runtime_error("unsupported graphics API");
	}
}
