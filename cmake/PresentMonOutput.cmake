include_guard(GLOBAL)

function(pmon_configure_output_directories)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
        set(pmon_architecture "Win32")
        set(pmon_arch_tag "x86")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
        set(pmon_architecture "x64")
        set(pmon_arch_tag "x64")
    else()
        message(
            FATAL_ERROR
            "Unsupported generator platform: ${CMAKE_GENERATOR_PLATFORM}. "
            "Use x64 or Win32."
        )
    endif()

    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        set(default_output_root "${PROJECT_SOURCE_DIR}/build/production")
    else()
        set(default_output_root "${PROJECT_SOURCE_DIR}/build")
    endif()
    set(
        PMON_OUTPUT_ROOT
        "${default_output_root}"
        CACHE PATH
        "Root directory for externally consumed build artifacts"
    )

    cmake_path(ABSOLUTE_PATH PMON_OUTPUT_ROOT NORMALIZE OUTPUT_VARIABLE normalized_output_root)
    cmake_path(
        ABSOLUTE_PATH
        PROJECT_SOURCE_DIR
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        NORMALIZE
        OUTPUT_VARIABLE normalized_source_root
    )
    set(developer_output_root "${normalized_source_root}/build")
    string(TOLOWER "${normalized_output_root}" normalized_output_root_key)
    string(TOLOWER "${developer_output_root}" developer_output_root_key)
    if(
        PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION"
        AND normalized_output_root_key STREQUAL developer_output_root_key
    )
        message(
            FATAL_ERROR
            "Production and developer builds cannot share PMON_OUTPUT_ROOT. "
            "Use a profile-specific path such as ${default_output_root}."
        )
    endif()

    set(
        PMON_OUTPUT_ROOT
        "${normalized_output_root}"
        CACHE PATH
        "Root directory for externally consumed build artifacts"
        FORCE
    )

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>" PARENT_SCOPE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>" PARENT_SCOPE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>" PARENT_SCOPE)
    set(CMAKE_PDB_OUTPUT_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>" PARENT_SCOPE)
    set(
        CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY
        "${PMON_OUTPUT_ROOT}/obj/$<CONFIG>"
        PARENT_SCOPE
    )

    set(PMON_OUTPUT_ROOT "${PMON_OUTPUT_ROOT}" PARENT_SCOPE)
    set(PMON_ARCHITECTURE "${pmon_architecture}" PARENT_SCOPE)
    set(PMON_ARCH_TAG "${pmon_arch_tag}" PARENT_SCOPE)
endfunction()
