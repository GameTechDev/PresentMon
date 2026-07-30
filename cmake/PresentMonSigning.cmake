include_guard(GLOBAL)

function(pmon_find_signtool output_variable)
    if(PMON_SIGNTOOL_EXECUTABLE AND EXISTS "${PMON_SIGNTOOL_EXECUTABLE}")
        set(${output_variable} "${PMON_SIGNTOOL_EXECUTABLE}" PARENT_SCOPE)
        return()
    endif()

    set(windows_kits_root "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin")
    file(
        GLOB signtool_candidates
        LIST_DIRECTORIES FALSE
        "${windows_kits_root}/*/x64/signtool.exe"
        "${windows_kits_root}/*/x86/signtool.exe"
    )
    if(signtool_candidates)
        list(SORT signtool_candidates COMPARE NATURAL ORDER DESCENDING)
        list(GET signtool_candidates 0 signtool_path)
        set(${output_variable} "${signtool_path}" PARENT_SCOPE)
    else()
        set(${output_variable} "" PARENT_SCOPE)
    endif()
endfunction()

function(pmon_configure_signing)
    set(edss_script "${PMON_EDSS_SIGN_SCRIPT}")
    if(NOT edss_script AND DEFINED ENV{PMON_EDSS_SIGN_SCRIPT})
        set(edss_script "$ENV{PMON_EDSS_SIGN_SCRIPT}")
    endif()
    if(NOT edss_script AND EXISTS "C:/PresentMonBuilder/full-sign-build.ps1")
        set(edss_script "C:/PresentMonBuilder/full-sign-build.ps1")
    endif()

    pmon_find_signtool(signtool_path)

    set(selected_backend "NONE")
    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        if(PMON_SIGNING_BACKEND STREQUAL "EDSS")
            if(NOT edss_script OR NOT EXISTS "${edss_script}")
                message(FATAL_ERROR "The EDSS signing backend was requested but is unavailable.")
            endif()
            set(selected_backend "EDSS")
        elseif(PMON_SIGNING_BACKEND STREQUAL "SIGNTOOL")
            if(NOT signtool_path)
                message(FATAL_ERROR "The direct SignTool backend was requested but SignTool was not found.")
            endif()
            set(selected_backend "SIGNTOOL")
        elseif(edss_script AND EXISTS "${edss_script}")
            set(selected_backend "EDSS")
        elseif(signtool_path)
            set(selected_backend "SIGNTOOL")
        else()
            message(
                FATAL_ERROR
                "Production builds require signing, but neither the EDSS "
                "PowerShell backend nor SignTool is available."
            )
        endif()
    endif()

    set(PMON_EDSS_SIGN_SCRIPT "${edss_script}" CACHE FILEPATH "EDSS signing script" FORCE)
    set(PMON_SIGNTOOL_EXECUTABLE "${signtool_path}" CACHE FILEPATH "SignTool executable" FORCE)
    set(
        PMON_SELECTED_SIGNING_BACKEND
        "${selected_backend}"
        CACHE INTERNAL
        "Selected signing backend"
        FORCE
    )

    set(PMON_SELECTED_SIGNING_BACKEND "${selected_backend}" PARENT_SCOPE)
endfunction()
