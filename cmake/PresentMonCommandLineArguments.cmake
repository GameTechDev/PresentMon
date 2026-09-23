include_guard(GLOBAL)

function(_pmon_command_line_args_is_build_binary_dir out_var)
    cmake_path(SET _pmon_expected_build_dir "${CMAKE_SOURCE_DIR}/build" NORMALIZE)
    cmake_path(SET _pmon_actual_binary_dir "${CMAKE_BINARY_DIR}" NORMALIZE)
    if(_pmon_actual_binary_dir STREQUAL _pmon_expected_build_dir)
        set("${out_var}" TRUE PARENT_SCOPE)
    else()
        set("${out_var}" FALSE PARENT_SCOPE)
    endif()
endfunction()

function(_pmon_command_line_args_apply)
    set(_pmon_mappings
        "IntelPresentMon/KernelProcess|KernelProcess|IntelPresentMon/KernelProcess/KernelProcess.args.json"
        "IntelPresentMon/PresentMonService|PresentMonService|IntelPresentMon/PresentMonService/PresentMonService.args.json"
        "PresentMon|PresentMonConsole|PresentMon/PresentMonConsole.args.json"
        "IntelPresentMon/SampleClient|SampleClient|IntelPresentMon/SampleClient/SampleClient.args.json"
        "Tools/ETLTrimmer|ETLTrimmer|Tools/ETLTrimmer/ETLTrimmer.args.json"
    )

    set(_pmon_linked 0)

    foreach(_pmon_mapping IN LISTS _pmon_mappings)
        string(REPLACE "|" ";" _pmon_fields "${_pmon_mapping}")
        list(GET _pmon_fields 0 _pmon_subdir)
        list(GET _pmon_fields 1 _pmon_target)
        list(GET _pmon_fields 2 _pmon_source_rel)

        set(_pmon_vcxproj "${CMAKE_BINARY_DIR}/${_pmon_subdir}/${_pmon_target}.vcxproj")
        if(NOT TARGET "${_pmon_target}" AND NOT EXISTS "${_pmon_vcxproj}")
            continue()
        endif()

        set(_pmon_source "${CMAKE_SOURCE_DIR}/${_pmon_source_rel}")
        if(NOT EXISTS "${_pmon_source}")
            message(
                WARNING
                "Command line args symlink: missing source preset ${_pmon_source_rel}."
            )
            continue()
        endif()

        set(_pmon_link "${CMAKE_BINARY_DIR}/${_pmon_subdir}/${_pmon_target}.args.json")
        if(EXISTS "${_pmon_link}")
            file(REMOVE "${_pmon_link}")
        endif()

        file(CREATE_LINK "${_pmon_source}" "${_pmon_link}" SYMBOLIC RESULT _pmon_link_result)
        if(_pmon_link_result)
            message(
                WARNING
                "Command line args symlink: could not link\n"
                "  ${_pmon_link}\n"
                "  -> ${_pmon_source}\n"
                "  (${_pmon_link_result}). Enable Windows Developer Mode or run from an elevated shell."
            )
            continue()
        endif()

        math(EXPR _pmon_linked "${_pmon_linked} + 1")
    endforeach()

    if(_pmon_linked GREATER 0)
        message(STATUS "Linked ${_pmon_linked} Command Line Arguments preset(s) under ${CMAKE_BINARY_DIR}.")
    endif()
endfunction()

function(pmon_register_command_line_args_symlinks)
    if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        return()
    endif()

    _pmon_command_line_args_is_build_binary_dir(_pmon_is_build_dir)
    if(NOT _pmon_is_build_dir)
        if(PMON_LINK_COMMAND_LINE_ARGS)
            message(
                STATUS
                "PMON_LINK_COMMAND_LINE_ARGS is ON but the binary directory is not "
                "${CMAKE_SOURCE_DIR}/build; skipping command line args symlinks."
            )
        endif()
        return()
    endif()

    add_custom_target(
        pmon_link_command_line_args
        COMMAND
            ${CMAKE_COMMAND}
            -S "${CMAKE_SOURCE_DIR}"
            -B "${CMAKE_BINARY_DIR}"
            -DPMON_LINK_COMMAND_LINE_ARGS=ON
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        pmon_link_command_line_args
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    if(PMON_LINK_COMMAND_LINE_ARGS)
        _pmon_command_line_args_apply()
    endif()
endfunction()
