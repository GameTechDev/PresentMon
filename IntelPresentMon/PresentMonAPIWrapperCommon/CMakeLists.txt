add_library(
    pmon_api_wrapper_common
    STATIC
        EnumMap.cpp
        Exception.cpp
        Introspection.cpp
        PmErrorCodeProvider.cpp
)
add_library(pmon::api_wrapper_common ALIAS pmon_api_wrapper_common)

target_compile_definitions(pmon_api_wrapper_common PRIVATE _LIB)
target_include_directories(
    pmon_api_wrapper_common
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(
    pmon_api_wrapper_common
    PUBLIC
        pmon::common_utilities
    PRIVATE
        pmon::build_options
        pmon::warnings_default
)
target_compile_options(
    pmon_api_wrapper_common
    PRIVATE
        /EHa
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    pmon_api_wrapper_common
    INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    pmon_api_wrapper_common
    PROPERTIES
        OUTPUT_NAME PresentMonAPIWrapperCommon
        FOLDER "Service and SDK"
)

pmon_register_artifact("$<TARGET_FILE:pmon_api_wrapper_common>")
