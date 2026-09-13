# Script invoked via cmake -P (not include()). Packaging preflight; fails fast before MSBuild or stubbed package steps.

if(NOT DEFINED PMON_BUILD_CONFIG OR "${PMON_BUILD_CONFIG}" STREQUAL "")
    message(FATAL_ERROR "Packaging preflight requires PMON_BUILD_CONFIG (pass Release).")
endif()

if(NOT PMON_BUILD_CONFIG STREQUAL "Release")
    message(
        FATAL_ERROR
        "PresentMon packaging targets are Release-only. "
        "Requested configuration: ${PMON_BUILD_CONFIG}. "
        "Rebuild with --config Release."
    )
endif()

if(NOT PMON_ARCHITECTURE STREQUAL "x64")
    message(FATAL_ERROR "PresentMon packaging is x64-only. Current architecture: ${PMON_ARCHITECTURE}.")
endif()

foreach(required_option IN ITEMS
    PMON_BUILD_CONSOLE
    PMON_BUILD_SERVICE
    PMON_BUILD_SDK
    PMON_BUILD_UI
    PMON_BUILD_PROVIDER
)
    if(NOT ${required_option})
        message(
            FATAL_ERROR
            "Packaging requires ${required_option}=ON. Reconfigure with all package-producing components enabled."
        )
    endif()
endforeach()

if(NOT CMAKE_VS_MSBUILD_COMMAND OR NOT EXISTS "${CMAKE_VS_MSBUILD_COMMAND}")
    message(
        FATAL_ERROR
        "Packaging requires MSBuild from the Visual Studio generator "
        "(CMAKE_VS_MSBUILD_COMMAND). Reconfigure a fresh binary directory with a "
        "supported Visual Studio generator."
    )
endif()

set(dotnet_framework_reference_root
    "$ENV{ProgramFiles\(x86\)}/Reference Assemblies/Microsoft/Framework/.NETFramework/v4.8"
)
if(NOT IS_DIRECTORY "${dotnet_framework_reference_root}")
    message(
        FATAL_ERROR
        "Packaging requires the .NET Framework 4.8 targeting pack at "
        "\"${dotnet_framework_reference_root}\"."
    )
endif()

if(NOT PMON_SOURCE_DIR OR "${PMON_SOURCE_DIR}" STREQUAL "")
    message(FATAL_ERROR "Packaging preflight requires PMON_SOURCE_DIR (repository root).")
endif()

set(required_authoring_files
    "${PMON_SOURCE_DIR}/IntelPresentMon/PMInstaller/PMInstaller.wixproj"
    "${PMON_SOURCE_DIR}/IntelPresentMon/PMInstaller/PresentMon.wxs"
    "${PMON_SOURCE_DIR}/IntelPresentMon/PMInstaller/license.rtf"
    "${PMON_SOURCE_DIR}/IntelPresentMon/PMInstaller/en-us.wxl"
    "${PMON_SOURCE_DIR}/IntelPresentMon/PMInstallerLib/PMInstallerLib.wixproj"
    "${PMON_SOURCE_DIR}/IntelPresentMon/ServiceMergeModule/ServiceMergeModule.wixproj"
)
foreach(authoring_file IN LISTS required_authoring_files)
    if(NOT EXISTS "${authoring_file}")
        message(FATAL_ERROR "Missing WiX authoring input: ${authoring_file}")
    endif()
endforeach()

set(payload_release_dir "${PMON_OUTPUT_ROOT}/Release")
if(NOT IS_DIRECTORY "${payload_release_dir}")
    file(MAKE_DIRECTORY "${payload_release_dir}")
endif()
set(write_probe "${payload_release_dir}/.pmon_packaging_write_probe")
file(WRITE "${write_probe}" "probe\n")
file(REMOVE "${write_probe}")

set(wix_root "")
if(PMON_WIX_ROOT AND NOT "${PMON_WIX_ROOT}" STREQUAL "")
    set(wix_cache_input "${PMON_WIX_ROOT}")
    string(STRIP "${wix_cache_input}" wix_cache_input)
    while(wix_cache_input MATCHES "(.+)[/\\\\]$")
        set(wix_cache_input "${CMAKE_MATCH_1}")
    endwhile()
    cmake_path(ABSOLUTE_PATH wix_cache_input NORMALIZE OUTPUT_VARIABLE wix_root)
endif()
if(NOT wix_root AND DEFINED ENV{WIX} AND NOT "$ENV{WIX}" STREQUAL "")
    set(wix_env_raw $ENV{WIX})
    set(wix_env_input "${wix_env_raw}")
    string(STRIP "${wix_env_input}" wix_env_input)
    while(wix_env_input MATCHES "(.+)[/\\\\]$")
        set(wix_env_input "${CMAKE_MATCH_1}")
    endwhile()
    if(wix_env_input)
        cmake_path(ABSOLUTE_PATH wix_env_input NORMALIZE OUTPUT_VARIABLE wix_root)
    endif()
