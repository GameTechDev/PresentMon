include_guard(GLOBAL)

function(pmon_read_xml_property content property output_variable)
    string(
        REGEX MATCH
        "<${property}[^>]*>([^<]+)</${property}>"
        property_match
        "${content}"
    )
    if(NOT property_match)
        message(FATAL_ERROR "Could not read ${property} from Version.props.")
    endif()
    set(${output_variable} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

function(pmon_read_version version_props_path)
    if(NOT EXISTS "${version_props_path}")
        message(FATAL_ERROR "Version properties file not found: ${version_props_path}")
    endif()

    set_property(
        DIRECTORY APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS "${version_props_path}"
    )
    file(READ "${version_props_path}" version_props_content)
    pmon_read_xml_property(
        "${version_props_content}"
        "PresentMonVersion"
        parsed_version
    )
    pmon_read_xml_property(
        "${version_props_content}"
        "PresentMonFileVersion"
        parsed_file_version
    )

    string(TIMESTAMP copyright_year "%Y" UTC)

    set(PMON_VERSION "${parsed_version}" PARENT_SCOPE)
    set(PMON_FILE_VERSION "${parsed_file_version}" PARENT_SCOPE)
    set(PMON_PRODUCT_VERSION "${parsed_file_version}" PARENT_SCOPE)
    set(
        PMON_COPYRIGHT
        "Copyright (C) 2017-${copyright_year}"
        PARENT_SCOPE
    )
endfunction()

function(pmon_create_version_target)
    set(version_header "${PMON_OUTPUT_ROOT}/obj/generated/version.h")
    file(
        GENERATE
        OUTPUT "${version_header}"
        CONTENT "char const* PRESENT_MON_VERSION = \"${PMON_VERSION}\";\n"
    )

    add_library(pmon_version_header INTERFACE)
    add_library(pmon::version_header ALIAS pmon_version_header)
    target_include_directories(
        pmon_version_header
        INTERFACE
            "${PMON_OUTPUT_ROOT}/obj"
    )
    set_property(TARGET pmon_version_header PROPERTY FOLDER "Build")

    set(PMON_VERSION_HEADER "${version_header}" PARENT_SCOPE)
endfunction()

# Supplies the PM_VER_* definitions that IntelPresentMon .rc scripts expect.
# MSBuild injects these through the ResourceCompile section of Common.props.
function(pmon_create_version_resource_target)
    string(REPLACE "." "," file_version_numbers "${PMON_FILE_VERSION}")
    string(REPLACE "." "," product_version_numbers "${PMON_PRODUCT_VERSION}")

    add_library(pmon_version_resource INTERFACE)
    add_library(pmon::version_resource ALIAS pmon_version_resource)
    target_compile_definitions(
        pmon_version_resource
        INTERFACE
            "PM_VER_FILE_NUM=${file_version_numbers}"
            "PM_VER_PRODUCT_NUM=${product_version_numbers}"
            "PM_VER_FILE_STR=\"${PMON_FILE_VERSION}\""
            "PM_VER_PRODUCT_STR=\"${PMON_PRODUCT_VERSION}\""
            "PM_VER_COPYRIGHT=\"${PMON_COPYRIGHT}\""
    )
    set_property(TARGET pmon_version_resource PROPERTY FOLDER "Build")
endfunction()
