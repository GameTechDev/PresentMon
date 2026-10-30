include_guard(GLOBAL)

# Optional PresentBench2 vendor SDK fetch. Default builds are unaffected.
# Enable with:
#   -DPMON_PRESENTBENCH2_FETCH_XESS_FG=ON
#
# Public pins below are used when URL cache vars are empty. Override via
# -DPMON_PRESENTBENCH2_XESS_FG_URL=... and optional *_SHA256=... for internal pins.

function(pmon_pb2_find_sdk_root out_var extract_dir marker_relpath)
    if(EXISTS "${extract_dir}/${marker_relpath}")
        set(${out_var} "${extract_dir}" PARENT_SCOPE)
        return()
    endif()
    file(
        GLOB _pb2_children
        LIST_DIRECTORIES true
        "${extract_dir}/*"
    )
    foreach(_pb2_child IN LISTS _pb2_children)
        if(IS_DIRECTORY "${_pb2_child}" AND EXISTS "${_pb2_child}/${marker_relpath}")
            set(${out_var} "${_pb2_child}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(pmon_pb2_collect_runtime_dlls out_var search_root name_globs)
    set(_pb2_found "")
    foreach(_pb2_glob IN LISTS name_globs)
        file(
            GLOB_RECURSE _pb2_hits
            "${search_root}/${_pb2_glob}"
        )
        list(APPEND _pb2_found ${_pb2_hits})
    endforeach()
    set(_pb2_filtered "")
    foreach(_pb2_dll IN LISTS _pb2_found)
        string(TOLOWER "${_pb2_dll}" _pb2_dll_l)
        if(_pb2_dll_l MATCHES "aarch64|arm64ec|/arm64/|\\\\arm64\\\\")
            continue()
        endif()
        list(APPEND _pb2_filtered "${_pb2_dll}")
    endforeach()
    if(_pb2_filtered STREQUAL "")
        set(_pb2_filtered "${_pb2_found}")
    endif()
    set(${out_var} "${_pb2_filtered}" PARENT_SCOPE)
endfunction()

function(pmon_configure_presentbench2_sdks)
    set(
        _pb2_xess_default_url
        "https://github.com/intel/xess/releases/download/v3.0.2/XeSS_SDK_3.0.2.zip"
    )
    set(
        _pb2_xess_default_sha256
        "88B8A373F30E33F3558A77A93E634F11B8132FC3047EA1A8EDEEAD32B8471990"
    )
    set(
        PMON_PRESENTBENCH2_XESS_FG_URL
        "${_pb2_xess_default_url}"
        CACHE STRING
        "Pinned XeSS / XeSS-FG SDK archive URI (used when FETCH_XESS_FG=ON)"
    )
    set(
        PMON_PRESENTBENCH2_XESS_FG_SHA256
        "${_pb2_xess_default_sha256}"
        CACHE STRING
        "Expected SHA256 for XeSS-FG SDK archive"
    )
    set(PMON_PRESENTBENCH2_HAS_XESS_FG OFF PARENT_SCOPE)
    set(PMON_PRESENTBENCH2_XESS_FG_ROOT "" PARENT_SCOPE)
    set(PMON_PRESENTBENCH2_XESS_FG_RUNTIME_DLLS "" PARENT_SCOPE)

    if(PMON_PRESENTBENCH2_FETCH_XESS_FG)
        if(PMON_PRESENTBENCH2_XESS_FG_URL STREQUAL "")
            message(
                FATAL_ERROR
                "PMON_PRESENTBENCH2_FETCH_XESS_FG=ON requires PMON_PRESENTBENCH2_XESS_FG_URL"
            )
        endif()
        string(SHA256 _pb2_url_hash "${PMON_PRESENTBENCH2_XESS_FG_URL}")
        string(SUBSTRING "${_pb2_url_hash}" 0 16 _pb2_url_key)
        set(_pb2_xess_dir "${PMON_DEPENDENCY_ROOT}/presentbench2/xess-fg-${_pb2_url_key}")
        file(MAKE_DIRECTORY "${_pb2_xess_dir}")
        set(_pb2_xess_archive "${_pb2_xess_dir}/xess-fg.sdk")
        if(NOT EXISTS "${_pb2_xess_archive}")
            message(STATUS "Fetching XeSS-FG SDK for PresentBench2...")
            file(
                DOWNLOAD
                "${PMON_PRESENTBENCH2_XESS_FG_URL}"
                "${_pb2_xess_archive}"
                SHOW_PROGRESS
                STATUS _pb2_xess_status
            )
            list(GET _pb2_xess_status 0 _pb2_xess_code)
            if(NOT _pb2_xess_code EQUAL 0)
                message(FATAL_ERROR "XeSS-FG SDK download failed: ${_pb2_xess_status}")
            endif()
        endif()
        if(NOT PMON_PRESENTBENCH2_XESS_FG_SHA256 STREQUAL "")
            file(SHA256 "${_pb2_xess_archive}" _pb2_xess_hash)
            string(TOUPPER "${_pb2_xess_hash}" _pb2_xess_hash)
            string(TOUPPER "${PMON_PRESENTBENCH2_XESS_FG_SHA256}" _pb2_xess_expected)
            if(NOT _pb2_xess_hash STREQUAL _pb2_xess_expected)
                message(
                    FATAL_ERROR
                    "XeSS-FG SDK hash mismatch.\n"
                    "  expected: ${PMON_PRESENTBENCH2_XESS_FG_SHA256}\n"
                    "  actual:   ${_pb2_xess_hash}"
                )
            endif()
        endif()
        set(_pb2_xess_extract "${_pb2_xess_dir}/sdk")
        if(NOT EXISTS "${_pb2_xess_extract}")
            file(ARCHIVE_EXTRACT INPUT "${_pb2_xess_archive}" DESTINATION "${_pb2_xess_extract}")
        endif()
        pmon_pb2_find_sdk_root(_pb2_xess_root "${_pb2_xess_extract}" "inc/xess_fg/xefg_swapchain.h")
        if(_pb2_xess_root STREQUAL "")
            message(FATAL_ERROR "XeSS SDK extract missing inc/xess_fg under ${_pb2_xess_extract}")
        endif()
        pmon_pb2_collect_runtime_dlls(
            _pb2_xess_dlls
            "${_pb2_xess_root}"
            "libxess_fg.dll;libxell.dll"
        )
        set(PMON_PRESENTBENCH2_HAS_XESS_FG ON PARENT_SCOPE)
        set(PMON_PRESENTBENCH2_XESS_FG_ROOT "${_pb2_xess_root}" PARENT_SCOPE)
        set(PMON_PRESENTBENCH2_XESS_FG_RUNTIME_DLLS "${_pb2_xess_dlls}" PARENT_SCOPE)
        message(STATUS "PresentBench2 XeSS-FG SDK: ${_pb2_xess_root}")
    endif()
endfunction()
