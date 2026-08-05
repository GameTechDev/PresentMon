add_executable(pm_convert_csv pm_convert_csv.cpp)
target_compile_definitions(pm_convert_csv PRIVATE _CONSOLE)
target_link_libraries(
    pm_convert_csv
    PRIVATE
        pmon::build_options
        pmon::warnings_strict
        pmon::warnings_strict_link
        pmon::release_link_policy
)
set_target_properties(
    pm_convert_csv
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            "${PMON_OUTPUT_ROOT}/obj/runtime/pm_convert_csv-${PMON_ARCHITECTURE}-$<CONFIG>"
        FOLDER "Tools"
)
add_custom_command(
    TARGET pm_convert_csv
    POST_BUILD
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:pm_convert_csv>"
        "${PMON_OUTPUT_ROOT}/$<CONFIG>/pm_convert_csv.exe"
    VERBATIM
)
