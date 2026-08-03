# Script mode helper for the pmon_validate_cef target.
#
# Validates a restored CEF stage against IntelPresentMon/AppCef/cef-lock.json.
# It never downloads anything; a missing or damaged stage is reported with the
# exact restore command to run.

foreach(required_variable IN ITEMS PMON_CEF_VALIDATE_SCRIPT PMON_CEF_RESTORE_COMMAND)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required.")
    endif()
endforeach()

execute_process(
    COMMAND
        powershell.exe
        -NoLogo
        -NoProfile
        -NonInteractive
        -ExecutionPolicy Bypass
        -File "${PMON_CEF_VALIDATE_SCRIPT}"
        -Mode Stage
        -StageKind CMake
    RESULT_VARIABLE validate_result
)

if(NOT validate_result EQUAL 0)
    message(
        FATAL_ERROR
        "The fixed CMake CEF stage is missing or does not match the repository "
        "CEF lock.\n"
        "Restore it with:\n"
        "  ${PMON_CEF_RESTORE_COMMAND}\n"
    )
endif()
