// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/WinAPI.h>
#include <CommonUtilities/win/com/ComPtr.h>
#include <Core/source/gfx/base/Geometry.h>
#include <d3d11_2.h>
#include <optional>
#include <vector>

namespace p2c::gfx::impl
{
    using ::pmon::util::win::com::ComPtr;
    class FastRenderer
    {
    public:
        FastRenderer(ID3D11Device& device, const DimensionsI& dims);
        void StartFrame(ID3D11DeviceContext& context);
        void EndFrame(ID3D11DeviceContext& context);
        void StartLineBatch(const RectI& clip, bool anti);
        void StartTriangleBatch(const RectI& clip);
        void EndBatch();
        void StartLine(Vec2 pt, Color c);
        void AddVertex(Vec2 pt);
        void EndLine(Vec2 pt);
        void StartPeakChain(Vec2 top, Vec2 bottom, Color c);
        void AddPeakChain(Vec2 top, Vec2 bottom);
        void EndPeakChain(Vec2 top, Vec2 bottom);
        void EmitRect(const Rect& rect, Color c);
        void EmitLineRect(const Rect& rect, Color c);
        void EmitLineRectTop(const Rect& rect, Color c);
        void Resize(const DimensionsI& dims);
        void ResizeGeometryBuffersIfNecessary(ID3D11Device& device);
    private:
        // types
        class FastDimensions
        {
        public:
            FastDimensions(const DimensionsI& dims_in)
                :
                actualDimensions{ dims_in },
                halfDimensions{ Dimensions(dims_in) * .5f },
                inverseHalfDimensions{ 1.f / halfDimensions.width, 1.f / halfDimensions.height }
            {}
            const DimensionsI& GetActual() const { return actualDimensions; }
            const Dimensions& GetHalf() const { return halfDimensions; }
            const Dimensions& GetInverseHalf() const { return inverseHalfDimensions; }
        private:
            DimensionsI actualDimensions;
            Dimensions halfDimensions;
            Dimensions inverseHalfDimensions;
        };
        struct Point
        {
            Vec2 pos;
            alignas(16) Color color;
        };
        struct Batch
        {
            D3D11_PRIMITIVE_TOPOLOGY topology;
            UINT32 indexBegin;
            UINT32 indexEnd;
            UINT32 vertexOffset;
            RectI clip;
            bool useAntiAlias = false;
        };
        // functions
        void StartBatch(const RectI& clip, D3D11_PRIMITIVE_TOPOLOGY topology, bool anti);
        void WriteVertex(Vec2 pt);
        void WriteIndex(UINT32 i);
        void MakeVertexBuffer(ID3D11Device& device);
        void MakeIndexBuffer(ID3D11Device& device);
        // converts pixel coordinates to ndc
        Vec2 ConvertPoint(Vec2 pt) const;
        // data
        FastDimensions dims;
        UINT vertexBufferSize = 0x2'0000;
        UINT nVertices = 0;
        UINT indexBufferSize = 0x4'0000;
        UINT nIndices = 0;
        std::optional<Batch> activeBatch; // current batch being built, also serves to indicate whether a batch is open
        std::vector<Batch> batches;
        // chain variables
        Color chainColor;
        int chainSize = 0;
        // d3d stuff
        std::optional<D3D11_MAPPED_SUBRESOURCE> vertexMapping;
        std::optional<D3D11_MAPPED_SUBRESOURCE> indexMapping;
        ComPtr<ID3D11PixelShader> pPixelShader;
        ComPtr<ID3D11VertexShader> pVertexShader;
        ComPtr<ID3D11Buffer> pVertexBuffer;
        ComPtr<ID3D11Buffer> pIndexBuffer;
        ComPtr<ID3D11InputLayout> pInputLayout;
        ComPtr<ID3D11RasterizerState> pRasterizer;
        ComPtr<ID3D11RasterizerState> pRasterizerAnti;
        ComPtr<ID3D11DepthStencilState> pDepthStencil;
        ComPtr<ID3D11BlendState> pBlender;
        // logging tripwires
        std::optional<UINT> vertexLimitExceededAmount;
        std::optional<UINT> indexLimitExceededAmount;
    };
}