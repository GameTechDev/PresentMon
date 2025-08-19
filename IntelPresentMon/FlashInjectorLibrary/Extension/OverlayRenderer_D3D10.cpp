#include "OverlayRenderer_D3D10.h"
#include <dxgi1_6.h>
#include "Quad.h"

namespace GfxLayer::Extension
{
	OverlayRenderer_D3D10::OverlayRenderer_D3D10(const OverlayConfig& config, IDXGISwapChain3* pSwapChain, ID3D10Device* pDevice):
		OverlayRenderer(config, pSwapChain),
		m_pDevice(pDevice)
	{
		InitializeRenderState_(config);
	}

	void OverlayRenderer_D3D10::InitializeColorConstantBuffers_(const OverlayConfig& config)
	{
		// background
		{
			const D3D10_SUBRESOURCE_DATA initData{ .pSysMem = config.BackgroundColor.data() };
			D3D10_BUFFER_DESC bufferDesc{};
			bufferDesc.Usage = D3D10_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(config.BackgroundColor);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			auto hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pConstantBufferBackground);
			CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Background Constant Buffer)");
		}
		// flash
		{
			const D3D10_SUBRESOURCE_DATA initData{ .pSysMem = config.BarColor.data() };
			D3D10_BUFFER_DESC bufferDesc{};
			bufferDesc.Usage = D3D10_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(config.BarColor);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			auto hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pConstantBufferBar);
			CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Background Constant Buffer)");
		}
	}

	void OverlayRenderer_D3D10::InitializeRenderState_(const OverlayConfig& config)
	{
		// Load Shaders and Input Layout

		ComPtr<ID3D10Blob> pVSBlob = nullptr;
		Quad::CompileShader(Quad::pVertexShader, "VS", "vs_4_0", &pVSBlob);
		auto hr = m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pVertexShader);
		CheckResult(hr, "D3D10 - Failed to create ID3D10VerttexShader");

		ComPtr<ID3D10Blob> pPSBlob = nullptr;
		Quad::CompileShader(Quad::pPixelShader, "PS", "ps_4_0", &pPSBlob);
		hr = m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), &m_pPixelShader);
		CheckResult(hr, "D3D10 - Failed to create ID3D10PixelShader");

		D3D10_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = m_pDevice->CreateInputLayout(layout, 1, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pVertexLayout);
		CheckResult(hr, "D3D10 - Failed to create ID3D10InputLayout");

		// Load Vertex and Index buffers

		D3D10_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.Usage = D3D10_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(Quad::Vertex) * 4;
		bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		D3D10_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = Quad::Vertices;
		hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pVertexBuffer);
		CheckResult(hr, "D3D10 - Failed to create ID3D10Buffer (Vertex Buffer)");

		bufferDesc.Usage = D3D10_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(unsigned) * 6;
		bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		initData.pSysMem = Quad::Indices;
		hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pIndexBuffer);
		CheckResult(hr, "D3D10 - Failed to create ID3D10Buffer (Index Buffer)");

		InitializeColorConstantBuffers_(config);

		// Create ID3D10StateBlock to save / restore state

		D3D10_STATE_BLOCK_MASK stateBlockMask = { 0 };
		hr = D3D10StateBlockMaskEnableAll(&stateBlockMask);
		CheckResult(hr, "D3D10 - Failed to create D3D10_STATE_BLOCK_MASK");

		hr = D3D10CreateStateBlock(m_pDevice.Get(), &stateBlockMask, &m_pStateBlock);
		CheckResult(hr, "D3D10 - Failed to create ID3D10StateBlock");
	}

	void OverlayRenderer_D3D10::Render(bool renderBar)
	{
		ID3D10Buffer* pConstantBuffer = m_pConstantBufferBackground.Get();
		if (renderBar)
		{
			pConstantBuffer = m_pConstantBufferBar.Get();
		}

		// Capture the current state

		m_pStateBlock->Capture();

		// Cache the render target view

		auto* pSwapChain = GetSwapChain();
		auto  idx = pSwapChain->GetCurrentBackBufferIndex();
		if (!m_Rtvs[idx])
		{
			ComPtr<ID3D10Texture2D> pBackBuffer = nullptr;
			pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
			auto hr = m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_Rtvs[idx]);
			CheckResult(hr, "D3D10 - Failed to create ID3D10RenderTargetView");
		}

		// Render the overlay

		ID3D10RenderTargetView* pRtv = m_Rtvs[idx].Get();
		ID3D10Buffer* pVertexBuffer = m_pVertexBuffer.Get();
		UINT stride = sizeof(Quad::Vertex);
		UINT offset = 0;

		m_pDevice->ClearState();
		m_pDevice->RSSetViewports(1, &m_Viewport);
		m_pDevice->OMSetRenderTargets(1, &pRtv, nullptr);
		m_pDevice->IASetInputLayout(m_pVertexLayout.Get());
		m_pDevice->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		m_pDevice->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pDevice->VSSetShader(m_pVertexShader.Get());
		m_pDevice->PSSetConstantBuffers(0, 1, &pConstantBuffer);
		m_pDevice->PSSetShader(m_pPixelShader.Get());
		m_pDevice->DrawIndexed(6, 0, 0);

		// Restore the previous state

		m_pStateBlock->Apply();
	}

	void OverlayRenderer_D3D10::UpdateViewport(const OverlayConfig& cfg)
	{
		auto rect = GetScissorRect();
		m_Viewport.TopLeftX = rect.left;
		m_Viewport.TopLeftY = rect.top;
		m_Viewport.Width = rect.right - rect.left;
		m_Viewport.Height = rect.bottom - rect.top;
	}

	void OverlayRenderer_D3D10::UpdateConfig(const OverlayConfig& cfg)
	{
		UpdateViewport(cfg);
		InitializeColorConstantBuffers_(cfg);
	}

	void OverlayRenderer_D3D10::Resize(unsigned bufferCount, unsigned width, unsigned height)
	{
		OverlayRenderer::Resize(bufferCount, width, height);
		m_Rtvs.clear();
		m_Rtvs.resize(bufferCount);
	}
}