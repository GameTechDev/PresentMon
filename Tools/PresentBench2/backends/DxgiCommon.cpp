// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "DxgiCommon.h"
#include "../../../IntelPresentMon/CommonUtilities/log/Log.h"
#include "../../../IntelPresentMon/CommonUtilities/log/HrLogger.h"
#include "../../../IntelPresentMon/CommonUtilities/str/String.h"
#include <format>
#include <stdexcept>

namespace pb2
{
	DXGI_SWAP_EFFECT ToDxgiSwapEffect(SwapEffect effect)
	{
		switch (effect) {
		case SwapEffect::Discard: return DXGI_SWAP_EFFECT_DISCARD;
		case SwapEffect::Sequential: return DXGI_SWAP_EFFECT_SEQUENTIAL;
		case SwapEffect::FlipSequential: return DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		case SwapEffect::FlipDiscard: return DXGI_SWAP_EFFECT_FLIP_DISCARD;
		case SwapEffect::Count: break;
		}
		return DXGI_SWAP_EFFECT_FLIP_DISCARD;
	}

	AdapterSelection ResolveAdapter(uint32_t index, IDXGIFactory1* factory)
	{
		AdapterSelection selection;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		UINT i = 0;
		while (factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
			if (i == index) {
				DXGI_ADAPTER_DESC1 desc{};
				pmlog_hr << adapter->GetDesc1(&desc);
				selection.adapter = adapter;
				selection.luid = desc.AdapterLuid;
				selection.index = i;
				pmlog_info(std::format("Using DXGI adapter {}: {}", i,
					pmon::util::str::ToNarrow(desc.Description)));
				return selection;
			}
			++i;
			adapter.Reset();
		}
		throw std::runtime_error(std::format("DXGI adapter index {} not found ({} adapters)",
			index, i));
	}

	DXGI_SWAP_CHAIN_DESC1 MakeSwapChainDesc1(uint32_t width, uint32_t height,
		const SwapChainConfig& swap, const PresentConfig& present)
	{
		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = swap.bufferCount;
		desc.SwapEffect = ToDxgiSwapEffect(swap.swapEffect);
		desc.Flags = (present.flags & DXGI_PRESENT_ALLOW_TEARING) ?
			DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		return desc;
	}

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC MakeFullscreenDesc(WindowMode mode)
	{
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC desc{};
		desc.Windowed = (mode != WindowMode::Fullscreen) ? TRUE : FALSE;
		desc.RefreshRate = { 0, 0 };
		desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		return desc;
	}

	bool PrepareExclusiveFullscreen(IDXGISwapChain* swapChain, bool windowActive)
	{
		BOOL fullscreen = FALSE;
		pmlog_hr << swapChain->GetFullscreenState(&fullscreen, nullptr);
		if (!windowActive) {
			if (fullscreen) {
				pmlog_hr << swapChain->SetFullscreenState(FALSE, nullptr);
			}
			return false;
		}
		if (!fullscreen) {
			const HRESULT hr = swapChain->SetFullscreenState(TRUE, nullptr);
			if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
				return false;
			}
			pmlog_hr << hr;
		}
		return true;
	}
}
