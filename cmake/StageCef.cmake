# Script mode helper for the pmon_stage_cef_runtime target.
#
# Copies every CEF runtime file named by IntelPresentMon/AppCef/cef-lock.json
# from the restored stage into a build output directory, then validates the
# copied payload against the same lock. The ownership manifest records only
# files created by this command, allowing a later lock to remove its stale
# payload without touching unrelated files in the output directory.

foreach(
    required_variable
    IN ITEMS
        PMON_CEF_STAGE
        PMON_CEF_LOCK
        PMON_CEF_DESTINATION
        PMON_CEF_OWNERSHIP_MANIFEST
        PMON_CEF_VALIDATE_SCRIPT
)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required.")
    endif()
endforeach()

function(_pmon_normalize_cef_runtime_path input_path output_variable)
    set(path "${input_path}")
    string(REPLACE "\\" "/" path "${path}")

    cmake_path(IS_ABSOLUTE path is_absolute)
    if(
        NOT path
        OR is_absolute
        OR path MATCHES "(^|/)\\.\\.(/|$)"
        OR path MATCHES "^[A-Za-z]:"
        OR path MATCHES ";"
    )
        message(FATAL_ERROR "Unsafe relative path in the CEF runtime manifest: ${input_path}")
    endif()

    cmake_path(NORMAL_PATH path OUTPUT_VARIABLE normalized_path)
    if(normalized_path STREQUAL ".")
        message(FATAL_ERROR "Unsafe relative path in the CEF runtime manifest: ${input_path}")
    endif()

    set(${output_variable} "${normalized_path}" PARENT_SCOPE)
endfunction()

if(NOT EXISTS "${PMON_CEF_LOCK}")
    message(FATAL_ERROR "CEF lock file not found: ${PMON_CEF_LOCK}")
endif()

file(READ "${PMON_CEF_LOCK}" cef_lock_json)
string(JSON payload_count ERROR_VARIABLE json_error LENGTH "${cef_lock_json}" payload)
if(json_error)
    message(FATAL_ERROR "Failed to read the CEF lock payload: ${json_error}")
endif()
if(payload_count EQUAL 0)
    message(FATAL_ERROR "The CEF lock payload is empty: ${PMON_CEF_LOCK}")
endif()

math(EXPR last_payload_index "${payload_count} - 1")
set(current_output_paths)
set(current_output_keys)
set(current_source_files)
set(current_destination_files)
foreach(payload_index RANGE ${last_payload_index})
    string(JSON output_path GET "${cef_lock_json}" payload ${payload_index} path)
    string(JSON stage_path GET "${cef_lock_json}" payload ${payload_index} stagePath)
    _pmon_normalize_cef_runtime_path("${output_path}" output_path)
    _pmon_normalize_cef_runtime_path("${stage_path}" stage_path)

    string(TOLOWER "${output_path}" output_key)
    list(FIND current_output_keys "${output_key}" duplicate_output_index)
    if(NOT duplicate_output_index EQUAL -1)
        message(FATAL_ERROR "Duplicate output path in the CEF lock: ${output_path}")
    endif()
    list(APPEND current_output_paths "${output_path}")
    list(APPEND current_output_keys "${output_key}")

    set(source_file "${PMON_CEF_STAGE}/${stage_path}")
    if(NOT EXISTS "${source_file}" OR IS_DIRECTORY "${source_file}")
        message(
            FATAL_ERROR
            "Locked CEF file is missing from the stage: ${source_file}. "
            "Restore the CEF stage before staging the runtime payload."
        )
    endif()

    set(destination_file "${PMON_CEF_DESTINATION}/${output_path}")
    list(APPEND current_source_files "${source_file}")
    list(APPEND current_destination_files "${destination_file}")
endforeach()

