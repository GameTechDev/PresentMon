#include "OverlayRenderer_D3D10.h"
#include <dxgi1_6.h>
#include "Quad.h"

namespace GfxLayer::Extension
{
	HRESULT OverlayRenderer_D3D10::CreateBuffer_(UINT byteWidth,
		D3D10_USAGE usage,
		UINT bindFlags,
		UINT cpuAccessFlags,
		const void* initData,
		ID3D10Buffer** ppBuffer)
	{
		D3D10_BUFFER_DESC desc{};
		desc.Usage = usage;
		desc.ByteWidth = byteWidth;
		desc.BindFlags = bindFlags;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.MiscFlags = 0;

		D3D10_SUBRESOURCE_DATA srd{};
		const D3D10_SUBRESOURCE_DATA* pSRD = nullptr;
		if (initData) { srd.pSysMem = initData; pSRD = &srd; }

		return m_pDevice->CreateBuffer(&desc, pSRD, ppBuffer);
	}

	HRESULT OverlayRenderer_D3D10::CreateConstantBuffer_(const void* data,
		UINT dataSizeBytes,
		ID3D10Buffer** ppBuffer)
	{
		// Round up to 16-byte multiple
		UINT cbSize = (dataSizeBytes + 15u) & ~15u;
		return CreateBuffer_(cbSize, D3D10_USAGE_DEFAULT, D3D10_BIND_CONSTANT_BUFFER, 0, data, ppBuffer);
	}

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
			auto hr = CreateConstantBuffer_(config.BackgroundColor.data(),
				sizeof(config.BackgroundColor), &m_pConstantBufferBackground);
			CheckResult(hr, "D3D10 - Failed to create ID3D10Buffer (Background Constant Buffer)");
		}
		// flash
		{
			auto hr = CreateConstantBuffer_(config.BarColor.data(),
				sizeof(config.BarColor), &m_pConstantBufferBar);
			CheckResult(hr, "D3D10 - Failed to create ID3D10Buffer (Background Constant Buffer)");
		}
		// rainbow
		if (m_rainbowConstantBufferPtrs.empty()) {
			for (auto& color : GetRainbowColors()) {
				ComPtr<ID3D10Buffer> cb;
				auto hr = CreateConstantBuffer_(color.data(), sizeof(color), &cb);
				CheckResult(hr, "D3D10 - Failed to create ID3D10Buffer (Rainbow Constant Buffer)");
				m_rainbowConstantBufferPtrs.push_back(std::move(cb));
			}
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

	void OverlayRenderer_D3D10::Render(bool renderBar, bool useRainbow, bool enableBackground)
	{
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

		// set common state
		m_pDevice->ClearState();
		m_pDevice->OMSetRenderTargets(1, &pRtv, nullptr);
		m_pDevice->IASetInputLayout(m_pVertexLayout.Get());
		m_pDevice->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		m_pDevice->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pDevice->VSSetShader(m_pVertexShader.Get());
		m_pDevice->PSSetShader(m_pPixelShader.Get());
		// issue fg/bg draw calls
		const auto Draw = [&](ID3D10Buffer* pConstantBuffer, const D3D10_VIEWPORT& vp) {
			m_pDevice->RSSetViewports(1, &vp);
			m_pDevice->PSSetConstantBuffers(0, 1, &pConstantBuffer);
			m_pDevice->DrawIndexed(6, 0, 0);
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

		// Restore the previous state

		m_pStateBlock->Apply();
	}

	void OverlayRenderer_D3D10::UpdateViewport(const OverlayConfig& cfg)
	{
		const auto scissors = GetScissorRects();
		m_foregrountViewport = MakeViewport<D3D10_VIEWPORT>(scissors.fg);
		m_backgroundViewport = MakeViewport<D3D10_VIEWPORT>(scissors.bg);
		m_rainbowViewport = MakeViewport<D3D10_VIEWPORT>(scissors.rb);
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