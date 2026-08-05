include_guard(GLOBAL)

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

function(pmon_configure_options)
    option(PMON_BUILD_CONSOLE "Build the standalone PresentMon console" ON)
    option(PMON_BUILD_SERVICE "Build the PresentMon service" ON)
    option(PMON_BUILD_SDK "Build the PresentMon SDK and API libraries" ON)
    option(PMON_BUILD_UI "Build the Intel PresentMon UI" ON)
    option(PMON_BUILD_PROVIDER "Build the ETW provider" ON)
    option(PMON_BUILD_TOOLS "Build standalone tools" ON)

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

    set(
        PMON_UCI_ROOT
        ""
        CACHE PATH
        "UCI SDK root; PMON_UCI_SDK_DIR is used when this is empty"
    )
    set(
        PMON_EDSS_SIGN_SCRIPT
        ""
        CACHE FILEPATH
        "External EDSS signing script (PRODUCTION: set PMON_EDSS_SIGN_SCRIPT env or -D before configure)"
    )
    set(
        PMON_SIGNTOOL_EXECUTABLE
        ""
        CACHE FILEPATH
        "Path to SignTool for the PrivateCertStore backend"
    )
    set(
        PMON_SIGNTOOL_CERTIFICATE_STORE
        "PrivateCertStore"
        CACHE STRING
        "Certificate store used by the direct SignTool backend"
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

    set(PMON_DEPLOYMENT_PROFILE "${PMON_DEPLOYMENT_PROFILE}" PARENT_SCOPE)
    set(PMON_ENABLE_UCI "${PMON_ENABLE_UCI}" PARENT_SCOPE)
endfunction()
