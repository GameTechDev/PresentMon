// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <filesystem>
#include <optional>

namespace p2c::infra::util
{
	class FolderResolver
	{
	public:
		// types
		enum class Folder
		{
			App,
			Temp,
			Install,
			Documents,
		};
		// functions
		// defaulted subdir means using the cwd to store files of that category
		std::wstring Resolve(Folder f, std::wstring path = {}) const;
		std::filesystem::path ResolvePath(Folder f, std::filesystem::path path = {}) const;
		std::filesystem::path ResolveLogPath(std::filesystem::path path = {}) const;
		static FolderResolver& Get();
		static void SetDevMode();
		static void SetLogPathOverride(std::filesystem::path path = {});
		static std::filesystem::path ResolveInstallPath(std::filesystem::path path = {});

		static constexpr const wchar_t* loadoutsSubdirectory = L"Loadouts";
		static constexpr const wchar_t* capturesSubdirectory = L"Captures";
		static constexpr const wchar_t* etlSubdirectory = L"Etl";
		static constexpr const wchar_t* logsSubdirectory = L"logs";
	private:
		FolderResolver(std::wstring appPathSubdir = {}, std::wstring docPathSubdir = {}, bool createSubdirectories = true);

		std::wstring appPath;
		std::wstring docPath;
		inline static bool useDevMode = false;
		inline static std::optional<std::filesystem::path> logPathOverride;
	};
}
