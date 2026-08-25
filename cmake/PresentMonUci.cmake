include_guard(GLOBAL)

function(pmon_configure_uci)
    set(uci_root "${PMON_UCI_ROOT}")
    if(NOT uci_root AND DEFINED ENV{PMON_UCI_SDK_DIR})
        set(uci_root "$ENV{PMON_UCI_SDK_DIR}")
    endif()
    if(NOT uci_root)
        set(uci_root "${PROJECT_SOURCE_DIR}/IntelPresentMon/ControlLib/uci/external")
    endif()
    cmake_path(ABSOLUTE_PATH uci_root NORMALIZE OUTPUT_VARIABLE normalized_uci_root)

    set(uci_required_files
        "${normalized_uci_root}/include/uci/uci.h"
        "${normalized_uci_root}/include/uci/uci-data-records.h"
        "${normalized_uci_root}/unified-collector-interface.dll"
    )

    set(uci_sdk_valid TRUE)
    foreach(required_file IN LISTS uci_required_files)
        if(NOT EXISTS "${required_file}")
            set(uci_sdk_valid FALSE)
        endif()
    endforeach()
    if(NOT IS_DIRECTORY "${normalized_uci_root}/collectors")
        set(uci_sdk_valid FALSE)
    else()
        file(
            GLOB_RECURSE
            uci_collector_files
            LIST_DIRECTORIES FALSE
            "${normalized_uci_root}/collectors/*"
        )
        if(NOT uci_collector_files)
            set(uci_sdk_valid FALSE)
        endif()
    endif()

    set(uci_enabled FALSE)
    if(PMON_ENABLE_UCI STREQUAL "ON")
        if(NOT uci_sdk_valid)
            message(
                FATAL_ERROR
                "PMON_ENABLE_UCI=ON requires a valid UCI SDK at "
                "${normalized_uci_root}. Expected include/uci/uci.h, "
                "include/uci/uci-data-records.h, and "
                "unified-collector-interface.dll, plus a non-empty collectors directory."
            )
        endif()
        set(uci_enabled TRUE)
    elseif(PMON_ENABLE_UCI STREQUAL "AUTO" AND uci_sdk_valid)
        set(uci_enabled TRUE)
    endif()

    add_library(pmon_uci_sdk INTERFACE)
    add_library(pmon::uci_sdk ALIAS pmon_uci_sdk)
    set_property(TARGET pmon_uci_sdk PROPERTY FOLDER "ThirdParty")

    if(uci_enabled)
        target_include_directories(
            pmon_uci_sdk
            INTERFACE
                "${normalized_uci_root}/include"
        )
        message(STATUS "UCI SDK enabled from ${normalized_uci_root}")
    elseif(PMON_ENABLE_UCI STREQUAL "AUTO")
        message(STATUS "UCI SDK not found; building without UCI support")
    else()
        message(STATUS "UCI support disabled")
    endif()

    set(PMON_UCI_EFFECTIVE_ROOT "${normalized_uci_root}" CACHE INTERNAL "Resolved UCI SDK root" FORCE)
    set(PMON_UCI_ENABLED "${uci_enabled}" CACHE INTERNAL "UCI inclusion state" FORCE)
    set(PMON_UCI_RUNTIME_DLL "${normalized_uci_root}/unified-collector-interface.dll" CACHE INTERNAL "")

    set(PMON_UCI_EFFECTIVE_ROOT "${normalized_uci_root}" PARENT_SCOPE)
    set(PMON_UCI_ENABLED "${uci_enabled}" PARENT_SCOPE)
endfunction()
