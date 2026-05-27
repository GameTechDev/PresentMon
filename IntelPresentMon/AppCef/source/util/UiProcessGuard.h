#pragma once
#include <CommonUtilities/win/Handle.h>
#include <CommonUtilities/win/WinAPI.h>
#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace p2c::client::util
{
	constexpr const wchar_t* UiBrowserProcessMutexPrefix = L"Local\\IntelPresentMon.";
	constexpr const char* DefaultUiBrowserProcessMutexSuffix = "UiBrowserProcess";
	constexpr const wchar_t* UiBrowserWindowClassName = L"BrowserWindowClass";
	constexpr const wchar_t* UiBrowserWindowTitle = L"Intel PresentMon";
	constexpr const wchar_t* UiBrowserWindowMutexSuffixProperty = L"IntelPresentMon.UiMutexSuffixAtom";
	constexpr int UiAlreadyRunningExitCode = 2;

	inline std::wstring MakeUiBrowserProcessMutexName(std::string_view suffix)
	{
		std::wstring name{ UiBrowserProcessMutexPrefix };
		if (suffix.empty()) {
			suffix = DefaultUiBrowserProcessMutexSuffix;
		}
		name.append(suffix.begin(), suffix.end());
		return name;
	}

	inline bool IsUiBrowserProcessActive(std::string_view mutexSuffix)
	{
		const auto mutexName = MakeUiBrowserProcessMutexName(mutexSuffix);
		::pmon::util::win::Handle hMutex{ OpenMutexW(SYNCHRONIZE, FALSE, mutexName.c_str()) };
		return bool(hMutex);
	}

	inline std::pair<::pmon::util::win::Handle, bool> TryAcquireUiBrowserProcessMutex(std::string_view mutexSuffix)
	{
		const auto mutexName = MakeUiBrowserProcessMutexName(mutexSuffix);
		::pmon::util::win::Handle hMutex{ CreateMutexW(nullptr, FALSE, mutexName.c_str()) };
		if (!hMutex) {
			return { {}, false };
		}
		return { std::move(hMutex), GetLastError() != ERROR_ALREADY_EXISTS };
	}

	inline void SetUiBrowserWindowMutexSuffix(HWND hWnd, std::string_view mutexSuffix)
	{
		if (mutexSuffix.empty()) {
			mutexSuffix = DefaultUiBrowserProcessMutexSuffix;
		}
		const ATOM suffixAtom = GlobalAddAtomA(std::string{ mutexSuffix }.c_str());
		if (suffixAtom != 0) {
			if (!SetPropW(hWnd, UiBrowserWindowMutexSuffixProperty, (HANDLE)(ULONG_PTR)suffixAtom)) {
				GlobalDeleteAtom(suffixAtom);
			}
		}
	}

	inline void ClearUiBrowserWindowMutexSuffix(HWND hWnd)
	{
		if (const auto suffixAtom = (ATOM)(ULONG_PTR)RemovePropW(hWnd, UiBrowserWindowMutexSuffixProperty)) {
			GlobalDeleteAtom(suffixAtom);
		}
	}

	inline bool UiBrowserWindowMutexSuffixMatches(HWND hWnd, std::string_view mutexSuffix)
	{
		if (mutexSuffix.empty()) {
			mutexSuffix = DefaultUiBrowserProcessMutexSuffix;
		}
		const auto suffixAtom = (ATOM)(ULONG_PTR)GetPropW(hWnd, UiBrowserWindowMutexSuffixProperty);
		if (suffixAtom == 0) {
			return false;
		}
		std::array<char, MAX_PATH> suffix{};
		if (GlobalGetAtomNameA(suffixAtom, suffix.data(), (int)suffix.size()) == 0) {
			return false;
		}
		return std::string_view{ suffix.data() } == mutexSuffix;
	}

	inline BOOL CALLBACK FindUiBrowserWindowCallback(HWND hWnd, LPARAM lParam)
	{
		auto& params = *reinterpret_cast<std::pair<std::string_view, HWND>*>(lParam);
		if (!IsWindowVisible(hWnd)) {
			return TRUE;
		}

		std::array<wchar_t, MAX_PATH> className{};
		if (GetClassNameW(hWnd, className.data(), (int)className.size()) == 0 ||
			std::wstring_view{ className.data() } != UiBrowserWindowClassName) {
			return TRUE;
		}

		std::array<wchar_t, MAX_PATH> title{};
		if (GetWindowTextW(hWnd, title.data(), (int)title.size()) == 0 ||
			std::wstring_view{ title.data() } != UiBrowserWindowTitle) {
			return TRUE;
		}

		if (!UiBrowserWindowMutexSuffixMatches(hWnd, params.first)) {
			return TRUE;
		}

		params.second = hWnd;
		return FALSE;
	}

	inline HWND FindUiBrowserWindow(std::string_view mutexSuffix)
	{
		std::pair<std::string_view, HWND> params{ mutexSuffix, nullptr };
		EnumWindows(FindUiBrowserWindowCallback, reinterpret_cast<LPARAM>(&params));
		return params.second;
	}

	inline bool BringUiBrowserWindowToFront(std::string_view mutexSuffix)
	{
		const auto hWnd = FindUiBrowserWindow(mutexSuffix);
		if (hWnd == nullptr) {
			return false;
		}
		if (IsIconic(hWnd)) {
			ShowWindow(hWnd, SW_RESTORE);
		}
		else {
			ShowWindow(hWnd, SW_SHOW);
		}
		return SetForegroundWindow(hWnd) != FALSE;
	}
}
