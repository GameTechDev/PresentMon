// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "Backend.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <memory>
#include <string>

namespace pb2
{
	struct Dx12PresentCreateInfo
	{
		HWND hwnd = nullptr;
		ID3D12Device* device = nullptr;
		ID3D12CommandQueue* queue = nullptr;
		IDXGIFactory4* factory = nullptr;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
		bool exclusiveFullscreen = false;
	};

	struct Dx12FrameGenerationInputs
	{
		uint32_t presentId = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		ID3D12Resource* depth = nullptr;
		ID3D12Resource* motionVectors = nullptr;
		float viewMatrix[16]{};
		float projectionMatrix[16]{};
		float sceneTimeSec = 0.f;
	};

	class IDx12PresentComponent
	{
	public:
		virtual ~IDx12PresentComponent() = default;
		virtual void CreateSwapChain(const Dx12PresentCreateInfo& info,
			IDXGISwapChain3** swapChain) = 0;
		virtual void PreparePresent(const Dx12FrameGenerationInputs& inputs) = 0;
		virtual void CompletePresent(uint32_t presentId) = 0;
		virtual PresentResult GetLastPresentResult() const = 0;
		virtual IFrameMarkerSink* GetMarkerSink() { return nullptr; }
	};

	std::unique_ptr<IDx12PresentComponent> CreateDx12PresentComponent(
		const WorkloadConfig& config);
}
