pmon_compile_message_manifest(
    provider_manifest_resource
    "${CMAKE_CURRENT_SOURCE_DIR}/Intel-PresentMon.man"
    "Intel-PresentMon"
)

string(REPLACE "." "," pmon_file_version_numbers "${PMON_FILE_VERSION}")
string(REPLACE "." "," pmon_product_version_numbers "${PMON_PRODUCT_VERSION}")

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
)
target_compile_definitions(
    pmon_provider
    PRIVATE
        _WINDOWS
        _USRDLL
        "PM_VER_FILE_NUM=${pmon_file_version_numbers}"
        "PM_VER_PRODUCT_NUM=${pmon_product_version_numbers}"
        "PM_VER_FILE_STR=\"${PMON_FILE_VERSION}\""
        "PM_VER_PRODUCT_STR=\"${PMON_PRODUCT_VERSION}\""
        "PM_VER_COPYRIGHT=\"${PMON_COPYRIGHT}\""
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

pmon_register_artifact("$<TARGET_FILE:pmon_provider>")