set(previous_output_paths)
set(previous_output_keys)
if(EXISTS "${PMON_CEF_OWNERSHIP_MANIFEST}")
    file(STRINGS "${PMON_CEF_OWNERSHIP_MANIFEST}" previous_output_paths ENCODING UTF-8)
    foreach(previous_output_path IN LISTS previous_output_paths)
        if(previous_output_path STREQUAL "")
            continue()
        endif()

        _pmon_normalize_cef_runtime_path("${previous_output_path}" previous_output_path)
        string(TOLOWER "${previous_output_path}" previous_output_key)
        list(APPEND previous_output_keys "${previous_output_key}")
        list(FIND current_output_keys "${previous_output_key}" current_output_index)
        if(NOT current_output_index EQUAL -1)
            continue()
        endif()

        set(stale_file "${PMON_CEF_DESTINATION}/${previous_output_path}")
        if(IS_DIRECTORY "${stale_file}")
            message(FATAL_ERROR "Refusing to remove a directory recorded as a CEF file: ${stale_file}")
        endif()
        if(EXISTS "${stale_file}" OR IS_SYMLINK "${stale_file}")
            file(REMOVE "${stale_file}")
            if(EXISTS "${stale_file}" OR IS_SYMLINK "${stale_file}")
                message(FATAL_ERROR "Failed to remove stale CEF runtime file: ${stale_file}")
            endif()
            message(STATUS "Removed stale CEF runtime file: ${stale_file}")
        endif()
    endforeach()
endif()

# A stale file can block a new directory at the same path, so stale owned files
# must be gone before any current destination directories are created.
list(LENGTH current_output_paths current_output_count)
math(EXPR last_current_output_index "${current_output_count} - 1")
foreach(current_output_index RANGE ${last_current_output_index})
    list(GET current_output_paths ${current_output_index} output_path)
    list(GET current_output_keys ${current_output_index} output_key)
    list(GET current_source_files ${current_output_index} source_file)
    list(GET current_destination_files ${current_output_index} destination_file)

    # Handle the reverse transition, where an old owned path below this one
    # leaves an empty directory that now needs to become a file.
    if(IS_SYMLINK "${destination_file}")
        message(FATAL_ERROR "Refusing to replace a symlink with a CEF runtime file: ${destination_file}")
    endif()
    if(IS_DIRECTORY "${destination_file}")
        set(destination_was_owned_parent FALSE)
        foreach(previous_output_key IN LISTS previous_output_keys)
            string(FIND "${previous_output_key}" "${output_key}/" owned_parent_index)
            if(owned_parent_index EQUAL 0)
                set(destination_was_owned_parent TRUE)
                break()
            endif()
        endforeach()
        if(NOT destination_was_owned_parent)
            message(FATAL_ERROR "Refusing to replace an unrelated output directory: ${destination_file}")
        endif()

        file(GLOB_RECURSE remaining_files LIST_DIRECTORIES false "${destination_file}/*")
        if(remaining_files)
            message(FATAL_ERROR "Refusing to remove a nonempty output directory: ${destination_file}")
        endif()
        file(REMOVE_RECURSE "${destination_file}")
    endif()

    cmake_path(GET destination_file PARENT_PATH destination_directory)
    file(MAKE_DIRECTORY "${destination_directory}")
    file(COPY_FILE "${source_file}" "${destination_file}" ONLY_IF_DIFFERENT)
endforeach()

execute_process(
    COMMAND
        powershell.exe
        -NoLogo
        -NoProfile
        -NonInteractive
        -ExecutionPolicy Bypass
        -File "${PMON_CEF_VALIDATE_SCRIPT}"
        -Mode Output
        -OutputRoot "${PMON_CEF_DESTINATION}"
    RESULT_VARIABLE validate_result
)

if(NOT validate_result EQUAL 0)
    message(
        FATAL_ERROR
        "The CEF runtime payload in ${PMON_CEF_DESTINATION} does not match "
        "${PMON_CEF_LOCK}."
    )
endif()

list(SORT current_output_paths)
string(JOIN "\n" manifest_contents ${current_output_paths})
string(APPEND manifest_contents "\n")
cmake_path(GET PMON_CEF_OWNERSHIP_MANIFEST PARENT_PATH manifest_directory)
file(MAKE_DIRECTORY "${manifest_directory}")
set(manifest_temporary "${PMON_CEF_OWNERSHIP_MANIFEST}.tmp")
file(WRITE "${manifest_temporary}" "${manifest_contents}")
file(RENAME "${manifest_temporary}" "${PMON_CEF_OWNERSHIP_MANIFEST}")

# COPY_FILE leaves an unchanged output's timestamp alone. Touching all declared
# outputs after successful validation prevents an unchanged file from keeping
# the custom command perpetually older than a changed lock or helper script.
file(TOUCH_NOCREATE ${current_destination_files})

message(STATUS "Staged and validated ${payload_count} locked CEF files in ${PMON_CEF_DESTINATION}")
