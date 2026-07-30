include_guard(GLOBAL)

function(pmon_register_artifact artifact_path)
    cmake_path(ABSOLUTE_PATH artifact_path BASE_DIRECTORY "${PROJECT_SOURCE_DIR}")
    set_property(GLOBAL APPEND PROPERTY PMON_EXPECTED_ARTIFACTS "${artifact_path}")
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
