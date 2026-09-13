# Script invoked via cmake -P (not include()).

if(NOT DEFINED PMON_VERIFY_OUTPUT OR "${PMON_VERIFY_OUTPUT}" STREQUAL "")
    message(FATAL_ERROR "PresentMonPackagingVerifyOutput.cmake requires PMON_VERIFY_OUTPUT.")
endif()

if(NOT EXISTS "${PMON_VERIFY_OUTPUT}")
    message(
        FATAL_ERROR
        "Packaging step succeeded but the expected output is missing: ${PMON_VERIFY_OUTPUT}"
    )
endif()
