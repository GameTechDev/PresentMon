// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../AsyncEndpointManager.h"
#include "Core/source/infra/svc/Services.h"
#include "Core/source/infra/util/FolderResolver.h"
#include <commdlg.h>
#include <fstream>

namespace p2c::client::util::async
{
	class BrowseStoreSpec : public AsyncEndpoint
	{
	public:
        static constexpr std::string GetKey() { return "browseStoreSpec"; }
        BrowseStoreSpec() : AsyncEndpoint{ AsyncEndpoint::Environment::BrowserProcess } {}
        // {payload: string} => {written: bool}
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
                .lpstrTitle = L"Save Loadout As",
                .Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT,
            };

            const bool confirmed = GetSaveFileNameW(&ofn);
            bool written = false;
            bool failed = false;

            // try to write the file to disk
            if (confirmed && wcsnlen_s(pathBuffer, sizeof(pathBuffer)) > 0) {
                std::wofstream file{ pathBuffer };
                file << pArgObj->GetDictionary()->GetString("payload").ToWString();
                failed = !file.good();
                written = true;
            }

            // 0:int uid, 1:bool success, 2:dict args
            auto msg = CefProcessMessage::Create(AsyncEndpointManager::GetResolveMessageName());
            msg->GetArgumentList()->SetInt(0, (int)uid);
            if (!failed)
            {
                msg->GetArgumentList()->SetBool(1, true); // success
                msg->GetArgumentList()->SetValue(2, MakeCefObject(CefProp{ "written", written }));
            }
            else
            {
                msg->GetArgumentList()->SetBool(1, false); // failure
                msg->GetArgumentList()->SetDictionary(2, std::move(CefDictionaryValue::Create()));
            }
            pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(msg));
		}
	};
}