#pragma once
#include "../../CommonUtilities/win/WinAPI.h"
#include <dxgi1_6.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include "../Context.h"
#include "../NonCopyable.h"
#include "OverlayConfig.h"
#include <chrono>
#include <optional>

namespace GfxLayer::Extension
{
	void CheckResult(HRESULT result, const char* pMessage);

	struct ScissorRects
	{
		RECT fg;
		RECT bg;
		RECT rb;
	};

	class OverlayRenderer : public NonCopyable
	{
	public:
		OverlayRenderer(const OverlayConfig& cfg, IDXGISwapChain3* pSwapChain);
		virtual ~OverlayRenderer() = default;

		// called from a hook whenever the host is resizing its render targets
		virtual void Resize(unsigned bufferCount, unsigned width, unsigned height) = 0;

		void NewFrame();
		ScissorRects GetScissorRects() const;
		IDXGISwapChain3* GetSwapChain() const;

	protected:
		virtual void Render(bool renderBar, bool useRainbow, bool enableBackground) = 0;
		// this is triggered both by Resize and by UpdateConfig
		virtual void UpdateViewport(const OverlayConfig& cfg) = 0;
		// called from Render when it is detected that config has changed (via IPC action)
		virtual void UpdateConfig(const OverlayConfig& cfg) = 0;
		// get rainbow index for current flash frame
		size_t GetRainbowIndex() const;
		// get array of rainbow colors
		static constexpr const std::array<std::array<float, 4>, 16>& GetRainbowColors()
		{
			return m_rainbowColors;
		}
		template<class T>
		static T MakeViewport(const RECT& r)
		{
			return T{
				.TopLeftX = decltype(std::declval<T>().TopLeftX)(r.left),
				.TopLeftY = decltype(std::declval<T>().TopLeftY)(r.top),
				.Width = decltype(std::declval<T>().Width)(r.right - r.left),
				.Height = decltype(std::declval<T>().Height)(r.bottom - r.top),
			};
		}

	private:
		static constexpr std::array<std::array<float, 4>, 16> m_rainbowColors = { {
			{{1.00f, 0.00f, 0.00f, 1.0f}}, // red
			{{1.00f, 0.50f, 0.00f, 1.0f}}, // orange
			{{1.00f, 1.00f, 0.00f, 1.0f}}, // yellow
			{{0.75f, 1.00f, 0.00f, 1.0f}}, // chartreuse
			{{0.00f, 1.00f, 0.00f, 1.0f}}, // green
			{{0.00f, 1.00f, 0.60f, 1.0f}}, // spring green
			{{0.00f, 1.00f, 1.00f, 1.0f}}, // cyan
			{{0.00f, 0.60f, 1.00f, 1.0f}}, // azure
			{{0.00f, 0.00f, 1.00f, 1.0f}}, // blue
			{{0.40f, 0.00f, 1.00f, 1.0f}}, // indigo
			{{0.60f, 0.00f, 1.00f, 1.0f}}, // violet
			{{0.80f, 0.00f, 0.80f, 1.0f}}, // purple
			{{1.00f, 0.00f, 1.00f, 1.0f}}, // magenta
			{{1.00f, 0.00f, 0.60f, 1.0f}}, // rose
			{{1.00f, 0.20f, 0.40f, 1.0f}}, // salmon
			{{1.00f, 0.40f, 0.60f, 1.0f}}, // pink
		} };
		using clock = std::chrono::high_resolution_clock;
		OverlayConfig			m_currentConfig;
		ComPtr<IDXGISwapChain3>	m_pSwapChain;
		unsigned m_width = 0;
		unsigned m_height = 0;
		std::optional<clock::time_point> m_flashStartTime;
		bool m_clickHoldoff = false;
		size_t m_rainbowFrameIndex = 0;
	};
}