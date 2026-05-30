// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "FolderResolver.h"
#include <Core/source/win/WinAPI.h>
#include <CommonUtilities/file/PathUtils.h>
#include <ShlObj.h>
#include <filesystem>
#include <format>
#include <Core/source/infra/Logging.h>
#include <CommonUtilities/Exception.h>

namespace p2c::infra::util
{
	using namespace ::pmon::util;

	namespace
	{
		std::filesystem::path AppendRelativePath_(const std::filesystem::path& base, const std::filesystem::path& path)
		{
			if (path.empty()) {
				return base;
			}
			auto relative = path.has_root_path() ? path.relative_path() : path;
			auto native = relative.native();
			while (!native.empty() && (native.front() == L'\\' || native.front() == L'/')) {
				native.erase(native.begin());
			}
			if (native.empty()) {
				return base;
			}
			return base / std::filesystem::path{ native };
		}
	}

	void CreateSubdir(const std::wstring& docPath, const wchar_t* subdir)
	{
		try {
			std::filesystem::create_directory(std::format(L"{}\\{}", docPath, subdir));
		}
		catch (const std::exception&) {
			pmlog_error("Failed creating directory: " + std::format("{}\\{}", str::ToNarrow(docPath), str::ToNarrow(subdir)));
			throw Except<Exception>();
		}
	}

	FolderResolver::FolderResolver(std::wstring appPathSubdir, std::wstring docPathSubdir, bool createSubdirectories)
	{
		if (!appPathSubdir.empty()) {
			wchar_t* pPath = nullptr;
			if (auto hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &pPath); FAILED(hr)) {
				CoTaskMemFree(pPath);
				pPath = nullptr;
				pmlog_error("Failed getting local app data path");
				throw Except<Exception>();
			}
			const auto dir = std::format(L"{}\\{}", pPath, appPathSubdir);
			try {
				std::filesystem::create_directories(dir);
				appPath = dir;
			}
			catch (const std::exception&) {
				CoTaskMemFree(pPath);
				pPath = nullptr;
				pmlog_error("Failed creating directory: " + str::ToNarrow(dir));
				throw Except<Exception>();
			}
			if (pPath) {
				CoTaskMemFree(pPath);
			}
		}
		else {
			appPath = std::filesystem::current_path().wstring();
		}

		if (!docPathSubdir.empty()) {
			wchar_t* pPath = nullptr;
			if (auto hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pPath); FAILED(hr)) {
				CoTaskMemFree(pPath);
				pPath = nullptr;
				pmlog_error("Failed getting user documents path");
				throw Except<Exception>();
			}
			const auto dir = std::format(L"{}\\{}", pPath, docPathSubdir);
			try {
				std::filesystem::create_directories(dir);
				docPath = dir;
			}
			catch (const std::exception&) {
				CoTaskMemFree(pPath);
				pPath = nullptr;
				pmlog_error("Failed creating directory: " + str::ToNarrow(dir));
				throw Except<Exception>();
			}
			if (pPath) {
				CoTaskMemFree(pPath);
			}
		}
		else {
			docPath = std::filesystem::current_path().wstring();
		}

		// TODO: this really doesn't belong here, but here it stays until time for something saner
		if (createSubdirectories) {
			CreateSubdir(docPath, capturesSubdirectory);
			CreateSubdir(docPath, loadoutsSubdirectory);
			CreateSubdir(docPath, etlSubdirectory);
		}
	}

	std::filesystem::path FolderResolver::ResolveInstallPath(std::filesystem::path path)
	{
		return AppendRelativePath_(file::GetCurrentProcessModuleDirectory(), path);
	}

	std::wstring FolderResolver::Resolve(Folder f, std::wstring path) const
	{
		return ResolvePath(f, std::filesystem::path{ path }).wstring();
	}

	std::filesystem::path FolderResolver::ResolveLogPath(std::filesystem::path path) const
	{
		if (logPathOverride && !logPathOverride->empty()) {
			return AppendRelativePath_(*logPathOverride, path);
		}
		return AppendRelativePath_(ResolvePath(Folder::App, logsSubdirectory), path);
	}

	std::filesystem::path FolderResolver::ResolvePath(Folder f, std::filesystem::path path) const
	{
		switch (f) {
		case Folder::App:
			if (appPath.empty()) {
				pmlog_error("Failed to resolve app path: not initialized");
				throw Except<Exception>();
			}
			return AppendRelativePath_(std::filesystem::path{ appPath }, path);
		case Folder::Documents: {
			if (docPath.empty()) {
				pmlog_error("Failed to resolve documents path: not initialized");
				throw Except<Exception>();
			}
			return AppendRelativePath_(std::filesystem::path{ docPath }, path);
		}
		case Folder::Temp: {
			try {
				return AppendRelativePath_(std::filesystem::temp_directory_path(), path);
			}
			catch (...) {
				pmlog_error("failed resolving temp dir");
				throw Except<Exception>();
			}
		}
		case Folder::Install: {
			return ResolveInstallPath(path);
		}
		}
		return {};
	}
	FolderResolver& FolderResolver::Get()
	{
		static FolderResolver res{
			useDevMode ? L"" : L"Intel\\PresentMon",
			useDevMode ? L"" : L"PresentMon"
		};
		return res;
	}

	void FolderResolver::SetDevMode()
	{
		useDevMode = true;
	}

	void FolderResolver::SetLogPathOverride(std::filesystem::path path)
	{
		if (path.empty()) {
			logPathOverride.reset();
		}
		else {
			logPathOverride = std::move(path);
		}
	}
}
