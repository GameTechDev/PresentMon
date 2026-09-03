set(enum_metric_header "${PMON_OUTPUT_ROOT}/obj/generated/metadata/EnumMetric.h")

add_custom_command(
    OUTPUT "${enum_metric_header}"
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPMON_AWK_EXECUTABLE=${PROJECT_SOURCE_DIR}/Tools/awk.exe"
        "-DPMON_AWK_PROGRAM=${PROJECT_SOURCE_DIR}/Tools/generate/EnumMetric/metrics.awk"
        "-DPMON_METRICS_CSV=${CMAKE_CURRENT_SOURCE_DIR}/../metrics.csv"
        "-DPMON_ENUM_METRIC_OUTPUT=${enum_metric_header}"
        -P "${PROJECT_SOURCE_DIR}/cmake/PresentMonGenerateEnumMetric.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../metrics.csv"
        "${PROJECT_SOURCE_DIR}/Tools/generate/EnumMetric/metrics.awk"
        "${PROJECT_SOURCE_DIR}/cmake/PresentMonGenerateEnumMetric.cmake"
    COMMENT "Generating EnumMetric.h"
    VERBATIM
)
set_source_files_properties("${enum_metric_header}" PROPERTIES GENERATED TRUE)

find_package(boost_interprocess CONFIG REQUIRED)

add_library(
    pmon_interprocess
    STATIC
        source/DataStores.cpp
        source/Interprocess.cpp
        source/IntrospectionHelpers.cpp
        source/IntrospectionPopulators.cpp
        source/metadata/MetadataValidators.cpp
        source/MetricCapabilities.cpp
        source/ShmNamer.cpp
        "${enum_metric_header}"
)
add_library(pmon::interprocess ALIAS pmon_interprocess)

target_compile_definitions(pmon_interprocess PRIVATE _LIB)
target_include_directories(
    pmon_interprocess
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
        # IntrospectionMetadata.h resolves <metadata/EnumMetric.h> from here.
        "${PMON_OUTPUT_ROOT}/obj/generated"
)
target_link_libraries(
    pmon_interprocess
    PUBLIC
        pmon::common_utilities
        pmon::metrics
        Boost::interprocess
    PRIVATE
        pmon::build_options
        pmon::warnings_default
)
target_compile_options(
    pmon_interprocess
    PRIVATE
        /EHa
        /we4062
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    pmon_interprocess
    INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    pmon_interprocess
    PROPERTIES
        OUTPUT_NAME Interprocess
        FOLDER "Service and SDK"
)
