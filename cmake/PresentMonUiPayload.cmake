include_guard(GLOBAL)

function(_pmon_normalize_ui_payload_relative_path input_path output_variable)
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
        message(FATAL_ERROR "Unsafe relative path in the UI payload lock: ${input_path}")
    endif()

    cmake_path(NORMAL_PATH path OUTPUT_VARIABLE normalized_path)
    if(normalized_path STREQUAL ".")
        message(FATAL_ERROR "Unsafe relative path in the UI payload lock: ${input_path}")
    endif()

    set(${output_variable} "${normalized_path}" PARENT_SCOPE)
endfunction()

function(_pmon_add_tracked_payload_copy source_file destination_relative output_variable)
    set(extra_depends ${ARGN})
    set(runtime_destination "${PMON_OUTPUT_ROOT}/$<CONFIG>")
    set(runtime_output "${runtime_destination}/${destination_relative}")
    cmake_path(GET runtime_output PARENT_PATH runtime_output_directory)

    add_custom_command(
        OUTPUT "${runtime_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${runtime_output_directory}"
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${source_file}"
            "${runtime_output}"
        COMMAND "${CMAKE_COMMAND}" -E touch_nocreate "${runtime_output}"
        DEPENDS "${source_file}" ${extra_depends}
        COMMENT "Staging payload file ${destination_relative}"
        VERBATIM
    )

    set(${output_variable} "${runtime_output}" PARENT_SCOPE)
endfunction()

function(pmon_configure_ui_payload)
    set(app_cef_dir "${PROJECT_SOURCE_DIR}/IntelPresentMon/AppCef")
    set(web_ui_dir "${app_cef_dir}/ipm-ui-vue")
    set(web_dist_dir "${web_ui_dir}/dist")
    set(web_dist_lock "${app_cef_dir}/web-dist-lock.json")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${web_dist_lock}")

    set(web_configure_depends
        "${web_ui_dir}/package.json"
        "${web_ui_dir}/package-lock.json"
        "${web_ui_dir}/vite.config.ts"
        "${web_ui_dir}/index.html"
        "${web_ui_dir}/tsconfig.json"
        "${web_ui_dir}/tsconfig.app.json"
        "${web_ui_dir}/tsconfig.node.json"
        "${web_dist_lock}"
    )
    file(GLOB_RECURSE web_ui_sources CONFIGURE_DEPENDS
        "${web_ui_dir}/src/*"
    )
    list(APPEND web_configure_depends ${web_ui_sources})

    set(web_build_stamp "${CMAKE_BINARY_DIR}/obj/ipm-ui-vue/$<CONFIG>/build.stamp")
    add_custom_command(
        OUTPUT "${web_build_stamp}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/obj/ipm-ui-vue/$<CONFIG>"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/Scripts/web.py"
            build
            --repo-root "${PROJECT_SOURCE_DIR}"
            --dependency-root "${PMON_DEPENDENCY_ROOT}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${web_build_stamp}"
        DEPENDS ${web_configure_depends}
        COMMENT "Building ipm-ui-vue web assets ($<CONFIG>)"
        VERBATIM
    )

    file(READ "${web_dist_lock}" web_dist_lock_json)
    string(JSON output_count ERROR_VARIABLE json_error LENGTH "${web_dist_lock_json}" outputs)
    if(json_error)
        message(FATAL_ERROR "Failed to read the web dist lock: ${json_error}")
    endif()
    if(output_count EQUAL 0)
        message(FATAL_ERROR "The web dist lock outputs list is empty: ${web_dist_lock}")
    endif()

    set(payload_outputs "${web_build_stamp}")
    set(web_output_keys)
    math(EXPR last_output_index "${output_count} - 1")
    foreach(output_index RANGE ${last_output_index})
        string(JSON output_path GET "${web_dist_lock_json}" outputs ${output_index})
        _pmon_normalize_ui_payload_relative_path("${output_path}" output_path)

        string(TOLOWER "${output_path}" output_key)
        list(FIND web_output_keys "${output_key}" duplicate_output_index)
        if(NOT duplicate_output_index EQUAL -1)
            message(FATAL_ERROR "Duplicate web dist output path in the lock: ${output_path}")
        endif()
        list(APPEND web_output_keys "${output_key}")

        set(dist_input "${web_dist_dir}/${output_path}")
        _pmon_add_tracked_payload_copy(
            "${dist_input}"
            "ipm-ui-vue/${output_path}"
            staged_output
            "${web_build_stamp}"
            "${web_dist_lock}"
        )
        list(APPEND payload_outputs "${staged_output}")
    endforeach()

    foreach(preset_index RANGE 0 3)
        _pmon_add_tracked_payload_copy(
            "${web_ui_dir}/presets/preset-${preset_index}.json"
            "Presets/preset-${preset_index}.json"
            preset_output
        )
        list(APPEND payload_outputs "${preset_output}")
    endforeach()

    _pmon_add_tracked_payload_copy(
        "${web_ui_dir}/BlockLists/TargetBlockList.txt"
        "BlockLists/TargetBlockList.txt"
        blocklist_output
    )
    list(APPEND payload_outputs "${blocklist_output}")

    add_custom_target(
        pmon_stage_ui_payload
        DEPENDS ${payload_outputs}
    )
    set_target_properties(
        pmon_stage_ui_payload
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )
endfunction()

function(pmon_configure_kernel_payload)
    set(kernel_dir "${PROJECT_SOURCE_DIR}/IntelPresentMon/KernelProcess")
    set(payload_outputs)

    _pmon_add_tracked_payload_copy(
        "${kernel_dir}/presentmon-cli.cmd"
        "presentmon-cli.cmd"
        cmd_output
    )
    list(APPEND payload_outputs "${cmd_output}")

    _pmon_add_tracked_payload_copy(
        "${kernel_dir}/presentmon-cli.bat"
        "pathed/presentmon-cli.bat"
        bat_output
    )
    list(APPEND payload_outputs "${bat_output}")

    add_custom_target(
        pmon_stage_kernel_payload
        DEPENDS ${payload_outputs}
    )
    set_target_properties(
        pmon_stage_kernel_payload
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )
endfunction()

function(pmon_target_uses_ui_payload target)
    if(NOT TARGET pmon_stage_ui_payload)
        message(FATAL_ERROR "pmon_configure_ui_payload() must run before pmon_target_uses_ui_payload().")
    endif()

    add_dependencies(${target} pmon_stage_ui_payload)
    set_target_properties(
        pmon_stage_ui_payload
        PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD FALSE
    )
endfunction()

function(pmon_target_uses_kernel_payload target)
    if(NOT TARGET pmon_stage_kernel_payload)
        message(FATAL_ERROR "pmon_configure_kernel_payload() must run before pmon_target_uses_kernel_payload().")
    endif()

    add_dependencies(${target} pmon_stage_kernel_payload)
    set_target_properties(
        pmon_stage_kernel_payload
        PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD FALSE
    )
endfunction()
