include_guard(GLOBAL)

set(PMON_PACKAGING_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(PMON_PACKAGING_PYTHON_SCRIPT "${CMAKE_SOURCE_DIR}/Scripts/package.py")

set(
    PMON_WIX_ROOT
    ""
    CACHE PATH
    "Root of a WiX Toolset 3.x installation (optional; WIX env and standard Program Files paths are fallbacks)"
)

function(pmon_normalize_path_input input_path output_variable)
    set(path "${input_path}")
    string(STRIP "${path}" path)
    while(path MATCHES "(.+)[/\\\\]$")
        set(path "${CMAKE_MATCH_1}")
    endwhile()
    set(${output_variable} "${path}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_components_enabled output_variable)
    set(enabled TRUE)
    foreach(option_name IN ITEMS
        PMON_BUILD_CONSOLE
        PMON_BUILD_SERVICE
        PMON_BUILD_SDK
        PMON_BUILD_UI
        PMON_BUILD_PROVIDER
    )
        if(NOT ${option_name})
            set(enabled FALSE)
        endif()
    endforeach()
    set(${output_variable} "${enabled}" PARENT_SCOPE)
endfunction()

function(pmon_require_packaging_native_targets)
    set(required_native_targets
        PresentMonUI
        KernelProcess
        PresentMonService
        PresentMonAPI2
        PresentMonAPI2Loader
        pmon_provider
        PresentMonConsole
    )
    foreach(native_target IN LISTS required_native_targets)
        if(NOT TARGET ${native_target})
            message(
                FATAL_ERROR
                "Packaging requires CMake target \"${native_target}\". "
                "Enable the corresponding PMON_BUILD_* options in an x64 configure."
            )
        endif()
    endforeach()
endfunction()

function(pmon_resolve_wix_root output_variable reason_variable)
    set(resolved_root "")
    set(reason "")

    if(PMON_WIX_ROOT AND NOT "${PMON_WIX_ROOT}" STREQUAL "")
        set(wix_cache_input "${PMON_WIX_ROOT}")
        pmon_normalize_path_input("${wix_cache_input}" wix_cache_input)
        cmake_path(ABSOLUTE_PATH wix_cache_input NORMALIZE OUTPUT_VARIABLE resolved_root)
        if(NOT IS_DIRECTORY "${resolved_root}")
            set(reason "PMON_WIX_ROOT is not a directory: ${resolved_root}")
            set(resolved_root "")
        endif()
    endif()

    if(NOT resolved_root AND DEFINED ENV{WIX} AND NOT "$ENV{WIX}" STREQUAL "")
        set(wix_env_raw $ENV{WIX})
        pmon_normalize_path_input("${wix_env_raw}" wix_env_input)
        if(wix_env_input)
            cmake_path(ABSOLUTE_PATH wix_env_input NORMALIZE OUTPUT_VARIABLE env_wix_root)
            if(IS_DIRECTORY "${env_wix_root}")
                set(resolved_root "${env_wix_root}")
            else()
                set(reason "WIX environment variable is not a directory: ${env_wix_root}")
            endif()
        endif()
    endif()

    if(NOT resolved_root)
        file(
            GLOB wix_install_candidates
            LIST_DIRECTORIES TRUE
            "$ENV{ProgramFiles\(x86\)}/WiX Toolset v3.*"
        )
        if(wix_install_candidates)
            list(SORT wix_install_candidates COMPARE NATURAL ORDER DESCENDING)
            list(GET wix_install_candidates 0 resolved_root)
            set(reason "")
        elseif(NOT reason)
            set(reason "WiX Toolset 3 was not found. Set PMON_WIX_ROOT or the WIX environment variable.")
        endif()
    endif()

    if(resolved_root)
        string(TOLOWER "${resolved_root}" resolved_root_lower)
        if(resolved_root_lower MATCHES "toolset v4")
            set(reason "WiX Toolset 4 is not supported (resolved: ${resolved_root}).")
            set(resolved_root "")
        else()
            set(wix_candle "${resolved_root}/bin/candle.exe")
            set(wix_wix_exe "${resolved_root}/bin/wix.exe")
            if(EXISTS "${wix_wix_exe}" AND NOT EXISTS "${wix_candle}")
                set(
                    reason
                    "Resolved WiX path appears to be WiX Toolset 4 (wix.exe without candle.exe): ${resolved_root}"
                )
                set(resolved_root "")
            endif()
        endif()
    endif()

    set(${output_variable} "${resolved_root}" PARENT_SCOPE)
    set(${reason_variable} "${reason}" PARENT_SCOPE)
endfunction()

function(pmon_print_packaging_summary wix_root wix_reason)
    set(payload_release_dir "${PMON_OUTPUT_ROOT}/Release")
    set(msi_output "${payload_release_dir}/en-us/PresentMon.msi")
    set(msm_output "${payload_release_dir}/en-us/PresentMonSharedService.msm")

    message(STATUS "PresentMon packaging")
    if(wix_root)
        message(STATUS "  Packaging targets:  available (x64, all package components ON)")
        message(STATUS "  WiX root (preview):   ${wix_root}")
    elseif(wix_reason)
        message(STATUS "  Packaging targets:  unavailable (native developer builds remain available)")
        message(STATUS "  WiX root (preview):   not found (${wix_reason})")
    else()
        message(STATUS "  Packaging targets:  unavailable (native developer builds remain available)")
        message(STATUS "  WiX root (preview):   not found")
    endif()
    if(CMAKE_VS_MSBUILD_COMMAND)
        message(STATUS "  MSBuild:              ${CMAKE_VS_MSBUILD_COMMAND}")
    else()
        message(STATUS "  MSBuild:              unavailable (Visual Studio generator required for packaging)")
    endif()
    message(STATUS "  Payload root:         ${payload_release_dir}")
    message(STATUS "  UCI included:         ${PMON_UCI_ENABLED}")
    message(STATUS "  MSI output:           ${msi_output}")
    message(STATUS "  MSM output:           ${msm_output}")
    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        message(STATUS "  Production signature enforcement: active")
    else()
        message(STATUS "  Production signature enforcement: inactive")
    endif()
endfunction()

function(pmon_packaging_native_dir_with_trailing_sep input_path output_variable)
    file(TO_NATIVE_PATH "${input_path}" native_path)
    if(NOT native_path MATCHES "[/\\\\]$")
        string(APPEND native_path "\\")
    endif()
    set(${output_variable} "${native_path}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_msbuild_dir_path input_path output_variable)
    get_filename_component(abs_path "${input_path}" ABSOLUTE)
    file(TO_CMAKE_PATH "${abs_path}" cmake_path)
    string(REPLACE "\\" "/" msbuild_path "${cmake_path}")
    if(NOT msbuild_path MATCHES "/$")
        string(APPEND msbuild_path "/")
    endif()
    set(${output_variable} "${msbuild_path}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_msbuild_file_path input_path output_variable)
    get_filename_component(abs_path "${input_path}" ABSOLUTE)
    file(TO_CMAKE_PATH "${abs_path}" cmake_path)
    string(REPLACE "\\" "/" msbuild_path "${cmake_path}")
    set(${output_variable} "${msbuild_path}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_repo_root_native output_variable)
    pmon_packaging_msbuild_dir_path("${CMAKE_SOURCE_DIR}" native_root)
    set(${output_variable} "${native_root}" PARENT_SCOPE)
endfunction()

# The retained .wixproj files default WixTargetsPath to
# $(MSBuildExtensionsPath32)/Microsoft/WiX/v3.x/Wix.targets, which resolves
# inside whichever Visual Studio instance supplied MSBuild and so requires a
# Visual Studio WiX extension in that instance. The packaging lane selects the
# WiX 3 MSBuild integration itself instead: the targets come from the fixed WiX
# 3 MSBuild location, and the tasks assembly and tool directory come from the
# WiX 3 root already resolved by the PMON_WIX_ROOT/WIX/Program Files policy.
# Passing them as global properties makes both supported generators build the
# same .wixproj files against the same WiX 3 installation.
set(
    PMON_PACKAGING_WIX_TARGETS_PATH
    "$ENV{ProgramFiles\(x86\)}/MSBuild/Microsoft/WiX/v3.x/wix.targets"
)

function(pmon_packaging_wix_msbuild_integration_properties output_variable)
    pmon_packaging_msbuild_file_path(
        "${PMON_PACKAGING_WIX_TARGETS_PATH}"
        wix_targets_path_msbuild
    )
    pmon_packaging_msbuild_file_path(
        "${PMON_PACKAGING_WIX_ROOT}/bin/WixTasks.dll"
        wix_tasks_path_msbuild
    )
    # WixToolPath keeps its trailing separator because the WiX targets
    # concatenate it directly, as in $(WixToolPath)..\sdk\. These are the
    # forward-slash forms the other directory properties here already use, which
    # also keeps a trailing separator out of MSBuild command-line quoting.
    pmon_packaging_msbuild_dir_path(
        "${PMON_PACKAGING_WIX_ROOT}/bin"
        wix_tool_path_msbuild
    )
    set(
        ${output_variable}
        "/p:WixTargetsPath=${wix_targets_path_msbuild}"
        "/p:WixTasksPath=${wix_tasks_path_msbuild}"
        "/p:WixToolPath=${wix_tool_path_msbuild}"
        PARENT_SCOPE
    )
endfunction()

function(pmon_packaging_wix_cmake_common_msbuild_properties output_variable)
    pmon_packaging_repo_root_native(repo_root_native)
    pmon_packaging_wix_msbuild_integration_properties(wix_integration_props)
    pmon_packaging_msbuild_dir_path(
        "${PMON_OUTPUT_ROOT}/Release"
        payload_root_native
    )
    pmon_packaging_msbuild_dir_path(
        "${PMON_OUTPUT_ROOT}/Release"
        release_payload_dir_native
    )
    set(
        extension_dll
        "${PMON_OUTPUT_ROOT}/obj/PMInstallerExtension-Release/PMInstallerExtension.dll"
    )
    pmon_packaging_msbuild_file_path("${extension_dll}" extension_dll_native)
    set(
        wixlib_output
        "${PMON_OUTPUT_ROOT}/Release/PMInstallerLib.wixlib"
    )
    pmon_packaging_msbuild_file_path("${wixlib_output}" wixlib_output_native)
    set(uci_stage_dir "${CMAKE_BINARY_DIR}/packaging/Release/UciDistStage")
    set(uci_fragment "${CMAKE_BINARY_DIR}/packaging/Release/Generated/UciDist.wxs")
    pmon_packaging_msbuild_dir_path("${uci_stage_dir}" uci_stage_native)
    pmon_packaging_msbuild_file_path("${uci_fragment}" uci_fragment_native)
    set(provider_project_dir "${CMAKE_SOURCE_DIR}/Provider")
    pmon_packaging_msbuild_dir_path(
        "${provider_project_dir}"
        provider_project_dir_native
    )
    set(api2_project_dir "${CMAKE_SOURCE_DIR}/IntelPresentMon/PresentMonAPI2")
    pmon_packaging_msbuild_dir_path(
        "${api2_project_dir}"
        api2_project_dir_native
    )
    set(api2_header "${CMAKE_SOURCE_DIR}/IntelPresentMon/PresentMonAPI2/PresentMonAPI.h")
    pmon_packaging_msbuild_file_path("${api2_header}" api2_header_native)
    set(
        loader_import_lib
        "${PMON_OUTPUT_ROOT}/Release/PresentMonAPI2Loader.lib"
    )
    pmon_packaging_msbuild_file_path("${loader_import_lib}" loader_import_lib_native)
    set(
        props
        ${wix_integration_props}
        /p:PmonCMakePackaging=true
        /p:Configuration=Release
        /p:Platform=x86
        /p:BuildProjectReferences=false
        /p:PresentMonVersion=${PMON_VERSION}
        /p:PresentMonProductVersion=${PMON_PRODUCT_VERSION}
        "/p:PmonSolutionDir=${repo_root_native}"
        "/p:PmonPayloadRoot=${payload_root_native}"
        "/p:PmonInstallerExtensionDll=${extension_dll_native}"
        "/p:PmonWixLibPath=${wixlib_output_native}"
        "/p:PmonUciDistDir=${uci_stage_native}"
        "/p:PmonUciDistFragmentPath=${uci_fragment_native}"
        "/p:PmonProviderProjectDir=${provider_project_dir_native}"
        "/p:PmonPresentMonAPI2ProjectDir=${api2_project_dir_native}"
        "/p:PmonProviderTargetPath=${release_payload_dir_native}$<TARGET_FILE_NAME:pmon_provider>"
        "/p:PmonPresentMonTargetPath=${release_payload_dir_native}$<TARGET_FILE_NAME:PresentMonUI>"
        "/p:PmonPresentMonTargetDir=${release_payload_dir_native}"
        "/p:PmonPresentMonTargetFileName=$<TARGET_FILE_NAME:PresentMonUI>"
        "/p:PmonKernelProcessTargetPath=${release_payload_dir_native}$<TARGET_FILE_NAME:KernelProcess>"
        "/p:PmonKernelProcessTargetFileName=$<TARGET_FILE_NAME:KernelProcess>"
        "/p:PmonPresentMonServiceTargetPath=${release_payload_dir_native}$<TARGET_FILE_NAME:PresentMonService>"
        "/p:PmonPresentMonServiceTargetDir=${release_payload_dir_native}"
        "/p:PmonPresentMonServiceTargetFileName=$<TARGET_FILE_NAME:PresentMonService>"
        "/p:PmonPresentMonAPI2TargetPath=${release_payload_dir_native}$<TARGET_FILE_NAME:PresentMonAPI2>"
        "/p:PmonPresentMonAPI2TargetFileName=$<TARGET_FILE_NAME:PresentMonAPI2>"
        "/p:PmonPresentMonAPI2LoaderTargetPath=${release_payload_dir_native}$<TARGET_FILE_NAME:PresentMonAPI2Loader>"
        "/p:PmonPresentMonAPI2LoaderTargetDir=${release_payload_dir_native}"
        "/p:PmonPresentMonAPI2LoaderTargetFileName=$<TARGET_FILE_NAME:PresentMonAPI2Loader>"
        "/p:PmonPresentMonAPI2LoaderLibSource=${loader_import_lib_native}"
        "/p:PmonPresentMonAPI2HeaderSource=${api2_header_native}"
        "/p:PmonConsoleTargetPath=${release_payload_dir_native}$<TARGET_FILE_NAME:PresentMonConsole>"
        "/p:PmonConsoleTargetFileName=$<TARGET_FILE_NAME:PresentMonConsole>"
    )
    set(${output_variable} "${props}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_wixlib_msbuild_properties output_variable)
    pmon_packaging_wix_cmake_common_msbuild_properties(common_props)
    pmon_packaging_msbuild_dir_path(
        "${PMON_OUTPUT_ROOT}/Release"
        wixlib_output_dir_native
    )
    pmon_packaging_msbuild_dir_path(
        "${CMAKE_BINARY_DIR}/packaging/Release/intermediate/PMInstallerLib"
        wixlib_intermediate_native
    )
    set(
        props
        ${common_props}
        "/p:PmonOutputPath=${wixlib_output_dir_native}"
        "/p:PmonIntermediateOutputPath=${wixlib_intermediate_native}"
    )
    set(${output_variable} "${props}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_installer_msbuild_properties output_variable)
    pmon_packaging_wix_cmake_common_msbuild_properties(common_props)
    pmon_packaging_msbuild_dir_path(
        "${PMON_OUTPUT_ROOT}/Release"
        installer_output_dir_native
    )
    pmon_packaging_msbuild_dir_path(
        "${CMAKE_BINARY_DIR}/packaging/Release/intermediate/PMInstaller"
        installer_intermediate_native
    )
    set(
        props
        ${common_props}
        "/p:PmonOutputPath=${installer_output_dir_native}"
        "/p:PmonIntermediateOutputPath=${installer_intermediate_native}"
    )
    set(${output_variable} "${props}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_merge_module_msbuild_properties output_variable)
    pmon_packaging_wix_cmake_common_msbuild_properties(common_props)
    pmon_packaging_msbuild_dir_path(
        "${PMON_OUTPUT_ROOT}/Release/en-us"
        msm_output_dir_native
    )
    pmon_packaging_msbuild_dir_path(
        "${CMAKE_BINARY_DIR}/packaging/Release/intermediate/ServiceMergeModule"
        msm_intermediate_native
    )
    set(
        props
        ${common_props}
        "/p:PmonOutputPath=${msm_output_dir_native}"
        "/p:PmonIntermediateOutputPath=${msm_intermediate_native}"
    )
    set(${output_variable} "${props}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_resolve_wix_dll wix_root output_variable)
    set(wix_dll "${wix_root}/bin/wix.dll")
    if(NOT EXISTS "${wix_dll}")
        set(wix_dll "${wix_root}/SDK/wix.dll")
    endif()
    set(${output_variable} "${wix_dll}" PARENT_SCOPE)
endfunction()

function(pmon_packaging_preflight_script_args output_variable build_config)
    set(
        ${output_variable}
        "${CMAKE_COMMAND}"
        -DPMON_BUILD_CONFIG=${build_config}
        -DPMON_ARCHITECTURE=${PMON_ARCHITECTURE}
        -DPMON_BUILD_CONSOLE=${PMON_BUILD_CONSOLE}
        -DPMON_BUILD_SERVICE=${PMON_BUILD_SERVICE}
        -DPMON_BUILD_SDK=${PMON_BUILD_SDK}
        -DPMON_BUILD_UI=${PMON_BUILD_UI}
        -DPMON_BUILD_PROVIDER=${PMON_BUILD_PROVIDER}
        -DPMON_OUTPUT_ROOT=${PMON_OUTPUT_ROOT}
        -DPMON_VERSION=${PMON_VERSION}
        -DPMON_PRODUCT_VERSION=${PMON_PRODUCT_VERSION}
        -DPMON_DEPLOYMENT_PROFILE=${PMON_DEPLOYMENT_PROFILE}
        -DPMON_UCI_ENABLED=${PMON_UCI_ENABLED}
        # The root resolved at configure time, not the raw PMON_WIX_ROOT option,
        # which is empty whenever WiX came from WIX or Program Files. Preflight
        # must validate the same installation that WixTasksPath and WixToolPath
        # already encode, so this leaves it nothing to rediscover.
        -DPMON_WIX_ROOT=${PMON_PACKAGING_WIX_ROOT}
        -DPMON_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DPMON_CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
        -DCMAKE_VS_MSBUILD_COMMAND=${CMAKE_VS_MSBUILD_COMMAND}
        PARENT_SCOPE
    )
endfunction()

function(pmon_packaging_preflight_command output_variable build_config)
    pmon_packaging_preflight_script_args(preflight_args "${build_config}")
    set(
        ${output_variable}
        ${preflight_args}
        -P "${PMON_PACKAGING_CMAKE_DIR}/PresentMonPackagingPreflight.cmake"
        PARENT_SCOPE
    )
endfunction()

function(pmon_add_pm_installer_extension_target)
    if(TARGET PMInstallerExtension)
        return()
    endif()

    if(
        NOT DEFINED PMON_PACKAGING_WIX_ROOT
        OR "${PMON_PACKAGING_WIX_ROOT}" STREQUAL ""
    )
        message(
            FATAL_ERROR
            "PMInstallerExtension packaging requires PMON_PACKAGING_WIX_ROOT."
        )
    endif()
    set(wix_root "${PMON_PACKAGING_WIX_ROOT}")

    set(extension_project_dir "${CMAKE_SOURCE_DIR}/IntelPresentMon/PMInstallerExtension")
    set(extension_csproj "${extension_project_dir}/PMInstallerExtension.csproj")
    set(extension_output_dir "${PMON_OUTPUT_ROOT}/obj/PMInstallerExtension-Release")
    set(extension_dll "${extension_output_dir}/PMInstallerExtension.dll")
    set(extension_pdb "${extension_output_dir}/PMInstallerExtension.pdb")
    set(
        extension_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/PMInstallerExtension.stamp"
    )
    set(
        extension_intermediate_dir
        "${CMAKE_BINARY_DIR}/packaging/Release/intermediate/PMInstallerExtension"
    )

    pmon_packaging_resolve_wix_dll("${wix_root}" wix_dll)
    pmon_packaging_native_dir_with_trailing_sep("${wix_root}" wix_root_native)
    pmon_packaging_native_dir_with_trailing_sep(
        "${extension_output_dir}"
        extension_output_dir_native
    )
    pmon_packaging_native_dir_with_trailing_sep(
        "${extension_intermediate_dir}"
        extension_intermediate_dir_native
    )

    pmon_packaging_preflight_command(preflight_command Release)

    set(
        extension_depends
        "${extension_csproj}"
        "${extension_project_dir}/PMPreProcessor.cs"
        "${extension_project_dir}/Properties/AssemblyInfo.cs"
        "${wix_dll}"
    )

    add_custom_command(
        OUTPUT "${extension_stamp}"
        BYPRODUCTS "${extension_dll}" "${extension_pdb}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/packaging/Release"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${extension_output_dir}"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${extension_intermediate_dir}"
        COMMAND ${preflight_command}
        COMMAND
            "${CMAKE_VS_MSBUILD_COMMAND}"
            "${extension_csproj}"
            /t:Build
            /p:Configuration=Release
            /p:Platform=AnyCPU
            "/p:OutputPath=${extension_output_dir_native}"
            "/p:IntermediateOutputPath=${extension_intermediate_dir_native}"
            "/p:WIX=${wix_root_native}"
            /p:BuildProjectReferences=false
            /p:UseSharedCompilation=false
            /nodeReuse:false
            /m:1
            /nologo
            /v:minimal
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPMON_VERIFY_OUTPUT=${extension_dll}"
            -P "${PMON_PACKAGING_CMAKE_DIR}/PresentMonPackagingVerifyOutput.cmake"
        COMMAND "${CMAKE_COMMAND}" -E touch "${extension_stamp}"
        DEPENDS ${extension_depends}
        COMMENT "Building PMInstallerExtension (WiX preprocessor extension, Release)"
        VERBATIM
    )

    add_custom_target(
        PMInstallerExtension
        DEPENDS "${extension_stamp}"
    )
    set_target_properties(
        PMInstallerExtension
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
endfunction()

function(pmon_add_generate_uci_wix_target)
    if(TARGET pmon_generate_uci_wix)
        return()
    endif()

    if(
        NOT DEFINED PMON_PACKAGING_WIX_ROOT
        OR "${PMON_PACKAGING_WIX_ROOT}" STREQUAL ""
    )
        message(
            FATAL_ERROR
            "pmon_generate_uci_wix requires PMON_PACKAGING_WIX_ROOT."
        )
    endif()
    set(wix_root "${PMON_PACKAGING_WIX_ROOT}")

    set(packaging_script "${PMON_PACKAGING_PYTHON_SCRIPT}")
    set(uci_stage_dir "${CMAKE_BINARY_DIR}/packaging/Release/UciDistStage")
    set(uci_generated_dir "${CMAKE_BINARY_DIR}/packaging/Release/Generated")
    set(uci_fragment "${uci_generated_dir}/UciDist.wxs")
    set(uci_manifest "${uci_generated_dir}/.uci_runtime_manifest.txt")
    set(uci_stamp "${CMAKE_BINARY_DIR}/packaging/Release/UciDist.stamp")

    pmon_packaging_preflight_command(preflight_command Release)

    set(
        uci_command_depends
        "${packaging_script}"
        "${CMAKE_SOURCE_DIR}/Scripts/pmon/packaging.py"
    )
    if(PMON_UCI_ENABLED)
        list(APPEND uci_command_depends "${PMON_UCI_RUNTIME_DLL}")
        file(GLOB_RECURSE uci_sdk_files CONFIGURE_DEPENDS
            "${PMON_UCI_EFFECTIVE_ROOT}/*"
        )
        list(APPEND uci_command_depends ${uci_sdk_files})
    endif()

    if(PMON_UCI_ENABLED)
        set(uci_enabled_arg "true")
    else()
        set(uci_enabled_arg "false")
    endif()

    add_custom_command(
        OUTPUT "${uci_stamp}"
        BYPRODUCTS "${uci_fragment}" "${uci_manifest}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${uci_stage_dir}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${uci_generated_dir}"
        COMMAND ${preflight_command}
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${packaging_script}"
            stage-uci
            --enabled "${uci_enabled_arg}"
            --sdk-root "${PMON_UCI_EFFECTIVE_ROOT}"
            --stage-dir "${uci_stage_dir}"
            --manifest-path "${uci_manifest}"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${packaging_script}"
            generate-uci
            --enabled "${uci_enabled_arg}"
            --stage-dir "${uci_stage_dir}"
            --output-path "${uci_fragment}"
            --wix-root "${wix_root}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${uci_stamp}"
        DEPENDS ${uci_command_depends}
        COMMENT "Staging runtime UCI payload and generating UciDist.wxs (Release)"
        VERBATIM
    )

    add_custom_target(
        pmon_generate_uci_wix
        DEPENDS "${uci_stamp}"
    )
    set_target_properties(
        pmon_generate_uci_wix
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
endfunction()

function(pmon_add_pm_installer_lib_target)
    if(TARGET PMInstallerLib)
        return()
    endif()

    if(
        NOT DEFINED PMON_PACKAGING_WIX_ROOT
        OR "${PMON_PACKAGING_WIX_ROOT}" STREQUAL ""
    )
        message(
            FATAL_ERROR
            "PMInstallerLib packaging requires PMON_PACKAGING_WIX_ROOT."
        )
    endif()
    set(wix_root "${PMON_PACKAGING_WIX_ROOT}")

    set(wixlib_project_dir "${CMAKE_SOURCE_DIR}/IntelPresentMon/PMInstallerLib")
    set(wixlib_wixproj "${wixlib_project_dir}/PMInstallerLib.wixproj")
    set(
        wixlib_output
        "${PMON_OUTPUT_ROOT}/Release/PMInstallerLib.wixlib"
    )
    set(wixlib_stamp "${CMAKE_BINARY_DIR}/packaging/Release/PMInstallerLib.stamp")
    set(
        wixlib_intermediate_dir
        "${CMAKE_BINARY_DIR}/packaging/Release/intermediate/PMInstallerLib"
    )
    set(
        native_snapshot
        "${CMAKE_BINARY_DIR}/packaging/Release/PMInstallerLib.native_snapshot.txt"
    )
    set(
        verify_native_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(uci_fragment "${CMAKE_BINARY_DIR}/packaging/Release/Generated/UciDist.wxs")
    set(
        extension_dll
        "${PMON_OUTPUT_ROOT}/obj/PMInstallerExtension-Release/PMInstallerExtension.dll"
    )

    pmon_packaging_wixlib_msbuild_properties(wixlib_msbuild_props)
    pmon_packaging_native_dir_with_trailing_sep("${wix_root}" wix_root_native)
    pmon_packaging_preflight_command(preflight_command Release)

    set(uci_stamp "${CMAKE_BINARY_DIR}/packaging/Release/UciDist.stamp")

    set(
        wixlib_command_depends
        "${wixlib_wixproj}"
        "${wixlib_project_dir}/Library.wxs"
        "${wixlib_project_dir}/Library.en-us.wxl"
        "${CMAKE_SOURCE_DIR}/IntelPresentMon/PmonCMakePackaging.wixproj.props"
        "${extension_dll}"
        "${uci_fragment}"
        "${uci_stamp}"
        "${verify_native_script}"
        "$<TARGET_FILE:PresentMonService>"
        "$<TARGET_FILE:PresentMonAPI2>"
        "$<TARGET_FILE_DIR:PresentMonService>/ddETWExternal.xml"
    )

    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        list(
            APPEND
            wixlib_command_depends
            "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPayloadSigned.stamp"
            "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPackageInputsVerified.stamp"
        )
    endif()

    set(release_payload_dir "${PMON_OUTPUT_ROOT}/Release")
    set(
        wixlib_native_paths_list
        "${release_payload_dir}/$<TARGET_FILE_NAME:PresentMonService>|${release_payload_dir}/$<TARGET_FILE_NAME:PresentMonAPI2>|${release_payload_dir}/ddETWExternal.xml"
    )

    set(wixlib_production_relink_commands "")
    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        set(
            wixlib_production_relink_commands
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${wixlib_output}"
            COMMAND "${CMAKE_COMMAND}" -E remove_directory "${wixlib_intermediate_dir}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${wixlib_intermediate_dir}"
        )
    endif()

    add_custom_command(
        OUTPUT "${wixlib_stamp}"
        BYPRODUCTS "${wixlib_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/packaging/Release"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${wixlib_intermediate_dir}"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${PMON_OUTPUT_ROOT}/Release"
        COMMAND ${preflight_command}
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${verify_native_script}"
            native-snapshot record
            --snapshot-path "${native_snapshot}"
            --native-paths-list "${wixlib_native_paths_list}"
        ${wixlib_production_relink_commands}
        COMMAND
            "${CMAKE_VS_MSBUILD_COMMAND}"
            "${wixlib_wixproj}"
            /t:Build
            ${wixlib_msbuild_props}
            "/p:WIX=${wix_root_native}"
            /p:UseSharedCompilation=false
            /nodeReuse:false
            /m:1
            /nologo
            /v:minimal
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPMON_VERIFY_OUTPUT=${wixlib_output}"
            -P "${PMON_PACKAGING_CMAKE_DIR}/PresentMonPackagingVerifyOutput.cmake"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${verify_native_script}"
            native-snapshot verify
            --snapshot-path "${native_snapshot}"
            --native-paths-list "${wixlib_native_paths_list}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${wixlib_stamp}"
        DEPENDS ${wixlib_command_depends}
        COMMENT "Building PMInstallerLib.wixlib (WiX shared service library, Release)"
        VERBATIM
    )

    add_custom_target(
        PMInstallerLib
        DEPENDS "${wixlib_stamp}"
    )
    set_target_properties(
        PMInstallerLib
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
endfunction()

function(pmon_packaging_installer_native_pe_paths output_variable)
    set(release_dir "${PMON_OUTPUT_ROOT}/Release")
    set(
        ${output_variable}
        "${release_dir}/$<TARGET_FILE_NAME:PresentMonUI>|${release_dir}/$<TARGET_FILE_NAME:KernelProcess>|${release_dir}/$<TARGET_FILE_NAME:PresentMonService>|${release_dir}/$<TARGET_FILE_NAME:PresentMonAPI2>|${release_dir}/$<TARGET_FILE_NAME:PresentMonAPI2Loader>|${release_dir}/$<TARGET_FILE_NAME:pmon_provider>|${release_dir}/$<TARGET_FILE_NAME:PresentMonConsole>"
        PARENT_SCOPE
    )
endfunction()

function(pmon_packaging_merge_module_native_paths output_variable)
    set(release_dir "${PMON_OUTPUT_ROOT}/Release")
    set(
        ${output_variable}
        "${release_dir}/$<TARGET_FILE_NAME:PresentMonService>|${release_dir}/$<TARGET_FILE_NAME:PresentMonAPI2>|${release_dir}/ddETWExternal.xml"
        PARENT_SCOPE
    )
endfunction()

function(pmon_packaging_merge_module_native_pe_paths output_variable)
    set(release_dir "${PMON_OUTPUT_ROOT}/Release")
    set(
        ${output_variable}
        "${release_dir}/$<TARGET_FILE_NAME:PresentMonService>|${release_dir}/$<TARGET_FILE_NAME:PresentMonAPI2>"
        PARENT_SCOPE
    )
endfunction()

function(pmon_packaging_uci_runtime_paths uci_stage_dir uci_manifest_path)
    set(
        ${uci_stage_dir}
        "${CMAKE_BINARY_DIR}/packaging/Release/UciDistStage"
        PARENT_SCOPE
    )
    set(
        ${uci_manifest_path}
        "${CMAKE_BINARY_DIR}/packaging/Release/Generated/.uci_runtime_manifest.txt"
        PARENT_SCOPE
    )
endfunction()

function(pmon_add_developer_release_installer_to_default_build)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER")
        return()
    endif()

    # Visual Studio's ALL_BUILD project references are not configuration-
    # specific. Keep this bridge in ALL for both configurations, make its
    # command Release-only, and let the canonical installer target retain the
    # explicit packaging graph and Release-only preflight.
    string(
        CONCAT
        developer_release_installer_command
        "$<$<CONFIG:Release>:${CMAKE_COMMAND};--build;"
        "${CMAKE_BINARY_DIR};--config;Release;--target;"
        "PresentMonInstaller;--parallel;1>"
    )
    add_custom_target(
        pmon_developer_release_installer ALL
        COMMAND "${developer_release_installer_command}"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    set_target_properties(
        pmon_developer_release_installer
        PROPERTIES FOLDER "Packaging"
    )
endfunction()

function(pmon_add_present_mon_installer_target)
    if(TARGET PresentMonInstaller)
        return()
    endif()

    if(
        NOT DEFINED PMON_PACKAGING_WIX_ROOT
        OR "${PMON_PACKAGING_WIX_ROOT}" STREQUAL ""
    )
        message(
            FATAL_ERROR
            "PresentMonInstaller packaging requires PMON_PACKAGING_WIX_ROOT."
        )
    endif()
    set(wix_root "${PMON_PACKAGING_WIX_ROOT}")

    set(installer_project_dir "${CMAKE_SOURCE_DIR}/IntelPresentMon/PMInstaller")
    set(installer_wixproj "${installer_project_dir}/PMInstaller.wixproj")
    set(msi_output "${PMON_OUTPUT_ROOT}/Release/en-us/PresentMon.msi")
    set(installer_stamp "${CMAKE_BINARY_DIR}/packaging/Release/PresentMonInstaller.stamp")
    set(
        installer_intermediate_dir
        "${CMAKE_BINARY_DIR}/packaging/Release/intermediate/PMInstaller"
    )
    set(
        installer_log_dir
        "${CMAKE_BINARY_DIR}/packaging/Release/logs"
    )
    set(installer_binlog "${installer_log_dir}/PMInstaller.binlog")
    set(
        native_snapshot
        "${CMAKE_BINARY_DIR}/packaging/Release/PresentMonInstaller.native_snapshot.txt"
    )
    set(
        verify_native_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(
        link_preflight_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    pmon_packaging_uci_runtime_paths(uci_stage_dir uci_manifest_path)
    set(link_preflight_skip_pe_args "")
    set(
        wixlib_output
        "${PMON_OUTPUT_ROOT}/Release/PMInstallerLib.wixlib"
    )
    set(
        extension_dll
        "${PMON_OUTPUT_ROOT}/obj/PMInstallerExtension-Release/PMInstallerExtension.dll"
    )

    set(
        verify_msi_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(
        wixlib_native_snapshot
        "${CMAKE_BINARY_DIR}/packaging/Release/PMInstallerLib.native_snapshot.txt"
    )
    pmon_packaging_merge_module_native_paths(wixlib_native_paths)

    pmon_packaging_installer_msbuild_properties(installer_msbuild_props)
    pmon_packaging_native_dir_with_trailing_sep("${wix_root}" wix_root_native)
    pmon_packaging_preflight_command(preflight_command Release)
    pmon_packaging_installer_native_pe_paths(installer_native_pe_paths)

    set(wixlib_stamp "${CMAKE_BINARY_DIR}/packaging/Release/PMInstallerLib.stamp")

    set(
        installer_command_depends
        "${installer_wixproj}"
        "${installer_project_dir}/PresentMon.wxs"
        "${installer_project_dir}/CefBinaries.wxs"
        "${installer_project_dir}/CefResources.wxs"
        "${installer_project_dir}/en-us.wxl"
        "${installer_project_dir}/license.rtf"
        "${CMAKE_SOURCE_DIR}/IntelPresentMon/PmonCMakePackaging.wixproj.props"
        "${wixlib_output}"
        "${wixlib_stamp}"
        "${extension_dll}"
        "${verify_native_script}"
        "${link_preflight_script}"
        "${verify_msi_script}"
        "${CMAKE_SOURCE_DIR}/Scripts/pmon/packaging.py"
        "${wixlib_native_snapshot}"
    )

    set(installer_wixlib_snapshot_verify_command "")
    set(installer_embedded_native_verify_command "")
    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        set(
            installer_wixlib_snapshot_verify_command
            COMMAND
                "${PMON_PYTHON_EXECUTABLE}"
                "${verify_native_script}"
                native-snapshot verify
                --snapshot-path "${wixlib_native_snapshot}"
                --native-paths-list "${wixlib_native_paths}"
        )
        set(
            installer_embedded_native_verify_command
            COMMAND
                "${PMON_PYTHON_EXECUTABLE}"
                "${verify_msi_script}"
                verify-msi
                --msi-path "${msi_output}"
                --payload-root "${PMON_OUTPUT_ROOT}/Release"
                --repo-root "${CMAKE_SOURCE_DIR}"
                --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
                --native-paths-list "${installer_native_pe_paths}"
                --snapshot-path "${native_snapshot}"
                --skip-msi-authenticode
        )
    endif()

    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        list(
            APPEND
            installer_command_depends
            "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPayloadSigned.stamp"
            "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPackageInputsVerified.stamp"
        )
    endif()

    add_custom_command(
        OUTPUT "${installer_stamp}"
        BYPRODUCTS "${msi_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/packaging/Release"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${installer_intermediate_dir}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${installer_log_dir}"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${PMON_OUTPUT_ROOT}/Release/en-us"
        COMMAND ${preflight_command}
        ${installer_wixlib_snapshot_verify_command}
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${link_preflight_script}"
            verify-link-preflight
            --payload-root "${PMON_OUTPUT_ROOT}/Release"
            --repo-root "${CMAKE_SOURCE_DIR}"
            --dependency-root "${PMON_DEPENDENCY_ROOT}"
            --toolset "${PMON_VS_TOOLSET}"
            --deployment-profile "${PMON_DEPLOYMENT_PROFILE}"
            --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
            --native-paths-list "${installer_native_pe_paths}"
            --uci-enabled "${PMON_UCI_ENABLED}"
            --uci-stage-dir "${uci_stage_dir}"
            --uci-manifest-path "${uci_manifest_path}"
            ${link_preflight_skip_pe_args}
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${verify_native_script}"
            native-snapshot record
            --snapshot-path "${native_snapshot}"
            --native-paths-list "${installer_native_pe_paths}"
        COMMAND
            "${CMAKE_VS_MSBUILD_COMMAND}"
            "${installer_wixproj}"
            /t:Build
            ${installer_msbuild_props}
            "/p:WIX=${wix_root_native}"
            /p:UseSharedCompilation=false
            /nodeReuse:false
            "/bl:${installer_binlog}"
            /m:1
            /nologo
            /v:minimal
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPMON_VERIFY_OUTPUT=${msi_output}"
            -P "${PMON_PACKAGING_CMAKE_DIR}/PresentMonPackagingVerifyOutput.cmake"
        ${installer_embedded_native_verify_command}
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${verify_native_script}"
            native-snapshot verify
            --snapshot-path "${native_snapshot}"
            --native-paths-list "${installer_native_pe_paths}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${installer_stamp}"
        DEPENDS ${installer_command_depends}
        COMMENT "Linking PresentMon.msi (WiX product package, Release)"
        VERBATIM
    )

    add_custom_target(
        PresentMonInstaller
        DEPENDS "${installer_stamp}"
    )
    set_target_properties(
        PresentMonInstaller
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
    pmon_add_developer_release_installer_to_default_build()
endfunction()

function(pmon_add_present_mon_merge_module_target)
    if(TARGET PresentMonMergeModule)
        return()
    endif()

    if(
        NOT DEFINED PMON_PACKAGING_WIX_ROOT
        OR "${PMON_PACKAGING_WIX_ROOT}" STREQUAL ""
    )
        message(
            FATAL_ERROR
            "PresentMonMergeModule packaging requires PMON_PACKAGING_WIX_ROOT."
        )
    endif()
    set(wix_root "${PMON_PACKAGING_WIX_ROOT}")

    set(msm_project_dir "${CMAKE_SOURCE_DIR}/IntelPresentMon/ServiceMergeModule")
    set(msm_wixproj "${msm_project_dir}/ServiceMergeModule.wixproj")
    set(msm_output "${PMON_OUTPUT_ROOT}/Release/en-us/PresentMonSharedService.msm")
    set(msm_stamp "${CMAKE_BINARY_DIR}/packaging/Release/PresentMonMergeModule.stamp")
    set(
        msm_intermediate_dir
        "${CMAKE_BINARY_DIR}/packaging/Release/intermediate/ServiceMergeModule"
    )
    set(
        msm_log_dir
        "${CMAKE_BINARY_DIR}/packaging/Release/logs"
    )
    set(msm_binlog "${msm_log_dir}/ServiceMergeModule.binlog")
    set(
        native_snapshot
        "${CMAKE_BINARY_DIR}/packaging/Release/PresentMonMergeModule.native_snapshot.txt"
    )
    set(
        verify_native_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(
        production_pe_verify_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(
        wixlib_output
        "${PMON_OUTPUT_ROOT}/Release/PMInstallerLib.wixlib"
    )

    pmon_packaging_merge_module_msbuild_properties(msm_msbuild_props)
    pmon_packaging_native_dir_with_trailing_sep("${wix_root}" wix_root_native)
    pmon_packaging_preflight_command(preflight_command Release)
    pmon_packaging_merge_module_native_paths(msm_native_paths)
    pmon_packaging_merge_module_native_pe_paths(msm_native_pe_paths)
    pmon_packaging_uci_runtime_paths(uci_stage_dir uci_manifest_path)
    set(msm_production_pe_verify_command "")
    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        set(
            msm_production_pe_verify_command
            COMMAND
                "${PMON_PYTHON_EXECUTABLE}"
                "${production_pe_verify_script}"
                verify-production-inputs
                --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
                --native-paths-list "${msm_native_pe_paths}"
                --uci-enabled "${PMON_UCI_ENABLED}"
                --uci-stage-dir "${uci_stage_dir}"
                --uci-manifest-path "${uci_manifest_path}"
        )
    endif()

    set(wixlib_stamp "${CMAKE_BINARY_DIR}/packaging/Release/PMInstallerLib.stamp")

    set(msm_production_relink_commands "")
    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        set(
            msm_production_relink_commands
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${msm_output}"
            COMMAND "${CMAKE_COMMAND}" -E remove_directory "${msm_intermediate_dir}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${msm_intermediate_dir}"
        )
    endif()

    set(
        msm_command_depends
        "${msm_wixproj}"
        "${msm_project_dir}/ServiceMergeModule.wxs"
        "${CMAKE_SOURCE_DIR}/IntelPresentMon/PmonCMakePackaging.wixproj.props"
        "${wixlib_output}"
        "${wixlib_stamp}"
        "${verify_native_script}"
        "${production_pe_verify_script}"
    )

    add_custom_command(
        OUTPUT "${msm_stamp}"
        BYPRODUCTS "${msm_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/packaging/Release"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${msm_intermediate_dir}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${msm_log_dir}"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${PMON_OUTPUT_ROOT}/Release/en-us"
        COMMAND ${preflight_command}
        ${msm_production_pe_verify_command}
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${verify_native_script}"
            native-snapshot record
            --snapshot-path "${native_snapshot}"
            --native-paths-list "${msm_native_paths}"
        ${msm_production_relink_commands}
        COMMAND
            "${CMAKE_VS_MSBUILD_COMMAND}"
            "${msm_wixproj}"
            /t:Build
            ${msm_msbuild_props}
            "/p:WIX=${wix_root_native}"
            /p:UseSharedCompilation=false
            /nodeReuse:false
            "/bl:${msm_binlog}"
            /m:1
            /nologo
            /v:minimal
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPMON_VERIFY_OUTPUT=${msm_output}"
            -P "${PMON_PACKAGING_CMAKE_DIR}/PresentMonPackagingVerifyOutput.cmake"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${verify_native_script}"
            native-snapshot verify
            --snapshot-path "${native_snapshot}"
            --native-paths-list "${msm_native_paths}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${msm_stamp}"
        DEPENDS ${msm_command_depends}
        COMMENT "Linking PresentMonSharedService.msm (WiX merge module, Release)"
        VERBATIM
    )

    add_custom_target(
        PresentMonMergeModule
        DEPENDS "${msm_stamp}"
    )
    set_target_properties(
        PresentMonMergeModule
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
endfunction()

function(pmon_add_verify_production_package_inputs_target)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        return()
    endif()

    if(TARGET pmon_verify_production_package_inputs)
        return()
    endif()

    set(
        production_pe_verify_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(
        verify_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPackageInputsVerified.stamp"
    )
    pmon_packaging_installer_native_pe_paths(installer_native_pe_paths)
    pmon_packaging_uci_runtime_paths(uci_stage_dir uci_manifest_path)
    set(
        payload_sign_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPayloadSigned.stamp"
    )

    add_custom_command(
        OUTPUT "${verify_stamp}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/packaging/Release"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${production_pe_verify_script}"
            verify-production-inputs
            --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
            --native-paths-list "${installer_native_pe_paths}"
            --uci-enabled "${PMON_UCI_ENABLED}"
            --uci-stage-dir "${uci_stage_dir}"
            --uci-manifest-path "${uci_manifest_path}"
            --skip-uci
        COMMAND "${CMAKE_COMMAND}" -E touch "${verify_stamp}"
        DEPENDS
            "${production_pe_verify_script}"
            "${CMAKE_SOURCE_DIR}/Scripts/pmon/packaging.py"
            "${payload_sign_stamp}"
        COMMENT "Verify production Authenticode on EDSS native PE inputs before UCI staging (Release)"
        VERBATIM
    )

    add_custom_target(
        pmon_verify_production_package_inputs
        DEPENDS "${verify_stamp}"
    )
    set_target_properties(
        pmon_verify_production_package_inputs
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
endfunction()

function(pmon_add_sign_production_installer_target)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        return()
    endif()

    if(TARGET pmon_sign_production_installer)
        return()
    endif()

    set(
        sign_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(
        verify_msi_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(msi_output "${PMON_OUTPUT_ROOT}/Release/en-us/PresentMon.msi")
    set(
        sign_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/PresentMonInstallerSigned.stamp"
    )
    set(
        native_snapshot
        "${CMAKE_BINARY_DIR}/packaging/Release/PresentMonInstaller.native_snapshot.txt"
    )
    set(
        installer_packaging_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/PresentMonInstaller.stamp"
    )
    pmon_packaging_installer_native_pe_paths(installer_native_pe_paths)

    set(
        payload_sign_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPayloadSigned.stamp"
    )

    set(sign_skip_edss_args "")
    if(PMON_PACKAGING_MSI_EDSS_SIGN_SKIP)
        set(sign_skip_edss_args --skip-edss)
    endif()

    set(sign_mode_args "")
    if(PMON_EDSS_IN_PROCESS_SIGNING)
        set(sign_mode_args --in-process-signing)
    else()
        set(sign_mode_args --impersonated-signing)
    endif()

    add_custom_command(
        OUTPUT "${sign_stamp}"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${sign_script}"
            sign-installer
            --msi-path "${msi_output}"
            --payload-root "${PMON_OUTPUT_ROOT}/Release"
            --repo-root "${CMAKE_SOURCE_DIR}"
            --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
            --edss-script "${PMON_EDSS_SIGN_SCRIPT}"
            --native-paths-list "${installer_native_pe_paths}"
            --snapshot-path "${native_snapshot}"
            ${sign_mode_args}
            ${sign_skip_edss_args}
        COMMAND "${CMAKE_COMMAND}" -E touch "${sign_stamp}"
        DEPENDS
            "${sign_script}"
            "${verify_msi_script}"
            "${CMAKE_SOURCE_DIR}/Scripts/pmon/packaging.py"
            "${installer_packaging_stamp}"
            "${native_snapshot}"
            "${payload_sign_stamp}"
            "${msi_output}"
        COMMENT "Production MSI signing gate and post-sign verification (Release)"
        VERBATIM
    )

    add_custom_target(
        pmon_sign_production_installer
        DEPENDS
            PresentMonInstaller
            "${sign_stamp}"
    )
    set_target_properties(
        pmon_sign_production_installer
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
endfunction()

function(pmon_add_require_signed_production_payload_target)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        return()
    endif()

    if(TARGET pmon_require_signed_production_payload)
        return()
    endif()

    set(
        preflight_script
        "${PMON_PACKAGING_PYTHON_SCRIPT}"
    )
    set(
        payload_sign_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPayloadSigned.stamp"
    )
    pmon_packaging_installer_native_pe_paths(installer_native_pe_paths)
    pmon_packaging_uci_runtime_paths(uci_stage_dir uci_manifest_path)

    add_custom_target(
        pmon_require_signed_production_payload
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${preflight_script}"
            require-signed-payload
            --stamp "${payload_sign_stamp}"
            --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
            --native-paths-list "${installer_native_pe_paths}"
            --uci-enabled "${PMON_UCI_ENABLED}"
            --uci-stage-dir "${uci_stage_dir}"
            --uci-manifest-path "${uci_manifest_path}"
        DEPENDS
            "${preflight_script}"
            "${CMAKE_SOURCE_DIR}/Scripts/pmon/packaging.py"
            "${payload_sign_stamp}"
        COMMENT "Require signed+verified production payload before packaging (Release)"
        VERBATIM
    )
    set_target_properties(
        pmon_require_signed_production_payload
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )

    if(TARGET pmon_sign_production_payload)
        add_dependencies(
            pmon_require_signed_production_payload
            pmon_sign_production_payload
        )
    endif()

    if(NOT TARGET pmon_production_payload_signed)
        add_custom_target(
            pmon_production_payload_signed
            DEPENDS
                pmon_require_signed_production_payload
            COMMENT
                "Production gate: EDSS payload signed (stamp) and verified before packaging"
        )
        set_target_properties(
            pmon_production_payload_signed
            PROPERTIES
                EXCLUDE_FROM_DEFAULT_BUILD ON
                FOLDER "Packaging"
        )
    endif()
endfunction()

function(pmon_add_production_payload_signed_packaging_dependencies)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        return()
    endif()
    if(NOT TARGET pmon_production_payload_signed)
        return()
    endif()

    foreach(
        packaging_target
        IN
        ITEMS
            PMInstallerExtension
            pmon_generate_uci_wix
            pmon_verify_production_package_inputs
    )
        if(TARGET ${packaging_target})
            add_dependencies(
                ${packaging_target}
                pmon_production_payload_signed
            )
        endif()
    endforeach()
endfunction()

function(pmon_register_packaging_targets)
    if(NOT PMON_BUILD_SCOPE STREQUAL "FULL")
        message(STATUS "PresentMon packaging targets are disabled by PMON_BUILD_SCOPE=${PMON_BUILD_SCOPE}.")
        return()
    endif()

    if(NOT PMON_ARCHITECTURE STREQUAL "x64")
        message(
            STATUS
            "PresentMon packaging targets are unavailable: packaging requires an x64 configure."
        )
        return()
    endif()

    pmon_packaging_components_enabled(components_enabled)
    if(NOT components_enabled)
        set(missing_options "")
        foreach(option_name IN ITEMS
            PMON_BUILD_CONSOLE
            PMON_BUILD_SERVICE
            PMON_BUILD_SDK
            PMON_BUILD_UI
            PMON_BUILD_PROVIDER
        )
            if(NOT ${option_name})
                if(missing_options)
                    set(missing_options "${missing_options}, ${option_name}=OFF")
                else()
                    set(missing_options "${option_name}=OFF")
                endif()
            endif()
        endforeach()
        message(
            STATUS
            "PresentMon packaging targets are unavailable: enable ${missing_options}."
        )
        return()
    endif()

    pmon_require_packaging_native_targets()

    pmon_resolve_wix_root(wix_root wix_reason)
    pmon_print_packaging_summary("${wix_root}" "${wix_reason}")

    if(NOT wix_root)
        set(
            PMON_PACKAGING_AVAILABLE
            FALSE
            CACHE INTERNAL
            "PresentMon packaging targets registered"
            FORCE
        )
        if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
            message(FATAL_ERROR "Production configuration requires WiX Toolset 3. ${wix_reason}")
        endif()
        if(PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER")
            add_custom_target(
                PresentMonInstaller
                COMMAND
                    "${CMAKE_COMMAND}" -E echo
                    "Developer Release requires WiX Toolset 3 to build PresentMon.msi. ${wix_reason}"
                COMMAND "${CMAKE_COMMAND}" -E false
                COMMENT "PresentMon.msi is unavailable without WiX Toolset 3"
                VERBATIM
            )
            set_target_properties(
                PresentMonInstaller
                PROPERTIES
                    EXCLUDE_FROM_DEFAULT_BUILD ON
                    FOLDER "Packaging"
            )
            pmon_add_developer_release_installer_to_default_build()
        endif()
        return()
    endif()

    set(PMON_PACKAGING_AVAILABLE TRUE CACHE INTERNAL "PresentMon packaging targets registered" FORCE)
    set(
        PMON_PACKAGING_WIX_ROOT
        "${wix_root}"
        CACHE INTERNAL
        "Resolved WiX Toolset 3 root used by packaging MSBuild invocations"
    )

    pmon_add_pm_installer_extension_target()
    pmon_add_generate_uci_wix_target()
    pmon_add_pm_installer_lib_target()
    pmon_add_present_mon_installer_target()
    pmon_add_present_mon_merge_module_target()
    pmon_add_verify_production_package_inputs_target()
    pmon_add_require_signed_production_payload_target()
    pmon_add_production_payload_signed_packaging_dependencies()
    pmon_add_sign_production_installer_target()
    set(pmon_packaging_payload_targets
        PresentMonUI
        KernelProcess
        PresentMonService
        PresentMonAPI2
        PresentMonAPI2Loader
        pmon_provider
        PresentMonConsole
    )
    set(pmon_packaging_staging_targets
        pmon_stage_cef_runtime
        pmon_verify_cef
        pmon_stage_ui_payload
        pmon_stage_kernel_payload
        pmon_compile_shaders
    )
    if(TARGET pmon_developer_release_installer)
        add_dependencies(
            pmon_developer_release_installer
            ${pmon_packaging_payload_targets}
            ${pmon_packaging_staging_targets}
        )
    endif()
    add_dependencies(PresentMonInstaller PMInstallerExtension)
    add_dependencies(PresentMonInstaller PMInstallerLib)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        add_dependencies(PresentMonInstaller ${pmon_packaging_staging_targets})
    endif()
    if(TARGET pmon_verify_production_package_inputs)
        add_dependencies(PresentMonInstaller pmon_verify_production_package_inputs)
        add_dependencies(PMInstallerLib pmon_verify_production_package_inputs)
    endif()
    add_dependencies(PMInstallerLib PMInstallerExtension)
    add_dependencies(PMInstallerLib pmon_generate_uci_wix)
    add_dependencies(PresentMonMergeModule PMInstallerLib)
    add_dependencies(PresentMonMergeModule pmon_generate_uci_wix)

    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION" AND NOT PMON_PACKAGING_MSI_EDSS_SIGN_SKIP)
        message(
            STATUS
            "pmon_sign_production_installer: invokes PMON_EDSS_SIGN_SCRIPT with -SignMsiOnly, "
            "-OutputRoot, -RepoRoot, -Verify, and -InProcessSigning or -ImpersonatedSigning (MSI only; MSM is "
            "unsigned per legacy policy; natives are signed by pmon_sign_production_payload). "
            "Use -DPMON_PACKAGING_MSI_EDSS_SIGN_SKIP=ON "
            "to verify a manually signed MSI without invoking EDSS."
        )
    endif()

    if(NOT TARGET pmon_build_packages)
        add_custom_target(
            pmon_build_packages
            DEPENDS
                PresentMonInstaller
                PresentMonMergeModule
            COMMENT "Build unsigned PresentMon MSI and MSM"
        )
        set_target_properties(
            pmon_build_packages
            PROPERTIES
                EXCLUDE_FROM_DEFAULT_BUILD ON
                FOLDER "Packaging"
        )
    endif()

    if(TARGET pmon_production_payload_signed)
        add_dependencies(pmon_build_packages pmon_production_payload_signed)
    elseif(TARGET pmon_require_signed_production_payload)
        add_dependencies(pmon_build_packages pmon_require_signed_production_payload)
    endif()

    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION" AND NOT TARGET pmon_production_packaging_full)
        # Serial gate: payload EDSS (step 1) before unsigned MSI/MSM link (step 2). Not used by
        # pmon_build_packages alone (stamp + preflight only via pmon_require_signed_production_payload).
        add_custom_target(
            pmon_production_packaging_after_payload_sign
            DEPENDS
                pmon_build_packages
            COMMENT "Production packaging: build packages after payload sign (internal ordering)"
        )
        set_target_properties(
            pmon_production_packaging_after_payload_sign
            PROPERTIES
                EXCLUDE_FROM_DEFAULT_BUILD ON
                FOLDER "Packaging"
        )
        if(TARGET pmon_sign_production_payload)
            add_dependencies(
                pmon_production_packaging_after_payload_sign
                pmon_sign_production_payload
            )
        endif()
        add_dependencies(pmon_sign_production_installer pmon_production_packaging_after_payload_sign)

        add_custom_target(
            pmon_production_packaging_full
            COMMENT
                "Production packaging: EDSS natives, build MSI/MSM, SignMsiOnly"
        )
        if(TARGET pmon_sign_production_payload)
            add_dependencies(
                pmon_production_packaging_full
                pmon_sign_production_payload
            )
        endif()
        if(TARGET pmon_sign_production_installer)
            add_dependencies(
                pmon_production_packaging_full
                pmon_sign_production_installer
            )
        endif()
        set_target_properties(
            pmon_production_packaging_full
            PROPERTIES
                EXCLUDE_FROM_DEFAULT_BUILD ON
                FOLDER "Packaging"
        )

    endif()
endfunction()
