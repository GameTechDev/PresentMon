set(metrics_sources
    "${CMAKE_CURRENT_LIST_DIR}/mc/AnimationErrorTracker.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/DisplayFrameQueue.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsCalculator.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsCalculatorAnimation.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsCalculatorCpuGpu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsCalculatorDisplay.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsCalculatorInput.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsCalculatorInstrumented.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsCalculatorInterFrameEvent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/MetricsTypes.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/SwapChainState.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/mc/UnifiedSwapChain.cpp"
)

add_library(pmon_metrics STATIC ${metrics_sources})
add_library(pmon::metrics ALIAS pmon_metrics)
target_compile_definitions(pmon_metrics PRIVATE _LIB)
target_include_directories(
    pmon_metrics
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(
    pmon_metrics
    PUBLIC
        pmon::common_utilities
        pmon::present_data
    PRIVATE
        pmon::build_options
        pmon::warnings_default
)
target_compile_options(
    pmon_metrics
    PRIVATE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)
target_link_options(
    pmon_metrics
    INTERFACE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)
set_target_properties(
    pmon_metrics
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY
            "${PMON_OUTPUT_ROOT}/lib/Metrics-${PMON_ARCHITECTURE}-$<CONFIG>"
        FOLDER "Foundation"
)
