# Invoked from KernelProcess POST_BUILD (developer profile). Signs and verifies
# Release outputs only, matching KernelProcess.vcxproj Release post-build.
if(NOT PMON_BUILD_CONFIG STREQUAL "Release")
    return()
endif()

if(NOT PMON_SIGNTOOL_EXECUTABLE OR NOT EXISTS "${PMON_SIGNTOOL_EXECUTABLE}")
    message(FATAL_ERROR "SignTool was not found. Install the Windows SDK or set PMON_SIGNTOOL_EXECUTABLE.")
endif()

if(NOT PMON_TARGET_FILE OR NOT EXISTS "${PMON_TARGET_FILE}")
    message(FATAL_ERROR "KernelProcess output was not found: ${PMON_TARGET_FILE}")
endif()

set(sign_command
    "${PMON_SIGNTOOL_EXECUTABLE}"
    sign
    /a
    /v
    /s
    "${PMON_SIGNTOOL_CERTIFICATE_STORE}"
    /n
    "${PMON_SIGNTOOL_CERTIFICATE_NAME}"
    /t
    "${PMON_SIGNTOOL_TIMESTAMP_URL}"
    /fd
    sha1
    "${PMON_TARGET_FILE}"
)
execute_process(
    COMMAND ${sign_command}
    RESULT_VARIABLE sign_result
)
if(sign_result)
    message(
        FATAL_ERROR
        "KernelProcess SignTool sign failed with exit code ${sign_result}. "
        "Ensure the test certificate is installed (see BUILDING.md)."
    )
endif()

execute_process(
    COMMAND
        "${PMON_SIGNTOOL_EXECUTABLE}"
        verify
        /pa
        /v
        "${PMON_TARGET_FILE}"
    RESULT_VARIABLE verify_result
)
if(verify_result)
    message(
        FATAL_ERROR
        "KernelProcess SignTool verify failed with exit code ${verify_result}."
    )
endif()
