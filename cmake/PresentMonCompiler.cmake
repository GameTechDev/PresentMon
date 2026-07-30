include_guard(GLOBAL)

function(pmon_create_compiler_targets)
    set(
        CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        PARENT_SCOPE
    )

    add_library(pmon_build_options INTERFACE)
    add_library(pmon::build_options ALIAS pmon_build_options)
    target_compile_options(
        pmon_build_options
        INTERFACE
            /MP
            /sdl
            /external:W0
            $<$<COMPILE_LANGUAGE:CXX>:/permissive->
            $<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>
    )
    target_compile_definitions(
        pmon_build_options
        INTERFACE
            CEREAL_THREAD_SAFE=1
            BOOST_ALLOW_DEPRECATED_HEADERS
            UNICODE
            _UNICODE
    )
    set_property(TARGET pmon_build_options PROPERTY FOLDER "Build")

    add_library(pmon_warnings_default INTERFACE)
    add_library(pmon::warnings_default ALIAS pmon_warnings_default)
    target_compile_options(pmon_warnings_default INTERFACE /W3)
    set_property(TARGET pmon_warnings_default PROPERTY FOLDER "Build")

    add_library(pmon_warnings_strict INTERFACE)
    add_library(pmon::warnings_strict ALIAS pmon_warnings_strict)
    target_compile_options(pmon_warnings_strict INTERFACE /W4 /WX)
    target_link_options(pmon_warnings_strict INTERFACE /WX)
    set_property(TARGET pmon_warnings_strict PROPERTY FOLDER "Build")
endfunction()
