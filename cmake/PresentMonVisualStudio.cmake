include_guard(GLOBAL)

# Visual Studio generator and platform toolset policy.
#
# The -G argument is the user-facing Visual Studio version selector. Each
# supported generator is paired with exactly one native platform toolset, and
# cross-pairings (for example Visual Studio 2026 with v143) are not supported:
#
#     Visual Studio 17 2022 -> v143
#     Visual Studio 18 2026 -> v145
#
# Users are not required to pass -T. CMake selects the native toolset by
# default and the selection is validated after project() initializes the
# toolchain. CMake also selects the concrete Visual Studio installation and
# records it in CMAKE_GENERATOR_INSTANCE; that instance is authoritative for
# every Visual Studio-owned tool this build uses, so no module may run an
# independent latest-version search.
#
# Normalized results are published as internal cache entries:
#
#     PMON_VS_GENERATOR      the validated Visual Studio generator
#     PMON_VS_TOOLSET        the validated toolset family (v143 or v145)
#     PMON_VS_INSTANCE_ROOT  the installation directory of the selected instance
#
# PMON_VS_TOOLSET deliberately carries only the toolset family. It never
# carries an MSVC servicing version such as 14.50 or 14.51, because it is used
# to qualify compiled dependency directories that must remain stable across
# Visual Studio servicing updates.

set(PMON_SUPPORTED_VS_GENERATORS "Visual Studio 17 2022" "Visual Studio 18 2026")

# Maps a supported generator to its native platform toolset, or to an empty
# string when the generator is not supported.
function(_pmon_native_toolset_for_generator generator out_var)
    if("${generator}" STREQUAL "Visual Studio 17 2022")
        set("${out_var}" "v143" PARENT_SCOPE)
    elseif("${generator}" STREQUAL "Visual Studio 18 2026")
        set("${out_var}" "v145" PARENT_SCOPE)
    else()
        set("${out_var}" "" PARENT_SCOPE)
    endif()
endfunction()

function(_pmon_supported_generator_summary out_var)
    set(summary "")
    foreach(generator IN LISTS PMON_SUPPORTED_VS_GENERATORS)
        _pmon_native_toolset_for_generator("${generator}" toolset)
        string(APPEND summary "  -G \"${generator}\"  (platform toolset ${toolset})\n")
    endforeach()
    set("${out_var}" "${summary}" PARENT_SCOPE)
endfunction()

# Extracts and validates the installation location from a generator-instance
# specification that includes a leading location field.
function(_pmon_instance_root_from_specification specification out_var)
    # CMAKE_GENERATOR_INSTANCE is either <location>, <location>,<key>=<value>...
    # or <key>=<value>... The installation location is the leading field.
    string(REPLACE "," ";" instance_fields "${specification}")
    list(GET instance_fields 0 instance_root)
    if(instance_root MATCHES "^[A-Za-z_][A-Za-z0-9_]*=")
        message(
            FATAL_ERROR
            "CMAKE_GENERATOR_INSTANCE (${specification}) does not "
            "specify an installation location. PresentMon needs the location "
            "of the selected Visual Studio instance. Configure a fresh binary "
            "directory without CMAKE_GENERATOR_INSTANCE, or include the "
            "installation path in its value."
        )
    endif()

    cmake_path(SET instance_root NORMALIZE "${instance_root}")
    if(NOT IS_DIRECTORY "${instance_root}")
        message(
            FATAL_ERROR
            "The Visual Studio instance selected by CMake does not exist:\n"
            "  ${instance_root}\n"
            "Install ${CMAKE_GENERATOR} or configure a fresh binary directory."
        )
    endif()

    set("${out_var}" "${instance_root}" PARENT_SCOPE)
endfunction()

