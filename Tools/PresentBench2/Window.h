// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "Config.h"
#include <cstdint>
#include <string>

namespace pb2
{
	class IFrameLoopWindow
	{
	public:
		virtual ~IFrameLoopWindow() = default;
		virtual bool ProcessMessages() = 0;
		virtual bool IsRenderActive() const = 0;
		virtual bool InjectMouseClick() = 0;
		virtual bool InjectKeyboardTap() = 0;
	};

	class BenchWindow final : public IFrameLoopWindow
	{
	public:
		BenchWindow() = default;
		~BenchWindow() override;
		BenchWindow(const BenchWindow&) = delete;
		BenchWindow& operator=(const BenchWindow&) = delete;

		void Create(HINSTANCE instance, const BenchConfig& config);
		HWND GetHwnd() const { return hwnd_; }
		uint32_t GetClientWidth() const { return width_; }
		uint32_t GetClientHeight() const { return height_; }
		bool ProcessMessages() override;
		bool IsRenderActive() const override { return !pauseWhenInactive_ || active_; }
		bool InjectMouseClick() override;
		bool InjectKeyboardTap() override;

	private:
		void Destroy();
		static LRESULT CALLBACK WndProc_(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		HWND hwnd_ = nullptr;
		HINSTANCE instance_ = nullptr;
		std::wstring className_;
		uint32_t width_ = 1280;
		uint32_t height_ = 720;
		bool pauseWhenInactive_ = false;
		bool active_ = true;
	};
}
