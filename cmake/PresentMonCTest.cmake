include_guard(GLOBAL)

# VSTest display-name classification validated against PresentMonAPI2Tests.dll
# (128 tests: 113 deterministic, 15 environment).
set(
    PMON_API2_CTEST_ENVIRONMENT_NAME_REGEX
    "RealtimeOpenSessionTest|RealtimeTrackProcessTest|RealtimeFrameMetricsTest|ApplicationLaunch|TrackUntrack|TrackPresenter|TargetTracked|UntrackOnClose|UntrackOnMurder|OpenServiceTest|DynamicQueryWithoutTracked|ServiceCrash|EtwFlush|TelemetryPeriod|FrameStoreRealtime|FrameStorePaced|FrameStoreBackpressured|FrameStorePlayback|LegacyBackpressured"
)

function(pmon_normalize_test_data_path input_path output_variable)
    file(TO_CMAKE_PATH "${input_path}" normalized_path)
    get_filename_component(
        normalized_path
        "${normalized_path}"
        ABSOLUTE
        BASE_DIR "${PROJECT_SOURCE_DIR}/Tests"
    )
    set(${output_variable} "${normalized_path}" PARENT_SCOPE)
endfunction()

function(pmon_configure_presentmon_test_data_directory)
    set(
        PMON_PRESENTMON_TESTS_CUSTOM_DIR
        ""
        CACHE PATH
        "Custom PresentMon ETL/CSV test directory. Empty uses the pinned Tests/AuxData/Data directory."
    )

    pmon_normalize_test_data_path("${PMON_AUX_TESTDATA_DIR}" canonical_data_dir)
    string(TOLOWER "${canonical_data_dir}" canonical_data_dir_lower)

    set(custom_dir "")
    set(custom_dir_source "")
    if(PMON_PRESENTMON_TESTS_CUSTOM_DIR)
        set(custom_dir "${PMON_PRESENTMON_TESTS_CUSTOM_DIR}")
        set(custom_dir_source "PMON_PRESENTMON_TESTS_CUSTOM_DIR")
    endif()

    file(
        GLOB local_runsettings_files
        CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/Tests/PresentMonTests.local.runsettings"
    )
    if(NOT custom_dir AND local_runsettings_files)
        list(GET local_runsettings_files 0 runsettings)
        file(READ "${runsettings}" runsettings_text)
        string(
            REGEX MATCH
            "<PRESENTMON_ADDITIONAL_TEST_DIR>([^<]+)</PRESENTMON_ADDITIONAL_TEST_DIR>"
            _
            "${runsettings_text}"
        )
        if(CMAKE_MATCH_1)
            string(STRIP "${CMAKE_MATCH_1}" local_dir)
            if(local_dir)
                pmon_normalize_test_data_path("${local_dir}" local_dir)
                string(TOLOWER "${local_dir}" local_dir_lower)
                if(NOT local_dir_lower STREQUAL canonical_data_dir_lower)
                    set(custom_dir "${local_dir}")
                    set(custom_dir_source "PresentMonTests.local.runsettings")
                endif()
            endif()
        endif()
    endif()

    if(custom_dir)
        pmon_normalize_test_data_path("${custom_dir}" custom_dir)
    endif()
    if(custom_dir AND NOT IS_DIRECTORY "${custom_dir}")
        message(
            WARNING
            "PresentMon custom test directory is missing: ${custom_dir}. Falling back to ${canonical_data_dir}."
        )
        set(custom_dir "")
        set(custom_dir_source "")
    endif()

    # PresentMonTests always runs Tests/Gold plus exactly one optional directory. That
    # optional slot holds the pinned AuxData by default; a custom directory redirects the
    # slot rather than adding a second one, so the AuxData cases stop being registered.
    if(custom_dir)
        set(test_data_dir "${custom_dir}")
        message(
            STATUS
            "PresentMon test data: ${test_data_dir} (${custom_dir_source}; replaces pinned AuxData)"
        )
        file(GLOB custom_etl_files "${custom_dir}/test_case_*.etl")
        if(NOT custom_etl_files)
            message(
                WARNING
                "PresentMon custom test directory ${custom_dir} contains no "
                "test_case_<digits>.etl files, so it will contribute no tests and the "
                "pinned AuxData cases are no longer registered either. Only Tests/Gold "
                "cases will run. Name ETL files test_case_<digits>.etl with matching gold "
                "CSVs, or clear PMON_PRESENTMON_TESTS_CUSTOM_DIR to restore AuxData."
            )
        endif()
    else()
        set(test_data_dir "${canonical_data_dir}")
        message(STATUS "PresentMon test data: ${test_data_dir} (pinned AuxData default)")
    endif()

    set(PMON_PRESENTMON_TESTS_DATA_DIR "${test_data_dir}" PARENT_SCOPE)
