include_guard(GLOBAL)

# VSTest Name filters validated against build/Debug/PresentMonAPI2Tests.dll (128 tests:
# 113 deterministic, 15 environment).
set(
    PMON_API2_CTEST_ENVIRONMENT_FILTER
    "Name~RealtimeOpenSessionTest|Name~RealtimeTrackProcessTest|Name~RealtimeFrameMetricsTest|Name~ApplicationLaunch|Name~TrackUntrack|Name~TrackPresenter|Name~TargetTracked|Name~UntrackOnClose|Name~UntrackOnMurder|Name~OpenServiceTest|Name~DynamicQueryWithoutTracked|Name~ServiceCrash|Name~EtwFlush|Name~TelemetryPeriod|Name~FrameStoreRealtime|Name~FrameStorePaced|Name~FrameStoreBackpressured|Name~FrameStorePlayback|Name~LegacyBackpressured"
)
set(
    PMON_API2_CTEST_DETERMINISTIC_FILTER
    "Name!~RealtimeOpenSessionTest&Name!~RealtimeTrackProcessTest&Name!~RealtimeFrameMetricsTest&Name!~ApplicationLaunch&Name!~TrackUntrack&Name!~TrackPresenter&Name!~TargetTracked&Name!~UntrackOnClose&Name!~UntrackOnMurder&Name!~OpenServiceTest&Name!~DynamicQueryWithoutTracked&Name!~ServiceCrash&Name!~EtwFlush&Name!~TelemetryPeriod&Name!~FrameStoreRealtime&Name!~FrameStorePaced&Name!~FrameStoreBackpressured&Name!~FrameStorePlayback&Name!~LegacyBackpressured"
)

function(pmon_configure_presentmon_optional_test_dir)
    set(
        PMON_PRESENTMON_TESTS_OPTTESTDIR
        ""
        CACHE PATH
        "Optional PresentMonTests ETL/CSV directory (--opttestdir). Empty: read Tests/PresentMonTests.local.runsettings when present."
    )

    if(PMON_PRESENTMON_TESTS_OPTTESTDIR)
        if(NOT IS_DIRECTORY "${PMON_PRESENTMON_TESTS_OPTTESTDIR}")
            message(
                WARNING
                "PMON_PRESENTMON_TESTS_OPTTESTDIR is not a directory: ${PMON_PRESENTMON_TESTS_OPTTESTDIR}"
            )
        else()
            message(
                STATUS
                "PresentMonTests optional directory: ${PMON_PRESENTMON_TESTS_OPTTESTDIR} (cache)"
            )
        endif()
        return()
    endif()

    set(runsettings "${PROJECT_SOURCE_DIR}/Tests/PresentMonTests.local.runsettings")
    if(NOT EXISTS "${runsettings}")
        return()
    endif()

    file(READ "${runsettings}" runsettings_text)
    string(
        REGEX MATCH
        "<PRESENTMON_ADDITIONAL_TEST_DIR>([^<]+)</PRESENTMON_ADDITIONAL_TEST_DIR>"
        _
        "${runsettings_text}"
    )
    if(NOT CMAKE_MATCH_1)
        return()
    endif()

    string(STRIP "${CMAKE_MATCH_1}" optional_dir)
    if(NOT optional_dir)
        return()
    endif()
    if(NOT IS_DIRECTORY "${optional_dir}")
        message(
            STATUS
            "PresentMonTests.local.runsettings optional directory is missing: ${optional_dir}"
        )
        return()
    endif()

    set(
        PMON_PRESENTMON_TESTS_OPTTESTDIR
        "${optional_dir}"
        CACHE PATH
        "Optional PresentMonTests ETL/CSV directory (--opttestdir). Empty: read Tests/PresentMonTests.local.runsettings when present."
        FORCE
    )
    message(
        STATUS
        "PresentMonTests optional directory: ${optional_dir} (PresentMonTests.local.runsettings)"
    )
endfunction()

