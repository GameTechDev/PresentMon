// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../AsyncEndpointManager.h"
#include <fstream>
#include "../CefValues.h"
#include "Core/source/infra/svc/Services.h"
#include "Core/source/infra/util/FolderResolver.h"
#include <commdlg.h>

namespace p2c::client::util::async
{
	class BrowseReadSpec : public AsyncEndpoint
	{
	public:
        static constexpr std::string GetKey() { return "browseReadSpec"; }
		BrowseReadSpec() : AsyncEndpoint{ AsyncEndpoint::Environment::BrowserProcess } {}
        // {} => {payload: string}
		void ExecuteOnBrowser(uint64_t uid, CefRefPtr<CefValue> pArgObj, CefRefPtr<CefBrowser> pBrowser) const override
		{
            using infra::util::FolderResolver;
            using infra::svc::Services;
            std::wstring startPath;
            if (auto fr = Services::ResolveOrNull<FolderResolver>()) {
                startPath = fr->Resolve(FolderResolver::Folder::Documents, L"Loadouts\\");
            }

            wchar_t pathBuffer[MAX_PATH] = { 0 };
            OPENFILENAMEW ofn{
                .lStructSize = sizeof(ofn),
                .hwndOwner = pBrowser->GetHost()->GetWindowHandle(),
                .lpstrFilter = L"All (*.*)\0*.*\0JSON (*.json)\0*.json\0",
                .nFilterIndex = 2,
                .lpstrFile = pathBuffer,
                .nMaxFile = sizeof(pathBuffer),
                .lpstrInitialDir = startPath.c_str(),
                .lpstrTitle = L"Load Loadout",
                .Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
            };

            std::wstring payload;
            if (GetOpenFileNameW(&ofn) && wcsnlen_s(pathBuffer, sizeof(pathBuffer)) > 0) {
                std::wifstream file{ pathBuffer };
                payload = std::wstring(
                    std::istreambuf_iterator<wchar_t>{ file },
                    std::istreambuf_iterator<wchar_t>{}
                );
            }

            // 0:int uid, 1:bool success, 2:dict args
            auto msg = CefProcessMessage::Create(AsyncEndpointManager::GetResolveMessageName());
            {
                const auto argList = msg->GetArgumentList();
                argList->SetInt(0, (int)uid);
                argList->SetBool(1, true); // success
                argList->SetValue(2, MakeCefObject(CefProp{ "payload", std::move(payload) }));
            }
            pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(msg));
		}
	};
}