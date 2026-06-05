// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../CefValues.h"
#include "../CliOptions.h"
#include "../../DataBindAccessor.h"
#include <Versioning/BuildId.h>
#include <PresentMonAPI2/PresentMonAPI.h>
#include <CommonUtilities/log/GlobalPolicy.h>
#include <include/cef_version.h>
#include <crtversion.h>
#include <format>
#include <string_view>

#ifndef PM_VER_PRODUCT_STR
#define PM_VER_PRODUCT_STR "unknown"
#endif

#ifndef PM_VER_FILE_STR
#define PM_VER_FILE_STR "unknown"
#endif

#ifndef PM_BUILD_WINSDK_VERSION_STR
#define PM_BUILD_WINSDK_VERSION_STR "unknown"
#endif

#ifndef PM_BUILD_CRT_RUNTIME_STR
#define PM_BUILD_CRT_RUNTIME_STR "unknown"
#endif

namespace p2c::client::util::async
{
    inline std::string FormatApiVersion_(const PM_VERSION& version)
    {
        auto formatted = std::format("{}.{}.{}", version.major, version.minor, version.patch);
        if (version.tag[0] != '\0') {
            formatted += std::format(" {}", version.tag);
        }
        return formatted + std::format(" {} {}", version.hash, version.config);
    }

    inline std::string FormatApiHeaderVersion_()
    {
        return std::format("{}.{}.0", PM_API_VERSION_MAJOR, PM_API_VERSION_MINOR);
    }

    inline std::string FormatCrtVersion_()
    {
#if defined(_VC_CRT_MAJOR_VERSION) && defined(_VC_CRT_MINOR_VERSION) && defined(_VC_CRT_BUILD_VERSION) && defined(_VC_CRT_RBUILD_VERSION)
        return std::format("{}.{}.{}.{}",
            _VC_CRT_MAJOR_VERSION,
            _VC_CRT_MINOR_VERSION,
            _VC_CRT_BUILD_VERSION,
            _VC_CRT_RBUILD_VERSION);
#elif defined(_VC_CRT_MAJOR_VERSION) && defined(_VC_CRT_MINOR_VERSION) && defined(_VC_CRT_BUILD_VERSION)
        return std::format("{}.{}.{}",
            _VC_CRT_MAJOR_VERSION,
            _VC_CRT_MINOR_VERSION,
            _VC_CRT_BUILD_VERSION);
#else
        return "unknown";
#endif
    }

    inline std::string FormatCrtRuntime_()
    {
        constexpr std::string_view runtime{ PM_BUILD_CRT_RUNTIME_STR };
        if constexpr (runtime == "unknown") {
            return "unknown";
        }
        constexpr std::string_view linkType =
            runtime.find("DLL") == std::string_view::npos ? "static" : "dynamic";
        return std::format("{} ({})", runtime, linkType);
    }

    inline std::string FormatVerboseModules_(const cli::Options& opt)
    {
        if (!opt.logVerboseModules) {
            return "None";
        }

        const auto& modules = *opt.logVerboseModules;
        if (modules.empty()) {
            return "None";
        }

        std::string formatted;
        for (const auto module : modules) {
            if (!formatted.empty()) {
                formatted += ", ";
            }
            formatted += log::GetVerboseModuleName(module);
        }
        return formatted;
    }

    class GetAppInfo : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "getAppInfo"; }
        GetAppInfo() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}
        // {} => AppInfo
        Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor& accessor) const override
        {
            const auto& opt = cli::Options::Get();

#ifdef NDEBUG
            constexpr bool isDebugBuild = false;
#else
            constexpr bool isDebugBuild = true;
#endif
            const auto pKernelWrapper = accessor.GetKernelWrapper();
            const auto pClient = pKernelWrapper ? pKernelWrapper->pClient.get() : nullptr;

            auto info = MakeCefObject(
                CefProp{ "productName", std::string{ "Intel(R) PresentMon" } },
                CefProp{ "productVersion", std::string{ PM_VER_PRODUCT_STR } },
                CefProp{ "fileVersion", std::string{ PM_VER_FILE_STR } },
                CefProp{ "apiVersion", FormatApiHeaderVersion_() },
                CefProp{ "middlewareApiVersion", pClient ? pClient->GetMiddlewareApiVersion() : std::string{ "unknown" } },
                CefProp{ "buildId", std::string{ ::pmon::bid::BuildIdUid() } },
                CefProp{ "buildHash", std::string{ ::pmon::bid::BuildIdLongHash() } },
                CefProp{ "buildHashShort", std::string{ ::pmon::bid::BuildIdShortHash() } },
                CefProp{ "buildDateTime", std::string{ ::pmon::bid::BuildIdTimestamp() } },
                CefProp{ "buildConfig", std::string{ ::pmon::bid::BuildIdConfig() } },
                CefProp{ "buildDirty", ::pmon::bid::BuildIdDirtyFlag() },
                CefProp{ "isDebugBuild", isDebugBuild },
                CefProp{ "compileDate", std::string{ __DATE__ } },
                CefProp{ "compileTime", std::string{ __TIME__ } },
                CefProp{ "serviceBuildId", pClient ? pClient->GetServiceBuildId() : std::string{ "unknown" } },
                CefProp{ "serviceBuildTime", pClient ? pClient->GetServiceBuildTime() : std::string{ "unknown" } },
                CefProp{ "serviceVersion", pClient ? pClient->GetServiceVersion() : std::string{ "unknown" } },
                CefProp{ "cefVersion", std::string{ CEF_VERSION } },
                CefProp{ "cefVersionMajor", CEF_VERSION_MAJOR },
                CefProp{ "cefVersionMinor", CEF_VERSION_MINOR },
                CefProp{ "cefVersionPatch", CEF_VERSION_PATCH },
                CefProp{ "chromeVersion", std::format("{}.{}.{}.{}",
                    CHROME_VERSION_MAJOR,
                    CHROME_VERSION_MINOR,
                    CHROME_VERSION_BUILD,
                    CHROME_VERSION_PATCH) },
                CefProp{ "chromeVersionMajor", CHROME_VERSION_MAJOR },
                CefProp{ "chromeVersionMinor", CHROME_VERSION_MINOR },
                CefProp{ "chromeVersionBuild", CHROME_VERSION_BUILD },
                CefProp{ "chromeVersionPatch", CHROME_VERSION_PATCH },
                CefProp{ "cefProcessType", opt.cefType.AsOptional().value_or("browser") },
                CefProp{ "msvcVersion", std::format("{}.{} ({})", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER) },
                CefProp{ "winSdkVersion", std::string{ PM_BUILD_WINSDK_VERSION_STR } },
                CefProp{ "crtVersion", FormatCrtVersion_() },
                CefProp{ "crtRuntime", FormatCrtRuntime_() },
                CefProp{ "logLevel", log::GetLevelName(log::GlobalPolicy::Get().GetLogLevel()) },
                CefProp{ "verboseModules", FormatVerboseModules_(opt) },
                CefProp{ "devModeEnabled", (bool)opt.enableUiDevOptions },
                CefProp{ "debugBlocklistEnabled", (bool)opt.filesWorking },
                CefProp{ "chromiumDebugEnabled", (bool)opt.enableChromiumDebug }
            );
            return Result{ true, std::move(info) };
        }
    };
}