function(pmon_configure_presentmon_test_runsettings)
    set(gold_dir "${PROJECT_SOURCE_DIR}/Tests/Gold")
    set(discovery_args "--golddir=${gold_dir}")
    set(env_xml "")
    if(PMON_PRESENTMON_TESTS_OPTTESTDIR AND IS_DIRECTORY "${PMON_PRESENTMON_TESTS_OPTTESTDIR}")
        string(APPEND discovery_args " --opttestdir=${PMON_PRESENTMON_TESTS_OPTTESTDIR}")
        set(
            env_xml
            "      <PRESENTMON_ADDITIONAL_TEST_DIR>${PMON_PRESENTMON_TESTS_OPTTESTDIR}</PRESENTMON_ADDITIONAL_TEST_DIR>"
        )
    endif()

    set(
        generated_runsettings
        "${CMAKE_BINARY_DIR}/PresentMonTests.generated.runsettings"
    )
    set(PMON_PRESENTMON_TESTS_DISCOVERY_ARGS "${discovery_args}")
    set(PMON_PRESENTMON_TESTS_ENV_XML "${env_xml}")
    configure_file(
        "${PROJECT_SOURCE_DIR}/Tests/PresentMonTests.generated.runsettings.in"
        "${generated_runsettings}"
        @ONLY
    )
    set(PMON_PRESENTMON_TESTS_GENERATED_RUNSETTINGS "${generated_runsettings}" CACHE FILEPATH
        "Google Test Adapter runsettings for the CMake build tree" FORCE
    )
    message(
        STATUS
        "PresentMonTests Test Explorer (CMake): select runsettings file: ${generated_runsettings}"
    )
endfunction()

