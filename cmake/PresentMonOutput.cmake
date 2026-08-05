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

    set(
        PMON_OUTPUT_ROOT
        "${PROJECT_SOURCE_DIR}/build"
        CACHE PATH
        "Root directory for externally consumed build artifacts"
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
