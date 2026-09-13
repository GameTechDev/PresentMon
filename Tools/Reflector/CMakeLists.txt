cmake_minimum_required(VERSION 4.2.2)

if(NOT WIN32)
    message(FATAL_ERROR "PresentMon Reflector supports Windows only.")
endif()

cmake_path(SET PMON_REPO_ROOT NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
list(APPEND CMAKE_MODULE_PATH "${PMON_REPO_ROOT}/cmake")

include(PresentMonVersion)
pmon_read_version("${PMON_REPO_ROOT}/Version.props")

project(
    PresentMonReflector
    VERSION "${PMON_VERSION}"
    DESCRIPTION "Intel PresentMon reflection code generator"
)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(
        FATAL_ERROR
        "PresentMon Reflector requires an x64 generator platform (pass -A x64)."
    )
endif()

find_program(
    PMON_DOTNET_EXECUTABLE
    dotnet
    DOC "The .NET SDK dotnet CLI"
)
if(NOT PMON_DOTNET_EXECUTABLE)
    message(
        FATAL_ERROR
        "The .NET SDK (dotnet) is required but was not found on PATH. "
        "Install the .NET 8 SDK and retry configure."
    )
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

set(PMON_REFLECTOR_PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/Reflector.csproj")

add_custom_target(
    pmon_reflector
    COMMAND
        "${PMON_DOTNET_EXECUTABLE}"
        build
        "${PMON_REFLECTOR_PROJECT}"
        -c
        $<IF:$<CONFIG:Debug>,Debug,Release>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Reflector (.NET)"
    VERBATIM
)
set_target_properties(pmon_reflector PROPERTIES FOLDER "Tools")

add_custom_target(
    pmon_reflector_codegen
    COMMAND
        "${PMON_DOTNET_EXECUTABLE}"
        run
        --project
        "${PMON_REFLECTOR_PROJECT}"
        -c
        $<IF:$<CONFIG:Debug>,Debug,Release>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Running Reflector code generation"
    VERBATIM
)
set_target_properties(pmon_reflector_codegen PROPERTIES FOLDER "Tools")

message(STATUS "")
message(STATUS "PresentMon Reflector configuration")
message(STATUS "  Version:    ${PMON_VERSION}")
message(STATUS "  Generator:  ${CMAKE_GENERATOR}")
message(STATUS "  .NET SDK:   ${PMON_DOTNET_EXECUTABLE}")
message(STATUS "")
message(STATUS "Targets: pmon_reflector, pmon_reflector_codegen")