function(pmon_apply_presentmon_tests_vs_debugger target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    set(
        debugger_args
        "--presentmon=${PMON_OUTPUT_ROOT}/$(Configuration)/PresentMon-${PMON_VERSION}-${PMON_ARCH_TAG}.exe"
        "--golddir=${PROJECT_SOURCE_DIR}/Tests/Gold"
    )
    if(PMON_PRESENTMON_TESTS_OPTTESTDIR AND IS_DIRECTORY "${PMON_PRESENTMON_TESTS_OPTTESTDIR}")
        list(APPEND debugger_args "--opttestdir=${PMON_PRESENTMON_TESTS_OPTTESTDIR}")
    endif()
    list(JOIN debugger_args " " debugger_args_line)

    set_target_properties(
        ${target_name}
        PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${PMON_OUTPUT_ROOT}/$(Configuration)"
            VS_DEBUGGER_COMMAND_ARGUMENTS "${debugger_args_line}"
    )
endfunction()

function(pmon_find_vstest_console output_variable)
    if(DEFINED PMON_VSTEST_CONSOLE AND PMON_VSTEST_CONSOLE)
        set(${output_variable} "${PMON_VSTEST_CONSOLE}" PARENT_SCOPE)
        return()
    endif()

    set(vswhere "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe")
    if(NOT EXISTS "${vswhere}")
        message(FATAL_ERROR "vswhere.exe is required to locate vstest.console.exe.")
    endif()

    execute_process(
        COMMAND
            "${vswhere}"
            -latest
            -products
            "*"
            -find
            "**/vstest.console.exe"
        OUTPUT_VARIABLE vstest_candidates
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    string(REPLACE "\n" ";" vstest_list "${vstest_candidates}")
    list(FILTER vstest_list EXCLUDE REGEX "arm64")
    if(NOT vstest_list)
        message(
            FATAL_ERROR
            "vstest.console.exe was not found. Install the Visual Studio Test Tools for C++ workload."
        )
    endif()
    list(GET vstest_list 0 vstest_console)

    set(PMON_VSTEST_CONSOLE "${vstest_console}" CACHE FILEPATH "Path to vstest.console.exe")
    set(${output_variable} "${vstest_console}" PARENT_SCOPE)
endfunction()

function(pmon_register_vstest_dll)
    set(options)
    set(oneValueArgs TEST_NAME TARGET TEST_CASE_FILTER)
    set(multiValueArgs LABELS)
    cmake_parse_arguments(
        ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    if(NOT ARG_TEST_NAME OR NOT ARG_TARGET)
        message(FATAL_ERROR "pmon_register_vstest_dll requires TEST_NAME and TARGET.")
    endif()
    if(NOT TARGET ${ARG_TARGET})
        message(FATAL_ERROR "pmon_register_vstest_dll TARGET ${ARG_TARGET} does not exist.")
    endif()

    pmon_find_vstest_console(vstest_console)

    set(test_command
        "${vstest_console}"
        /InIsolation
        "$<TARGET_FILE:${ARG_TARGET}>"
    )
    if(ARG_TEST_CASE_FILTER)
        list(APPEND test_command "/TestCaseFilter:${ARG_TEST_CASE_FILTER}")
    endif()

    add_test(NAME ${ARG_TEST_NAME} COMMAND ${test_command})
    set_tests_properties(
        ${ARG_TEST_NAME}
        PROPERTIES
            LABELS "${ARG_LABELS}"
            WORKING_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>"
    )
endfunction()

function(pmon_register_presentmon_gold_test)
    set(options)
    set(oneValueArgs TEST_NAME TARGET CONSOLE_TARGET GOLD_DIR GTEST_FILTER OPTTEST_DIR)
    set(multiValueArgs LABELS)
    cmake_parse_arguments(
        ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    if(NOT ARG_TEST_NAME OR NOT ARG_TARGET OR NOT ARG_CONSOLE_TARGET OR NOT ARG_GOLD_DIR)
        message(
            FATAL_ERROR
            "pmon_register_presentmon_gold_test requires TEST_NAME, TARGET, CONSOLE_TARGET, and GOLD_DIR."
        )
    endif()

    set(test_command
        "$<TARGET_FILE:${ARG_TARGET}>"
        "--presentmon=$<TARGET_FILE:${ARG_CONSOLE_TARGET}>"
        "--golddir=${ARG_GOLD_DIR}"
    )
    if(ARG_GTEST_FILTER)
        list(APPEND test_command "--gtest_filter=${ARG_GTEST_FILTER}")
    endif()
    if(ARG_OPTTEST_DIR)
        list(APPEND test_command "--opttestdir=${ARG_OPTTEST_DIR}")
    endif()

    add_test(NAME ${ARG_TEST_NAME} COMMAND ${test_command})
    set_tests_properties(
        ${ARG_TEST_NAME}
        PROPERTIES
            LABELS "${ARG_LABELS}"
            WORKING_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>"
    )
endfunction()

function(pmon_collect_ctest_build_dependencies output_list)
    set(deps)
    if(TARGET PresentMonConsole)
        list(APPEND deps PresentMonConsole)
    endif()
    if(TARGET PresentMonTests)
        list(APPEND deps PresentMonTests)
    endif()
    if(TARGET PresentMonUnitTests)
        list(APPEND deps PresentMonUnitTests)
    endif()
    if(TARGET PresentMonAPI2Tests)
        list(APPEND deps PresentMonAPI2Tests)
        if(TARGET PresentMonService)
            list(APPEND deps PresentMonService PresentMonAPI2 PresentMonAPI2Loader KernelProcess PresentMonUI)
        endif()
        if(TARGET pmon_stage_ui_payload)
            list(APPEND deps pmon_stage_ui_payload)
        endif()
        if(TARGET pmon_stage_kernel_payload)
            list(APPEND deps pmon_stage_kernel_payload)
        endif()
    endif()
    set(${output_list} "${deps}" PARENT_SCOPE)
endfunction()

function(pmon_register_all_ctests)
    pmon_configure_presentmon_optional_test_dir()
    pmon_configure_presentmon_test_runsettings()
    if(TARGET PresentMonTests)
        pmon_apply_presentmon_tests_vs_debugger(PresentMonTests)
    endif()
    set(presentmon_opttest_dir "")
    if(PMON_PRESENTMON_TESTS_OPTTESTDIR AND IS_DIRECTORY "${PMON_PRESENTMON_TESTS_OPTTESTDIR}")
        set(presentmon_opttest_dir "${PMON_PRESENTMON_TESTS_OPTTESTDIR}")
    endif()

    if(PMON_BUILD_PRESENTMON_TESTS AND TARGET PresentMonTests AND TARGET PresentMonConsole)
        pmon_register_presentmon_gold_test(
            TEST_NAME presentmon.console.gold
            TARGET PresentMonTests
            CONSOLE_TARGET PresentMonConsole
            GOLD_DIR "${PROJECT_SOURCE_DIR}/Tests/Gold"
            GTEST_FILTER "GoldEtlCsvTests.*"
            OPTTEST_DIR "${presentmon_opttest_dir}"
            LABELS console console.gold deterministic
        )
        pmon_register_presentmon_gold_test(
            TEST_NAME presentmon.console.commandline
            TARGET PresentMonTests
            CONSOLE_TARGET PresentMonConsole
            GOLD_DIR "${PROJECT_SOURCE_DIR}/Tests/Gold"
            GTEST_FILTER "CommandLineTests.*"
            OPTTEST_DIR "${presentmon_opttest_dir}"
            LABELS console console.realtime environment
        )
    endif()

    if(PMON_BUILD_PRESENTMON_TESTS AND TARGET PresentMonTests AND TARGET PresentMonConsole)
        if(PMON_CTEST_FULL_CSV_TESTS OR IS_DIRECTORY "${PROJECT_SOURCE_DIR}/Tests/Full")
            pmon_register_presentmon_gold_test(
                TEST_NAME presentmon.console.full_csv
                TARGET PresentMonTests
                CONSOLE_TARGET PresentMonConsole
                GOLD_DIR "${PROJECT_SOURCE_DIR}/Tests/Full"
                GTEST_FILTER "GoldEtlCsvTests.*"
                OPTTEST_DIR "${presentmon_opttest_dir}"
                LABELS console console.gold deterministic
            )
        endif()
    endif()

    if(PMON_BUILD_UNIT_TESTS AND TARGET PresentMonUnitTests)
        pmon_register_vstest_dll(
            TEST_NAME presentmon.unit
            TARGET PresentMonUnitTests
            LABELS unit deterministic
        )
    endif()

    if(PMON_BUILD_API2_TESTS AND TARGET PresentMonAPI2Tests)
        pmon_register_vstest_dll(
            TEST_NAME presentmon.api2.deterministic
            TARGET PresentMonAPI2Tests
            TEST_CASE_FILTER "${PMON_API2_CTEST_DETERMINISTIC_FILTER}"
            LABELS api2 deterministic
        )
        pmon_register_vstest_dll(
            TEST_NAME presentmon.api2.environment
            TARGET PresentMonAPI2Tests
            TEST_CASE_FILTER "${PMON_API2_CTEST_ENVIRONMENT_FILTER}"
            LABELS api2 api2.realtime environment
        )
    endif()

    pmon_register_ctest_convenience_targets()
endfunction()

function(pmon_register_ctest_convenience_targets)
    if(TARGET pmon_run_deterministic_tests)
        return()
    endif()

    add_custom_target(
        pmon_run_all_tests
        COMMAND
            ${CMAKE_CTEST_COMMAND}
            --test-dir
            "${CMAKE_BINARY_DIR}"
            -C
            $<CONFIG>
            --output-on-failure
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        pmon_run_all_tests
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    add_custom_target(
        pmon_run_deterministic_tests
        COMMAND
            ${CMAKE_CTEST_COMMAND}
            --test-dir
            "${CMAKE_BINARY_DIR}"
            -C
            $<CONFIG>
            -L
            deterministic
            --output-on-failure
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        pmon_run_deterministic_tests
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    pmon_collect_ctest_build_dependencies(ctest_build_deps)
    if(ctest_build_deps)
        add_dependencies(pmon_run_all_tests ${ctest_build_deps})
        add_dependencies(pmon_run_deterministic_tests ${ctest_build_deps})
    endif()
endfunction()
