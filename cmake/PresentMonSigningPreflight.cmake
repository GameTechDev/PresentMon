# Script invoked via cmake -P (not include()).

if(NOT DEFINED PMON_BUILD_CONFIG OR "${PMON_BUILD_CONFIG}" STREQUAL "")
    message(FATAL_ERROR "Signing preflight requires PMON_BUILD_CONFIG (pass Release).")
endif()

if(NOT PMON_BUILD_CONFIG STREQUAL "Release")
    message(
        FATAL_ERROR
        "PresentMon signing targets are Release-only. "
        "Requested configuration: ${PMON_BUILD_CONFIG}. "
        "Rebuild with --config Release."
    )
endif()
