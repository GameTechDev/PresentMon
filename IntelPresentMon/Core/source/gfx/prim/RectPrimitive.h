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
	using pmon::util::win::com::ComPtr;
	class RectPrimitive : public DrawablePrimitive
	{
	public:
		RectPrimitive(const Rect& rect, std::shared_ptr<BrushPrimitive> pBrushPrim, Graphics& gfx);
		~RectPrimitive() override;
		void Draw(Graphics& gfx) const override;
	private:
		// independent
		ComPtr<ID2D1RectangleGeometry> pGeometry;
		// dependent
		ComPtr<ID2D1GeometryRealization> pRealization;
		std::shared_ptr<BrushPrimitive> pBrushPrim;
		ComPtr<ID2D1Brush> pBrush;
	};
}