#include "OverlayRenderer_D3D12.h"
#include <dxgi1_6.h>

namespace GfxLayer::Extension
{
	OverlayRenderer_D3D12::OverlayRenderer_D3D12(const OverlayConfig& cfg, IDXGISwapChain3* pSwapChain, ID3D12CommandQueue* pCmdQueue):
		OverlayRenderer(cfg, pSwapChain),
		m_pCmdQueue(pCmdQueue),
		m_config{ cfg }
	{
		m_pCmdQueue->GetDevice(IID_PPV_ARGS(&m_pDevice));
		m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	void OverlayRenderer_D3D12::Resize(unsigned bufferCount, unsigned width, unsigned height)
	{
		// Preserve the existing number of buffers in the swap chain
		//     https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
		//     BufferCount: Set this number to zero to preserve the existing number of buffers in the swap chain.
		static unsigned s_BufferCount = 0;
		if (bufferCount > 0)
		{
			s_BufferCount = bufferCount;
		}

		OverlayRenderer::Resize(bufferCount, width, height);

		// Create RTV descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = s_BufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		auto hr = m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap));
		CheckResult(hr, "Failed to create ID3D12DescriptorHeap (RTVs)");

		// Create an resources needed for each buffer in the swap chain
		m_CmdAllocators.resize(s_BufferCount);
		m_CmdLists.resize(s_BufferCount);
		for (unsigned i = 0; i < s_BufferCount; ++i)
		{
			ComPtr<ID3D12Resource> pBackBuffer;
			GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));

			// Create RTV
			auto rtvHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
			rtvHandle.ptr = rtvHandle.ptr + (m_RtvDescriptorSize * i);
			m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, rtvHandle);

			// Create CommandAllocator
			hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CmdAllocators[i]));
			CheckResult(hr, "Failed to create ID3D12CommandAllocator");

			// Create (and Close) CommandList
			hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CmdAllocators[i].Get(), nullptr, IID_PPV_ARGS(&m_CmdLists[i]));
			CheckResult(hr, "Failed to create ID3D12GraphicsCommandList");
			m_CmdLists[i]->Close();
		}
	}

	void OverlayRenderer_D3D12::Render(bool renderBar, bool useRainbow, bool enableBackground)
	{
		const float* pColor = nullptr;
		if (renderBar) {
			if (useRainbow) {
				pColor = GetRainbowColors().at(GetRainbowIndex()).data();
			}
			else {
				pColor = m_config.BarColor.data();
			}
		}
		else {
			pColor = m_config.BackgroundColor.data();
		}


		auto backBufferIdx = GetSwapChain()->GetCurrentBackBufferIndex();
		auto rtvHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr = rtvHandle.ptr + (m_RtvDescriptorSize * backBufferIdx);

		auto* pCmdAllocator = m_CmdAllocators[backBufferIdx].Get();
		auto* pCmdList = m_CmdLists[backBufferIdx].Get();
		pCmdAllocator->Reset();
		pCmdList->Reset(pCmdAllocator, nullptr);
		const auto scissors = GetScissorRects();
		const auto bgWidth = scissors.bg.right - scissors.bg.left;
		const auto fgWidth = scissors.fg.right - scissors.fg.left;
		if (useRainbow) {
			pCmdList->ClearRenderTargetView(rtvHandle, GetRainbowColors().at(GetRainbowIndex()).data(), 1, &scissors.rb);
		}
		// draw background before flash bar if enabled, but only if A) not flash or B) background larger than flash
		if (enableBackground && (bgWidth > fgWidth || !renderBar)) {
			pCmdList->ClearRenderTargetView(rtvHandle, m_config.BackgroundColor.data(), 1, &scissors.bg);
		}
		if (renderBar) {
			pCmdList->ClearRenderTargetView(rtvHandle, m_config.BarColor.data(), 1, &scissors.fg);
		}
		pCmdList->Close();

		ID3D12CommandList* pCommandLists[] = { pCmdList };
		m_pCmdQueue->ExecuteCommandLists(_countof(pCommandLists), pCommandLists);
	}

	void OverlayRenderer_D3D12::UpdateViewport(const OverlayConfig& cfg)
	{}

	void OverlayRenderer_D3D12::UpdateConfig(const OverlayConfig& cfg)
	{
		m_config = cfg;
	}
}