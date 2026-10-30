// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "Window.h"
#include "../../IntelPresentMon/CommonUtilities/log/Log.h"
#include "../../IntelPresentMon/CommonUtilities/str/String.h"
#include <format>
#include <stdexcept>
#include <string>

namespace pb2
{
	namespace
	{
		constexpr wchar_t kClassPrefix[] = L"PresentBench2WindowClass";

		MONITORINFOEXW GetMonitorInfoByIndex(uint32_t index, bool& ok)
		{
			struct EnumCtx {
				uint32_t target = 0;
				uint32_t current = 0;
				MONITORINFOEXW info{};
				bool found = false;
			} ctx;
			ctx.target = index;
			ctx.info.cbSize = sizeof(ctx.info);
			EnumDisplayMonitors(nullptr, nullptr,
				[](HMONITOR monitor, HDC, LPRECT, LPARAM lparam) -> BOOL {
					auto* p = reinterpret_cast<EnumCtx*>(lparam);
					if (p->current == p->target) {
						p->info.cbSize = sizeof(p->info);
						if (GetMonitorInfoW(monitor, &p->info)) {
							p->found = true;
						}
						return FALSE;
					}
					++p->current;
					return TRUE;
				}, (LPARAM)&ctx);
			ok = ctx.found;
			return ctx.info;
		}
	}

	BenchWindow::~BenchWindow()
	{
		Destroy();
	}

	void BenchWindow::Create(HINSTANCE instance, const BenchConfig& config)
	{
		instance_ = instance;
		width_ = config.swap.width;
		height_ = config.swap.height;
		pauseWhenInactive_ = config.mode == WindowMode::Fullscreen;
		className_ = kClassPrefix;

		WNDCLASSEXW wc{};
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc_;
		wc.hInstance = instance_;
		wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = className_.c_str();
		if (!RegisterClassExW(&wc)) {
			throw std::runtime_error("RegisterClassExW failed");
		}

		DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
		DWORD exStyle = 0;
		int x = CW_USEDEFAULT;
		int y = CW_USEDEFAULT;
		int w = (int)width_;
		int h = (int)height_;

		bool monitorOk = false;
		const auto mi = GetMonitorInfoByIndex(config.monitor, monitorOk);
		if (config.mode == WindowMode::Borderless || config.mode == WindowMode::Fullscreen) {
			style = WS_POPUP;
			if (monitorOk) {
				x = mi.rcMonitor.left;
				y = mi.rcMonitor.top;
				w = mi.rcMonitor.right - mi.rcMonitor.left;
				h = mi.rcMonitor.bottom - mi.rcMonitor.top;
				width_ = (uint32_t)w;
				height_ = (uint32_t)h;
			}
			else {
				pmlog_warn("Requested monitor not found; using configured width/height");
				x = 0;
				y = 0;
			}
		}
		else {
			RECT rc{ 0, 0, (LONG)width_, (LONG)height_ };
			AdjustWindowRectEx(&rc, style, FALSE, exStyle);
			w = rc.right - rc.left;
			h = rc.bottom - rc.top;
			if (monitorOk) {
				x = mi.rcWork.left + 40;
				y = mi.rcWork.top + 40;
			}
		}

		const auto title = std::format(L"PresentBench2 - {} - {}",
			pmon::util::str::ToWide(ApiName(config.api)),
			pmon::util::str::ToWide(ModeName(config.mode)));
		hwnd_ = CreateWindowExW(exStyle, className_.c_str(), title.c_str(),
			style, x, y, w, h, nullptr, nullptr, instance_, this);
		if (!hwnd_) {
			UnregisterClassW(className_.c_str(), instance_);
			throw std::runtime_error("CreateWindowExW failed");
		}
		ShowWindow(hwnd_, SW_SHOW);
		UpdateWindow(hwnd_);
		active_ = GetForegroundWindow() == hwnd_;
	}

	void BenchWindow::Destroy()
	{
		if (hwnd_) {
			DestroyWindow(hwnd_);
			hwnd_ = nullptr;
		}
		if (instance_ && !className_.empty()) {
			UnregisterClassW(className_.c_str(), instance_);
			className_.clear();
		}
	}

	bool BenchWindow::ProcessMessages()
	{
		MSG msg{};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				return false;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		return true;
	}

	bool BenchWindow::InjectMouseClick()
	{
		if (!hwnd_) {
			return false;
		}
		RECT rc{};
		if (!GetClientRect(hwnd_, &rc)) {
			return false;
		}
		POINT pt{ (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
		if (!ClientToScreen(hwnd_, &pt)) {
			return false;
		}
		SetForegroundWindow(hwnd_);
		if (GetForegroundWindow() != hwnd_ || !SetCursorPos(pt.x, pt.y)) {
			return false;
		}
		const HWND target = GetAncestor(WindowFromPoint(pt), GA_ROOT);
		if (target != hwnd_) {
			return false;
		}
		INPUT inputs[2]{};
		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		inputs[1].type = INPUT_MOUSE;
		inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		return SendInput(2, inputs, sizeof(INPUT)) == 2;
	}

	bool BenchWindow::InjectKeyboardTap()
	{
		if (!hwnd_) {
			return false;
		}
		SetForegroundWindow(hwnd_);
		if (GetForegroundWindow() != hwnd_) {
			return false;
		}
		INPUT inputs[2]{};
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = 'A';
		inputs[1].type = INPUT_KEYBOARD;
		inputs[1].ki.wVk = 'A';
		inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
		return SendInput(2, inputs, sizeof(INPUT)) == 2;
	}

	LRESULT CALLBACK BenchWindow::WndProc_(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_NCCREATE) {
			const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
		}
		auto* window = reinterpret_cast<BenchWindow*>(
			GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		switch (msg) {
		case WM_ACTIVATEAPP:
			if (window) {
				window->active_ = wParam != FALSE;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) {
				PostQuitMessage(0);
				return 0;
			}
			break;
		default:
			break;
		}
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}
}
