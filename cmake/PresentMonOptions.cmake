include_guard(GLOBAL)

include(CMakeDependentOption)

function(pmon_define_enum_cache variable default_value description)
    set(allowed_values ${ARGN})

    if(NOT DEFINED ${variable})
        set(${variable} "${default_value}")
    endif()

    string(TOUPPER "${${variable}}" normalized_value)
    if(NOT normalized_value IN_LIST allowed_values)
        list(JOIN allowed_values ", " allowed_values_text)
        message(
            FATAL_ERROR
            "${variable} must be one of: ${allowed_values_text}. "
            "Received: ${${variable}}"
        )
    endif()

    set(${variable} "${normalized_value}" CACHE STRING "${description}" FORCE)
    set_property(CACHE ${variable} PROPERTY STRINGS ${allowed_values})
endfunction()

function(pmon_define_build_scope_cache)
    pmon_define_enum_cache(
        PMON_BUILD_SCOPE
        FULL
        "PresentMon component set"
        FULL
        CONSOLE
        PROVIDER
    )
endfunction()

function(pmon_configure_options)
    pmon_define_build_scope_cache()
    pmon_define_enum_cache(
        PMON_DEPLOYMENT_PROFILE
        DEVELOPER
        "PresentMon deployment policy"
        DEVELOPER
        PRODUCTION
    )
    pmon_define_enum_cache(
        PMON_ENABLE_UCI
        AUTO
        "UCI integration mode"
        AUTO
        ON
        OFF
    )

    if(PMON_BUILD_SCOPE STREQUAL "FULL")
        option(PMON_BUILD_CONSOLE "Build the standalone PresentMon console" ON)
        option(PMON_BUILD_SERVICE "Build the PresentMon service" ON)
        option(PMON_BUILD_SDK "Build the PresentMon SDK and API libraries" ON)
        option(PMON_BUILD_UI "Build the Intel PresentMon UI" ON)
        option(PMON_BUILD_PROVIDER "Build the ETW provider" ON)
        option(PMON_BUILD_TOOLS "Build standalone tools" ON)

        set(build_tests_default ON)
        if(PMON_DEPLOYMENT_PROFILE STREQUAL "PRODUCTION")
            set(build_tests_default OFF)
        endif()
        option(
            PMON_BUILD_TESTS
            "Build test projects and register CTest entries"
            ${build_tests_default}
        )
    elseif(PMON_BUILD_SCOPE STREQUAL "CONSOLE")
        set(PMON_BUILD_CONSOLE ON CACHE BOOL "Build the standalone PresentMon console" FORCE)
        set(PMON_BUILD_SERVICE OFF CACHE BOOL "Build the PresentMon service" FORCE)
        set(PMON_BUILD_SDK OFF CACHE BOOL "Build the PresentMon SDK and API libraries" FORCE)
        set(PMON_BUILD_UI OFF CACHE BOOL "Build the Intel PresentMon UI" FORCE)
        set(PMON_BUILD_PROVIDER OFF CACHE BOOL "Build the ETW provider" FORCE)
        set(PMON_BUILD_TOOLS OFF CACHE BOOL "Build standalone tools" FORCE)
        option(PMON_BUILD_TESTS "Build test projects and register CTest entries" OFF)
    elseif(PMON_BUILD_SCOPE STREQUAL "PROVIDER")
        set(PMON_BUILD_CONSOLE OFF CACHE BOOL "Build the standalone PresentMon console" FORCE)
        set(PMON_BUILD_SERVICE OFF CACHE BOOL "Build the PresentMon service" FORCE)
        set(PMON_BUILD_SDK OFF CACHE BOOL "Build the PresentMon SDK and API libraries" FORCE)
        set(PMON_BUILD_UI OFF CACHE BOOL "Build the Intel PresentMon UI" FORCE)
        set(PMON_BUILD_PROVIDER ON CACHE BOOL "Build the ETW provider" FORCE)
        set(PMON_BUILD_TOOLS OFF CACHE BOOL "Build standalone tools" FORCE)
        set(PMON_BUILD_TESTS OFF CACHE BOOL "Build test projects and register CTest entries" FORCE)
    endif()
    option(
        PMON_AUTO_PREPARE_DEPENDENCIES
        "Restore and validate CEF, web, and auxiliary test-data inputs during configure"
        ON
    )
    option(
        PMON_CTEST_FULL_CSV_TESTS
        "Register CTest entry for the Tests/Full gold CSV suite"
        OFF
    )

    cmake_dependent_option(
        PMON_BUILD_PRESENTMON_TESTS
        "Build PresentMonTests (GTest console regression)"
        ON
        "PMON_BUILD_TESTS;PMON_BUILD_CONSOLE"
        OFF
    )
    cmake_dependent_option(
        PMON_BUILD_UNIT_TESTS
        "Build PresentMonUnitTests (Visual Studio C++ unit tests; requires Core/UI stack, x64 only)"
        ON
        "PMON_BUILD_TESTS;PMON_BUILD_SDK;PMON_BUILD_UI"
        OFF
    )
    cmake_dependent_option(
        PMON_BUILD_API2_TESTS
        "Build PresentMonAPI2Tests (Visual Studio C++ integration tests; x64 only)"
        ON
        "PMON_BUILD_TESTS;PMON_BUILD_SERVICE;PMON_BUILD_SDK;PMON_BUILD_UI"
        OFF
    )

    set(
        PMON_UCI_ROOT
        ""
        CACHE PATH
        "UCI SDK root; PMON_UCI_SDK_DIR is used when this is empty"
    )
    set(
        PMON_CEF_SOURCE
        ""
        CACHE STRING
        "Optional locked CEF archive or URI used during automatic dependency preparation"
    )
    set(
        PMON_EDSS_SIGN_SCRIPT
        ""
        CACHE FILEPATH
        "External EDSS signing script (PRODUCTION: set PMON_EDSS_SIGN_SCRIPT env or -D before configure)"
    )
    option(
        PMON_EDSS_IN_PROCESS_SIGNING
        "Pass -InProcessSigning to PMON_EDSS_SIGN_SCRIPT (SignTool as the current user; default). When OFF, pass -ImpersonatedSigning."
        ON
    )
    cmake_dependent_option(
        PMON_PACKAGING_MSI_EDSS_SIGN_SKIP
        "Skip EDSS invoke on pmon_sign_production_installer; run MSI Authenticode and embedded PE verification only"
        OFF
        "PMON_DEPLOYMENT_PROFILE STREQUAL PRODUCTION"
        OFF
    )
    set(
        PMON_SIGNTOOL_EXECUTABLE
        ""
        CACHE FILEPATH
        "Path to SignTool for the developer My-store backend"
    )
    set(
        PMON_SIGNTOOL_CERTIFICATE_STORE
        "PrivateCertStore"
        CACHE STRING
        "Certificate store used by the direct SignTool backend (SignTool /s PrivateCertStore; legacy MSBuild parity)"
    )
    set(
        PMON_SIGNTOOL_CERTIFICATE_NAME
        "Test Certificate - For Internal Use Only"
        CACHE STRING
        "Certificate subject used by the direct SignTool backend"
    )
    set(
        PMON_SIGNTOOL_TIMESTAMP_URL
        "http://timestamp.comodoca.com/authenticode"
        CACHE STRING
        "Timestamp server URL for the developer SignTool post-build step"
    )

    foreach(option_name IN ITEMS
        PMON_BUILD_SCOPE
        PMON_DEPLOYMENT_PROFILE
        PMON_ENABLE_UCI
        PMON_BUILD_CONSOLE
        PMON_BUILD_SERVICE
        PMON_BUILD_SDK
        PMON_BUILD_UI
        PMON_BUILD_PROVIDER
        PMON_BUILD_TOOLS
        PMON_BUILD_TESTS
        PMON_BUILD_PRESENTMON_TESTS
        PMON_BUILD_UNIT_TESTS
        PMON_BUILD_API2_TESTS
    )
        set(${option_name} "${${option_name}}" PARENT_SCOPE)
    endforeach()
endfunction()
