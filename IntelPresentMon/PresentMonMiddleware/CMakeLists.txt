find_package(boost_circular_buffer CONFIG REQUIRED)
find_package(boost_container CONFIG REQUIRED)

add_library(
    pmon_middleware
    STATIC
        DynamicQuery.cpp
        DynamicStat.cpp
        FrameEventQuery.cpp
        FrameMetricsSource.cpp
        LogSetup.cpp
        MetricBinding.cpp
        Middleware.cpp
        QueryValidation.cpp
)
add_library(pmon::middleware ALIAS pmon_middleware)
# ActionClient.h includes the generated AllActions.h. INTERFACE libraries carry
# no build rule, so the ordering edge has to be stated on the real target.
add_dependencies(pmon_middleware pmon_service_actions)

# MetricBinding.cpp expands the metric binding tables past the default COFF
# section limit.
set_source_files_properties(
    MetricBinding.cpp
    PROPERTIES COMPILE_OPTIONS /bigobj
)

target_compile_definitions(
    pmon_middleware
    PRIVATE
        _LIB
        $<$<CONFIG:Debug>:_CRTDBG_MAP_ALLOC>
)
target_include_directories(
    pmon_middleware
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(
    pmon_middleware
    PUBLIC
        pmon::common_utilities
        pmon::interprocess
        pmon::metrics
        pmon::api_wrapper_common
        pmon::versioning
        # Headers only: the action protocol definitions live with the service.
        pmon::service_headers
        Boost::circular_buffer
        Boost::container
    PRIVATE
        pmon::build_options
        pmon::warnings_default
)
target_compile_options(
    pmon_middleware
    PRIVATE
        /EHa
        /we4062
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    pmon_middleware
    INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    pmon_middleware
    PROPERTIES
        OUTPUT_NAME PresentMonMiddleware
        FOLDER "Service and SDK"
)
