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
include(PresentMonArtifacts)
include(PresentMonUci)
include(PresentMonSigning)

pmon_configure_options()
pmon_configure_output_directories()
pmon_create_compiler_targets()
pmon_configure_uci()
pmon_configure_signing()
pmon_add_artifact_verification_target()

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
message(STATUS "")
message(STATUS "Phase 1 configures build infrastructure only; product targets are added in later phases.")
