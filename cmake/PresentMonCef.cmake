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

    file(READ "${cef_lock}" cef_lock_json)
    string(JSON payload_count ERROR_VARIABLE json_error LENGTH "${cef_lock_json}" payload)
    if(json_error)
        message(FATAL_ERROR "Failed to read the CEF lock payload: ${json_error}")
    endif()
    if(payload_count EQUAL 0)
        message(FATAL_ERROR "The CEF lock payload is empty: ${cef_lock}")
    endif()

    set(runtime_destination "${PMON_OUTPUT_ROOT}/$<CONFIG>")
    set(runtime_outputs)
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
        set(runtime_output "${runtime_destination}/${output_path}")
        set(stage_input "${cef_root}/${stage_path}")
        cmake_path(GET runtime_output PARENT_PATH runtime_output_directory)
        list(APPEND runtime_outputs "${runtime_output}")

        add_custom_command(
            OUTPUT "${runtime_output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${runtime_output_directory}"
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "${stage_input}"
                "${runtime_output}"
            COMMAND "${CMAKE_COMMAND}" -E touch_nocreate "${runtime_output}"
            DEPENDS "${stage_input}" "${cef_lock}"
            COMMENT "Staging CEF runtime file ${output_path}"
            VERBATIM
        )
    endforeach()

    add_custom_target(
        pmon_stage_cef_runtime
        DEPENDS ${runtime_outputs}
    )
    set_target_properties(
        pmon_stage_cef_runtime
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    # Explicit CI verification validates both the restored stage and the
    # runtime files copied to the product output directory.
    add_custom_target(
        pmon_verify_cef
        COMMAND
            ${powershell_command}
            -File "${batch_dir}/validate-cef.ps1"
            -Mode Stage
            -StageKind CMake
        COMMAND
            ${powershell_command}
            -File "${batch_dir}/validate-cef.ps1"
            -Mode Output
            -OutputRoot "${runtime_destination}"
        DEPENDS pmon_stage_cef_runtime
        COMMENT "Verifying the CEF stage and staged runtime payload"
        USES_TERMINAL
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

# Links a target against the CEF headers and libraries and stages the runtime
# payload next to it. Restore and validation remain explicit operations; normal
# product builds only perform incremental staging.
function(pmon_target_uses_cef target)
    if(NOT TARGET pmon_stage_cef_runtime)
        message(FATAL_ERROR "pmon_configure_cef() must run before pmon_target_uses_cef().")
    endif()

    target_link_libraries(${target} PRIVATE pmon::cef)
    add_dependencies(${target} pmon_stage_cef_runtime)

    set_target_properties(
        pmon_stage_cef_runtime
        PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD FALSE
    )
endfunction()
