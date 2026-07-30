include_guard(GLOBAL)

function(pmon_configure_output_directories)
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
endfunction()
