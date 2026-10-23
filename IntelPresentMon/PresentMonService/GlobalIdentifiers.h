#pragma once

namespace pmon::gid
{
	inline constexpr const char* defaultControlPipeName = R"(\\.\pipe\sharedpresentmonsvcnamedpipe)";
	inline constexpr const wchar_t* sharedWindowsServiceName = L"PresentMonSharedService";
	inline constexpr const wchar_t* registryPath = LR"(SOFTWARE\INTEL\PresentMon\Service)";
	inline constexpr const char* middlewarePathKey = "sharedMiddlewarePath";
}