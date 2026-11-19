// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "../Backend.h"
#include "../CubeScene.h"
#include "../CubeShaders.h"
#include "../Dx12PresentComponent.h"
#include "DxgiCommon.h"
#include "HlslCompile.h"
#include "../../../IntelPresentMon/CommonUtilities/log/HrLogger.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstring>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace pb2
{
	namespace
	{
		class Dx12Backend final : public IRenderBackend
		{
		public:
			Dx12Backend(HWND hwnd, const BenchConfig& config, uint32_t width, uint32_t height)
				:
				hwnd_(hwnd),
				config_(config),
				width_(width),
				height_(height)
			{
				try {
				presentComponent_ = CreateDx12PresentComponent(config_.workload);
				UINT factoryFlags = 0;
				if (config.enableDebugLayer) {
					ComPtr<ID3D12Debug> debug;
					if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
						debug->EnableDebugLayer();
						factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
					}
				}
				pmlog_hr << CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_));
				const auto selection = ResolveAdapter(config.adapter, factory_.Get());
				pmlog_hr << D3D12CreateDevice(selection.adapter.Get(), D3D_FEATURE_LEVEL_11_0,
					IID_PPV_ARGS(&device_));

				D3D12_COMMAND_QUEUE_DESC qd{};
				qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
				pmlog_hr << device_->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue_));
				CreateSwapchain_();
				CreateFrameSlots_();
				pmlog_hr << device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
				fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
				if (!fenceEvent_) {
					throw std::runtime_error("CreateEventW failed");
				}
				CreateSceneResources_();
				}
				catch (...) {
					Cleanup_();
					throw;
				}
			}

			~Dx12Backend() override
			{
				Cleanup_();
			}

		private:
			void Cleanup_()
			{
				WaitGpuIdle_();
				ReleaseSceneResources_();
				if (fenceEvent_) {
					CloseHandle(fenceEvent_);
					fenceEvent_ = nullptr;
				}
				slots_.clear();
				ReleaseFrameTargets_();
				if (swapchain_ && config_.mode == WindowMode::Fullscreen) {
					swapchain_->SetFullscreenState(FALSE, nullptr);
				}
				swapchain_.Reset();
				presentComponent_.reset();
			}

		public:
			IFrameMarkerSink* GetMarkerSink() override
			{
				return presentComponent_ ? presentComponent_->GetMarkerSink() : nullptr;
			}
			bool PrepareFrame(bool windowActive) override
			{
				if (config_.mode != WindowMode::Fullscreen) {
					return true;
				}
				return PrepareExclusiveFullscreen(swapchain_.Get(), windowActive);
			}
			void WaitForFrameSlot() override
			{
				auto& slot = slots_[slotIndex_];
				if (slot.fenceValue == 0) {
					return;
				}
				if (fence_->GetCompletedValue() < slot.fenceValue) {
					fence_->SetEventOnCompletion(slot.fenceValue, fenceEvent_);
					WaitForSingleObject(fenceEvent_, INFINITE);
				}
			}

			void Render(const FrameInfo& info) override
			{
				auto& slot = slots_[slotIndex_];
				pmlog_hr << slot.allocator->Reset();
				pmlog_hr << list_->Reset(slot.allocator.Get(), pso_.Get());

				const UINT idx = swapchain_->GetCurrentBackBufferIndex();
				D3D12_RESOURCE_BARRIER startBarriers[3] = {
					Transition_(backbuffers_[idx].Get(),
						D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
					Transition_(velocityTargets_[idx].Get(),
						D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
						D3D12_RESOURCE_STATE_RENDER_TARGET),
					Transition_(depths_[idx].Get(),
						D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
						D3D12_RESOURCE_STATE_DEPTH_WRITE),
				};
				list_->ResourceBarrier(3, startBarriers);

				D3D12_CPU_DESCRIPTOR_HANDLE targets[] = { rtvs_[idx], velocityRtvs_[idx] };
				list_->OMSetRenderTargets(2, targets, FALSE, &dsvs_[idx]);
				D3D12_VIEWPORT vp{};
				vp.Width = (float)width_;
				vp.Height = (float)height_;
				vp.MaxDepth = 1.f;
				list_->RSSetViewports(1, &vp);
				D3D12_RECT scissor{ 0, 0, (LONG)width_, (LONG)height_ };
				list_->RSSetScissorRects(1, &scissor);

				const float color[4] = { kClearR, kClearG, kClearB, 1.f };
				for (uint32_t i = 0; i < info.clears; ++i) {
					list_->ClearRenderTargetView(rtvs_[idx], color, 0, nullptr);
				}
				const float zeroMotion[4]{};
				list_->ClearRenderTargetView(velocityRtvs_[idx], zeroMotion, 0, nullptr);
				list_->ClearDepthStencilView(dsvs_[idx], D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

				for (uint32_t i = 0; i < info.psoCompiles; ++i) {
					CompileDummyPso_();
				}
				DrawCube_(info, slot);
				pendingBackbufferIndex_ = idx;
				D3D12_RESOURCE_BARRIER endBarriers[3] = {
					Transition_(backbuffers_[idx].Get(),
						D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
					Transition_(velocityTargets_[idx].Get(),
						D3D12_RESOURCE_STATE_RENDER_TARGET,
						D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
					Transition_(depths_[idx].Get(),
						D3D12_RESOURCE_STATE_DEPTH_WRITE,
						D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
				};
				list_->ResourceBarrier(3, endBarriers);
				list_->Close();
				ID3D12CommandList* lists[] = { list_.Get() };
				queue_->ExecuteCommandLists(1, lists);
			}

			PresentResult Present(const FrameInfo& info) override
			{
				const UINT idx = pendingBackbufferIndex_;
				Dx12FrameGenerationInputs inputs{};
				inputs.presentId = info.frameId;
				inputs.width = width_;
				inputs.height = height_;
				inputs.depth = depths_[idx].Get();
				inputs.motionVectors = velocityTargets_[idx].Get();
				inputs.sceneTimeSec = info.sceneTimeSec;
				BuildFrameGenerationMatrices_(inputs);
				presentComponent_->PreparePresent(inputs);

				pmlog_hr << swapchain_->Present(config_.present.syncInterval, config_.present.flags);
				presentComponent_->CompletePresent(info.frameId);
				const auto result = presentComponent_->GetLastPresentResult();

				previousSceneTimeSec_ = info.sceneTimeSec;
				havePreviousFrame_ = true;

				const UINT64 fenceValue = ++fenceValue_;
				queue_->Signal(fence_.Get(), fenceValue);
				slots_[slotIndex_].fenceValue = fenceValue;
				slotIndex_ = (slotIndex_ + 1) % slots_.size();
				return result;
			}

		private:
			struct FrameSlot
			{
				ComPtr<ID3D12CommandAllocator> allocator;
				ComPtr<ID3D12Resource> cb;
				UINT64 fenceValue = 0;
			};

			static D3D12_RESOURCE_BARRIER Transition_(ID3D12Resource* resource,
				D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
			{
				D3D12_RESOURCE_BARRIER barrier{};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = resource;
				barrier.Transition.StateBefore = before;
				barrier.Transition.StateAfter = after;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				return barrier;
			}

			void CreateSwapchain_()
			{
				Dx12PresentCreateInfo createInfo{};
				createInfo.hwnd = hwnd_;
				createInfo.device = device_.Get();
				createInfo.queue = queue_.Get();
				createInfo.factory = factory_.Get();
				createInfo.swapChainDesc = MakeSwapChainDesc1(width_, height_,
					config_.swap, config_.present);
				createInfo.fullscreenDesc = MakeFullscreenDesc(config_.mode);
				createInfo.exclusiveFullscreen = (config_.mode == WindowMode::Fullscreen);
				presentComponent_->CreateSwapChain(createInfo,
					swapchain_.ReleaseAndGetAddressOf());
				factory_->MakeWindowAssociation(hwnd_, DXGI_MWA_NO_ALT_ENTER);
				CreateFrameTargets_();
			}

			void CreateFrameSlots_()
			{
				slots_.resize(config_.framesInFlight);
				for (auto& slot : slots_) {
					pmlog_hr << device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
						IID_PPV_ARGS(&slot.allocator));
				}
				pmlog_hr << device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
					slots_[0].allocator.Get(), nullptr, IID_PPV_ARGS(&list_));
				list_->Close();
			}

			void CreateFrameTargets_()
			{
				const UINT bufferCount = config_.swap.bufferCount;
				D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
				heapDesc.NumDescriptors = bufferCount * 2u;
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				pmlog_hr << device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap_));
				const UINT stride = device_->GetDescriptorHandleIncrementSize(
					D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
				backbuffers_.resize(bufferCount);
				rtvs_.resize(bufferCount);
				for (UINT i = 0; i < bufferCount; ++i) {
					pmlog_hr << swapchain_->GetBuffer(i, IID_PPV_ARGS(&backbuffers_[i]));
					device_->CreateRenderTargetView(backbuffers_[i].Get(), nullptr, handle);
					rtvs_[i] = handle;
					handle.ptr += stride;
				}

				velocityTargets_.resize(bufferCount);
				velocityRtvs_.resize(bufferCount);
				for (UINT i = 0; i < bufferCount; ++i) {
					D3D12_RESOURCE_DESC velocityDesc{};
					velocityDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					velocityDesc.Width = width_;
					velocityDesc.Height = height_;
					velocityDesc.DepthOrArraySize = 1;
					velocityDesc.MipLevels = 1;
					velocityDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
					velocityDesc.SampleDesc.Count = 1;
					velocityDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
					D3D12_CLEAR_VALUE clear{};
					clear.Format = velocityDesc.Format;
					D3D12_HEAP_PROPERTIES heap{};
					heap.Type = D3D12_HEAP_TYPE_DEFAULT;
					pmlog_hr << device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
						&velocityDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
						&clear, IID_PPV_ARGS(&velocityTargets_[i]));
					device_->CreateRenderTargetView(velocityTargets_[i].Get(), nullptr, handle);
					velocityRtvs_[i] = handle;
					handle.ptr += stride;
				}

				D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
				dsvHeapDesc.NumDescriptors = bufferCount;
				dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				pmlog_hr << device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_));
				const UINT dsvStride = device_->GetDescriptorHandleIncrementSize(
					D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
				D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
					dsvHeap_->GetCPUDescriptorHandleForHeapStart();
				depths_.resize(bufferCount);
				dsvs_.resize(bufferCount);
				for (UINT i = 0; i < bufferCount; ++i) {
					D3D12_RESOURCE_DESC depthDesc{};
					depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					depthDesc.Width = width_;
					depthDesc.Height = height_;
					depthDesc.DepthOrArraySize = 1;
					depthDesc.MipLevels = 1;
					depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
					depthDesc.SampleDesc.Count = 1;
					depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
					D3D12_CLEAR_VALUE clear{};
					clear.Format = DXGI_FORMAT_D32_FLOAT;
					clear.DepthStencil.Depth = 1.f;
					D3D12_HEAP_PROPERTIES heap{};
					heap.Type = D3D12_HEAP_TYPE_DEFAULT;
					pmlog_hr << device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
						&depthDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
						&clear, IID_PPV_ARGS(&depths_[i]));
					D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
					dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
					device_->CreateDepthStencilView(depths_[i].Get(), &dsvDesc, dsvHandle);
					dsvs_[i] = dsvHandle;
					dsvHandle.ptr += dsvStride;
				}
			}

			void ReleaseFrameTargets_()
			{
				depths_.clear();
				velocityTargets_.clear();
				dsvHeap_.Reset();
				dsvs_.clear();
				rtvs_.clear();
				velocityRtvs_.clear();
				backbuffers_.clear();
				rtvHeap_.Reset();
			}

			void BuildFrameGenerationMatrices_(Dx12FrameGenerationInputs& inputs) const
			{
				const CameraMatrices cam = BuildCamera(width_, height_, false);
				MatTranspose(cam.view, inputs.viewMatrix);
				MatTranspose(cam.projection, inputs.projectionMatrix);
			}

			void CreateSceneResources_()
			{
				D3D12_ROOT_PARAMETER rootParam{};
				rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				rootParam.Descriptor.ShaderRegister = 0;
				rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				D3D12_ROOT_SIGNATURE_DESC rsDesc{};
				rsDesc.NumParameters = 1;
				rsDesc.pParameters = &rootParam;
				rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
				ComPtr<ID3DBlob> rsBlob;
				ComPtr<ID3DBlob> rsErr;
				pmlog_hr << D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
					&rsBlob, &rsErr);
				pmlog_hr << device_->CreateRootSignature(0, rsBlob->GetBufferPointer(),
					rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig_));

				const D3D_SHADER_MACRO macros[] = {
					{ "MOTION_VECTORS", "1" },
					{ nullptr, nullptr },
				};
				vsBlob_ = CompileHlsl(shaders::kHlslSm5, "VSMain", "vs_5_0", macros);
				psBlob_ = CompileHlsl(shaders::kHlslSm5, "PSMain", "ps_5_0", macros);

				D3D12_INPUT_ELEMENT_DESC inputElems[] = {
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				};

				D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
				psoDesc.pRootSignature = rootSig_.Get();
				psoDesc.VS = { vsBlob_->GetBufferPointer(), vsBlob_->GetBufferSize() };
				psoDesc.PS = { psBlob_->GetBufferPointer(), psBlob_->GetBufferSize() };
				psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
				psoDesc.SampleMask = UINT_MAX;
				psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
				psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
				psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
				psoDesc.RasterizerState.DepthClipEnable = TRUE;
				psoDesc.DepthStencilState.DepthEnable = TRUE;
				psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				psoDesc.InputLayout = { inputElems, 2 };
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				psoDesc.NumRenderTargets = 2;
				psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
				psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16_FLOAT;
				psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
				psoDesc.SampleDesc.Count = 1;
				pmlog_hr << device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));

				const auto& verts = CubeVertices();
				const auto& indices = CubeIndices();
				const size_t vbSize = sizeof(CubeVertex) * verts.size();
				const size_t ibSize = sizeof(uint16_t) * indices.size();
				CreateUploadBuffer_(vbSize, vb_);
				CreateUploadBuffer_(ibSize, ib_);
				void* mapped = nullptr;
				vb_->Map(0, nullptr, &mapped);
				std::memcpy(mapped, verts.data(), vbSize);
				vb_->Unmap(0, nullptr);
				ib_->Map(0, nullptr, &mapped);
				std::memcpy(mapped, indices.data(), ibSize);
				ib_->Unmap(0, nullptr);

				vbView_.BufferLocation = vb_->GetGPUVirtualAddress();
				vbView_.SizeInBytes = (UINT)vbSize;
				vbView_.StrideInBytes = sizeof(CubeVertex);
				ibView_.BufferLocation = ib_->GetGPUVirtualAddress();
				ibView_.SizeInBytes = (UINT)ibSize;
				ibView_.Format = DXGI_FORMAT_R16_UINT;

				const size_t cbDataSize = sizeof(FrameConstants) + sizeof(float) * 16u;
				const size_t cbSize = (cbDataSize + 255u) & ~255u;
				for (auto& slot : slots_) {
					CreateUploadBuffer_(cbSize, slot.cb);
				}
			}

			void CreateUploadBuffer_(size_t size, ComPtr<ID3D12Resource>& out)
			{
				D3D12_HEAP_PROPERTIES heap{};
				heap.Type = D3D12_HEAP_TYPE_UPLOAD;
				D3D12_RESOURCE_DESC desc{};
				desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				desc.Width = size;
				desc.Height = 1;
				desc.DepthOrArraySize = 1;
				desc.MipLevels = 1;
				desc.SampleDesc.Count = 1;
				desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				pmlog_hr << device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
					D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&out));
			}

			void ReleaseSceneResources_()
			{
				for (auto& slot : slots_) {
					slot.cb.Reset();
				}
				ib_.Reset();
				vb_.Reset();
				pso_.Reset();
				rootSig_.Reset();
				psBlob_.Reset();
				vsBlob_.Reset();
			}

			void DrawCube_(const FrameInfo& info, FrameSlot& slot)
			{
				const FrameConstants constants = BuildFrameConstants(info.sceneTimeSec,
					width_, height_, false);
				void* mapped = nullptr;
				slot.cb->Map(0, nullptr, &mapped);
				std::memcpy(mapped, &constants, sizeof(constants));
				const float previousTime = havePreviousFrame_ ?
					previousSceneTimeSec_ : info.sceneTimeSec;
				const FrameConstants previous = BuildFrameConstants(previousTime,
					width_, height_, false);
				std::memcpy((char*)mapped + sizeof(constants), previous.worldViewProj,
					sizeof(previous.worldViewProj));
				slot.cb->Unmap(0, nullptr);

				list_->SetGraphicsRootSignature(rootSig_.Get());
				list_->SetPipelineState(pso_.Get());
				list_->SetGraphicsRootConstantBufferView(0, slot.cb->GetGPUVirtualAddress());
				list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				list_->IASetVertexBuffers(0, 1, &vbView_);
				list_->IASetIndexBuffer(&ibView_);
				list_->DrawIndexedInstanced(kCubeIndexCount, 1, 0, 0, 0);
			}

			void WaitGpuIdle_()
			{
				if (!queue_ || !fence_ || !fenceEvent_) {
					return;
				}
				const UINT64 value = ++fenceValue_;
				queue_->Signal(fence_.Get(), value);
				if (fence_->GetCompletedValue() < value) {
					fence_->SetEventOnCompletion(value, fenceEvent_);
					WaitForSingleObject(fenceEvent_, INFINITE);
				}
			}

			void CompileDummyPso_()
			{
				D3D12_INPUT_ELEMENT_DESC inputElems[] = {
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				};
				D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
				psoDesc.pRootSignature = rootSig_.Get();
				psoDesc.VS = { vsBlob_->GetBufferPointer(), vsBlob_->GetBufferSize() };
				psoDesc.PS = { psBlob_->GetBufferPointer(), psBlob_->GetBufferSize() };
				psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
					D3D12_COLOR_WRITE_ENABLE_ALL;
				psoDesc.SampleMask = UINT_MAX;
				psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
				psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
				psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
				psoDesc.RasterizerState.DepthBias = (INT)++psoCompileSerial_;
				psoDesc.RasterizerState.DepthClipEnable = TRUE;
				psoDesc.DepthStencilState.DepthEnable = TRUE;
				psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				psoDesc.InputLayout = { inputElems, 2 };
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				psoDesc.NumRenderTargets = 2;
				psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
				psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16_FLOAT;
				psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
				psoDesc.SampleDesc.Count = 1;
				ComPtr<ID3D12PipelineState> pso;
				pmlog_hr << device_->CreateGraphicsPipelineState(
					&psoDesc, IID_PPV_ARGS(&pso));
			}

			HWND hwnd_ = nullptr;
			BenchConfig config_{};
			uint32_t width_ = 0;
			uint32_t height_ = 0;
			ComPtr<IDXGIFactory4> factory_;
			ComPtr<ID3D12Device> device_;
			ComPtr<ID3D12CommandQueue> queue_;
			ComPtr<IDXGISwapChain3> swapchain_;
			ComPtr<ID3D12DescriptorHeap> rtvHeap_;
			ComPtr<ID3D12DescriptorHeap> dsvHeap_;
			ComPtr<ID3D12GraphicsCommandList> list_;
			ComPtr<ID3D12Fence> fence_;
			HANDLE fenceEvent_ = nullptr;
			UINT64 fenceValue_ = 0;
			std::vector<FrameSlot> slots_;
			size_t slotIndex_ = 0;
			UINT pendingBackbufferIndex_ = 0;
			std::vector<ComPtr<ID3D12Resource>> backbuffers_;
			std::vector<ComPtr<ID3D12Resource>> velocityTargets_;
			std::vector<ComPtr<ID3D12Resource>> depths_;
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs_;
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> velocityRtvs_;
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvs_;
			ComPtr<ID3D12RootSignature> rootSig_;
			ComPtr<ID3D12PipelineState> pso_;
			ComPtr<ID3D12Resource> vb_;
			ComPtr<ID3D12Resource> ib_;
			ComPtr<ID3DBlob> vsBlob_;
			ComPtr<ID3DBlob> psBlob_;
			D3D12_VERTEX_BUFFER_VIEW vbView_{};
			D3D12_INDEX_BUFFER_VIEW ibView_{};
			uint32_t psoCompileSerial_ = 0;
			bool havePreviousFrame_ = false;
			float previousSceneTimeSec_ = 0.f;
			std::unique_ptr<IDx12PresentComponent> presentComponent_;
		};
	}

	std::unique_ptr<IRenderBackend> CreateDx12Backend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height)
	{
		return std::make_unique<Dx12Backend>(hwnd, config, width, height);
	}
}
