add_executable(ETLTrimmer main.cpp)
target_compile_definitions(ETLTrimmer PRIVATE _CONSOLE)
target_compile_options(
    ETLTrimmer
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    ETLTrimmer
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_libraries(
    ETLTrimmer
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        pmon::release_link_policy
        pmon::common_utilities
        pmon::present_data
        ole32
        oleaut32
        tdh
)
set_target_properties(ETLTrimmer PROPERTIES FOLDER "Tools")
