// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "FolderResolver.h"
#include <Core/source/win/WinAPI.h>
#include <ShlObj.h>
#include <filesystem>
#include <format>
#include <Core/source/infra/log/Logging.h>

namespace p2c::infra::util
{
	FolderResolver::FolderResolver(std::wstring appPathSubdir, std::wstring docPathSubdir, bool createSubdirectories)
	{
		if (!appPathSubdir.empty())
		{
			wchar_t* pPath = nullptr;
			if (auto hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &pPath); FAILED(hr))
			{
				CoTaskMemFree(pPath);
				pPath = nullptr;
				// TODO: logging: we can't use logging service here during resolve creation
				p2clog.note(L"Failed getting local app data path").commit();
			}
			const auto dir = std::format(L"{}\\{}", pPath, appPathSubdir);
			try {
				std::filesystem::create_directories(dir);
				appPath = dir;
			}
			catch (const std::exception& e) {
				CoTaskMemFree(pPath);
				pPath = nullptr;
				// TODO: logging: we can't use logging service here during resolve creation
				p2clog.note(L"Failed creating directory: " + dir).nested(e).commit();
			}
			if (pPath)
			{
				CoTaskMemFree(pPath);
			}
		}
		else
		{
			appPath = std::filesystem::current_path().wstring();
		}

		if (!docPathSubdir.empty())
		{
			wchar_t* pPath = nullptr;
			if (auto hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pPath); FAILED(hr))
			{
				CoTaskMemFree(pPath);
				pPath = nullptr;
				// TODO: logging: we can't use logging service here during resolve creation
				p2clog.note(L"Failed getting user documents path").commit();
			}
			const auto dir = std::format(L"{}\\{}", pPath, docPathSubdir);
			try {
				std::filesystem::create_directories(dir);
				docPath = dir;
			}
			catch (const std::exception& e) {
				CoTaskMemFree(pPath);
				pPath = nullptr;
				// TODO: logging: we can't use logging service here during resolve creation
				p2clog.note(L"Failed creating directory: " + dir).nested(e).commit();
			}
			if (pPath)
			{
				CoTaskMemFree(pPath);
			}
		}
		else
		{
			appPath = std::filesystem::current_path().wstring();
		}

		// TODO: this really doesn't belong here, but here it stays until time for something saner
		if (createSubdirectories) {
			// captures folder 
			try {
				std::filesystem::create_directory(std::format(L"{}\\{}", docPath, capturesSubdirectory));
			}
			catch (const std::exception& e) {
				// TODO: logging: we can't use logging service here during resolve creation
				p2clog.note(L"Failed creating directory: " + std::format(L"{}\\{}", docPath, capturesSubdirectory)).nested(e).commit();
			}
			// custom loadouts folder
			try {
				std::filesystem::create_directory(std::format(L"{}\\{}", docPath, loadoutsSubdirectory));
			}
			catch (const std::exception& e) {
				// TODO: logging: we can't use logging service here during resolve creation
				p2clog.note(L"Failed creating directory: " + std::format(L"{}\\{}", docPath, loadoutsSubdirectory)).nested(e).commit();
			}
		}
	}

	std::wstring FolderResolver::Resolve(Folder f, std::wstring path) const
	{
		switch (f)
		{
		case Folder::App:
			if (appPath.empty())
			{
				p2clog.note(L"Failed to resolve app path: not initialized").commit();
				return {};
			}
			if (path.empty())
			{
				return appPath;
			}
			else
			{
				return std::format(L"{}\\{}", appPath, path);
			}
		case Folder::Documents:
		{
			if (docPath.empty())
			{
				p2clog.note(L"Failed to resolve documents path: not initialized").commit();
				return {};
			}
			if (path.empty())
			{
				return docPath;
			}
			else
			{
				return std::format(L"{}\\{}", docPath, path);
			}
		}
		case Folder::Temp:
		{
			wchar_t tempPath[MAX_PATH + 1];
			if (!GetTempPathW(MAX_PATH, tempPath))
			{
				p2clog.note(L"failed resolving temp dir").hr().commit();
			}
			return std::format(L"{}{}", tempPath, path);
		}
		case Folder::Install:
		{
			wchar_t modulePath[MAX_PATH];
			::GetModuleFileNameW(nullptr, modulePath, (DWORD)std::size(modulePath));
			const auto dir = std::filesystem::path{ modulePath }.remove_filename().wstring();
			return std::format(L"{}{}", dir, path);
		}
		}
		return {};
	}
}