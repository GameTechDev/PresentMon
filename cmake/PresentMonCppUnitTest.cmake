include_guard(GLOBAL)

function(_pmon_cppunittest_arch_tag output_variable)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
        set(${output_variable} "x86" PARENT_SCOPE)
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
        set(${output_variable} "x64" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "CppUnitTest framework lookup supports x64 and Win32 only.")
    endif()
endfunction()

function(pmon_find_cppunittest_framework)
    if(DEFINED PMON_CPPUNITTEST_INCLUDE_DIR AND DEFINED PMON_CPPUNITTEST_LIBRARY)
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER)
        message(FATAL_ERROR "pmon_find_cppunittest_framework() requires an active C++ toolchain.")
    endif()

    get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_host_dir "${_compiler_dir}" DIRECTORY)
    get_filename_component(_bin_dir "${_host_dir}" DIRECTORY)
    get_filename_component(_msvc_toolset_dir "${_bin_dir}" DIRECTORY)
    get_filename_component(_vc_tools_dir "${_msvc_toolset_dir}/../.." ABSOLUTE)

    set(_include_dir "${_vc_tools_dir}/Auxiliary/VS/UnitTest/include")
    _pmon_cppunittest_arch_tag(_unittest_arch)
    set(_library_dir "${_vc_tools_dir}/Auxiliary/VS/UnitTest/lib/${_unittest_arch}")
    set(_library "${_library_dir}/Microsoft.VisualStudio.TestTools.CppUnitTestFramework.lib")

    if(NOT EXISTS "${_include_dir}/CppUnitTest.h")
        set(vswhere "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe")
        if(EXISTS "${vswhere}")
            execute_process(
                COMMAND
                    "${vswhere}"
                    -latest
                    -products
                    "*"
                    -find
                    "**/CppUnitTest.h"
                OUTPUT_VARIABLE header_candidates
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            string(REPLACE "\n" ";" header_list "${header_candidates}")
            if(header_list)
                list(GET header_list 0 header_path)
            endif()
            if(header_path AND EXISTS "${header_path}")
                get_filename_component(_include_dir "${header_path}" DIRECTORY)
                get_filename_component(_unittest_root "${_include_dir}/.." ABSOLUTE)
                set(_library_dir "${_unittest_root}/lib/${_unittest_arch}")
                set(_library "${_library_dir}/Microsoft.VisualStudio.TestTools.CppUnitTestFramework.lib")
            endif()
        endif()
    endif()

    if(NOT EXISTS "${_include_dir}/CppUnitTest.h")
        message(
            FATAL_ERROR
            "Microsoft C++ Unit Test Framework headers not found under ${_include_dir}. "
            "Install the Visual Studio Test Tools for C++ workload."
        )
    endif()
    if(NOT EXISTS "${_library}")
        message(
            FATAL_ERROR
            "Microsoft C++ Unit Test Framework library not found at ${_library}."
        )
    endif()

    set(PMON_CPPUNITTEST_INCLUDE_DIR "${_include_dir}" CACHE INTERNAL "CppUnitTest include directory")
    set(PMON_CPPUNITTEST_LIBRARY "${_library}" CACHE INTERNAL "CppUnitTest import library")
endfunction()

function(pmon_apply_cppunittest_framework target)
    pmon_find_cppunittest_framework()
    target_include_directories(${target} PRIVATE "${PMON_CPPUNITTEST_INCLUDE_DIR}")
    target_link_libraries(${target} PRIVATE "${PMON_CPPUNITTEST_LIBRARY}")
    target_link_options(${target} PRIVATE /SUBSYSTEM:WINDOWS)
endfunction()
