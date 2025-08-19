// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "DrawablePrimitive.h"
#include "../base/Geometry.h"
#include <CommonUtilities/win/com/ComPtr.h>
#include "TextStylePrimitive.h"
#include <string>
#include <optional>
#include "Enums.h"
#include "ForwardInterfaces.h"
#include <memory>


namespace p2c::gfx::prim
{
	using pmon::util::win::com::ComPtr;
	class TextPrimitive : public DrawablePrimitive
	{
	public:
		TextPrimitive(const std::wstring& text, const TextStylePrimitive& style, const Dimensions& dims, std::shared_ptr<BrushPrimitive> pBrushPrim, Graphics& gfx, std::optional<Vec2> pos = std::nullopt);
		void SetMaxDimensions(const Dimensions& dims);
		Dimensions GetActualDimensions() const;
		Dimensions GetMaxDimensions() const;
		void SetPosition(const Vec2& pos);
		void SetAlignment(Alignment align);
		void SetJustification(Justification justify);
		void SetText(const std::wstring& text, Graphics& gfx);
		~TextPrimitive() override;
		void Draw(Graphics& gfx) const override;
	private:
		// dependent
		std::shared_ptr<BrushPrimitive> pBrushPrim;
		ComPtr<ID2D1Brush> pBrush;
		// independent
		std::optional<Vec2> position;
		ComPtr<IDWriteTextLayout> pLayout;
	};
}