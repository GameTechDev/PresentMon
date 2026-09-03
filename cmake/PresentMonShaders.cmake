include_guard(GLOBAL)

# Resolves fxc.exe, the Windows SDK HLSL shader compiler used by the legacy
# Shaders.vcxitems FxCompile items. No BuildCustomizations import selects it
# in MSBuild, so it resolves through the ambient SDK tool directory rather
# than a pinned version. CMake must resolve the same way: against the SDK it
# selected for the rest of the build (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION),
# not the newest SDK installed or whatever fxc.exe happens to be first on
# PATH. Either of those could silently pick a different SDK version than the
# rest of the product on a machine with multiple SDKs installed.
function(_pmon_find_fxc_compiler output_variable)
    if(PMON_FXC_EXECUTABLE)
        # Resolved to an absolute path before EXISTS/IS_DIRECTORY: both are
        # documented as well-defined only for full paths, and a relative
        # override otherwise validates against CMAKE_CURRENT_SOURCE_DIR here
        # while the custom build command runs with the generated project
        # file's directory as its working directory, not the source root. An
        # unresolved relative path would configure successfully and then fail
        # every build with "the system cannot find the path specified."
        cmake_path(
            ABSOLUTE_PATH PMON_FXC_EXECUTABLE
            BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
            NORMALIZE
            OUTPUT_VARIABLE resolved_fxc_executable
        )
        if(NOT EXISTS "${resolved_fxc_executable}")
            message(
                FATAL_ERROR
                "PMON_FXC_EXECUTABLE is set to '${PMON_FXC_EXECUTABLE}', which "
                "resolves to '${resolved_fxc_executable}', and that does not "
                "exist."
            )
        endif()
        if(IS_DIRECTORY "${resolved_fxc_executable}")
            message(
                FATAL_ERROR
                "PMON_FXC_EXECUTABLE is set to '${PMON_FXC_EXECUTABLE}', which "
                "resolves to '${resolved_fxc_executable}', and that is a "
                "directory. Set it to the fxc.exe file itself."
            )
        endif()
        set(${output_variable} "${resolved_fxc_executable}" PARENT_SCOPE)
        return()
    endif()

    if(NOT CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
        message(
            FATAL_ERROR
            "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION is not set; cannot "
            "resolve the matching fxc.exe. Set PMON_FXC_EXECUTABLE to an "
            "explicit path to override."
        )
    endif()

    set(
        sdk_fxc
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/fxc.exe"
    )
    if(NOT EXISTS "${sdk_fxc}")
        message(
            FATAL_ERROR
            "The HLSL shader compiler fxc.exe was not found for the Windows "
            "SDK CMake selected for this build "
            "(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}) at ${sdk_fxc}. "
            "Install that SDK's tools, or set PMON_FXC_EXECUTABLE to an "
            "explicit fxc.exe path to override."
        )
    endif()

    # Not cached into PMON_FXC_EXECUTABLE: that variable is reserved for an
    # explicit override, checked above. Caching the auto-discovered path
    # there would make it indistinguishable from a real override on the next
    # configure, so a later SDK change would keep resolving to the stale
    # compiler instead of following CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION.
    set(${output_variable} "${sdk_fxc}" PARENT_SCOPE)
endfunction()

# Compiles one HLSL source into the shipping .cso payload at
# <PMON_OUTPUT_ROOT>/<Configuration>/Shaders/<name>.cso, matching the legacy
# ObjectFileOutput path $(OutDir)Shaders\%(Filename).cso. The legacy
# FxCompile items apply the same ShaderModel/ShaderType across Debug and
# Release, so one add_custom_command() rule, parameterized by $<CONFIG>,
# covers every configuration; each built configuration still invokes fxc.exe
# separately, since OUTPUT and the compiled flags both vary by $<CONFIG>.
function(_pmon_add_hlsl_shader)
    set(one_value_args SOURCE PROFILE ENTRY OUTPUT_VARIABLE)
    cmake_parse_arguments(ARG "" "${one_value_args}" "" ${ARGN})

    _pmon_find_fxc_compiler(fxc_executable)

    get_filename_component(shader_name "${ARG_SOURCE}" NAME_WE)
    set(output_directory "${PMON_OUTPUT_ROOT}/$<CONFIG>/Shaders")
    set(output_file "${output_directory}/${shader_name}.cso")

    # /Zi (Debug only, see below) embeds the compiled source path into the
    # .cso debug chunk. Passing it in native backslash form byte-for-byte
    # matches the legacy MSBuild-invoked fxc.exe, which always receives
    # backslash paths; CMake's own forward-slash form only changes those
    # embedded path bytes; it does not change the compiled bytecode.
    file(TO_NATIVE_PATH "${ARG_SOURCE}" native_source)

    # Shaders.vcxitems declares no EnableDebuggingInformation or
    # DisableOptimizations metadata, but the imported Microsoft.Cpp.Common.props
    # defaults both to true for FxCompile items whenever $(UseDebugLibraries) is
    # true (CefNano.vcxproj sets it true for Debug|x64, false for Release|x64),
    # which is /Od /Zi; Release gets neither flag. This is not visible in any
    # PresentMon project file and was confirmed by capturing the real fxc.exe
    # command line from a direct MSBuild invocation of CefNano.vcxproj in both
    # configurations. CMake has no UseDebugLibraries equivalent to key off of,
    # so this reproduces the resulting behavior directly off $<CONFIG:Debug>.
    add_custom_command(
        OUTPUT "${output_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
        COMMAND
            "${fxc_executable}"
            /nologo
            /T "${ARG_PROFILE}"
            /E "${ARG_ENTRY}"
            /Fo "${output_file}"
            "$<$<CONFIG:Debug>:/Od;/Zi>"
            "${native_source}"
        DEPENDS "${ARG_SOURCE}" "${fxc_executable}"
        COMMENT "Compiling HLSL shader ${shader_name} (${ARG_PROFILE})"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )

    set(${ARG_OUTPUT_VARIABLE} "${output_file}" PARENT_SCOPE)
endfunction()

# Configures the two shaders compiled by the legacy PresentMonUI build.
function(pmon_configure_shaders)
    set(
        PMON_FXC_EXECUTABLE
        ""
        CACHE FILEPATH
        "Optional explicit fxc.exe path; the CMake-selected Windows SDK's fxc.exe is used when empty"
    )

    set(shaders_dir "${PROJECT_SOURCE_DIR}/IntelPresentMon/Shaders")

    _pmon_add_hlsl_shader(
        SOURCE "${shaders_dir}/Line_PS.hlsl"
        PROFILE ps_4_0
        ENTRY main
        OUTPUT_VARIABLE pixel_shader_output
    )
    _pmon_add_hlsl_shader(
        SOURCE "${shaders_dir}/Line_VS.hlsl"
        PROFILE vs_4_0
        ENTRY main
        OUTPUT_VARIABLE vertex_shader_output
    )

    add_custom_target(
        pmon_compile_shaders
        DEPENDS
            "${pixel_shader_output}"
            "${vertex_shader_output}"
    )
    set_target_properties(
        pmon_compile_shaders
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD FALSE
            FOLDER "Build"
    )
endfunction()

# Makes shader compilation part of a product target's normal build.
function(pmon_target_uses_shaders target)
    if(NOT TARGET pmon_compile_shaders)
        message(FATAL_ERROR "pmon_configure_shaders() must run before pmon_target_uses_shaders().")
    endif()

    add_dependencies(${target} pmon_compile_shaders)
endfunction()
