#pragma once
#include "../../CommonUtilities/win/WinAPI.h"
#include <d3d12.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include "OverlayRenderer.h"

namespace GfxLayer::Extension
{
	class OverlayRenderer_D3D12 : public OverlayRenderer
	{
	public:
		OverlayRenderer_D3D12(const OverlayConfig& cfg, IDXGISwapChain3* pSwapChain, ID3D12CommandQueue* pCmdQueue);
		~OverlayRenderer_D3D12() = default;

		void											Resize(unsigned bufferCount, unsigned width, unsigned height) override;

	protected:
		void			Render(bool renderBar) override;
		// this is triggered both by Resize and by UpdateConfig
		void			UpdateViewport(const OverlayConfig& cfg) override;
		// called from Render when it is detected that config has changed (via IPC action)
		void			UpdateConfig(const OverlayConfig& cfg) override;

	private:
		OverlayConfig m_config;
		ComPtr<ID3D12Device>                            m_pDevice{};
		ComPtr<ID3D12CommandQueue>                      m_pCmdQueue{};
		std::vector<ComPtr<ID3D12CommandAllocator>>     m_CmdAllocators{};
		std::vector<ComPtr<ID3D12GraphicsCommandList>>  m_CmdLists{};
		ComPtr<ID3D12DescriptorHeap>                    m_RtvHeap{};
		UINT                                            m_RtvDescriptorSize{};
	};
}