endfunction()

function(pmon_configure_presentmon_test_runsettings)
    set(gold_dir "${PROJECT_SOURCE_DIR}/Tests/Gold")
    set(test_data_dir "${PMON_PRESENTMON_TESTS_DATA_DIR}")
    foreach(xml_variable IN ITEMS gold_dir test_data_dir)
        string(REPLACE "&" "&amp;" ${xml_variable}_xml "${${xml_variable}}")
        string(REPLACE "<" "&lt;" ${xml_variable}_xml "${${xml_variable}_xml}")
        string(REPLACE ">" "&gt;" ${xml_variable}_xml "${${xml_variable}_xml}")
    endforeach()
    set(
        discovery_args
        "--golddir=&quot;${gold_dir_xml}&quot; --opttestdir=&quot;${test_data_dir_xml}&quot;"
    )
    set(
        env_xml
        "      <PRESENTMON_ADDITIONAL_TEST_DIR>${test_data_dir_xml}</PRESENTMON_ADDITIONAL_TEST_DIR>"
    )
    if(PMON_ARCHITECTURE STREQUAL "x64")
        set(target_platform "x64")
    else()
        set(target_platform "x86")
    endif()

    set(
        generated_runsettings
        "${CMAKE_BINARY_DIR}/PresentMonTests.generated.runsettings"
    )
    set(PMON_PRESENTMON_TESTS_DISCOVERY_ARGS "${discovery_args}")
    set(PMON_PRESENTMON_TESTS_ENV_XML "${env_xml}")
    set(PMON_PRESENTMON_TESTS_TARGET_PLATFORM "${target_platform}")
    configure_file(
        "${PROJECT_SOURCE_DIR}/Tests/PresentMonTests.generated.runsettings.in"
        "${generated_runsettings}"
        @ONLY
    )
    set(PMON_PRESENTMON_TESTS_GENERATED_RUNSETTINGS "${generated_runsettings}" CACHE FILEPATH
        "Google Test Adapter runsettings for the CMake build tree" FORCE
    )
    set(PMON_PRESENTMON_TESTS_GENERATED_RUNSETTINGS "${generated_runsettings}" PARENT_SCOPE)
    message(STATUS "CMake Test Explorer runsettings: ${generated_runsettings} (attached automatically)")
endfunction()

function(pmon_apply_test_runsettings runsettings_path)
    foreach(target_name IN ITEMS PresentMonTests PresentMonUnitTests PresentMonAPI2Tests)
        if(TARGET ${target_name})
            set_property(
                TARGET ${target_name}
                PROPERTY VS_GLOBAL_RunSettingsFilePath "${runsettings_path}"
            )
        endif()
    endforeach()
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
    list(APPEND debugger_args "--opttestdir=${PMON_PRESENTMON_TESTS_DATA_DIR}")
    list(JOIN debugger_args " " debugger_args_line)

    set_target_properties(
        ${target_name}
        PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${PMON_OUTPUT_ROOT}/$(Configuration)"
            VS_DEBUGGER_COMMAND_ARGUMENTS "${debugger_args_line}"
    )
endfunction()

