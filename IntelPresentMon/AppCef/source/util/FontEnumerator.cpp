// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "FontEnumerator.h"
#include <CommonUtilities/win/WinAPI.h>
#include <CommonUtilities/win/com/ComPtr.h>
#include "Logging.h"
#include <CommonUtilities/log/HrLogger.h>
#include <dwrite.h>


namespace p2c::client::util
{
    using pmon::util::win::com::ComPtr;

	FontEnumerator::FontEnumerator()
	{
        ComPtr<IDWriteFactory> pWriteFactory;
        pmlog_hr << DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            (IUnknown**)&pWriteFactory
        );
        ComPtr<IDWriteFontCollection> pFonts;
        pmlog_hr << pWriteFactory->GetSystemFontCollection(&pFonts);
        wchar_t locale[LOCALE_NAME_MAX_LENGTH];
        if (!GetUserDefaultLocaleName(locale, (int)std::size(locale)))
        {
            pmlog_error().hr();
        }
        unsigned const count = pFonts->GetFontFamilyCount();
        for (unsigned familyIndex = 0; familyIndex != count; ++familyIndex)
        {
            ComPtr<IDWriteFontFamily> pFamily;
            pmlog_hr << pFonts->GetFontFamily(familyIndex, &pFamily);
            ComPtr<IDWriteLocalizedStrings> pNames;
            pmlog_hr << pFamily->GetFamilyNames(&pNames);

            unsigned nameIndex;
            BOOL exists;
            pmlog_hr << pNames->FindLocaleName(locale, &nameIndex, &exists);
            if (exists)
            {
                wchar_t name[64];
                pmlog_hr << pNames->GetString(nameIndex, name, (unsigned)std::size(name));
                names.push_back(name);
            }
            else if (pNames->GetCount() > 0)
            {
                wchar_t name[64];
                pmlog_hr << pNames->GetString(0, name, (unsigned)std::size(name));
                names.push_back(name);
            }
        }
	}

    const std::vector<std::wstring>& FontEnumerator::GetNames() const
    {
        return names;
    }
}
