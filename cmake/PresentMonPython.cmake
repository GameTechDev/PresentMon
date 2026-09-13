include_guard(GLOBAL)

function(pmon_configure_python)
    set(Python3_FIND_VIRTUALENV FIRST)
    if(PMON_PYTHON_EXECUTABLE)
        set(Python3_EXECUTABLE "${PMON_PYTHON_EXECUTABLE}")
    elseif(WIN32)
        find_program(pmon_python_launcher NAMES py.exe py NO_CACHE)
        if(pmon_python_launcher)
            execute_process(
                COMMAND
                    "${pmon_python_launcher}"
                    -3
                    -c
                    "import sys; assert sys.version_info >= (3, 10); print(sys.executable)"
                RESULT_VARIABLE launcher_result
                OUTPUT_VARIABLE launcher_python
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(NOT launcher_result AND launcher_python)
                file(TO_CMAKE_PATH "${launcher_python}" launcher_python)
                if(EXISTS "${launcher_python}")
                    set(Python3_EXECUTABLE "${launcher_python}")
                endif()
            endif()
        endif()
    endif()
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    set(
        PMON_PYTHON_EXECUTABLE
        "${Python3_EXECUTABLE}"
        CACHE FILEPATH
        "Python interpreter used by PresentMon build tooling"
        FORCE
    )
    set(PMON_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" PARENT_SCOPE)
endfunction()

function(pmon_prepare_dependencies)
    if(NOT PMON_AUTO_PREPARE_DEPENDENCIES)
        message(STATUS "Automatic non-vcpkg dependency preparation is disabled.")
        return()
    endif()

    set(arguments
        "${PMON_PYTHON_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/Scripts/bootstrap.py"
        --repo-root "${PROJECT_SOURCE_DIR}"
        --dependency-root "${PMON_DEPENDENCY_ROOT}"
        --architecture "${PMON_ARCHITECTURE}"
        --generator "${CMAKE_GENERATOR}"
        --platform "${PMON_ARCHITECTURE}"
        --toolset "${PMON_VS_TOOLSET}"
        --generator-instance "${CMAKE_GENERATOR_INSTANCE}"
    )
    if(PMON_ARCHITECTURE STREQUAL "x64" AND PMON_BUILD_UI)
        list(APPEND arguments --with-cef --with-web)
        if(PMON_CEF_SOURCE)
            list(APPEND arguments --cef-source "${PMON_CEF_SOURCE}")
        endif()
    endif()
    if(PMON_BUILD_TESTS)
        list(APPEND arguments --with-auxdata)
    endif()

    execute_process(
        COMMAND ${arguments}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
    )
    if(result)
        message(FATAL_ERROR "PresentMon dependency preparation failed with exit code ${result}.")
    endif()
endfunction()
