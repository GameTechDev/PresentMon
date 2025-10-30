#pragma once
#include "../../CommonUtilities/win/WinAPI.h"
#include <d3d10_1.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include "OverlayRenderer.h"

#include <d3dcompiler.h>
#include <d3d10_1.h>
#include <d3d10_1.h>

namespace GfxLayer::Extension
{
	class OverlayRenderer_D3D10 : public OverlayRenderer
	{
	public:
		OverlayRenderer_D3D10(const OverlayConfig& config, IDXGISwapChain3* pSwapChain, ID3D10Device* pDevice);
		~OverlayRenderer_D3D10() = default;

		void Resize(unsigned bufferCount, unsigned width, unsigned height) override;

	protected:
		void Render(bool renderBar, bool useRainbow, bool enableBackground) override;
		// this is triggered both by Resize and by UpdateConfig
		void UpdateViewport(const OverlayConfig& cfg) override;
		// called from Render when it is detected that config has changed (via IPC action)
		void UpdateConfig(const OverlayConfig& cfg) override;

	private:
		void InitializeRenderState_(const OverlayConfig& config);
		void InitializeColorConstantBuffers_(const OverlayConfig& config);
		// Generic buffer creator
		HRESULT CreateBuffer_(UINT byteWidth,
			D3D10_USAGE usage,
			UINT bindFlags,
			UINT cpuAccessFlags,
			const void* initData,
			ID3D10Buffer** ppBuffer);
		// Constant-buffer creator (rounds size to 16B)
		HRESULT CreateConstantBuffer_(const void* data,
			UINT dataSizeBytes,
			ID3D10Buffer** ppBuffer);

		ComPtr<ID3D10Device>						m_pDevice{};
		std::vector<ComPtr<ID3D10RenderTargetView>>	m_Rtvs;
		ComPtr<ID3D10Buffer>						m_pVertexBuffer;
		ComPtr<ID3D10Buffer>						m_pIndexBuffer;
		ComPtr<ID3D10Buffer>						m_pConstantBufferBar;
		ComPtr<ID3D10Buffer>						m_pConstantBufferBackground;
		std::vector<ComPtr<ID3D10Buffer>>			m_rainbowConstantBufferPtrs;
		ComPtr<ID3D10VertexShader>					m_pVertexShader;
		ComPtr<ID3D10PixelShader>					m_pPixelShader;
		ComPtr<ID3D10InputLayout>					m_pVertexLayout;
		ComPtr<ID3D10StateBlock>					m_pStateBlock;

		D3D10_VIEWPORT								m_foregrountViewport{};
		D3D10_VIEWPORT								m_backgroundViewport{};
		D3D10_VIEWPORT								m_rainbowViewport{};
	};
}