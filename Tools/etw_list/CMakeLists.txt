add_executable(etw_list etw_list.cpp)
target_compile_definitions(etw_list PRIVATE _CONSOLE)
target_link_libraries(
    etw_list
    PRIVATE
        pmon::build_options
        pmon::warnings_strict
        pmon::warnings_strict_link
        pmon::release_link_policy
        pmon::version_header
        advapi32
        ole32
        tdh
)
set_target_properties(
    etw_list
    PROPERTIES
        OUTPUT_NAME "etw_list-${PMON_VERSION}-${PMON_ARCH_TAG}"
        FOLDER "Tools"
)
