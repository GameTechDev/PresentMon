include_guard(GLOBAL)

# One target for CLI and preset builds: F5-ready capture stack plus CTest binaries.
function(pmon_add_developer_aggregate_targets)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER")
        return()
    endif()
    if(NOT PMON_ARCHITECTURE STREQUAL "x64")
        return()
    endif()
    if(TARGET pmon_developer_debug)
        return()
    endif()

    set(deps)
    if(TARGET KernelProcess)
        list(APPEND deps KernelProcess)
    endif()
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
    endif()

    if(NOT deps)
        return()
    endif()

    add_custom_target(
        pmon_developer_debug
        DEPENDS ${deps}
    )
    set_target_properties(
        pmon_developer_debug
        PROPERTIES
            FOLDER "Build"
    )
endfunction()
