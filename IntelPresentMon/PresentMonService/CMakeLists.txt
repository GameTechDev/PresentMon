# AllActions.h is a tracked, maintained artifact that aggregates acts/*.h, so it
# is regenerated in place rather than in the build tree. The script rewrites the
# file only when the aggregated set actually changed.
file(
    GLOB service_action_headers
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/acts/*.h"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/AllActions.h"
    COMMAND
        cmd /c "${CMAKE_CURRENT_SOURCE_DIR}/build-scripts/pre-build.bat"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/build-scripts/pre-build.bat"
        ${service_action_headers}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating AllActions.h"
    VERBATIM
)
add_custom_target(
    pmon_service_actions
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/AllActions.h"
)
set_property(TARGET pmon_service_actions PROPERTY FOLDER "Build")

# Header-only view of the service used by clients that speak the action
# protocol. It deliberately carries no objects so that linking a client does not
# pull service implementation code into the client binary, matching the MSBuild
# behaviour where a reference to an Application project contributes headers and
# build ordering only.
add_library(pmon_service_headers INTERFACE)
add_library(pmon::service_headers ALIAS pmon_service_headers)
add_dependencies(pmon_service_headers pmon_service_actions)
target_include_directories(
    pmon_service_headers
    INTERFACE
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/IntelPresentMon"
)
set_property(TARGET pmon_service_headers PROPERTY FOLDER "Service and SDK")

add_library(
    pmon_service_core
    STATIC
        ActionExecutionContext.cpp
        ActionServer.cpp
        EtwLogger.cpp
        EtwLogSession.cpp
        LogSetup.cpp
        MockPresentMonSession.cpp
        PMMainThread.cpp
        PresentMon.cpp
        PresentMonSession.cpp
        RealtimePresentMonSession.cpp
        Service.cpp
        testing/TestControl.cpp
)
add_library(pmon::service_core ALIAS pmon_service_core)
add_dependencies(pmon_service_core pmon_service_actions)

target_link_libraries(
    pmon_service_core
    PUBLIC
        pmon::service_headers
        pmon::common_utilities
        pmon::present_data
        pmon::control
        pmon::interprocess
        pmon::api_wrapper_common
        pmon::versioning
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        pmon::version_resource
        pdh
        shlwapi
        tdh
)
target_compile_options(
    pmon_service_core
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    pmon_service_core
    INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    pmon_service_core
    PROPERTIES
        OUTPUT_NAME PresentMonServiceCore
        ARCHIVE_OUTPUT_DIRECTORY "${PMON_OUTPUT_ROOT}/obj/$<CONFIG>"
        FOLDER "Service and SDK"
)

add_executable(
    PresentMonService
        ActionRegistration.cpp
        ServiceMain.cpp
        PresentMonService.rc
)
add_dependencies(PresentMonService pmon_service_actions)
target_link_libraries(
    PresentMonService
    PRIVATE
        pmon::service_core
        pmon::build_options
        pmon::warnings_default
        pmon::version_resource
        pmon::release_link_policy
)
target_compile_options(
    PresentMonService
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    PresentMonService
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    PresentMonService
    PROPERTIES
        FOLDER "Service and SDK"
)

# The service reads this manifest from its own directory at runtime.
add_custom_command(
    TARGET PresentMonService
    POST_BUILD
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/PresentMon/ddETWExternal.xml"
        "$<TARGET_FILE_DIR:PresentMonService>/ddETWExternal.xml"
    COMMENT "Staging ddETWExternal.xml"
    VERBATIM
)

if(PMON_UCI_ENABLED)
    add_custom_command(
        TARGET PresentMonService
        POST_BUILD
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${PMON_UCI_RUNTIME_DLL}"
            "$<TARGET_FILE_DIR:PresentMonService>/"
        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${PMON_UCI_EFFECTIVE_ROOT}/collectors"
            "$<TARGET_FILE_DIR:PresentMonService>/collectors"
        COMMENT "Staging UCI runtime payload"
        VERBATIM
    )
endif()
