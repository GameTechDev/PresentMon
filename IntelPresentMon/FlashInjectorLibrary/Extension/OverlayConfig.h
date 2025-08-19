#pragma once
#include <array>

namespace GfxLayer::Extension
{
	struct OverlayConfig
	{
		float BarSize = 0.2f;
		float BarRightShift = 0.5f;
		std::array<float, 4> BarColor = { 1.f, 1.f, 1.f, 1.f };
		bool  RenderBackground = false;
		std::array<float, 4> BackgroundColor = { 0.f, 0.f, 0.f, 1.f };
	};
}