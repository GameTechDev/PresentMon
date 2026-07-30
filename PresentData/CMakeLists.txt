set(present_data_sources
    Debug.cpp
    GpuTrace.cpp
    InterPresentActivity.cpp
    NvidiaTraceConsumer.cpp
    PresentMonTraceConsumer.cpp
    PresentMonTraceSession.cpp
    TraceConsumer.cpp
    TraceLogging.cpp
)

add_library(pmon_present_data STATIC ${present_data_sources})
add_library(pmon::present_data ALIAS pmon_present_data)
target_include_directories(
    pmon_present_data
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)
target_compile_definitions(
    pmon_present_data
    PRIVATE
        _LIB
        _WIN32_WINNT=0x0601
        NTDDI_VERSION=0x06010000
        WIN32_LEAN_AND_MEAN
)
target_compile_options(
    pmon_present_data
    PRIVATE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)
target_link_options(
    pmon_present_data
    INTERFACE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)
target_link_libraries(
    pmon_present_data
    PUBLIC
        pmon::common_utilities
    PRIVATE
        pmon::build_options
        pmon::warnings_strict
        advapi32
        tdh
)
set_target_properties(
    pmon_present_data
    PROPERTIES
        OUTPUT_NAME PresentData
        ARCHIVE_OUTPUT_DIRECTORY
            "${PMON_OUTPUT_ROOT}/lib/PresentData-${PMON_ARCHITECTURE}-$<CONFIG>"
        FOLDER "Foundation"
)
add_custom_command(
    TARGET pmon_present_data
    POST_BUILD
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:pmon_present_data>"
        "${PMON_OUTPUT_ROOT}/$<CONFIG>/PresentData.lib"
    VERBATIM
)

pmon_register_artifact("${PMON_OUTPUT_ROOT}/$<CONFIG>/PresentData.lib")
