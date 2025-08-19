// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/WinAPI.h>
#include <CommonUtilities/win/com/ComPtr.h>
#include <Core/source/gfx/base/InternalGeometry.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <dcomp.h>
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <dwrite.h>
#include <memory>
#include <string_view>
#include "impl/FastRenderer.h"

// TODO: after comptr is self-implemented, forward-decl Ifaces and stop including winapi stuff


namespace p2c::gfx
{
    using ::pmon::util::win::com::ComPtr;

    namespace prim
    {
        class RetainedPrimitive;
    }

    class Graphics
    {
        friend prim::RetainedPrimitive;
    public:
        Graphics(HWND hWnd, DimensionsI dimensions, float upscaleFactor, bool enableTearing, bool enableAlpha);
        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics&) = delete;
        ~Graphics();
        void BeginFrame();
        void EndFrame();
        const DimensionsI& GetDimensions() const;
        void Resize(DimensionsI dimensions);
        // fastline interface
        void FastLineBatchStart(const RectI& clip, bool aa = false);
        void FastTriangleBatchStart(const RectI& clip);
        void FastBatchEnd();
        void FastLineStart(Vec2 pos, Color color);
        void FastLineAdd(Vec2 pos);
        void FastLineEnd(Vec2 pos);
        void FastPeakStart(Vec2 top, Vec2 bottom, Color color);
        void FastPeakAdd(Vec2 top, Vec2 bottom);
        void FastPeakEnd(Vec2 top, Vec2 bottom);
        void FastRectEmit(const Rect& rect, Color color);
        void FastLineRectEmit(const Rect& rect, Color color);
        // "upside down U" shape
        void FastLineRectTopEmit(const Rect& rect, Color color);
    private:
        // functions
        void FreeBackbufferDependentResources_();
        void CreateBackbufferDependentResources_();
        // data
        bool tearingActive = false;
        bool enableAlpha;
        DimensionsI dims;
        ComPtr<ID3D11Device> pDevice;
        ComPtr<ID3D11DeviceContext> pContext3d;
        ComPtr<IDXGISwapChain1> pSwapChain;
        ComPtr<ID3D11RenderTargetView> pTarget;
        ComPtr<IDCompositionDevice> pCompositionDevice;
        ComPtr<IDCompositionTarget> pCompositionTarget;
        ComPtr<IDCompositionVisual> pCompositionVisual;
        ComPtr<ID2D1Factory3> pFactory2d;
        ComPtr<ID2D1DeviceContext2> pContext2d;
        ComPtr<IDWriteFactory> pWriteFactory;
        std::optional<impl::FastRenderer> fastRenderer;
    };
}