// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Dx12PresentComponent.h"
#include "../../IntelPresentMon/CommonUtilities/log/HrLogger.h"
#include <wrl/client.h>
#include <stdexcept>

#if defined(PMON_PRESENTBENCH2_HAS_XESS_FG)
#include "vendor/XessFgAdapter.h"
#endif

using Microsoft::WRL::ComPtr;

namespace pb2
{
	namespace
	{
		class NativeDx12PresentComponent final : public IDx12PresentComponent
		{
		public:
			void CreateSwapChain(const Dx12PresentCreateInfo& info,
				IDXGISwapChain3** swapChain) override
			{
				ComPtr<IDXGISwapChain1> swapChain1;
				const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fsDesc =
					info.exclusiveFullscreen ? &info.fullscreenDesc : nullptr;
				pmlog_hr << info.factory->CreateSwapChainForHwnd(info.queue, info.hwnd,
					&info.swapChainDesc, fsDesc, nullptr, &swapChain1);
				pmlog_hr << swapChain1.CopyTo(IID_PPV_ARGS(swapChain));
			}

			void PreparePresent(const Dx12FrameGenerationInputs&) override {}
			void CompletePresent(uint32_t) override
			{
				status_ = { 1, false };
			}
			PresentResult GetLastPresentResult() const override { return status_; }

		private:
			PresentResult status_{};
		};
	}

	std::unique_ptr<IDx12PresentComponent> CreateDx12PresentComponent(
		const WorkloadConfig& config)
	{
		if (!config.useXessFg) {
			return std::make_unique<NativeDx12PresentComponent>();
		}
#if defined(PMON_PRESENTBENCH2_HAS_XESS_FG)
		return CreateXessFgPresentComponent(config.frameGenerationInterpolatedFrames);
#else
		throw std::runtime_error(
			"XeSS-FG SDK not fetched (configure with -DPMON_PRESENTBENCH2_FETCH_XESS_FG=ON)");
#endif
	}
}
