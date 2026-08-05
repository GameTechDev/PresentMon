include_guard(GLOBAL)



set(PMON_SIGNING_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")



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

    elseif(PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER")

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

            "${CMAKE_COMMAND}"

            -DPMON_BUILD_CONFIG=$<CONFIG>

            -DPMON_TARGET_FILE=$<TARGET_FILE:${target}>

            -DPMON_SIGNTOOL_EXECUTABLE=${PMON_SIGNTOOL_EXECUTABLE}

            -DPMON_SIGNTOOL_CERTIFICATE_STORE=${PMON_SIGNTOOL_CERTIFICATE_STORE}

            -DPMON_SIGNTOOL_CERTIFICATE_NAME=${PMON_SIGNTOOL_CERTIFICATE_NAME}

            -DPMON_SIGNTOOL_TIMESTAMP_URL=${PMON_SIGNTOOL_TIMESTAMP_URL}

            -P "${PMON_SIGNING_CMAKE_DIR}/pmon_kernel_process_developer_sign.cmake"

        COMMENT "Sign-sign Release for local dev"

        VERBATIM

    )

endfunction()



function(pmon_add_production_signing_targets)

    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")

        return()

    endif()

    # Run after Release payload is built (build/Release under PMON_OUTPUT_ROOT).

    add_custom_target(

        pmon_sign_production_payload

        COMMAND

            powershell

            -NoProfile

            -ExecutionPolicy

            Bypass

            -File

            "${PMON_EDSS_SIGN_SCRIPT}"

            -OutputRoot

            "${PMON_OUTPUT_ROOT}/Release"

            -Verify

        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"

        COMMENT "Signing the production payload through EDSS"

        USES_TERMINAL

    )

endfunction()



function(pmon_add_installer_scaffolding)

    if(NOT PMON_BUILD_UI)

        return()

    endif()

    if(NOT PMON_DEPLOYMENT_PROFILE STREQUAL "DEVELOPER")

        return()

    endif()



    set(installer_project "${CMAKE_SOURCE_DIR}/IntelPresentMon/PMInstaller/PMInstaller.wixproj")

    add_custom_target(

        pmon_build_installer

        COMMAND

            "${CMAKE_COMMAND}" -E echo

            "WiX MSI packaging is Phase 6. Until CMake integration lands, build ${installer_project} with MSBuild (Release-EDSS-MSI) after signing the payload."

        COMMENT "WiX installer scaffolding (Phase 6)"

        VERBATIM

    )

endfunction()

