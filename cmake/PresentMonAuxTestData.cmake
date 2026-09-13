include_guard(GLOBAL)

function(pmon_configure_aux_testdata)
    set(tests_dir "${PROJECT_SOURCE_DIR}/Tests")
    set(aux_data_dir "${tests_dir}/AuxData/Data")
    add_custom_target(
        pmon_ensure_aux_testdata
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/Scripts/auxdata.py"
            ensure
            --repo-root "${PROJECT_SOURCE_DIR}"
            --dependency-root "${PMON_DEPENDENCY_ROOT}"
        WORKING_DIRECTORY "${tests_dir}"
        COMMENT "Ensuring pinned auxiliary test data is available"
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        pmon_ensure_aux_testdata
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    add_custom_target(
        pmon_restore_aux_testdata
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/Scripts/auxdata.py"
            restore
            --repo-root "${PROJECT_SOURCE_DIR}"
            --dependency-root "${PMON_DEPENDENCY_ROOT}"
        WORKING_DIRECTORY "${tests_dir}"
        COMMENT "Restoring pinned auxiliary test data into Tests/AuxData"
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        pmon_restore_aux_testdata
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
            FOLDER "Build"
    )

    set(
        PMON_AUX_TESTDATA_DIR
        "${aux_data_dir}"
        CACHE INTERNAL
        "Canonical pinned auxiliary test data directory"
        FORCE
    )
    set(PMON_AUX_TESTDATA_DIR "${aux_data_dir}" PARENT_SCOPE)
endfunction()
