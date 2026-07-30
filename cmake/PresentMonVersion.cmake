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
        "Copyright (C) 2017-${copyright_year} Intel Corporation"
        PARENT_SCOPE
    )
endfunction()
