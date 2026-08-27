// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "IgclControlLibCompatibility.h"

#include "../Logging.h"

// Per-GPU ControlApiPath values are read from each present DISPLAY adapter
// driver registry key (SetupDi DIREG_DRV), analogous to indexed
// IDeviceConfigFactory GPU enumeration in the shared C# helper.

#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "version.lib")

namespace pmon::tel::igcl
{
    namespace
    {
        constexpr wchar_t kControlApiPathValueName[] = L"ControlApiPath";

        bool IsPreHotfixControlLibVersion_(ControlLibFileVersion version) noexcept
        {
            if (version.major < 1) {
                return true;
            }
            if (version.major > 1) {
                return false;
            }

            if (version.minor < 2) {
                return true;
            }
            if (version.minor > 2) {
                return false;
            }

            if (version.build > 269) {
                return false;
            }
            if (version.build < 269) {
                return true;
            }
            return version.privatePart < 269;
        }

        bool IsZesInitOnlyControlLibVersion_(ControlLibFileVersion version) noexcept
        {
            if (version.major > 1) {
                return true;
            }
            return version.major == 1 && version.minor >= 3;
        }

        std::optional<std::wstring> ReadRegistryStringValue_(HKEY key, const wchar_t* valueName)
        {
            DWORD valueType = 0;
            DWORD bufferBytes = 0;
            const LSTATUS sizeStatus = RegQueryValueExW(
                key,
                valueName,
                nullptr,
                &valueType,
                nullptr,
                &bufferBytes);
            if (sizeStatus != ERROR_SUCCESS || bufferBytes == 0) {
                return std::nullopt;
            }
            if (valueType != REG_SZ && valueType != REG_EXPAND_SZ) {
                return std::nullopt;
            }

            std::wstring value;
            value.resize((size_t)bufferBytes / sizeof(wchar_t));
            const LSTATUS readStatus = RegQueryValueExW(
                key,
                valueName,
                nullptr,
                &valueType,
                reinterpret_cast<LPBYTE>(value.data()),
                &bufferBytes);
            if (readStatus != ERROR_SUCCESS) {
                return std::nullopt;
            }

            while (!value.empty() && value.back() == L'\0') {
                value.pop_back();
            }
            if (value.empty()) {
                return std::nullopt;
            }

            if (valueType == REG_EXPAND_SZ) {
                const DWORD expandedChars = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
                if (expandedChars == 0) {
                    return std::nullopt;
                }
                std::wstring expanded;
                expanded.resize((size_t)expandedChars);
                if (ExpandEnvironmentStringsW(value.c_str(), expanded.data(), expandedChars) == 0) {
                    return std::nullopt;
                }
                while (!expanded.empty() && expanded.back() == L'\0') {
                    expanded.pop_back();
                }
                value = std::move(expanded);
            }

            return value;
        }

        std::optional<ControlLibFileVersion> TryReadPeFileVersion_(const std::wstring& path)
        {
            DWORD handle = 0;
            const DWORD infoSize = GetFileVersionInfoSizeW(path.c_str(), &handle);
            if (infoSize == 0) {
                return std::nullopt;
            }

            std::vector<std::byte> infoBuffer((size_t)infoSize);
            if (!GetFileVersionInfoW(path.c_str(), 0, infoSize, infoBuffer.data())) {
                return std::nullopt;
            }

            VS_FIXEDFILEINFO* pFileInfo = nullptr;
            UINT fileInfoLength = 0;
            if (!VerQueryValueW(
                    infoBuffer.data(),
                    L"\\",
                    reinterpret_cast<LPVOID*>(&pFileInfo),
                    &fileInfoLength)
                || pFileInfo == nullptr) {
                return std::nullopt;
            }

            ControlLibFileVersion version{};
            version.major = HIWORD(pFileInfo->dwFileVersionMS);
            version.minor = LOWORD(pFileInfo->dwFileVersionMS);
            version.build = HIWORD(pFileInfo->dwFileVersionLS);
            version.privatePart = LOWORD(pFileInfo->dwFileVersionLS);
            return version;
        }

        std::optional<std::vector<ControlLibFileVersion>> CollectControlLibFileVersionsFromRegistry_()
        {
            std::vector<ControlLibFileVersion> versions;

            const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
                &GUID_DEVCLASS_DISPLAY,
                nullptr,
                nullptr,
                DIGCF_PRESENT);
            if (deviceInfoSet == INVALID_HANDLE_VALUE) {
                return std::nullopt;
            }

            SP_DEVINFO_DATA deviceInfoData{};
            deviceInfoData.cbSize = sizeof(deviceInfoData);

            for (DWORD deviceIndex = 0;; ++deviceIndex) {
                if (!SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData)) {
                    if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                        break;
                    }
                    SetupDiDestroyDeviceInfoList(deviceInfoSet);
                    return std::nullopt;
                }

                const HKEY driverKey = SetupDiOpenDevRegKey(
                    deviceInfoSet,
                    &deviceInfoData,
                    DICS_FLAG_GLOBAL,
                    0,
                    DIREG_DRV,
                    KEY_READ);
                if (driverKey == INVALID_HANDLE_VALUE) {
                    continue;
                }

                const auto controlApiPath = ReadRegistryStringValue_(driverKey, kControlApiPathValueName);
                RegCloseKey(driverKey);
                if (!controlApiPath.has_value()) {
                    continue;
                }

                std::error_code fsError;
                if (!std::filesystem::exists(*controlApiPath, fsError)) {
                    continue;
                }

                const auto fileVersion = TryReadPeFileVersion_(*controlApiPath);
                if (fileVersion.has_value()) {
                    versions.push_back(*fileVersion);
                }
            }

            SetupDiDestroyDeviceInfoList(deviceInfoSet);
            return versions;
        }
    }

    bool IsPreHotfixControlLibVersion(ControlLibFileVersion version) noexcept
    {
        return IsPreHotfixControlLibVersion_(version);
    }

    bool IsZesInitOnlyControlLibVersion(ControlLibFileVersion version) noexcept
    {
        return IsZesInitOnlyControlLibVersion_(version);
    }

    bool IsControlLibVersionMismatch(std::span<const ControlLibFileVersion> versions) noexcept
    {
        bool hasPreHotfix = false;
        bool hasZesInitOnly = false;
        for (const ControlLibFileVersion& version : versions) {
            if (IsPreHotfixControlLibVersion_(version)) {
                hasPreHotfix = true;
            }
            if (IsZesInitOnlyControlLibVersion_(version)) {
                hasZesInitOnly = true;
            }
            if (hasPreHotfix && hasZesInitOnly) {
                return true;
            }
        }
        return false;
    }

    bool AreIgclControlLibsMismatched() noexcept
    {
        try {
            const auto versions = CollectControlLibFileVersionsFromRegistry_();
            if (!versions.has_value()) {
                pmlog_warn("IGCL control library compatibility check could not enumerate display adapters; skipping ctlInit");
                return true;
            }
            if (IsControlLibVersionMismatch(*versions)) {
                pmlog_warn("Mixed IGCL control library versions detected across GPUs; skipping ctlInit");
                return true;
            }
            return false;
        }
        catch (...) {
            pmlog_warn("IGCL control library compatibility check failed; skipping ctlInit");
            return true;
        }
    }
}
