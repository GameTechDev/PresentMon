// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "RetainedPrimitive.h"
#include <CommonUtilities/win/com/ComPtr.h>
#include "../base/Geometry.h"
#include "ForwardInterfaces.h"


namespace p2c::gfx::prim
{
	using pmon::util::win::com::ComPtr;
	class BrushPrimitive : public RetainedPrimitive
	{
		friend class DrawablePrimitive;
	public:
		BrushPrimitive(const Color& color, Graphics& gfx);
		~BrushPrimitive() override;
		void SetColor(const Color& color);
	private:
		// allows drawable primitives to access D2D interface handle
		ComPtr<ID2D1Brush> GetInterface() const;

		// independent
		Color color;
		// dependent
		ComPtr<ID2D1SolidColorBrush> pBrush;
	};
}