# Resolves vstest.console.exe from the Visual Studio instance CMake selected and
# published as PMON_VS_INSTANCE_ROOT. No other Visual Studio installation is
# consulted. PMON_VSTEST_CONSOLE is a PresentMon-owned internal result and is
# recomputed on every configure rather than honored from the cache.
function(pmon_find_vstest_console output_variable)
    set(
        vstest_console
        "${PMON_VS_INSTANCE_ROOT}/Common7/IDE/CommonExtensions/Microsoft/TestWindow/vstest.console.exe"
    )
    if(NOT EXISTS "${vstest_console}")
        message(
            FATAL_ERROR
            "vstest.console.exe not found:\n"
            "  ${vstest_console}\n"
            "Selected Visual Studio instance:\n"
            "  ${PMON_VS_INSTANCE_ROOT}\n"
            "Install the C++ Test Tools workload component in that instance. "
            "PresentMon does not substitute another Visual Studio installation."
        )
    endif()

    set(PMON_VSTEST_CONSOLE "${vstest_console}" CACHE INTERNAL "Path to vstest.console.exe")
    set(${output_variable} "${vstest_console}" PARENT_SCOPE)
endfunction()

function(pmon_register_vstest_leaf_tests)
    set(options)
    set(
        oneValueArgs
        TARGET
        TEST_PREFIX
        ENVIRONMENT_NAME_REGEX
        TEST_ENVIRONMENT
        FIXTURES_REQUIRED
        RESOURCE_LOCK
    )
    set(multiValueArgs BASE_LABELS DEFAULT_LABELS ENVIRONMENT_LABELS)
    cmake_parse_arguments(
        ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    if(NOT ARG_TARGET OR NOT ARG_TEST_PREFIX)
        message(
            FATAL_ERROR
            "pmon_register_vstest_leaf_tests requires TARGET and TEST_PREFIX."
        )
    endif()
    if(NOT TARGET ${ARG_TARGET})
        message(
            FATAL_ERROR
            "pmon_register_vstest_leaf_tests TARGET ${ARG_TARGET} does not exist."
        )
    endif()

    pmon_find_vstest_console(vstest_console)
    list(JOIN ARG_BASE_LABELS "," base_labels)
    list(JOIN ARG_DEFAULT_LABELS "," default_labels)
    list(JOIN ARG_ENVIRONMENT_LABELS "," environment_labels)

    set(discovery_dir "${CMAKE_BINARY_DIR}/vstest-discovery")
    file(MAKE_DIRECTORY "${discovery_dir}")
    set(include_base "${discovery_dir}/${ARG_TARGET}")
    set(ctest_include_file "${include_base}-$<CONFIG>-include.cmake")
    set(discovered_tests_file "${include_base}-$<CONFIG>-tests.cmake")
    set(discovery_script "${PROJECT_SOURCE_DIR}/Scripts/discover_tests.py")

    string(CONCAT include_content
        "if(EXISTS [==[$<TARGET_FILE:${ARG_TARGET}>]==])\n"
        "  execute_process(\n"
        "    COMMAND [==[${PMON_PYTHON_EXECUTABLE}]==] [==[${discovery_script}]==] vstest\n"
        "      --vstest-console [==[${vstest_console}]==]\n"
        "      --test-container [==[$<TARGET_FILE:${ARG_TARGET}>]==]\n"
        "      --output-file [==[${discovered_tests_file}]==]\n"
        "      --test-prefix [==[${ARG_TEST_PREFIX}]==]\n"
        "      --working-directory [==[$<TARGET_FILE_DIR:${ARG_TARGET}>]==]\n"
        "      --base-labels [==[${base_labels}]==]\n"
        "      --default-labels [==[${default_labels}]==]\n"
        "      --environment-labels [==[${environment_labels}]==]\n"
        "      --environment-name-regex [==[${ARG_ENVIRONMENT_NAME_REGEX}]==]\n"
        "      --test-environment [==[${ARG_TEST_ENVIRONMENT}]==]\n"
        "      --fixtures-required [==[${ARG_FIXTURES_REQUIRED}]==]\n"
        "      --resource-lock [==[${ARG_RESOURCE_LOCK}]==]\n"
        "    RESULT_VARIABLE discovery_result\n"
        "    OUTPUT_VARIABLE discovery_stdout\n"
        "    ERROR_VARIABLE discovery_stderr\n"
        "  )\n"
        "  if(NOT discovery_result EQUAL 0)\n"
        "    message(FATAL_ERROR \"VSTest discovery failed for ${ARG_TARGET}:\\n\${discovery_stdout}\\n\${discovery_stderr}\")\n"
        "  endif()\n"
        "  include([==[${discovered_tests_file}]==])\n"
        "else()\n"
        "  add_test(${ARG_TARGET}_NOT_BUILT ${ARG_TARGET}_NOT_BUILT)\n"
        "endif()\n"
    )
    file(GENERATE
        OUTPUT "${ctest_include_file}"
        CONTENT "${include_content}"
    )

    if(CMAKE_CONFIGURATION_TYPES)
        string(
            REPLACE
            "-$<CONFIG>-include.cmake"
            "-\${CTEST_CONFIGURATION_TYPE}-include.cmake"
            include_file_for_ctest
            "${ctest_include_file}"
        )
        set(wrapper_file "${include_base}-include.cmake")
        file(WRITE "${wrapper_file}"
            "if(EXISTS \"${include_file_for_ctest}\")\n"
            "  include(\"${include_file_for_ctest}\")\n"
            "else()\n"
            "  add_test(${ARG_TARGET}_NOT_BUILT ${ARG_TARGET}_NOT_BUILT)\n"
            "endif()\n"
        )
        set(ctest_include_file "${wrapper_file}")
    endif()

    set_property(
        DIRECTORY
        APPEND
        PROPERTY TEST_INCLUDE_FILES "${ctest_include_file}"
    )
endfunction()

function(pmon_register_auxdata_fixture)
    if(TARGET pmon_ensure_aux_testdata)
        add_test(
            NAME presentmon.auxdata.ensure
            COMMAND
                "${PMON_PYTHON_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/Scripts/auxdata.py"
                ensure
                --repo-root "${PROJECT_SOURCE_DIR}"
                --dependency-root "${PMON_DEPENDENCY_ROOT}"
        )
        set_tests_properties(
            presentmon.auxdata.ensure
            PROPERTIES
                FIXTURES_SETUP pmon_auxdata
                LABELS "testdata"
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Tests"
        )
    endif()
endfunction()

function(pmon_register_presentmon_gtest_group)
    set(options ALLOW_EMPTY)
    set(
        oneValueArgs
        TARGET
        CONSOLE_TARGET
        GROUP_NAME
        TEST_PREFIX
        TEST_FILTER
        GOLD_DIRECTORY
        OPTIONAL_TEST_DIRECTORY
        FIXTURES_REQUIRED
        RESOURCE_LOCK
    )
    set(multiValueArgs LABELS)
    cmake_parse_arguments(
        ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    if(
        NOT ARG_TARGET
        OR NOT ARG_CONSOLE_TARGET
        OR NOT ARG_GROUP_NAME
        OR NOT ARG_TEST_PREFIX
        OR NOT ARG_TEST_FILTER
        OR NOT ARG_GOLD_DIRECTORY
    )
        message(
            FATAL_ERROR
            "pmon_register_presentmon_gtest_group is missing a required argument."
        )
    endif()

    list(JOIN ARG_LABELS "," labels_csv)
    # Optional data-driven groups register nothing when their data is absent. Without
    # this, discovery raises and the generated include aborts every CTest invocation in
    # the tree, not just this group.
    set(allow_empty_argument "")
    if(ARG_ALLOW_EMPTY)
        set(allow_empty_argument "      --allow-empty\n")
    endif()
    set(include_base "${CMAKE_CURRENT_BINARY_DIR}/${ARG_TARGET}-${ARG_GROUP_NAME}")
    set(ctest_include_file "${include_base}-$<CONFIG>-include.cmake")
    set(discovered_tests_file "${include_base}-$<CONFIG>-tests.cmake")
    set(discovery_script "${PROJECT_SOURCE_DIR}/Scripts/discover_tests.py")
    string(CONCAT include_content
        "if(EXISTS [==[$<TARGET_FILE:${ARG_TARGET}>]==])\n"
        "  execute_process(\n"
        "    COMMAND [==[${PMON_PYTHON_EXECUTABLE}]==] [==[${discovery_script}]==] gtest\n"
        "      --test-executable [==[$<TARGET_FILE:${ARG_TARGET}>]==]\n"
        "      --output-file [==[${discovered_tests_file}]==]\n"
        "      --test-prefix [==[${ARG_TEST_PREFIX}]==]\n"
        "      --test-filter [==[${ARG_TEST_FILTER}]==]\n"
        "      --presentmon-path [==[$<TARGET_FILE:${ARG_CONSOLE_TARGET}>]==]\n"
        "      --gold-directory [==[${ARG_GOLD_DIRECTORY}]==]\n"
        "      --optional-test-directory [==[${ARG_OPTIONAL_TEST_DIRECTORY}]==]\n"
        "      --working-directory [==[$<TARGET_FILE_DIR:${ARG_TARGET}>]==]\n"
        "      --labels [==[${labels_csv}]==]\n"
        "      --fixtures-required [==[${ARG_FIXTURES_REQUIRED}]==]\n"
        "      --resource-lock [==[${ARG_RESOURCE_LOCK}]==]\n"
        "${allow_empty_argument}"
        "    RESULT_VARIABLE discovery_result\n"
        "    OUTPUT_VARIABLE discovery_stdout\n"
        "    ERROR_VARIABLE discovery_stderr\n"
        "  )\n"
        "  if(NOT discovery_result EQUAL 0)\n"
        "    message(FATAL_ERROR \"Google Test discovery failed for ${ARG_GROUP_NAME}:\\n\${discovery_stdout}\\n\${discovery_stderr}\")\n"
        "  endif()\n"
        "  include([==[${discovered_tests_file}]==])\n"
        "else()\n"
        "  add_test(${ARG_TARGET}_${ARG_GROUP_NAME}_NOT_BUILT ${ARG_TARGET}_${ARG_GROUP_NAME}_NOT_BUILT)\n"
        "endif()\n"
    )
    file(GENERATE
        OUTPUT "${ctest_include_file}"
        CONTENT "${include_content}"
    )

    if(CMAKE_CONFIGURATION_TYPES)
        string(
            REPLACE
            "-$<CONFIG>-include.cmake"
            "-\${CTEST_CONFIGURATION_TYPE}-include.cmake"
            include_file_for_ctest
            "${ctest_include_file}"
        )
        set(wrapper_file "${include_base}-include.cmake")
        file(WRITE "${wrapper_file}"
            "if(EXISTS \"${include_file_for_ctest}\")\n"
            "  include(\"${include_file_for_ctest}\")\n"
            "else()\n"
            "  add_test(${ARG_TARGET}_${ARG_GROUP_NAME}_NOT_BUILT ${ARG_TARGET}_${ARG_GROUP_NAME}_NOT_BUILT)\n"
            "endif()\n"
        )
        set(ctest_include_file "${wrapper_file}")
    endif()

    set_property(
        DIRECTORY
        APPEND
        PROPERTY TEST_INCLUDE_FILES "${ctest_include_file}"
    )
endfunction()

function(pmon_register_presentmon_gtest_leaf_tests target_name console_target_name)
    if(NOT TARGET ${target_name} OR NOT TARGET ${console_target_name})
        message(
            FATAL_ERROR
            "pmon_register_presentmon_gtest_leaf_tests requires existing test and console targets."
        )
    endif()

    pmon_register_presentmon_gtest_group(
        TARGET ${target_name}
        CONSOLE_TARGET ${console_target_name}
        GROUP_NAME gold
        TEST_PREFIX "PresentMonTests."
        TEST_FILTER "GoldEtlCsvTests.*"
        GOLD_DIRECTORY "${PROJECT_SOURCE_DIR}/Tests/Gold"
        OPTIONAL_TEST_DIRECTORY "${PMON_PRESENTMON_TESTS_DATA_DIR}"
        LABELS "PresentMonTests;GoldEtlCsvTests;console;console.gold;deterministic"
        FIXTURES_REQUIRED pmon_auxdata
        RESOURCE_LOCK pmon_console
    )
    pmon_register_presentmon_gtest_group(
        TARGET ${target_name}
        CONSOLE_TARGET ${console_target_name}
        GROUP_NAME commandline
        TEST_PREFIX "PresentMonTests."
        TEST_FILTER "CommandLineTests.*"
        GOLD_DIRECTORY "${PROJECT_SOURCE_DIR}/Tests/Gold"
        LABELS "PresentMonTests;CommandLineTests;console;console.realtime;environment"
        RESOURCE_LOCK pmon_console
    )

    if(PMON_CTEST_FULL_CSV_TESTS OR IS_DIRECTORY "${PROJECT_SOURCE_DIR}/Tests/Full")
        if(PMON_CTEST_FULL_CSV_TESTS AND NOT IS_DIRECTORY "${PROJECT_SOURCE_DIR}/Tests/Full")
            message(
                WARNING
                "PMON_CTEST_FULL_CSV_TESTS is ON but ${PROJECT_SOURCE_DIR}/Tests/Full does "
                "not exist. The Full CSV suite will register no tests until that directory "
                "contains test_case_<digits>.etl files with matching gold CSVs."
            )
        endif()
        # ALLOW_EMPTY: the Full data set is optional and is not part of the repository.
        pmon_register_presentmon_gtest_group(
            TARGET ${target_name}
            CONSOLE_TARGET ${console_target_name}
            GROUP_NAME full
            TEST_PREFIX "PresentMonTests.Full."
            TEST_FILTER "GoldEtlCsvTests.*"
            GOLD_DIRECTORY "${PROJECT_SOURCE_DIR}/Tests/Full"
            LABELS "PresentMonTests;GoldEtlCsvTests;console;console.gold;full;deterministic"
            RESOURCE_LOCK pmon_console
            ALLOW_EMPTY
        )
    endif()
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
    pmon_configure_presentmon_test_runsettings()
    pmon_apply_test_runsettings("${PMON_PRESENTMON_TESTS_GENERATED_RUNSETTINGS}")
    if(TARGET PresentMonTests)
        pmon_apply_presentmon_tests_vs_debugger(PresentMonTests)
    endif()
    pmon_register_auxdata_fixture()

    if(PMON_BUILD_UNIT_TESTS AND TARGET PresentMonUnitTests)
        pmon_register_vstest_leaf_tests(
            TARGET PresentMonUnitTests
            TEST_PREFIX "PresentMonUnitTests."
            BASE_LABELS PresentMonUnitTests unit
            DEFAULT_LABELS deterministic
            RESOURCE_LOCK pmon_vstest
        )
    endif()

    if(PMON_BUILD_API2_TESTS AND TARGET PresentMonAPI2Tests)
        pmon_register_vstest_leaf_tests(
            TARGET PresentMonAPI2Tests
            TEST_PREFIX "PresentMonAPI2Tests."
            BASE_LABELS PresentMonAPI2Tests api2
            DEFAULT_LABELS deterministic
            ENVIRONMENT_LABELS api2.realtime environment
            ENVIRONMENT_NAME_REGEX "${PMON_API2_CTEST_ENVIRONMENT_NAME_REGEX}"
            TEST_ENVIRONMENT
                "PRESENTMON_ADDITIONAL_TEST_DIR=${PMON_PRESENTMON_TESTS_DATA_DIR}"
            FIXTURES_REQUIRED pmon_auxdata
            RESOURCE_LOCK pmon_vstest
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
