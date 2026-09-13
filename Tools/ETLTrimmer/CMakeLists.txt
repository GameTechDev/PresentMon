include(CheckIncludeFileCXX)
check_include_file_cxx(atlbase.h PMON_ETLTRIMMER_HAS_ATL)
if(NOT PMON_ETLTRIMMER_HAS_ATL)
    unset(PMON_ETLTRIMMER_HAS_ATL CACHE)
    message(
        FATAL_ERROR
        "ETLTrimmer requires the Visual Studio Installer component "
        "\"C++ ATL for latest ${PMON_VS_TOOLSET} build tools (x86 & x64)\". "
        "Install that component and configure again, or configure with "
        "-DPMON_BUILD_TOOLS=OFF to intentionally omit developer tools. "
        "Selected Visual Studio instance: ${PMON_VS_INSTANCE_ROOT}"
    )
endif()

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
