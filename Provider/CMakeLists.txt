pmon_compile_message_manifest(
    provider_manifest_resource
    "${CMAKE_CURRENT_SOURCE_DIR}/Intel-PresentMon.man"
    "Intel-PresentMon"
)

add_library(
    pmon_provider
    SHARED
        dllmain.cpp
        Version.rc
        "${provider_manifest_resource}"
)
target_link_libraries(
    pmon_provider
    PRIVATE
        pmon::build_options
        pmon::warnings_strict
        pmon::warnings_strict_link
        pmon::release_link_policy
        pmon::version_resource
)
target_compile_definitions(
    pmon_provider
    PRIVATE
        _WINDOWS
        _USRDLL
        $<$<STREQUAL:${PMON_ARCHITECTURE},Win32>:ISX86>
)
target_compile_options(
    pmon_provider
    PRIVATE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)
target_link_options(
    pmon_provider
    PRIVATE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)

if(PMON_ARCHITECTURE STREQUAL "Win32")
    set(provider_output_name "Intel-PresentMon32")
else()
    set(provider_output_name "Intel-PresentMon")
endif()
set_target_properties(
    pmon_provider
    PROPERTIES
        OUTPUT_NAME "${provider_output_name}"
        FOLDER "PresentMon/Provider"
)