# Selects the Visual Studio instance before project() initializes languages.
# CMake has not populated a default CMAKE_GENERATOR_INSTANCE at this point. If
# the user did not provide an explicit instance, use the generator-specific
# COMNTOOLS environment selector documented by CMake. The selector is scoped
# to this CMake process and ensures that CMake and pre-project dependency setup
# use the same installation.
function(_pmon_select_vs_instance_root out_var)
    if(CMAKE_GENERATOR_INSTANCE)
        _pmon_instance_root_from_specification(
            "${CMAKE_GENERATOR_INSTANCE}"
            instance_root
        )
        set("${out_var}" "${instance_root}" PARENT_SCOPE)
        return()
    endif()

    if(CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022")
        set(instance_environment "VS170COMNTOOLS")
        set(common_tools "$ENV{VS170COMNTOOLS}")
        set(version_range "[17.0,18.0)")
    else()
        set(instance_environment "VS180COMNTOOLS")
        set(common_tools "$ENV{VS180COMNTOOLS}")
        set(version_range "[18.0,19.0)")
    endif()

    if(common_tools)
        cmake_path(SET common_tools NORMALIZE "${common_tools}")
        # COMNTOOLS commonly ends with a trailing separator. cmake_path
        # PARENT_PATH treats that as part of the path and only removes the
        # separator, so strip it before walking up to the instance root.
        string(REGEX REPLACE "[/\\\\]+$" "" common_tools "${common_tools}")
        cmake_path(GET common_tools PARENT_PATH common7_root)
        cmake_path(GET common7_root PARENT_PATH instance_root)
        if(NOT IS_DIRECTORY "${instance_root}/Common7/Tools")
            message(
                FATAL_ERROR
                "${instance_environment} does not identify a valid Visual "
                "Studio installation:\n"
                "  ${common_tools}\n"
                "Fix or unset ${instance_environment}, then configure a fresh "
                "binary directory."
            )
        endif()
    else()
        set(
            vswhere
            "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe"
        )
        if(NOT EXISTS "${vswhere}")
            message(
                FATAL_ERROR
                "Visual Studio Installer discovery is unavailable because "
                "vswhere.exe was not found at:\n  ${vswhere}"
            )
        endif()

        execute_process(
            COMMAND
                "${vswhere}"
                -latest
                -products *
                -version "${version_range}"
                -requires Microsoft.Component.MSBuild
                -property installationPath
            RESULT_VARIABLE vswhere_result
            OUTPUT_VARIABLE instance_root
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_VARIABLE vswhere_error
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        if(NOT vswhere_result EQUAL 0 OR NOT instance_root)
            message(
                FATAL_ERROR
                "No Visual Studio instance compatible with generator "
                "${CMAKE_GENERATOR} was found. Install that Visual Studio "
                "version with MSBuild and its native C++ toolset.\n"
                "vswhere: ${vswhere_error}"
            )
        endif()
        cmake_path(SET instance_root NORMALIZE "${instance_root}")
    endif()

    if(NOT IS_DIRECTORY "${instance_root}/Common7/Tools")
        message(
            FATAL_ERROR
            "Visual Studio Installer discovery returned an invalid "
            "installation for generator ${CMAKE_GENERATOR}:\n"
            "  ${instance_root}"
        )
    endif()

    if(CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022")
        set(ENV{VS170COMNTOOLS} "${instance_root}/Common7/Tools")
    else()
        set(ENV{VS180COMNTOOLS} "${instance_root}/Common7/Tools")
    endif()

    set("${out_var}" "${instance_root}" PARENT_SCOPE)
endfunction()

function(_pmon_resolve_recorded_vs_instance_root out_var)
    if(NOT CMAKE_GENERATOR_INSTANCE)
        message(
            FATAL_ERROR
            "CMake did not record a Visual Studio instance for generator "
            "${CMAKE_GENERATOR}. PresentMon resolves every Visual Studio-owned "
            "tool from the selected instance, so configuration cannot continue."
        )
    endif()
    _pmon_instance_root_from_specification(
        "${CMAKE_GENERATOR_INSTANCE}"
        instance_root
    )
    set("${out_var}" "${instance_root}" PARENT_SCOPE)
endfunction()

# Runs before project(). Accepts only the supported Visual Studio generators,
# rejects an explicit generator/toolset cross-pairing, and establishes the
# expected toolset family early enough for dependency modules to qualify their
# compiled artifact directories before the vcpkg toolchain is initialized.
function(pmon_configure_toolchain)
    _pmon_supported_generator_summary(supported_summary)

    _pmon_native_toolset_for_generator("${CMAKE_GENERATOR}" expected_toolset)
    if(NOT expected_toolset)
        message(
            FATAL_ERROR
            "Unsupported CMake generator \"${CMAKE_GENERATOR}\".\n"
            "PresentMon supports only these generators:\n"
            "${supported_summary}"
            "Configure a fresh binary directory with a supported generator. "
            "CMake cannot change the generator of an existing binary tree."
        )
    endif()

    # -T is optional. When it names a toolset, reject a cross-pairing here,
    # before any dependency restore work runs.
    if(CMAKE_GENERATOR_TOOLSET)
        string(REPLACE "," ";" toolset_fields "${CMAKE_GENERATOR_TOOLSET}")
        list(GET toolset_fields 0 requested_toolset)
        string(STRIP "${requested_toolset}" requested_toolset)
        string(TOLOWER "${requested_toolset}" requested_toolset)
        # Fields such as host=x64 or version=14.44 select details of the native
        # toolset rather than replacing it; only a leading toolset name pairs.
        if(requested_toolset AND NOT requested_toolset MATCHES "=")
            if(NOT requested_toolset STREQUAL "${expected_toolset}")
                message(
                    FATAL_ERROR
                    "Generator/toolset cross-pairing is not supported.\n"
                    "  Generator:        ${CMAKE_GENERATOR}\n"
                    "  Native toolset:   ${expected_toolset}\n"
                    "  Requested (-T):   ${CMAKE_GENERATOR_TOOLSET}\n"
                    "PresentMon enforces these pairings:\n"
                    "${supported_summary}"
                    "Drop -T and let CMake select the native toolset."
                )
            endif()
        endif()
    endif()

    _pmon_select_vs_instance_root(instance_root)

    set(
        PMON_VS_GENERATOR
        "${CMAKE_GENERATOR}"
        CACHE INTERNAL
        "Validated Visual Studio generator"
    )
    set(
        PMON_VS_TOOLSET
        "${expected_toolset}"
        CACHE INTERNAL
        "Validated platform toolset family qualifying compiled dependencies"
    )
    set(
        PMON_VS_INSTANCE_ROOT
        "${instance_root}"
        CACHE INTERNAL
        "Installation root of the Visual Studio instance selected by CMake"
    )
endfunction()

# Runs after project(). Confirms CMake selected the native toolset and retained
# the Visual Studio instance resolved before dependency initialization.
function(pmon_validate_toolchain)
    if(NOT PMON_VS_TOOLSET)
        message(
            FATAL_ERROR
            "pmon_configure_toolchain() must run before project()."
        )
    endif()

    if(NOT CMAKE_VS_PLATFORM_TOOLSET)
        message(
            FATAL_ERROR
            "CMake did not report a Visual Studio platform toolset for "
            "generator ${CMAKE_GENERATOR}. PresentMon requires the "
            "${PMON_VS_TOOLSET} platform toolset."
        )
    endif()

    string(TOLOWER "${CMAKE_VS_PLATFORM_TOOLSET}" selected_toolset)
    if(NOT selected_toolset STREQUAL "${PMON_VS_TOOLSET}")
        message(
            FATAL_ERROR
            "Generator ${CMAKE_GENERATOR} requires platform toolset "
            "${PMON_VS_TOOLSET}, but CMake selected "
            "${CMAKE_VS_PLATFORM_TOOLSET}.\n"
            "PresentMon does not support cross-pairings. Install the "
            "${PMON_VS_TOOLSET} build tools in the selected Visual Studio "
            "instance and configure a fresh binary directory."
        )
    endif()

    _pmon_resolve_recorded_vs_instance_root(selected_instance_root)
    if(NOT selected_instance_root STREQUAL "${PMON_VS_INSTANCE_ROOT}")
        message(
            FATAL_ERROR
            "The Visual Studio generator instance changed during configure.\n"
            "  Before project(): ${PMON_VS_INSTANCE_ROOT}\n"
            "  After project():  ${selected_instance_root}\n"
            "Configure a fresh binary directory and do not modify "
            "CMAKE_GENERATOR_INSTANCE in project code or a toolchain file."
        )
    endif()
endfunction()
