// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "RetainedPrimitive.h"
#include <CommonUtilities/win/com/ComPtr.h>
#include "../base/Geometry.h"
#include <string>
#include "Enums.h"
#include "ForwardInterfaces.h"



namespace p2c::gfx::prim
{
	using pmon::util::win::com::ComPtr;
	class TextStylePrimitive : public RetainedPrimitive
	{
	public:
		// functions
		TextStylePrimitive(const std::wstring& family, float size, Graphics& gfx, Style style = Style::Normal, Weight weight = Weight::Normal);
		~TextStylePrimitive() override;
		void SetAlignment(Alignment align);
		void SetJustification(Justification justify);
		operator IDWriteTextFormat*() const;
	private:
		// independent
		ComPtr<IDWriteTextFormat> pFormat;
	};
}