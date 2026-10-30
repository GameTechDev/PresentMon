// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../AsyncEndpointManager.h"
#include "../CefValues.h"
#include "Core/source/infra/util/FolderResolver.h"
#include <filesystem>
#include <shobjidl.h>
#include <wrl/client.h>

namespace p2c::client::util::async
{
	class BrowseLogFolder : public AsyncEndpoint
	{
	public:
		static constexpr std::string GetKey() { return "browseLogFolder"; }
		BrowseLogFolder() : AsyncEndpoint{ AsyncEndpoint::Environment::BrowserProcess } {}

		void ExecuteOnBrowser(uint64_t uid, CefRefPtr<CefValue> pArgObj, CefRefPtr<CefBrowser> pBrowser) const override
		{
			using FR = infra::util::FolderResolver;
			std::filesystem::path startPath;
			const auto initialPath = pArgObj->GetDictionary()->GetString("initialPath").ToWString();
			std::error_code pathError;
			if (!initialPath.empty() && std::filesystem::is_directory(initialPath, pathError)) {
				startPath = initialPath;
			}
			else {
				startPath = FR::Get().ResolveLogPath();
				std::filesystem::create_directories(startPath, pathError);
			}

			std::wstring selectedPath;
			Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
			if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(dialog.GetAddressOf())))) {
				FILEOPENDIALOGOPTIONS options = 0;
				if (SUCCEEDED(dialog->GetOptions(&options))) {
					dialog->SetOptions((FILEOPENDIALOGOPTIONS)
						(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST));
				}
				dialog->SetTitle(L"Select Diagnostic Log Folder");

				Microsoft::WRL::ComPtr<IShellItem> startItem;
				if (SUCCEEDED(SHCreateItemFromParsingName(startPath.c_str(), nullptr,
					IID_PPV_ARGS(startItem.GetAddressOf())))) {
					dialog->SetFolder(startItem.Get());
				}

				if (SUCCEEDED(dialog->Show(pBrowser->GetHost()->GetWindowHandle()))) {
					Microsoft::WRL::ComPtr<IShellItem> result;
					if (SUCCEEDED(dialog->GetResult(result.GetAddressOf()))) {
						wchar_t* pPath = nullptr;
						if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &pPath))) {
							selectedPath = pPath;
							CoTaskMemFree(pPath);
						}
					}
				}
			}

			auto msg = CefProcessMessage::Create(AsyncEndpointManager::GetResolveMessageName());
			const auto args = msg->GetArgumentList();
			args->SetInt(0, (int)uid);
			args->SetBool(1, true);
			args->SetValue(2, MakeCefObject(CefProp{ "path", std::move(selectedPath) }));
			pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(msg));
		}
	};
}
