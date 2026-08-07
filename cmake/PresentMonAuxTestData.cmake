include_guard(GLOBAL)

function(pmon_configure_aux_testdata)
    set(tests_dir "${PROJECT_SOURCE_DIR}/Tests")
    set(
        powershell_command
        powershell.exe
        -NoLogo
        -NoProfile
        -NonInteractive
        -ExecutionPolicy Bypass
    )

    add_custom_target(
        pmon_restore_aux_testdata
        COMMAND
            ${powershell_command}
            -File "${tests_dir}/pull-aux.ps1"
            -MainRepoPath "${tests_dir}"
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
endfunction()
