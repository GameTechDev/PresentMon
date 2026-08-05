cmake_minimum_required(VERSION 3.25)

if(NOT WIN32)
    message(FATAL_ERROR "PresentMon currently supports Windows only.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(PresentMonVersion)
include(PresentMonVcpkg)
pmon_read_version("${CMAKE_CURRENT_SOURCE_DIR}/Version.props")
pmon_configure_vcpkg()

project(
    PresentMon
    VERSION "${PMON_VERSION}"
    DESCRIPTION "Intel PresentMon"
    LANGUAGES C CXX
)

if(NOT MSVC)
    message(FATAL_ERROR "PresentMon currently requires the Microsoft Visual C++ toolchain.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_CONFIGURATION_TYPES)
    set(
        CMAKE_CONFIGURATION_TYPES
        "Debug;Release"
        CACHE STRING
        "Supported build configurations"
        FORCE
    )
endif()

include(PresentMonOptions)
include(PresentMonOutput)
include(PresentMonCompiler)
include(PresentMonMessageCompiler)
include(PresentMonUci)
include(PresentMonSigning)
include(PresentMonCef)
include(PresentMonShaders)
include(PresentMonUiPayload)

pmon_configure_options()
pmon_configure_output_directories()
pmon_create_compiler_targets()
pmon_create_version_target()
pmon_create_version_resource_target()
pmon_configure_uci()
pmon_configure_signing()
if(PMON_ARCHITECTURE STREQUAL "x64")
    pmon_configure_cef()
else()
    set(PMON_CEF_EFFECTIVE_ROOT "Not applicable")
endif()
if(PMON_ARCHITECTURE STREQUAL "x64" AND PMON_BUILD_UI)
    pmon_configure_shaders()
    pmon_configure_ui_payload()
    pmon_configure_kernel_payload()
endif()

add_subdirectory(IntelPresentMon/CommonUtilities)
add_subdirectory(PresentData)
include(IntelPresentMon/CommonUtilities/Metrics.cmake)

if(PMON_BUILD_CONSOLE)
    add_subdirectory(PresentMon)
endif()
if(PMON_BUILD_PROVIDER)
    add_subdirectory(Provider)
endif()
if(PMON_BUILD_TOOLS)
    if(PMON_ARCHITECTURE STREQUAL "x64")
        add_subdirectory(ETLTrimmer)
    endif()
    add_subdirectory(Tools/etw_list)
    add_subdirectory(Tools/pm_convert_csv)
endif()

# The service, SDK, and UI stacks have no maintained Win32 configuration.
if(PMON_ARCHITECTURE STREQUAL "x64")
    if(PMON_BUILD_SERVICE OR PMON_BUILD_SDK OR PMON_BUILD_UI)
        add_subdirectory(IntelPresentMon/Versioning)
        add_subdirectory(IntelPresentMon/Interprocess)
        add_subdirectory(IntelPresentMon/ControlLib)
        add_subdirectory(IntelPresentMon/PresentMonAPIWrapperCommon)
        add_subdirectory(IntelPresentMon/PresentMonAPIWrapper)
        add_subdirectory(IntelPresentMon/PresentMonService)
        add_subdirectory(IntelPresentMon/PresentMonMiddleware)
        add_subdirectory(IntelPresentMon/PresentMonAPI2)
        add_subdirectory(IntelPresentMon/PresentMonAPI2Loader)
    endif()
    if(PMON_BUILD_SDK)
        add_subdirectory(IntelPresentMon/SampleClient)
    endif()
    if(PMON_BUILD_UI)
        add_subdirectory(IntelPresentMon/Core)
        add_subdirectory(IntelPresentMon/AppCef)
        add_subdirectory(IntelPresentMon/KernelProcess)
    endif()
endif()

message(STATUS "")
message(STATUS "PresentMon configuration")
message(STATUS "  Version:            ${PMON_VERSION}")
message(STATUS "  File version:       ${PMON_FILE_VERSION}")
message(STATUS "  Generator:          ${CMAKE_GENERATOR}")
message(STATUS "  Architecture:       ${CMAKE_GENERATOR_PLATFORM}")
message(STATUS "  Deployment profile: ${PMON_DEPLOYMENT_PROFILE}")
message(STATUS "  Output root:        ${PMON_OUTPUT_ROOT}")
message(STATUS "  UCI mode:           ${PMON_ENABLE_UCI}")
message(STATUS "  UCI included:       ${PMON_UCI_ENABLED}")
message(STATUS "  Signing backend:    ${PMON_SELECTED_SIGNING_BACKEND}")
message(STATUS "  CEF stage:          ${PMON_CEF_EFFECTIVE_ROOT}")
message(STATUS "")
message(STATUS "Configured enabled PresentMon targets.")
