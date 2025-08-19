#include "OverlayRenderer.h"
#include <cassert>
#include "../Logging.h"
#include "OverlayConfigPack.h"

namespace GfxLayer::Extension
{
	void CheckResult(HRESULT result, const char* pMessage)
	{
		if (FAILED(result))
		{
			LOGE << "Error: " << pMessage << " (0x" << std::hex << result << ")";
			assert(0);
		}
	}

	OverlayRenderer::OverlayRenderer(const OverlayConfig& cfg, IDXGISwapChain3* pSwapChain):
		m_currentConfig{ cfg },
		m_pSwapChain(pSwapChain)
	{}

	RECT OverlayRenderer::GetScissorRect() const
	{
		const float rectWidth = m_width * m_currentConfig.BarSize;
		RECT scissor;
		scissor.left = LONG((m_width - rectWidth) * m_currentConfig.BarRightShift);
		scissor.top = 0;
		scissor.right = LONG(scissor.left + rectWidth);
		scissor.bottom = m_height;
		return scissor;
	}

	IDXGISwapChain3* OverlayRenderer::GetSwapChain() const
	{
		return m_pSwapChain.Get();
	}

	void OverlayRenderer::Resize(unsigned bufferCount, unsigned width, unsigned height)
	{
		m_width = width;
		m_height = height;
		UpdateViewport(m_currentConfig);
	}

	void OverlayRenderer::NewFrame()
	{
		auto& pack = OverlayConfigPack::Get();
		if (pack.IsDirty()) {
			m_currentConfig = pack.Read();
			UpdateConfig(m_currentConfig);
		}

		static unsigned s_frameCounter = 0;
		static bool s_mouseClicked = false;

		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			if (!s_mouseClicked)
			{
				s_frameCounter = 16;
			}
			s_mouseClicked = true;
		}
		else
		{
			s_mouseClicked = false;
		}

		if (s_frameCounter > 0)
		{
			Render(true);
			--s_frameCounter;
		}
		else if (m_currentConfig.RenderBackground)
		{
			Render(false);
		}
	}
}

