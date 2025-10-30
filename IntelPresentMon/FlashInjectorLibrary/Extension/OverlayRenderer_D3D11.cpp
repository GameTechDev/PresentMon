#include "OverlayRenderer_D3D11.h"
#include <dxgi1_6.h>
#include "Quad.h"

namespace GfxLayer::Extension
{
	HRESULT OverlayRenderer_D3D11::CreateBuffer_(UINT byteWidth,
		D3D11_USAGE usage,
		UINT bindFlags,
		UINT cpuAccessFlags,
		const void* initData,
		ID3D11Buffer** ppBuffer)
	{
		D3D11_BUFFER_DESC desc{};
		desc.Usage = usage;
		desc.ByteWidth = byteWidth;
		desc.BindFlags = bindFlags;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA srd{};
		const D3D11_SUBRESOURCE_DATA* pSRD = nullptr;
		if (initData) { srd.pSysMem = initData; pSRD = &srd; }

		return m_pDevice->CreateBuffer(&desc, pSRD, ppBuffer);
	}

	HRESULT OverlayRenderer_D3D11::CreateConstantBuffer_(const void* data,
		UINT dataSizeBytes,
		ID3D11Buffer** ppBuffer)
	{
		// Round up to 16-byte multiple
		UINT cbSize = (dataSizeBytes + 15u) & ~15u;
		return CreateBuffer_(cbSize, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, data, ppBuffer);
	}

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
			auto hr = CreateConstantBuffer_(config.BackgroundColor.data(),
				sizeof(config.BackgroundColor), &m_pConstantBufferBackground);
			CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Background Constant Buffer)");
		}
		// flash
		{
			auto hr = CreateConstantBuffer_(config.BarColor.data(),
				sizeof(config.BarColor), &m_pConstantBufferBar);
			CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Flash Constant Buffer)");
		}
		// rainbow
		if (m_rainbowConstantBufferPtrs.empty()) {
			for (auto& color : GetRainbowColors()) {
				ComPtr<ID3D11Buffer> cb;
				auto hr = CreateConstantBuffer_(color.data(), sizeof(color), &cb);
				CheckResult(hr, "D3D11 - Failed to create ID3D11Buffer (Rainbow Constant Buffer)");
				m_rainbowConstantBufferPtrs.push_back(std::move(cb));
			}
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

		const D3D11_INPUT_ELEMENT_DESC layout[] = {
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

	void OverlayRenderer_D3D11::Render(bool renderBar, bool useRainbow, bool enableBackground)
	{
		// Cache the render target view

		auto* pSwapChain = GetSwapChain();
		auto  idx = pSwapChain->GetCurrentBackBufferIndex();
		if (!m_Rtvs[idx]) {
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

		// set common state
		m_pDeferredContext->OMSetRenderTargets(1, &pRtv, nullptr);
		m_pDeferredContext->IASetInputLayout(m_pVertexLayout.Get());
		m_pDeferredContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		m_pDeferredContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		m_pDeferredContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pDeferredContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
		m_pDeferredContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
		// issue fg/bg draw calls
		const auto Draw = [&](ID3D11Buffer* pConstantBuffer, const D3D11_VIEWPORT& vp) {
			m_pDeferredContext->RSSetViewports(1, &vp);
			m_pDeferredContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
			m_pDeferredContext->DrawIndexed(6, 0, 0);
		};
		if (useRainbow) {
			Draw(m_rainbowConstantBufferPtrs[GetRainbowIndex()].Get(), m_rainbowViewport);
		}
		// draw background before flash bar if enabled, but only if A) not flash or B) background larger than flash
		if (enableBackground && (m_backgroundViewport.Width > m_foregrountViewport.Width || !renderBar)) {
			Draw(m_pConstantBufferBackground.Get(), m_backgroundViewport);
		}
		if (renderBar) {
			Draw(m_pConstantBufferBar.Get(), m_foregrountViewport);
		}

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
		const auto scissors = GetScissorRects();
		m_foregrountViewport = MakeViewport<D3D11_VIEWPORT>(scissors.fg);
		m_backgroundViewport = MakeViewport<D3D11_VIEWPORT>(scissors.bg);
		m_rainbowViewport = MakeViewport<D3D11_VIEWPORT>(scissors.rb);
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