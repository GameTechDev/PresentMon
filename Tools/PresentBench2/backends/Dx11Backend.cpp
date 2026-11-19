// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "../Backend.h"
#include "../CubeScene.h"
#include "../CubeShaders.h"
#include "DxgiCommon.h"
#include "HlslCompile.h"
#include "../../../IntelPresentMon/CommonUtilities/PrecisionWaiter.h"
#include "../../../IntelPresentMon/CommonUtilities/log/HrLogger.h"
#include <d3d11.h>
#include <dxgi1_5.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace pb2
{
	namespace
	{
		class Dx11Backend final : public IRenderBackend
		{
		public:
			Dx11Backend(HWND hwnd, const BenchConfig& config, uint32_t width, uint32_t height)
				:
				hwnd_(hwnd),
				config_(config),
				width_(width),
				height_(height)
			{
				UINT flags = config.enableDebugLayer ? D3D11_CREATE_DEVICE_DEBUG : 0;
				const std::array levels{ D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
				D3D_FEATURE_LEVEL got{};
				ComPtr<IDXGIFactory1> factory;
				pmlog_hr << CreateDXGIFactory1(IID_PPV_ARGS(&factory));
				const auto selection = ResolveAdapter(config.adapter, factory.Get());
				pmlog_hr << D3D11CreateDevice(selection.adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN,
					nullptr, flags, levels.data(), (UINT)levels.size(), D3D11_SDK_VERSION,
					&device_, &got, &context_);
				CreateSwapchain_();
				CreateSceneResources_();
				CreateQueries_();
			}

			~Dx11Backend() override
			{
				for (auto& slot : slots_) {
					slot.query.Reset();
				}
				slots_.clear();
				ReleaseSceneResources_();
				dsv_.Reset();
				depth_.Reset();
				rtv_.Reset();
				if (swapchain_ && config_.mode == WindowMode::Fullscreen) {
					swapchain_->SetFullscreenState(FALSE, nullptr);
				}
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
				if (!slot.issued) {
					return;
				}
				BOOL done = FALSE;
				HRESULT hr = S_FALSE;
				while ((hr = context_->GetData(
					slot.query.Get(), &done, sizeof(done), 0)) == S_FALSE) {
					queryWaiter_.WaitUnbuffered(0.0001);
				}
				pmlog_hr << hr;
				slot.issued = false;
			}

			void Render(const FrameInfo& info) override
			{
				const float color[4] = { kClearR, kClearG, kClearB, 1.f };
				for (uint32_t i = 0; i < info.clears; ++i) {
					context_->ClearRenderTargetView(rtv_.Get(), color);
				}
				context_->ClearDepthStencilView(dsv_.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
				DrawCube_(info);
			}

			PresentResult Present(const FrameInfo&) override
			{
				pmlog_hr << swapchain_->Present(config_.present.syncInterval, config_.present.flags);
				context_->End(slots_[slotIndex_].query.Get());
				slots_[slotIndex_].issued = true;
				slotIndex_ = (slotIndex_ + 1) % slots_.size();
				return {};
			}

		private:
			struct FrameSlot
			{
				ComPtr<ID3D11Query> query;
				bool issued = false;
			};

			void CreateQueries_()
			{
				slots_.resize(config_.framesInFlight);
				D3D11_QUERY_DESC qd{};
				qd.Query = D3D11_QUERY_EVENT;
				for (auto& slot : slots_) {
					pmlog_hr << device_->CreateQuery(&qd, &slot.query);
				}
			}

			void CreateSwapchain_()
			{
				ComPtr<IDXGIDevice> dxgiDevice;
				device_.As(&dxgiDevice);
				ComPtr<IDXGIAdapter> adapter;
				dxgiDevice->GetAdapter(&adapter);
				ComPtr<IDXGIFactory2> factory;
				adapter->GetParent(IID_PPV_ARGS(&factory));

				const auto desc = MakeSwapChainDesc1(width_, height_,
					config_.swap, config_.present);
				const auto fsDesc = MakeFullscreenDesc(config_.mode);
				const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fsPtr =
					(config_.mode == WindowMode::Fullscreen) ? &fsDesc : nullptr;
				pmlog_hr << factory->CreateSwapChainForHwnd(device_.Get(), hwnd_, &desc, fsPtr,
					nullptr, &swapchain_);
				factory->MakeWindowAssociation(hwnd_, DXGI_MWA_NO_ALT_ENTER);

				ComPtr<ID3D11Texture2D> backbuffer;
				pmlog_hr << swapchain_->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
				pmlog_hr << device_->CreateRenderTargetView(backbuffer.Get(), nullptr, &rtv_);

				D3D11_TEXTURE2D_DESC depthDesc{};
				depthDesc.Width = width_;
				depthDesc.Height = height_;
				depthDesc.MipLevels = 1;
				depthDesc.ArraySize = 1;
				depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
				depthDesc.SampleDesc.Count = 1;
				depthDesc.Usage = D3D11_USAGE_DEFAULT;
				depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				pmlog_hr << device_->CreateTexture2D(&depthDesc, nullptr, &depth_);
				pmlog_hr << device_->CreateDepthStencilView(depth_.Get(), nullptr, &dsv_);

				context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), dsv_.Get());
				D3D11_VIEWPORT vp{};
				vp.Width = (float)width_;
				vp.Height = (float)height_;
				vp.MaxDepth = 1.f;
				context_->RSSetViewports(1, &vp);
			}

			void CreateSceneResources_()
			{
				auto vsBlob = CompileHlsl(shaders::kHlslSm5, "VSMain", "vs_5_0");
				auto psBlob = CompileHlsl(shaders::kHlslSm5, "PSMain", "ps_5_0");
				pmlog_hr << device_->CreateVertexShader(vsBlob->GetBufferPointer(),
					vsBlob->GetBufferSize(), nullptr, &vs_);
				pmlog_hr << device_->CreatePixelShader(psBlob->GetBufferPointer(),
					psBlob->GetBufferSize(), nullptr, &ps_);

				const D3D11_INPUT_ELEMENT_DESC layout[] = {
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
						D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
						D3D11_INPUT_PER_VERTEX_DATA, 0 },
				};
				pmlog_hr << device_->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(),
					vsBlob->GetBufferSize(), &inputLayout_);

				const auto& verts = CubeVertices();
				const auto& indices = CubeIndices();
				D3D11_BUFFER_DESC vbDesc{};
				vbDesc.ByteWidth = (UINT)(sizeof(CubeVertex) * verts.size());
				vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
				vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				D3D11_SUBRESOURCE_DATA vbData{};
				vbData.pSysMem = verts.data();
				pmlog_hr << device_->CreateBuffer(&vbDesc, &vbData, &vb_);

				D3D11_BUFFER_DESC ibDesc{};
				ibDesc.ByteWidth = (UINT)(sizeof(uint16_t) * indices.size());
				ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
				ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				D3D11_SUBRESOURCE_DATA ibData{};
				ibData.pSysMem = indices.data();
				pmlog_hr << device_->CreateBuffer(&ibDesc, &ibData, &ib_);

				D3D11_BUFFER_DESC cbDesc{};
				cbDesc.ByteWidth = sizeof(FrameConstants);
				cbDesc.Usage = D3D11_USAGE_DYNAMIC;
				cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				pmlog_hr << device_->CreateBuffer(&cbDesc, nullptr, &cb_);

				D3D11_DEPTH_STENCIL_DESC dsDesc{};
				dsDesc.DepthEnable = TRUE;
				dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
				dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
				pmlog_hr << device_->CreateDepthStencilState(&dsDesc, &dss_);

				D3D11_RASTERIZER_DESC rsDesc{};
				rsDesc.FillMode = D3D11_FILL_SOLID;
				rsDesc.CullMode = D3D11_CULL_BACK;
				rsDesc.FrontCounterClockwise = FALSE;
				rsDesc.DepthClipEnable = TRUE;
				pmlog_hr << device_->CreateRasterizerState(&rsDesc, &rs_);
			}

			void ReleaseSceneResources_()
			{
				rs_.Reset();
				dss_.Reset();
				cb_.Reset();
				ib_.Reset();
				vb_.Reset();
				inputLayout_.Reset();
				ps_.Reset();
				vs_.Reset();
			}

			void DrawCube_(const FrameInfo& info)
			{
				const FrameConstants constants = BuildFrameConstants(info.sceneTimeSec,
					width_, height_, false);
				D3D11_MAPPED_SUBRESOURCE mapped{};
				pmlog_hr << context_->Map(cb_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
				std::memcpy(mapped.pData, &constants, sizeof(constants));
				context_->Unmap(cb_.Get(), 0);

				const UINT stride = sizeof(CubeVertex);
				const UINT offset = 0;
				context_->IASetInputLayout(inputLayout_.Get());
				context_->IASetVertexBuffers(0, 1, vb_.GetAddressOf(), &stride, &offset);
				context_->IASetIndexBuffer(ib_.Get(), DXGI_FORMAT_R16_UINT, 0);
				context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				context_->VSSetShader(vs_.Get(), nullptr, 0);
				context_->PSSetShader(ps_.Get(), nullptr, 0);
				context_->VSSetConstantBuffers(0, 1, cb_.GetAddressOf());
				context_->PSSetConstantBuffers(0, 1, cb_.GetAddressOf());
				context_->OMSetDepthStencilState(dss_.Get(), 0);
				context_->RSSetState(rs_.Get());
				context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), dsv_.Get());
				context_->DrawIndexed(kCubeIndexCount, 0, 0);
			}

			HWND hwnd_ = nullptr;
			BenchConfig config_{};
			uint32_t width_ = 0;
			uint32_t height_ = 0;
			pmon::util::PrecisionWaiter queryWaiter_{ 0. };
			ComPtr<ID3D11Device> device_;
			ComPtr<ID3D11DeviceContext> context_;
			ComPtr<IDXGISwapChain1> swapchain_;
			ComPtr<ID3D11RenderTargetView> rtv_;
			ComPtr<ID3D11Texture2D> depth_;
			ComPtr<ID3D11DepthStencilView> dsv_;
			ComPtr<ID3D11VertexShader> vs_;
			ComPtr<ID3D11PixelShader> ps_;
			ComPtr<ID3D11InputLayout> inputLayout_;
			ComPtr<ID3D11Buffer> vb_;
			ComPtr<ID3D11Buffer> ib_;
			ComPtr<ID3D11Buffer> cb_;
			ComPtr<ID3D11DepthStencilState> dss_;
			ComPtr<ID3D11RasterizerState> rs_;
			std::vector<FrameSlot> slots_;
			size_t slotIndex_ = 0;
		};
	}

	std::unique_ptr<IRenderBackend> CreateDx11Backend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height)
	{
		return std::make_unique<Dx11Backend>(hwnd, config, width, height);
	}
}
