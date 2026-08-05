# BuildId.cpp includes "generated/build_id.h" relative to its own directory, so
# the header must stay in the source tree. IntelPresentMon/Versioning/generated
# is gitignored, which keeps the generated output untracked.
set(build_id_output_dir "${CMAKE_CURRENT_SOURCE_DIR}/generated")

# The script has its own signature-based change detection and rewrites the
# header only when the worktree actually changed, so it is safe to run on every
# build. MSBuild forces the same behaviour with a nonexistent custom-build
# output.
add_custom_target(
    pmon_build_id
    COMMAND
        powershell.exe
        -ExecutionPolicy Bypass
        -NoProfile
        -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pre-build.ps1"
    BYPRODUCTS
        "${build_id_output_dir}/build_id.h"
        "${build_id_output_dir}/signature_a_prev.txt"
        "${build_id_output_dir}/signature_b_prev.txt"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating build IDs"
    VERBATIM
)
set_property(TARGET pmon_build_id PROPERTY FOLDER "Build")

add_library(
    pmon_versioning
    STATIC
        BuildId.cpp
        PresentMonAPIVersion.cpp
)
add_library(pmon::versioning ALIAS pmon_versioning)
add_dependencies(pmon_versioning pmon_build_id)

target_compile_definitions(pmon_versioning PRIVATE _LIB)
target_include_directories(
    pmon_versioning
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(
    pmon_versioning
    PRIVATE
        pmon::build_options
        pmon::warnings_default
)
target_compile_options(
    pmon_versioning
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    pmon_versioning
    INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    pmon_versioning
    PROPERTIES
        OUTPUT_NAME IPMVersioning
        FOLDER "Foundation"
)
