include_guard(GLOBAL)

# Microsoft C++ Unit Test Framework discovery.
#
# The framework ships with the Visual Studio instance CMake selected, which
# pmon_configure_toolchain() published as PMON_VS_INSTANCE_ROOT. Both artifacts
# are derived directly beneath that instance; no other Visual Studio
# installation is consulted and no compiler-path or vswhere search runs here.
#
# PMON_CPPUNITTEST_INCLUDE_DIR and PMON_CPPUNITTEST_LIBRARY are PresentMon-owned
# internal results. They are recomputed from the selected instance on every
# configure rather than honored from the cache.

function(_pmon_cppunittest_arch_tag output_variable)
    if(PMON_ARCHITECTURE STREQUAL "Win32")
        set(${output_variable} "x86" PARENT_SCOPE)
    elseif(PMON_ARCHITECTURE STREQUAL "x64")
        set(${output_variable} "x64" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "CppUnitTest framework lookup supports x64 and Win32 only.")
    endif()
endfunction()

function(pmon_find_cppunittest_framework)
    set(_unittest_root "${PMON_VS_INSTANCE_ROOT}/VC/Auxiliary/VS/UnitTest")
    set(_include_dir "${_unittest_root}/include")
    _pmon_cppunittest_arch_tag(_unittest_arch)
    set(_library "${_unittest_root}/lib/${_unittest_arch}/Microsoft.VisualStudio.TestTools.CppUnitTestFramework.lib")

    if(NOT EXISTS "${_include_dir}/CppUnitTest.h")
        message(
            FATAL_ERROR
            "Microsoft C++ Unit Test Framework header not found:\n"
            "  ${_include_dir}/CppUnitTest.h\n"
            "Selected Visual Studio instance:\n"
            "  ${PMON_VS_INSTANCE_ROOT}\n"
            "Install the C++ Test Tools workload component in that instance. "
            "PresentMon does not substitute another Visual Studio installation."
        )
    endif()
    if(NOT EXISTS "${_library}")
        message(
            FATAL_ERROR
            "Microsoft C++ Unit Test Framework ${_unittest_arch} library not found:\n"
            "  ${_library}\n"
            "Selected Visual Studio instance:\n"
            "  ${PMON_VS_INSTANCE_ROOT}\n"
            "Install the C++ Test Tools workload component in that instance. "
            "PresentMon does not substitute another Visual Studio installation."
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
