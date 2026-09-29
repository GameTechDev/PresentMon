if(MSVC_VERSION LESS 1930)
    message(
        FATAL_ERROR
        "PresentMon fuzzers require Visual Studio 2022 version 17.0 or newer."
    )
endif()

get_filename_component(pmon_msvc_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
set(
    pmon_asan_runtime
    "${pmon_msvc_bin_dir}/clang_rt.asan_dynamic-x86_64.dll"
)
if(NOT EXISTS "${pmon_asan_runtime}")
    message(
        FATAL_ERROR
        "The MSVC AddressSanitizer runtime was not found at:\n"
        "  ${pmon_asan_runtime}\n"
        "Install the Visual Studio C++ AddressSanitizer component."
    )
endif()

function(pmon_add_fuzzer target)
    add_executable(${target} ${ARGN})
    target_link_libraries(
        ${target}
        PRIVATE
            pmon::build_options
            pmon::warnings_default
    )
    target_link_options(${target} PRIVATE /INCREMENTAL:NO)
    set_target_properties(
        ${target}
        PROPERTIES
            FOLDER "Fuzzing"
            MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase
            VS_GLOBAL_EnableAsan true
            VS_GLOBAL_EnableFuzzer true
    )
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${pmon_asan_runtime}"
            "$<TARGET_FILE_DIR:${target}>/"
        COMMENT "Staging the MSVC AddressSanitizer runtime"
        VERBATIM
    )
endfunction()

pmon_add_fuzzer(
    PresentMonServiceProtocolFuzzer
    PresentMonServiceProtocolFuzzer.cpp
)
target_link_libraries(
    PresentMonServiceProtocolFuzzer
    PRIVATE
        pmon::interprocess
)
