include_guard(GLOBAL)



set(PMON_SIGNING_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")



function(pmon_find_signtool output_variable)

    if(PMON_SIGNTOOL_EXECUTABLE AND EXISTS "${PMON_SIGNTOOL_EXECUTABLE}")

        set(${output_variable} "${PMON_SIGNTOOL_EXECUTABLE}" PARENT_SCOPE)

        return()

    endif()



    set(windows_kits_root "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin")

    if(PMON_ARCHITECTURE STREQUAL "Win32")
        set(signtool_glob_suffix "x86")
    else()
        set(signtool_glob_suffix "x64")
    endif()

    file(

        GLOB signtool_candidates

        LIST_DIRECTORIES FALSE

        "${windows_kits_root}/*/${signtool_glob_suffix}/signtool.exe"

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

    set(edss_script "")
    if(PMON_EDSS_SIGN_SCRIPT AND NOT "${PMON_EDSS_SIGN_SCRIPT}" STREQUAL "")
        set(edss_script "${PMON_EDSS_SIGN_SCRIPT}")
    elseif(DEFINED ENV{PMON_EDSS_SIGN_SCRIPT} AND NOT "$ENV{PMON_EDSS_SIGN_SCRIPT}" STREQUAL "")
        set(edss_script "$ENV{PMON_EDSS_SIGN_SCRIPT}")
    endif()



    pmon_find_signtool(signtool_path)



    set(selected_backend "NONE")

    if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")

        if(NOT edss_script OR NOT EXISTS "${edss_script}")

            message(
                FATAL_ERROR
                "Production builds require an external EDSS signing script (not in this repo). "
                "Set the PMON_EDSS_SIGN_SCRIPT environment variable or -DPMON_EDSS_SIGN_SCRIPT "
                "to the full path of your signing PowerShell script (e.g. under PresentMonBuilder) "
                "before configuring. The file must exist at configure time."
            )

        endif()

        set(selected_backend "EDSS")

    elseif(
        PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER"
        AND PMON_BUILD_UI
    )

        set(selected_backend "SIGNTOOL")

    endif()



    set(PMON_EDSS_SIGN_SCRIPT "${edss_script}" CACHE FILEPATH "EDSS signing script" FORCE)

    set(PMON_SIGNTOOL_EXECUTABLE "${signtool_path}" CACHE FILEPATH "SignTool executable" FORCE)

    set(

        PMON_SELECTED_SIGNING_BACKEND

        "${selected_backend}"

        CACHE INTERNAL

        "Signing lane implied by PMON_DEPLOYMENT_PROFILE"

        FORCE

    )



    set(PMON_SELECTED_SIGNING_BACKEND "${selected_backend}" PARENT_SCOPE)

endfunction()



function(pmon_apply_kernel_process_manifest target)

    target_link_options(

        ${target}

        PRIVATE

            # Debug keeps the default non-uiAccess manifest in every profile.

            # Release enables uiAccess for both DEVELOPER and PRODUCTION profiles.

            $<$<CONFIG:Release>:/MANIFESTUAC:level='asInvoker'>

            $<$<CONFIG:Release>:/MANIFESTUAC:uiAccess='true'>

    )

endfunction()



function(pmon_attach_kernel_process_developer_signing target)
    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER")
        return()
    endif()
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/Scripts/sign.py"
            developer
            --configuration $<CONFIG>
            --target "$<TARGET_FILE:${target}>"
            --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
            --certificate-store "${PMON_SIGNTOOL_CERTIFICATE_STORE}"
            --certificate-name "${PMON_SIGNTOOL_CERTIFICATE_NAME}"
            --timestamp-url "${PMON_SIGNTOOL_TIMESTAMP_URL}"
        COMMENT "Signing Release KernelProcess for local development"
        VERBATIM
    )
endfunction()


function(pmon_add_explicit_developer_signing_target signing_target binary_target component_name)
    add_custom_target(
        ${signing_target}
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPMON_BUILD_CONFIG=$<CONFIG>"
            -P "${PMON_SIGNING_CMAKE_DIR}/PresentMonSigningPreflight.cmake"
        COMMAND
            "${PMON_PYTHON_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/Scripts/sign.py"
            developer
            --configuration $<CONFIG>
            --target "$<TARGET_FILE:${binary_target}>"
            --signtool "${PMON_SIGNTOOL_EXECUTABLE}"
            --certificate-store "${PMON_SIGNTOOL_CERTIFICATE_STORE}"
            --certificate-name "${PMON_SIGNTOOL_CERTIFICATE_NAME}"
            --timestamp-url "${PMON_SIGNTOOL_TIMESTAMP_URL}"
        DEPENDS
            ${binary_target}
            "${PROJECT_SOURCE_DIR}/Scripts/sign.py"
            "${PROJECT_SOURCE_DIR}/Scripts/pmon/signing.py"
            "${PMON_SIGNING_CMAKE_DIR}/PresentMonSigningPreflight.cmake"
        COMMENT "Signing Release ${component_name} for local development"
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        ${signing_target}
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Signing"
    )
endfunction()



function(pmon_add_production_signing_targets)

    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")

        return()

    endif()
    # Run after Release payload is built (build/Release under PMON_OUTPUT_ROOT).

    set(pmon_production_sign_mode_args "")

    if(PMON_EDSS_IN_PROCESS_SIGNING)

        set(pmon_production_sign_mode_args -InProcessSigning)

    else()

        set(pmon_production_sign_mode_args -ImpersonatedSigning)

    endif()

    set(pmon_production_sign_scope_args "")
    if(PMON_ARCHITECTURE STREQUAL "Win32")
        if(PMON_BUILD_SCOPE STREQUAL "CONSOLE")
            set(pmon_production_sign_scope_args -SignWin32ConsoleOnly)
        elseif(PMON_BUILD_SCOPE STREQUAL "PROVIDER")
            set(pmon_production_sign_scope_args -SignWin32ProviderOnly)
        endif()
    endif()

    set(
        payload_sign_stamp
        "${CMAKE_BINARY_DIR}/packaging/Release/ProductionPayloadSigned.stamp"
    )

    set(standalone_win32_signing_preflight "")
    if(
        PMON_ARCHITECTURE STREQUAL "Win32"
        AND (
            PMON_BUILD_SCOPE STREQUAL "CONSOLE"
            OR PMON_BUILD_SCOPE STREQUAL "PROVIDER"
        )
    )
        set(
            standalone_win32_signing_preflight
            COMMAND
                "${CMAKE_COMMAND}"
                "-DPMON_BUILD_CONFIG=$<CONFIG>"
                -P "${PMON_SIGNING_CMAKE_DIR}/PresentMonSigningPreflight.cmake"
        )
    endif()

    add_custom_target(
        pmon_sign_production_payload
        ${standalone_win32_signing_preflight}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/packaging/Release"
        COMMAND
            powershell
            -NoProfile
            -ExecutionPolicy
            Bypass
            -File
            "${PMON_EDSS_SIGN_SCRIPT}"
            -OutputRoot
            "${PMON_OUTPUT_ROOT}/Release"
            -RepoRoot
            "${CMAKE_SOURCE_DIR}"
            ${pmon_production_sign_mode_args}
            ${pmon_production_sign_scope_args}
            -Verify
        COMMAND "${CMAKE_COMMAND}" -E touch "${payload_sign_stamp}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Signing the production payload through EDSS"
        USES_TERMINAL
        VERBATIM
    )
    set_target_properties(
        pmon_sign_production_payload
        PROPERTIES
            EXCLUDE_FROM_DEFAULT_BUILD ON
            FOLDER "Packaging"
    )
    foreach(
        payload_target
        IN ITEMS
            PresentMonUI
            KernelProcess
            PresentMonService
            PresentMonAPI2
            PresentMonAPI2Loader
            pmon_provider
            PresentMonConsole
            pmon_stage_cef_runtime
            pmon_stage_ui_payload
            pmon_stage_kernel_payload
            pmon_compile_shaders
    )
        if(TARGET ${payload_target})
            add_dependencies(pmon_sign_production_payload ${payload_target})
        endif()
    endforeach()

endfunction()



function(pmon_add_win32_component_signing_targets)
    if(NOT PMON_ARCHITECTURE STREQUAL "Win32")
        return()
    endif()

    if(PMON_BUILD_SCOPE STREQUAL "CONSOLE")
        set(binary_target PresentMonConsole)
        set(developer_signing_target pmon_sign_developer_console)
        set(production_signing_target pmon_sign_production_console)
        set(component_name "Win32 console")
    elseif(PMON_BUILD_SCOPE STREQUAL "PROVIDER")
        set(binary_target pmon_provider)
        set(developer_signing_target pmon_sign_developer_provider)
        set(production_signing_target pmon_sign_production_provider)
        set(component_name "Win32 provider")
    else()
        return()
    endif()

    if(PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER")
        pmon_add_explicit_developer_signing_target(
            ${developer_signing_target}
            ${binary_target}
            "${component_name}"
        )
    elseif(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
        add_custom_target(${production_signing_target})
        add_dependencies(${production_signing_target} pmon_sign_production_payload)
        set_target_properties(
            ${production_signing_target}
            PROPERTIES
                EXCLUDE_FROM_DEFAULT_BUILD ON
                FOLDER "Signing"
        )
    endif()
endfunction()
