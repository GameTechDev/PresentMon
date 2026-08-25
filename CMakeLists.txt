cmake_minimum_required(VERSION 4.2.2)

if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "PresentMon currently supports Windows only.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(PresentMonOptions)

if(DEFINED PMON_BUILD_SCOPE)
    string(TOUPPER "${PMON_BUILD_SCOPE}" _pmon_requested_build_scope)
    if(_pmon_requested_build_scope STREQUAL "REFLECTOR")
        message(
            FATAL_ERROR
            "Reflector is a separate CMake project. Configure from Tools/Reflector "
            "(see BUILDING.md). Example:\n"
            "  cmake -B build-reflector-vs2026-x64 -S Tools/Reflector "
            "-G \"Visual Studio 18 2026\" -A x64"
        )
    endif()
endif()

pmon_define_build_scope_cache()

include(PresentMonVisualStudio)
include(PresentMonVersion)
include(PresentMonVcpkg)
# The generator and its native toolset are settled before any dependency work
# so that compiled dependency directories can be qualified by toolset family.
pmon_configure_toolchain()
pmon_read_version("${CMAKE_CURRENT_SOURCE_DIR}/Version.props")
pmon_configure_vcpkg()

project(
    PresentMon
    VERSION "${PMON_VERSION}"
    DESCRIPTION "Intel PresentMon"
    LANGUAGES C CXX
)

pmon_validate_toolchain()

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
include(PresentMonPython)
include(PresentMonOutput)
include(PresentMonCompiler)
include(PresentMonMessageCompiler)
include(PresentMonUci)
include(PresentMonSigning)
include(PresentMonCef)
include(PresentMonShaders)
include(PresentMonUiPayload)
include(PresentMonAuxTestData)
include(PresentMonCTest)

pmon_configure_options()
pmon_configure_output_directories()
pmon_configure_python()
pmon_create_compiler_targets()
pmon_create_version_target()
pmon_create_version_resource_target()
pmon_configure_uci()
pmon_configure_signing()
pmon_prepare_dependencies()
if(PMON_ARCHITECTURE STREQUAL "x64" AND PMON_BUILD_UI)
    pmon_configure_cef()
else()
    set(
        PMON_CEF_EFFECTIVE_ROOT
        "Not applicable"
        CACHE INTERNAL
        "Resolved CEF stage"
        FORCE
    )
endif()
if(PMON_ARCHITECTURE STREQUAL "x64" AND PMON_BUILD_UI)
    pmon_configure_shaders()
    pmon_configure_ui_payload()
    pmon_configure_kernel_payload()
endif()

if(NOT PMON_BUILD_SCOPE STREQUAL "PROVIDER")
    add_subdirectory(IntelPresentMon/CommonUtilities)
    add_subdirectory(PresentData)
    include(IntelPresentMon/CommonUtilities/Metrics.cmake)
endif()

if(PMON_BUILD_CONSOLE)
    add_subdirectory(PresentMon)
endif()
if(PMON_BUILD_PROVIDER)
    add_subdirectory(Provider)
endif()
if(PMON_BUILD_TOOLS)
    if(PMON_ARCHITECTURE STREQUAL "x64")
        add_subdirectory(Tools/ETLTrimmer)
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

if(PMON_BUILD_TESTS)
    enable_testing()
    pmon_configure_aux_testdata()
    pmon_configure_presentmon_test_data_directory()
    if(PMON_ARCHITECTURE STREQUAL "x64")
        # IntelPresentMon/UnitTests and PresentMonAPI2Tests are x64-only (see Phase5-Tests.md).
    else()
        if(PMON_BUILD_UNIT_TESTS OR PMON_BUILD_API2_TESTS)
            message(
                STATUS
                "Intel unit/API2 test options are set but ${PMON_ARCHITECTURE} has no test targets; only PresentMonTests may apply."
            )
        endif()
    endif()
    if(PMON_BUILD_PRESENTMON_TESTS)
        add_subdirectory(Tests)
    endif()
    if(PMON_ARCHITECTURE STREQUAL "x64")
        if(PMON_BUILD_UNIT_TESTS)
            add_subdirectory(IntelPresentMon/UnitTests)
        endif()
        if(PMON_BUILD_API2_TESTS)
            add_subdirectory(IntelPresentMon/PresentMonAPI2Tests)
        endif()
    endif()
    pmon_register_all_ctests()
endif()

pmon_add_production_signing_targets()
pmon_add_win32_component_signing_targets()
include(PresentMonPackaging)
pmon_register_packaging_targets()

message(STATUS "")
message(STATUS "PresentMon configuration")
message(STATUS "  Version:            ${PMON_VERSION}")
message(STATUS "  File version:       ${PMON_FILE_VERSION}")
message(STATUS "  Generator:          ${CMAKE_GENERATOR}")
message(STATUS "  Platform toolset:   ${PMON_VS_TOOLSET}")
message(STATUS "  VS instance:        ${PMON_VS_INSTANCE_ROOT}")
message(STATUS "  Architecture:       ${PMON_ARCHITECTURE}")
message(STATUS "  Build scope:        ${PMON_BUILD_SCOPE}")
message(STATUS "  Deployment profile: ${PMON_DEPLOYMENT_PROFILE}")
message(STATUS "  Output root:        ${PMON_OUTPUT_ROOT}")
message(STATUS "  Dependency root:    ${PMON_DEPENDENCY_ROOT}")
message(STATUS "  Python:             ${PMON_PYTHON_EXECUTABLE}")
message(STATUS "  UCI mode:           ${PMON_ENABLE_UCI}")
message(STATUS "  UCI included:       ${PMON_UCI_ENABLED}")
message(STATUS "  Signing backend:    ${PMON_SELECTED_SIGNING_BACKEND}")
message(STATUS "  CEF stage:          ${PMON_CEF_EFFECTIVE_ROOT}")
message(STATUS "  Build tests:        ${PMON_BUILD_TESTS}")
if(PMON_BUILD_TESTS)
    message(STATUS "  PresentMonTests:    ${PMON_BUILD_PRESENTMON_TESTS}")
    message(STATUS "  Unit tests:         ${PMON_BUILD_UNIT_TESTS}")
    message(STATUS "  API2 tests:         ${PMON_BUILD_API2_TESTS}")
    message(STATUS "  CTest full CSV:     ${PMON_CTEST_FULL_CSV_TESTS}")
endif()
message(STATUS "")
message(STATUS "Configured enabled PresentMon targets.")
