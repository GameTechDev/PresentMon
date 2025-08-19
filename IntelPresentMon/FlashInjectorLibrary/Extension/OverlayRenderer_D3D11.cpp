#include "OverlayRenderer_D3D11.h"
#include <dxgi1_6.h>
#include "Quad.h"

namespace GfxLayer::Extension
{
	OverlayRenderer_D3D11::OverlayRenderer_D3D11(const OverlayConfig& config,
		IDXGISwapChain3* pSwapChain, ID3D11Device* pDevice) :
		OverlayRenderer(config, pSwapChain),
		m_pDevice(pDevice)
	{
		InitializeRenderState_(config);
	}

	void OverlayRenderer_D3D11::InitializeColorConstantBuffers_(const OverlayConfig& config)
	{
		// background
		{
			const D3D11_SUBRESOURCE_DATA initData{ .pSysMem = config.BackgroundColor.data() };
			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(config.BackgroundColor);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			auto hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pConstantBufferBackground);
			CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Background Constant Buffer)");
		}
		// flash
		{
			const D3D11_SUBRESOURCE_DATA initData{ .pSysMem = config.BarColor.data() };
			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(config.BarColor);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			auto hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pConstantBufferBar);
			CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Background Constant Buffer)");
		}
	}

	void OverlayRenderer_D3D11::InitializeRenderState_(const OverlayConfig& config)
	{
		ComPtr<ID3D10Blob> pVSBlob = nullptr;
		Quad::CompileShader(Quad::pVertexShader, "VS", "vs_5_0", &pVSBlob);
		auto hr = m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
		CheckResult(hr, "D3D11 - Failed to create ID3D11VertexShader");

		ComPtr<ID3D10Blob> pPSBlob = nullptr;
		Quad::CompileShader(Quad::pPixelShader, "PS", "ps_5_0", &pPSBlob);
		hr = m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
		CheckResult(hr, "D3D11 - Failed to create ID3D11PixelShader");

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = m_pDevice->CreateInputLayout(layout, 1, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pVertexLayout);
		CheckResult(hr, "D3D11 - Failed to create ID3D11InputLayout");

		D3D11_BUFFER_DESC bufferDesc = { 0 };
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(Quad::Vertex) * 4;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = Quad::Vertices;
		hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pVertexBuffer);
		CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Vertex Buffer)");

		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(DWORD) * 6;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		initData.pSysMem = Quad::Indices;
		hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pIndexBuffer);
		CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Index Buffer)");

		InitializeColorConstantBuffers_(config);

		// Create a deferred context
		hr = m_pDevice->CreateDeferredContext(0, &m_pDeferredContext);
		CheckResult(hr, "D3D11 - Failed to create ID3D11DeviceContext");
	}

	void OverlayRenderer_D3D11::Render(bool renderBar)
	{
		ID3D11Buffer* pConstantBuffer = m_pConstantBufferBackground.Get();
		if (renderBar)
		{
			pConstantBuffer = m_pConstantBufferBar.Get();
		}

		// Cache the render target view

		auto* pSwapChain = GetSwapChain();
		auto  idx = pSwapChain->GetCurrentBackBufferIndex();
		if (!m_Rtvs[idx])
		{
			ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
			pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
			auto hr = m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_Rtvs[idx]);
			CheckResult(hr, "D3D11 - Failed to create ID3D11RenderTargetView");
		}

		// Record the overlay rendering commands to the deferred context

		ID3D11RenderTargetView* pRtv = m_Rtvs[idx].Get();
		ID3D11Buffer* pVertexBuffer = m_pVertexBuffer.Get();
		UINT stride = sizeof(Quad::Vertex);
		UINT offset = 0;

		m_pDeferredContext->RSSetViewports(1, &m_Viewport);
		m_pDeferredContext->OMSetRenderTargets(1, &pRtv, nullptr);
		m_pDeferredContext->IASetInputLayout(m_pVertexLayout.Get());
		m_pDeferredContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		m_pDeferredContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		m_pDeferredContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pDeferredContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
		m_pDeferredContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
		m_pDeferredContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
		m_pDeferredContext->DrawIndexed(6, 0, 0);

		// Finish recording and obtain a command list
		
		ComPtr<ID3D11CommandList> pCmdList = nullptr;
		m_pDeferredContext->FinishCommandList(false, &pCmdList);

		// Execute the command list on the immediate context

		ComPtr<ID3D11DeviceContext> pImmediateContext = nullptr;
		m_pDevice->GetImmediateContext(&pImmediateContext);
		pImmediateContext->ExecuteCommandList(pCmdList.Get(), true);
	}

	void OverlayRenderer_D3D11::UpdateViewport(const OverlayConfig& cfg)
	{
		auto rect = GetScissorRect();
		m_Viewport.TopLeftX = FLOAT(rect.left);
		m_Viewport.TopLeftY = FLOAT(rect.top);
		m_Viewport.Width = FLOAT(rect.right - rect.left);
		m_Viewport.Height = FLOAT(rect.bottom - rect.top);
	}

	void OverlayRenderer_D3D11::UpdateConfig(const OverlayConfig& cfg)
	{
		UpdateViewport(cfg);
		InitializeColorConstantBuffers_(cfg);
	}

	void OverlayRenderer_D3D11::Resize(unsigned bufferCount, unsigned width, unsigned height)
	{
		OverlayRenderer::Resize(bufferCount, width, height);
		m_Rtvs.clear();
		m_Rtvs.resize(bufferCount);
	}
}