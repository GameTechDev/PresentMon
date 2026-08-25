include_guard(GLOBAL)

function(pmon_compile_message_manifest output_variable manifest_path output_name)
    if(NOT PMON_MC_EXECUTABLE)
        find_program(PMON_MC_EXECUTABLE mc.exe)
    endif()
    if(NOT PMON_MC_EXECUTABLE)
        set(
            sdk_mc
            "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/mc.exe"
        )
        if(EXISTS "${sdk_mc}")
            set(
                PMON_MC_EXECUTABLE
                "${sdk_mc}"
                CACHE FILEPATH
                "Windows message compiler"
                FORCE
            )
        endif()
    endif()
    if(NOT PMON_MC_EXECUTABLE)
        message(FATAL_ERROR "Windows SDK message compiler mc.exe was not found.")
    endif()

    get_filename_component(manifest_path "${manifest_path}" ABSOLUTE)
    set(output_directory "${PROJECT_BINARY_DIR}/generated/mc/${output_name}")
    set(generated_header "${output_directory}/${output_name}.h")
    set(generated_resource "${output_directory}/${output_name}.rc")

    add_custom_command(
        OUTPUT
            "${generated_header}"
            "${generated_resource}"
        BYPRODUCTS
            "${output_directory}/${output_name}_MSG00001.bin"
            "${output_directory}/${output_name}TEMP.BIN"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
        COMMAND
            "${PMON_MC_EXECUTABLE}"
            -um
            -v
            -z "${output_name}"
            -h "${output_directory}"
            -r "${output_directory}"
            "${manifest_path}"
        DEPENDS
            "${manifest_path}"
        COMMENT "Compiling message manifest ${manifest_path}"
        VERBATIM
    )

    set_source_files_properties(
        "${generated_header}"
        "${generated_resource}"
        PROPERTIES GENERATED TRUE
    )
    set(${output_variable} "${generated_resource}" PARENT_SCOPE)
endfunction()
