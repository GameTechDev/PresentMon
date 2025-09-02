#pragma once

namespace pmon::gid
{
	inline constexpr const char* defaultControlPipeName = R"(\\.\pipe\sharedpresentmonsvcnamedpipe)";
	inline constexpr const char* defaultIntrospectionNsmName = R"(Global\shared_pm2_bip_shm)";
	inline constexpr const char* defaultLogPipeBaseName = "shared-pm2-svc-log";
	inline constexpr const wchar_t* registryPath = LR"(SOFTWARE\INTEL\PresentMon\Service)";
	inline constexpr const char* middlewarePathKey = "sharedMiddlewarePath";
}