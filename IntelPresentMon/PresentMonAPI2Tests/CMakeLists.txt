include(PresentMonCppUnitTest)

find_package(boost_process CONFIG REQUIRED)
find_package(boost_filesystem CONFIG REQUIRED)
find_package(unofficial-vincentlaucsb-csv-parser CONFIG REQUIRED)

set(api2_gen_script "${PROJECT_SOURCE_DIR}/Scripts/generate_etl_csv_tests.py")
set(api2_csv "${CMAKE_CURRENT_SOURCE_DIR}/test_cases.csv")

set(api2_generated_headers)
foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    set(
        generated_header
        "${PMON_OUTPUT_ROOT}/obj/${config}/PresentMonAPI2Tests/Generated/EtlCsvTestCases.g.h"
    )
    add_custom_command(
        OUTPUT "${generated_header}"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${api2_gen_script}"
            --input-csv "${api2_csv}"
            --output-header "${generated_header}"
        DEPENDS
            "${api2_gen_script}"
            "${api2_csv}"
        COMMENT "Generating EtlCsvTestCases.g.h (${config})"
        VERBATIM
    )
    list(APPEND api2_generated_headers "${generated_header}")
endforeach()

add_custom_target(
    pmon_api2_etl_csv_cases
    DEPENDS ${api2_generated_headers}
)
set_target_properties(
    pmon_api2_etl_csv_cases
    PROPERTIES
        FOLDER "Build"
)

add_library(
    PresentMonAPI2Tests
    SHARED
        EtlTests.cpp
        InterimBroadcasterTests.cpp
        IpcComponentTests.cpp
        IpcMcIntegrationTests.cpp
        Logging.cpp
        MockTelemetryTests.cpp
        ModuleInit.cpp
        MultiClientTests.cpp
        PacedFrameTests.cpp
        PacedPollingTests.cpp
        RealtimeMetricTests.cpp
        UiLaunchTests.cpp
        "${PROJECT_SOURCE_DIR}/IntelPresentMon/CommonUtilities/test/MachineExpectations.cpp"
)
add_dependencies(PresentMonAPI2Tests pmon_api2_etl_csv_cases)
add_dependencies(PresentMonAPI2Tests pmon_ensure_aux_testdata)

target_include_directories(
    PresentMonAPI2Tests
    PRIVATE
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/IntelPresentMon"
        "${PMON_OUTPUT_ROOT}/obj/$<CONFIG>/PresentMonAPI2Tests/Generated"
)
target_link_libraries(
    PresentMonAPI2Tests
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        pmon::release_link_policy
        pmon::common_utilities
        pmon::interprocess
        pmon::api_wrapper
        pmon::middleware
        pmon::versioning
        PresentMonAPI2Loader
        PresentMonAPI2
        Boost::process
        Boost::filesystem
        unofficial::vincentlaucsb-csv-parser::csv
        ntdll
)
set_target_properties(
    PresentMonAPI2Tests
    PROPERTIES
        OUTPUT_NAME PresentMonAPI2Tests
        FOLDER "Tests"
)

pmon_apply_cppunittest_framework(PresentMonAPI2Tests)

set(api2_runtime_dependencies
    PresentMonService
    PresentMonAPI2
    PresentMonAPI2Loader
    KernelProcess
    PresentMonUI
    SampleClient
)
if(TARGET pmon_stage_ui_payload)
    list(APPEND api2_runtime_dependencies pmon_stage_ui_payload)
endif()
if(TARGET pmon_stage_kernel_payload)
    list(APPEND api2_runtime_dependencies pmon_stage_kernel_payload)
endif()
add_dependencies(PresentMonAPI2Tests ${api2_runtime_dependencies})
