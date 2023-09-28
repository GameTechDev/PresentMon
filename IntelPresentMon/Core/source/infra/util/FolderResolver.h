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
		FolderResolver(std::wstring appPathSubdir = {}, std::wstring docPathSubdir = {}, bool createSubdirectories = true);
		std::wstring Resolve(Folder f, std::wstring path = {}) const;
	private:
		static constexpr const wchar_t* loadoutsSubdirectory = L"Loadouts";
		static constexpr const wchar_t* capturesSubdirectory = L"Captures";
		std::wstring appPath;
		std::wstring docPath;
	};
}