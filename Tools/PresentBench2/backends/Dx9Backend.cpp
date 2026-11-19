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
#include "../../../IntelPresentMon/CommonUtilities/log/Log.h"
#include <d3d9.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstring>
#include <format>
#include <stdexcept>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace pb2
{
	namespace
	{
		UINT ToDx9PresentInterval(uint32_t syncInterval)
		{
			switch (syncInterval) {
			case 0: return D3DPRESENT_INTERVAL_IMMEDIATE;
			case 1: return D3DPRESENT_INTERVAL_ONE;
			case 2: return D3DPRESENT_INTERVAL_TWO;
			case 3: return D3DPRESENT_INTERVAL_THREE;
			default: return D3DPRESENT_INTERVAL_FOUR;
			}
		}

		UINT FindDx9AdapterByLuid(IDirect3D9Ex* d3dEx, LUID target)
		{
			const UINT count = d3dEx->GetAdapterCount();
			for (UINT i = 0; i < count; ++i) {
				LUID luid{};
				if (SUCCEEDED(d3dEx->GetAdapterLUID(i, &luid)) &&
					luid.LowPart == target.LowPart && luid.HighPart == target.HighPart) {
					return i;
				}
			}
			throw std::runtime_error("No D3D9 adapter matches the selected DXGI LUID");
		}

		class Dx9Backend final : public IRenderBackend
		{
		public:
			Dx9Backend(HWND hwnd, const BenchConfig& config, uint32_t width, uint32_t height)
				:
				hwnd_(hwnd),
				config_(config),
				width_(width),
				height_(height)
			{
				ComPtr<IDXGIFactory1> factory;
				pmlog_hr << CreateDXGIFactory1(IID_PPV_ARGS(&factory));
				const auto selection = ResolveAdapter(config.adapter, factory.Get());

				ComPtr<IDirect3D9Ex> d3dEx;
				pmlog_hr << Direct3DCreate9Ex(D3D_SDK_VERSION, &d3dEx);
				const UINT adapterOrdinal = FindDx9AdapterByLuid(d3dEx.Get(), selection.luid);
				d3dEx.Reset();

				d3d_.Attach(Direct3DCreate9(D3D_SDK_VERSION));
				if (!d3d_) {
					throw std::runtime_error("Direct3DCreate9 failed");
				}

				pp_.Windowed = config.mode != WindowMode::Fullscreen ? TRUE : FALSE;
				pp_.SwapEffect = D3DSWAPEFFECT_DISCARD;
				pp_.BackBufferFormat = D3DFMT_UNKNOWN;
				pp_.BackBufferWidth = width_;
				pp_.BackBufferHeight = height_;
				pp_.BackBufferCount = (UINT)(config.swap.bufferCount > 0 ?
					config.swap.bufferCount - 1 : 0);
				pp_.hDeviceWindow = hwnd_;
				pp_.EnableAutoDepthStencil = TRUE;
				pp_.AutoDepthStencilFormat = D3DFMT_D24S8;
				pp_.PresentationInterval = ToDx9PresentInterval(config.present.syncInterval);
				if (config.mode == WindowMode::Fullscreen) {
					D3DDISPLAYMODE mode{};
					pmlog_hr << d3d_->GetAdapterDisplayMode(adapterOrdinal, &mode);
					pp_.FullScreen_RefreshRateInHz = mode.RefreshRate;
					pp_.BackBufferFormat = mode.Format;
				}

				pmlog_hr << d3d_->CreateDevice(adapterOrdinal, D3DDEVTYPE_HAL, hwnd_,
					D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp_, &device_);
				pmlog_info(std::format("D3D9 adapter ordinal {}", adapterOrdinal));

				CreateSceneResources_();
				CreateQueries_();
			}

			~Dx9Backend() override
			{
				for (auto& slot : slots_) {
					slot.query.Reset();
				}
				slots_.clear();
				ReleaseSceneResources_();
				device_.Reset();
			}

			bool PrepareFrame(bool windowActive) override
			{
				if (config_.mode != WindowMode::Fullscreen) {
					return true;
				}
				if (!windowActive) {
					return false;
				}
				const HRESULT hr = device_->TestCooperativeLevel();
				if (hr == D3D_OK) {
					return true;
				}
				if (hr == D3DERR_DEVICELOST) {
					return false;
				}
				if (hr == D3DERR_DEVICENOTRESET) {
					return ResetDevice_();
				}
				pmlog_hr << hr;
				return false;
			}

			void WaitForFrameSlot() override
			{
				auto& slot = slots_[slotIndex_];
				if (!slot.issued) {
					return;
				}
				HRESULT hr = S_FALSE;
				while ((hr = slot.query->GetData(nullptr, 0, D3DGETDATA_FLUSH)) == S_FALSE) {
					queryWaiter_.WaitUnbuffered(0.0001);
				}
				pmlog_hr << hr;
				slot.issued = false;
			}

			void Render(const FrameInfo& info) override
			{
				pmlog_hr << device_->BeginScene();
				const D3DCOLOR color = D3DCOLOR_COLORVALUE(kClearR, kClearG, kClearB, 1.f);
				for (uint32_t i = 0; i < info.clears; ++i) {
					device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.f, 0);
				}
				DrawCube_(info);
				device_->EndScene();
			}

			PresentResult Present(const FrameInfo&) override
			{
				const HRESULT hr = device_->Present(nullptr, nullptr, nullptr, nullptr);
				if (hr == D3DERR_DEVICELOST) {
					return { 0, false };
				}
				pmlog_hr << hr;
				slots_[slotIndex_].query->Issue(D3DISSUE_END);
				slots_[slotIndex_].issued = true;
				slotIndex_ = (slotIndex_ + 1) % slots_.size();
				return {};
			}

		private:
			struct FrameSlot
			{
				ComPtr<IDirect3DQuery9> query;
				bool issued = false;
			};

			void CreateQueries_()
			{
				slots_.clear();
				slots_.resize(config_.framesInFlight);
				for (auto& slot : slots_) {
					pmlog_hr << device_->CreateQuery(D3DQUERYTYPE_EVENT, &slot.query);
				}
			}

			bool ResetDevice_()
			{
				slots_.clear();
				ReleaseSceneResources_();
				const HRESULT hr = device_->Reset(&pp_);
				if (hr == D3DERR_DEVICELOST) {
					return false;
				}
				pmlog_hr << hr;
				CreateSceneResources_();
				CreateQueries_();
				slotIndex_ = 0;
				return true;
			}

			void CreateSceneResources_()
			{
				auto vsBlob = CompileHlsl(shaders::kHlslSm3Vs, "main", "vs_3_0");
				auto psBlob = CompileHlsl(shaders::kHlslSm3Ps, "main", "ps_3_0");
				pmlog_hr << device_->CreateVertexShader(
					(const DWORD*)vsBlob->GetBufferPointer(), &vs_);
				pmlog_hr << device_->CreatePixelShader(
					(const DWORD*)psBlob->GetBufferPointer(), &ps_);

				const D3DVERTEXELEMENT9 elems[] = {
					{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
					{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
					D3DDECL_END()
				};
				pmlog_hr << device_->CreateVertexDeclaration(elems, &decl_);

				const auto& verts = CubeVertices();
				const auto& indices = CubeIndices();
				pmlog_hr << device_->CreateVertexBuffer((UINT)(sizeof(CubeVertex) * verts.size()),
					D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vb_, nullptr);
				void* vbData = nullptr;
				pmlog_hr << vb_->Lock(0, 0, &vbData, 0);
				std::memcpy(vbData, verts.data(), sizeof(CubeVertex) * verts.size());
				vb_->Unlock();

				pmlog_hr << device_->CreateIndexBuffer((UINT)(sizeof(uint16_t) * indices.size()),
					D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib_, nullptr);
				void* ibData = nullptr;
				pmlog_hr << ib_->Lock(0, 0, &ibData, 0);
				std::memcpy(ibData, indices.data(), sizeof(uint16_t) * indices.size());
				ib_->Unlock();
			}

			void ReleaseSceneResources_()
			{
				ib_.Reset();
				vb_.Reset();
				decl_.Reset();
				ps_.Reset();
				vs_.Reset();
			}

			void DrawCube_(const FrameInfo& info)
			{
				const FrameConstants constants = BuildFrameConstants(info.sceneTimeSec,
					width_, height_, false);
				device_->SetVertexShaderConstantF(0, constants.worldViewProj, 4);
				device_->SetVertexShaderConstantF(4, constants.world, 4);
				device_->SetPixelShaderConstantF(0, constants.materialColor, 1);
				device_->SetPixelShaderConstantF(1, constants.ambientColor, 1);
				device_->SetPixelShaderConstantF(2, constants.light0Pos, 1);
				device_->SetPixelShaderConstantF(3, constants.light0Color, 1);
				device_->SetPixelShaderConstantF(4, constants.light1Pos, 1);
				device_->SetPixelShaderConstantF(5, constants.light1Color, 1);
				device_->SetRenderState(D3DRS_ZENABLE, TRUE);
				device_->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
				device_->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
				device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				device_->SetRenderState(D3DRS_LIGHTING, FALSE);
				device_->SetVertexDeclaration(decl_.Get());
				device_->SetVertexShader(vs_.Get());
				device_->SetPixelShader(ps_.Get());
				device_->SetStreamSource(0, vb_.Get(), 0, sizeof(CubeVertex));
				device_->SetIndices(ib_.Get());
				device_->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, kCubeVertexCount, 0,
					kCubeIndexCount / 3);
			}

			HWND hwnd_ = nullptr;
			BenchConfig config_{};
			uint32_t width_ = 0;
			uint32_t height_ = 0;
			pmon::util::PrecisionWaiter queryWaiter_{ 0. };
			ComPtr<IDirect3D9> d3d_;
			ComPtr<IDirect3DDevice9> device_;
			D3DPRESENT_PARAMETERS pp_{};
			ComPtr<IDirect3DVertexShader9> vs_;
			ComPtr<IDirect3DPixelShader9> ps_;
			ComPtr<IDirect3DVertexDeclaration9> decl_;
			ComPtr<IDirect3DVertexBuffer9> vb_;
			ComPtr<IDirect3DIndexBuffer9> ib_;
			std::vector<FrameSlot> slots_;
			size_t slotIndex_ = 0;
		};
	}

	std::unique_ptr<IRenderBackend> CreateDx9Backend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height)
	{
		return std::make_unique<Dx9Backend>(hwnd, config, width, height);
	}
}
