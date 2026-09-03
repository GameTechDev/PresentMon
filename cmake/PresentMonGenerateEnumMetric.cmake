# Script invoked via cmake -P (not include()). Runs the metrics awk program and captures stdout into the generated header.
# Used instead of Tools/generate/EnumMetric/generate.cmd so the output location
# follows PMON_OUTPUT_ROOT instead of being hard-coded relative to the script.

cmake_path(GET PMON_ENUM_METRIC_OUTPUT PARENT_PATH output_directory)
file(MAKE_DIRECTORY "${output_directory}")

execute_process(
    COMMAND
        "${PMON_AWK_EXECUTABLE}"
        -f "${PMON_AWK_PROGRAM}"
        "${PMON_METRICS_CSV}"
    OUTPUT_FILE "${PMON_ENUM_METRIC_OUTPUT}"
    RESULT_VARIABLE awk_result
    ERROR_VARIABLE awk_error
)

if(NOT awk_result EQUAL 0)
    file(REMOVE "${PMON_ENUM_METRIC_OUTPUT}")
    message(
        FATAL_ERROR
        "Failed to generate ${PMON_ENUM_METRIC_OUTPUT} (exit ${awk_result}): ${awk_error}"
    )
endif()
