# Script mode verification for the explicit pmon_verify_cef target.
#
# This check is deliberately offline. It validates the fixed CMake stage,
# exercises manifest-owned stale runtime removal, and proves that an unrelated
# file in the shared output directory is not removed.

foreach(
    required_variable
    IN ITEMS
        PMON_CEF_STAGE
        PMON_CEF_LOCK
        PMON_CEF_DESTINATION
        PMON_CEF_OWNERSHIP_MANIFEST
        PMON_CEF_VALIDATE_SCRIPT
        PMON_CEF_STAGE_SCRIPT
)
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
    RESULT_VARIABLE stage_validate_result
)
if(NOT stage_validate_result EQUAL 0)
    message(FATAL_ERROR "The fixed CMake CEF stage failed verification.")
endif()

if(NOT EXISTS "${PMON_CEF_OWNERSHIP_MANIFEST}")
    message(FATAL_ERROR "CEF runtime ownership manifest not found: ${PMON_CEF_OWNERSHIP_MANIFEST}")
endif()

set(stale_relative_path ".pmon-cef-owned-stale-verification")
set(unrelated_relative_path ".pmon-cef-unrelated-verification")
set(stale_file "${PMON_CEF_DESTINATION}/${stale_relative_path}")
set(unrelated_file "${PMON_CEF_DESTINATION}/${unrelated_relative_path}")
set(stale_contents "owned stale CEF verification file\n")
set(unrelated_contents "unrelated CEF verification file\n")

foreach(sentinel IN ITEMS "${stale_file}" "${unrelated_file}")
    if(EXISTS "${sentinel}" OR IS_SYMLINK "${sentinel}")
        message(FATAL_ERROR "Refusing to overwrite a preexisting verification sentinel: ${sentinel}")
    endif()
endforeach()

file(READ "${PMON_CEF_OWNERSHIP_MANIFEST}" original_manifest)
file(WRITE "${stale_file}" "${stale_contents}")
file(WRITE "${unrelated_file}" "${unrelated_contents}")
file(APPEND "${PMON_CEF_OWNERSHIP_MANIFEST}" "${stale_relative_path}\n")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        -D "PMON_CEF_STAGE=${PMON_CEF_STAGE}"
        -D "PMON_CEF_LOCK=${PMON_CEF_LOCK}"
        -D "PMON_CEF_DESTINATION=${PMON_CEF_DESTINATION}"
        -D "PMON_CEF_OWNERSHIP_MANIFEST=${PMON_CEF_OWNERSHIP_MANIFEST}"
        -D "PMON_CEF_VALIDATE_SCRIPT=${PMON_CEF_VALIDATE_SCRIPT}"
        -P "${PMON_CEF_STAGE_SCRIPT}"
    RESULT_VARIABLE staging_result
)

if(NOT staging_result EQUAL 0)
    file(WRITE "${PMON_CEF_OWNERSHIP_MANIFEST}" "${original_manifest}")
    file(REMOVE "${stale_file}" "${unrelated_file}")
    message(FATAL_ERROR "CEF runtime staging failed during ownership verification.")
endif()

set(verification_error)
if(EXISTS "${stale_file}" OR IS_SYMLINK "${stale_file}")
    set(verification_error "Manifest-owned stale CEF runtime file was not removed: ${stale_file}")
elseif(NOT EXISTS "${unrelated_file}")
    set(verification_error "Unrelated output file was removed during CEF runtime staging: ${unrelated_file}")
else()
    file(READ "${unrelated_file}" actual_unrelated_contents)
    if(NOT "${actual_unrelated_contents}" STREQUAL "${unrelated_contents}")
        set(verification_error "Unrelated output file was modified during CEF runtime staging: ${unrelated_file}")
    endif()
endif()

file(STRINGS "${PMON_CEF_OWNERSHIP_MANIFEST}" verified_manifest_paths ENCODING UTF-8)
list(FIND verified_manifest_paths "${stale_relative_path}" stale_manifest_index)
if(NOT stale_manifest_index EQUAL -1)
    set(verification_error "Stale verification path remained in the CEF ownership manifest.")
endif()

file(REMOVE "${stale_file}" "${unrelated_file}")
if(verification_error)
    file(WRITE "${PMON_CEF_OWNERSHIP_MANIFEST}" "${original_manifest}")
    message(FATAL_ERROR "${verification_error}")
endif()

message(STATUS "CEF fixed-stage and runtime ownership verification passed.")
