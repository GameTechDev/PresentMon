find_package(GTest CONFIG REQUIRED)

add_executable(
    PresentMonTests
    CommandLineTests.cpp
    GoldEtlCsvTests.cpp
    PresentMonTests.cpp
    PresentMon.cpp
)
target_compile_definitions(PresentMonTests PRIVATE _CONSOLE)
target_link_libraries(
    PresentMonTests
    PRIVATE
        pmon::build_options
        pmon::warnings_strict
        pmon::warnings_strict_link
        pmon::release_link_policy
        pmon::version_header
        GTest::gtest
)
set_target_properties(
    PresentMonTests
    PROPERTIES
        OUTPUT_NAME "PresentMonTests-${PMON_VERSION}-${PMON_ARCH_TAG}"
        FOLDER "Tests"
)
add_dependencies(PresentMonTests PresentMonConsole)