endif()
if(NOT wix_root)
    file(
        GLOB wix_install_candidates
        LIST_DIRECTORIES TRUE
        "$ENV{ProgramFiles\(x86\)}/WiX Toolset v3.*"
    )
    if(wix_install_candidates)
        list(SORT wix_install_candidates COMPARE NATURAL ORDER DESCENDING)
        list(GET wix_install_candidates 0 wix_root)
    endif()
endif()

if(NOT wix_root OR NOT IS_DIRECTORY "${wix_root}")
    message(
        FATAL_ERROR
        "WiX Toolset 3 is required for packaging. Set -DPMON_WIX_ROOT to the WiX 3 install root "
        "or set the WIX environment variable. No WiX 3 root was found."
    )
endif()

string(TOLOWER "${wix_root}" wix_root_lower)
if(wix_root_lower MATCHES "wi[x ]? toolset v4" OR wix_root_lower MATCHES "/v4(\\|/|$)")
    message(
        FATAL_ERROR
        "WiX Toolset 4 is not supported. Use WiX Toolset 3.x. Resolved root: ${wix_root}"
    )
endif()

set(wix_bin "${wix_root}/bin")
set(wix_candle "${wix_bin}/candle.exe")
set(wix_wix_exe "${wix_bin}/wix.exe")
if(EXISTS "${wix_wix_exe}" AND NOT EXISTS "${wix_candle}")
    message(
        FATAL_ERROR
        "The resolved WiX path looks like WiX Toolset 4 (wix.exe without candle.exe). "
        "PresentMon requires WiX Toolset 3.x at: ${wix_root}"
    )
endif()

set(wix_dll "${wix_bin}/wix.dll")
if(NOT EXISTS "${wix_dll}")
    set(wix_dll "${wix_root}/SDK/wix.dll")
endif()
if(NOT EXISTS "${wix_dll}")
    message(
        FATAL_ERROR
        "WiX Toolset 3 installation is incomplete. Missing wix.dll under "
        "\"${wix_bin}\" or \"${wix_root}/SDK\"."
    )
endif()

set(required_wix_tools
    "${wix_bin}/heat.exe"
    "${wix_bin}/candle.exe"
    "${wix_bin}/light.exe"
    "${wix_bin}/WixTasks.dll"
    "${wix_bin}/WixUIExtension.dll"
    "${wix_bin}/WixUtilExtension.dll"
)
foreach(wix_tool IN LISTS required_wix_tools)
    if(NOT EXISTS "${wix_tool}")
        message(FATAL_ERROR "WiX Toolset 3 installation is incomplete. Missing: ${wix_tool}")
    endif()
endforeach()

# Every .wixproj is built against this fixed location rather than the WiX
# MSBuild integration of the Visual Studio instance that supplied MSBuild.
set(wix_targets_path "$ENV{ProgramFiles\(x86\)}/MSBuild/Microsoft/WiX/v3.x/wix.targets")
if(NOT EXISTS "${wix_targets_path}")
    message(
        FATAL_ERROR
        "The WiX Toolset 3 MSBuild targets are required for packaging but were "
        "not found at \"${wix_targets_path}\". Reinstall WiX Toolset 3.x with "
        "its MSBuild integration."
    )
endif()

set(msi_output "${PMON_OUTPUT_ROOT}/Release/en-us/PresentMon.msi")
set(msm_output "${PMON_OUTPUT_ROOT}/Release/en-us/PresentMonSharedService.msm")

message(STATUS "")
message(STATUS "PresentMon packaging preflight (Release)")
message(STATUS "  Deployment profile: ${PMON_DEPLOYMENT_PROFILE}")
message(STATUS "  Product version:    ${PMON_PRODUCT_VERSION}")
message(STATUS "  Console version:    ${PMON_VERSION}")
message(STATUS "  WiX root:           ${wix_root}")
message(STATUS "  MSBuild:            ${CMAKE_VS_MSBUILD_COMMAND}")
message(STATUS "  Payload root:       ${payload_release_dir}")
message(STATUS "  UCI included:       ${PMON_UCI_ENABLED}")
message(STATUS "  MSI output:         ${msi_output}")
message(STATUS "  MSM output:         ${msm_output}")
if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
    message(STATUS "  Production signature enforcement: active (PE preflight and MSI sign gate)")
    set(payload_sign_stamp "${PMON_CMAKE_BINARY_DIR}/packaging/Release/ProductionPayloadSigned.stamp")
    if(NOT EXISTS "${payload_sign_stamp}")
        message(
            FATAL_ERROR
            "Production packaging requires a signed native payload stamp at \"${payload_sign_stamp}\". "
            "Run the pmon_sign_production_payload target before pmon_build_packages."
        )
    endif()
else()
    message(STATUS "  Production signature enforcement: inactive (developer profile)")
endif()
message(STATUS "")
