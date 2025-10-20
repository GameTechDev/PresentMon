// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>

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
		static FolderResolver& Get();
		static void SetDevMode();

		static constexpr const wchar_t* loadoutsSubdirectory = L"Loadouts";
		static constexpr const wchar_t* capturesSubdirectory = L"Captures";
		static constexpr const wchar_t* etlSubdirectory = L"Etl";
	private:
		FolderResolver(std::wstring appPathSubdir = {}, std::wstring docPathSubdir = {}, bool createSubdirectories = true);

		std::wstring appPath;
		std::wstring docPath;
		inline static bool useDevMode = false;
	};
}