add_library(
    pmon_api_wrapper
    STATIC
        BlobContainer.cpp
        DiagnosticHandler.cpp
        DynamicQuery.cpp
        EtlLogger.cpp
        FixedQuery.cpp
        FrameQuery.cpp
        ProcessTracker.cpp
        Session.cpp
        StaticQuery.cpp
)
add_library(pmon::api_wrapper ALIAS pmon_api_wrapper)

target_compile_definitions(pmon_api_wrapper PRIVATE _LIB)
target_include_directories(
    pmon_api_wrapper
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(
    pmon_api_wrapper
    PUBLIC
        pmon::common_utilities
        pmon::api_wrapper_common
    PRIVATE
        pmon::build_options
        pmon::warnings_default
)
target_compile_options(
    pmon_api_wrapper
    PRIVATE
        /EHa
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    pmon_api_wrapper
    INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    pmon_api_wrapper
    PROPERTIES
        OUTPUT_NAME PresentMonAPIWrapper
        FOLDER "Service and SDK"
)
