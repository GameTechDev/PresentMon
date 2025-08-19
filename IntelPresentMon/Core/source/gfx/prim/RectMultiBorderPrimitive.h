// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "DrawablePrimitive.h"
#include "../base/Geometry.h"
#include <CommonUtilities/win/com/ComPtr.h>
#include "ForwardInterfaces.h"
#include <memory>


namespace p2c::gfx::prim
{
	// this primitive allows a rect border with different brushes (colors) for each side
	class RectMultiBorderPrimitive : public DrawablePrimitive
	{
	public:
		RectMultiBorderPrimitive(const Rect& rect, const Skirt& stroke,
			std::shared_ptr<BrushPrimitive> pBrushPrimLeft,
			std::shared_ptr<BrushPrimitive> pBrushPrimTop,
			std::shared_ptr<BrushPrimitive> pBrushPrimRight,
			std::shared_ptr<BrushPrimitive> pBrushPrimBottom,
			Graphics& gfx);
		~RectMultiBorderPrimitive() override;
		void Draw(Graphics& gfx) const override;
	private:
		struct Side
		{
			Side(std::shared_ptr<BrushPrimitive> pBrushPrim);
			// independent
			ComPtr<ID2D1RectangleGeometry> pGeometry;
			// dependent
			ComPtr<ID2D1GeometryRealization> pRealized;
			std::shared_ptr<BrushPrimitive> pBrushPrim;
			ComPtr<ID2D1Brush> pBrush;
		};
		Side left, top, right, bottom;
	};
}