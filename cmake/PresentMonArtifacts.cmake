include_guard(GLOBAL)

function(pmon_register_artifact artifact_path)
    if(IS_ABSOLUTE "${artifact_path}" OR artifact_path MATCHES "^\\$<")
        set(resolved_artifact_path "${artifact_path}")
    else()
        set(resolved_artifact_path "${PROJECT_SOURCE_DIR}/${artifact_path}")
    endif()
    set_property(GLOBAL APPEND PROPERTY PMON_EXPECTED_ARTIFACTS "${resolved_artifact_path}")
endfunction()

function(pmon_add_artifact_verification_target)
    get_property(expected_artifacts GLOBAL PROPERTY PMON_EXPECTED_ARTIFACTS)
    if(NOT expected_artifacts)
        set(expected_artifacts "")
    endif()

    set(manifest_path "${PROJECT_BINARY_DIR}/artifacts-$<CONFIG>.txt")
    file(
        GENERATE
        OUTPUT "${manifest_path}"
        CONTENT "$<JOIN:${expected_artifacts},\n>\n"
    )

    add_custom_target(
        pmon_verify_artifacts
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPMON_ARTIFACT_MANIFEST=${manifest_path}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/VerifyArtifacts.cmake"
        COMMENT "Verifying expected PresentMon artifacts"
        VERBATIM
    )
    set_property(TARGET pmon_verify_artifacts PROPERTY FOLDER "Build")
endfunction()
