// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "../Config.h"
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>

namespace pb2
{
	struct AdapterSelection
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		LUID luid{};
		uint32_t index = 0;
	};

	DXGI_SWAP_EFFECT ToDxgiSwapEffect(SwapEffect effect);
	AdapterSelection ResolveAdapter(uint32_t index, IDXGIFactory1* factory);
	DXGI_SWAP_CHAIN_DESC1 MakeSwapChainDesc1(uint32_t width, uint32_t height,
		const SwapChainConfig& swap, const PresentConfig& present);
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC MakeFullscreenDesc(WindowMode mode);
	bool PrepareExclusiveFullscreen(IDXGISwapChain* swapChain, bool windowActive);
}
