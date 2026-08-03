include_guard(GLOBAL)

function(_pmon_normalize_cef_relative_path input_path output_variable)
    set(path "${input_path}")
    string(REPLACE "\\" "/" path "${path}")

    cmake_path(IS_ABSOLUTE path is_absolute)
    if(
        NOT path
        OR is_absolute
        OR path MATCHES "(^|/)\\.\\.(/|$)"
        OR path MATCHES "^[A-Za-z]:"
        OR path MATCHES ";"
    )
        message(FATAL_ERROR "Unsafe relative path in the CEF lock: ${input_path}")
    endif()

    cmake_path(NORMAL_PATH path OUTPUT_VARIABLE normalized_path)
    if(normalized_path STREQUAL ".")
        message(FATAL_ERROR "Unsafe relative path in the CEF lock: ${input_path}")
    endif()

    set(${output_variable} "${normalized_path}" PARENT_SCOPE)
endfunction()

# CEF is an explicit restore dependency. Configure never downloads it, and the
# CMake stage is kept separate from the MSBuild stage in
# IntelPresentMon/AppCef/Cef so both build systems stay independent.
function(pmon_configure_cef)
    # Remove the obsolete arbitrary-path cache setting from existing build
    # trees. The two supported stages are selected by name in the CEF scripts.
    unset(PMON_CEF_ROOT CACHE)

    set(
        PMON_CEF_SOURCE
        ""
        CACHE STRING
        "Optional CEF archive or URI for pmon_restore_cef; the locked URI is used when empty"
    )

    set(app_cef_dir "${PROJECT_SOURCE_DIR}/IntelPresentMon/AppCef")
    set(batch_dir "${app_cef_dir}/Batch")
    set(cef_lock "${app_cef_dir}/cef-lock.json")
    set(cef_root "${PROJECT_SOURCE_DIR}/build/ThirdParty/cef")
    cmake_path(NORMAL_PATH cef_root OUTPUT_VARIABLE cef_root)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${cef_lock}")

    set(
        powershell_command
        powershell.exe
        -NoLogo
        -NoProfile
        -NonInteractive
        -ExecutionPolicy Bypass
    )

    set(restore_source_argument)
    if(PMON_CEF_SOURCE)
        set(restore_source_argument -SourcePath "${PMON_CEF_SOURCE}")
    endif()

    # Explicit, opt-in restore. The script builds the stage in a temporary
    # sibling directory, validates it, and publishes a complete stage.
    add_custom_target(
        pmon_restore_cef
        COMMAND
            ${powershell_command}
            -File "${batch_dir}/pull-cef.ps1"
            -StageKind CMake
            -Generator "${CMAKE_GENERATOR}"
            -Platform "${CMAKE_GENERATOR_PLATFORM}"
            -Toolset "${CMAKE_VS_PLATFORM_TOOLSET}"
            ${restore_source_argument}
        COMMENT "Restoring the locked CEF distribution into ${cef_root}"
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        pmon_restore_cef
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    set(
        restore_command
        "cmake --build \"${CMAKE_BINARY_DIR}\" --config $<CONFIG> --target pmon_restore_cef"
    )

    # Never downloads. Consumers of the CEF targets depend on this so a missing
    # or damaged stage fails the build with the restore command to run.
    add_custom_target(
        pmon_validate_cef
        COMMAND
            "${CMAKE_COMMAND}"
            -D "PMON_CEF_VALIDATE_SCRIPT=${batch_dir}/validate-cef.ps1"
            -D "PMON_CEF_RESTORE_COMMAND=${restore_command}"
            -P "${PROJECT_SOURCE_DIR}/cmake/ValidateCef.cmake"
        COMMENT "Validating the CEF stage at ${cef_root}"
        VERBATIM
    )
    set_target_properties(
        pmon_validate_cef
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    file(READ "${cef_lock}" cef_lock_json)
    string(JSON payload_count ERROR_VARIABLE json_error LENGTH "${cef_lock_json}" payload)
    if(json_error)
        message(FATAL_ERROR "Failed to read the CEF lock payload: ${json_error}")
    endif()
    if(payload_count EQUAL 0)
        message(FATAL_ERROR "The CEF lock payload is empty: ${cef_lock}")
    endif()

    set(runtime_destination "${PMON_OUTPUT_ROOT}/$<CONFIG>")
    set(
        runtime_manifest
        "${PMON_OUTPUT_ROOT}/obj/cef-runtime/$<CONFIG>/owned-files.txt"
    )
    set(runtime_outputs)
    set(stage_inputs)
    set(runtime_output_keys)
    math(EXPR last_payload_index "${payload_count} - 1")
    foreach(payload_index RANGE ${last_payload_index})
        string(JSON output_path GET "${cef_lock_json}" payload ${payload_index} path)
        string(JSON stage_path GET "${cef_lock_json}" payload ${payload_index} stagePath)
        _pmon_normalize_cef_relative_path("${output_path}" output_path)
        _pmon_normalize_cef_relative_path("${stage_path}" stage_path)

        string(TOLOWER "${output_path}" output_key)
        list(FIND runtime_output_keys "${output_key}" duplicate_output_index)
        if(NOT duplicate_output_index EQUAL -1)
            message(FATAL_ERROR "Duplicate output path in the CEF lock: ${output_path}")
        endif()

        list(APPEND runtime_output_keys "${output_key}")
        list(APPEND runtime_outputs "${runtime_destination}/${output_path}")
        list(APPEND stage_inputs "${cef_root}/${stage_path}")
    endforeach()

    # The manifest is an output of the same rule as the payload. When the lock
    # changes, StageCef uses its previous contents to remove only files that
    # this command owned and that are no longer in the lock.
    add_custom_command(
        OUTPUT
            ${runtime_outputs}
            "${runtime_manifest}"
        COMMAND
            "${CMAKE_COMMAND}"
            -D "PMON_CEF_STAGE=${cef_root}"
            -D "PMON_CEF_LOCK=${cef_lock}"
            -D "PMON_CEF_DESTINATION=${runtime_destination}"
            -D "PMON_CEF_OWNERSHIP_MANIFEST=${runtime_manifest}"
            -D "PMON_CEF_VALIDATE_SCRIPT=${batch_dir}/validate-cef.ps1"
            -P "${PROJECT_SOURCE_DIR}/cmake/StageCef.cmake"
        DEPENDS
            ${stage_inputs}
            "${cef_lock}"
            "${PROJECT_SOURCE_DIR}/cmake/StageCef.cmake"
            "${batch_dir}/cef-lock.psm1"
            "${batch_dir}/validate-cef.ps1"
        COMMENT "Staging the locked CEF runtime payload into ${runtime_destination}"
        VERBATIM
    )

    add_custom_target(
        pmon_stage_cef_runtime
        DEPENDS
            ${runtime_outputs}
            "${runtime_manifest}"
    )
    add_dependencies(pmon_stage_cef_runtime pmon_validate_cef)
    set_target_properties(
        pmon_stage_cef_runtime
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    # Explicit verification remains offline. In addition to validating the
    # stage and copied payload, it proves that manifest-owned stale files are
    # removed without touching unrelated output files.
    add_custom_target(
        pmon_verify_cef
        COMMAND
            "${CMAKE_COMMAND}"
            -D "PMON_CEF_STAGE=${cef_root}"
            -D "PMON_CEF_LOCK=${cef_lock}"
            -D "PMON_CEF_DESTINATION=${runtime_destination}"
            -D "PMON_CEF_OWNERSHIP_MANIFEST=${runtime_manifest}"
            -D "PMON_CEF_VALIDATE_SCRIPT=${batch_dir}/validate-cef.ps1"
            -D "PMON_CEF_STAGE_SCRIPT=${PROJECT_SOURCE_DIR}/cmake/StageCef.cmake"
            -P "${PROJECT_SOURCE_DIR}/cmake/VerifyCef.cmake"
        DEPENDS pmon_stage_cef_runtime
        COMMENT "Verifying fixed-stage CEF validation and runtime ownership"
        VERBATIM
    )
    set_target_properties(
        pmon_verify_cef
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    # Consumers use these targets instead of paths under AppCef/Cef.
    add_library(pmon_cef_headers INTERFACE)
    add_library(pmon::cef_headers ALIAS pmon_cef_headers)
    target_include_directories(pmon_cef_headers INTERFACE "${cef_root}/Include")
    set_property(TARGET pmon_cef_headers PROPERTY FOLDER "ThirdParty")

    add_library(pmon_cef_libcef SHARED IMPORTED GLOBAL)
    add_library(pmon::cef_libcef ALIAS pmon_cef_libcef)
    set_target_properties(
        pmon_cef_libcef
        PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION "${cef_root}/Bin/libcef.dll"
            IMPORTED_LOCATION_DEBUG "${cef_root}/Bin/libcef.dll"
            IMPORTED_LOCATION_RELEASE "${cef_root}/Bin/libcef.dll"
            IMPORTED_IMPLIB_DEBUG "${cef_root}/Lib/Debug/libcef.lib"
            IMPORTED_IMPLIB_RELEASE "${cef_root}/Lib/Release/libcef.lib"
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO "Release"
            MAP_IMPORTED_CONFIG_MINSIZEREL "Release"
    )

    add_library(pmon_cef_wrapper STATIC IMPORTED GLOBAL)
    add_library(pmon::cef_wrapper ALIAS pmon_cef_wrapper)
    set_target_properties(
        pmon_cef_wrapper
        PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION_DEBUG "${cef_root}/Lib/Debug/libcef_dll_wrapper.lib"
            IMPORTED_LOCATION_RELEASE "${cef_root}/Lib/Release/libcef_dll_wrapper.lib"
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO "Release"
            MAP_IMPORTED_CONFIG_MINSIZEREL "Release"
    )

    add_library(pmon_cef INTERFACE)
    add_library(pmon::cef ALIAS pmon_cef)
    target_link_libraries(
        pmon_cef
        INTERFACE
            pmon_cef_headers
            pmon_cef_wrapper
            pmon_cef_libcef
    )
    set_property(TARGET pmon_cef PROPERTY FOLDER "ThirdParty")

    set(PMON_CEF_EFFECTIVE_ROOT "${cef_root}" CACHE INTERNAL "Resolved CEF stage" FORCE)
    set(PMON_CEF_EFFECTIVE_ROOT "${cef_root}" PARENT_SCOPE)
endfunction()

# Links a target against the locked CEF headers and libraries and makes its
# build fail early when the CEF stage is missing or does not match the lock.
function(pmon_target_uses_cef target)
    if(NOT TARGET pmon_validate_cef)
        message(FATAL_ERROR "pmon_configure_cef() must run before pmon_target_uses_cef().")
    endif()
    target_link_libraries(${target} PRIVATE pmon::cef)
    add_dependencies(${target} pmon_validate_cef)
endfunction()
