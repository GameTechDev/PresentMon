include_guard(GLOBAL)

function(pmon_create_compiler_targets)
    set(
        CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        PARENT_SCOPE
    )

    # Shared defaults from the tracked legacy projects.
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE PARENT_SCOPE)
    set(
        CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<$<CONFIG:Debug>:EditAndContinue>$<$<CONFIG:Release>:ProgramDatabase>"
        PARENT_SCOPE
    )
    set(CMAKE_VS_JUST_MY_CODE_DEBUGGING "$<CONFIG:Debug>" PARENT_SCOPE)

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
            $<$<CONFIG:Release>:/Gy>
            $<$<CONFIG:Release>:/Oi>
    )
    # Most targets are Unicode. The few MultiByte ones (SampleClient, CefNano)
    # opt out by setting the PMON_MULTIBYTE property on the consuming target.
    target_compile_definitions(
        pmon_build_options
        INTERFACE
            CEREAL_THREAD_SAFE=1
            BOOST_ALLOW_DEPRECATED_HEADERS
            "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:PMON_MULTIBYTE>>>:UNICODE;_UNICODE>"
            "$<$<BOOL:$<TARGET_PROPERTY:PMON_MULTIBYTE>>:_MBCS>"
    )
    set_property(TARGET pmon_build_options PROPERTY FOLDER "Build")

    add_library(pmon_warnings_default INTERFACE)
    add_library(pmon::warnings_default ALIAS pmon_warnings_default)
    target_compile_options(pmon_warnings_default INTERFACE /W3)
    set_property(TARGET pmon_warnings_default PROPERTY FOLDER "Build")

    add_library(pmon_warnings_strict INTERFACE)
    add_library(pmon::warnings_strict ALIAS pmon_warnings_strict)
    target_compile_options(pmon_warnings_strict INTERFACE /W4 /WX)
    set_property(TARGET pmon_warnings_strict PROPERTY FOLDER "Build")

    # Keep linker /WX off static libraries, which would propagate it to consumers.
    add_library(pmon_warnings_strict_link INTERFACE)
    add_library(pmon::warnings_strict_link ALIAS pmon_warnings_strict_link)
    target_link_options(pmon_warnings_strict_link INTERFACE /WX)
    set_property(TARGET pmon_warnings_strict_link PROPERTY FOLDER "Build")

    # Shared legacy Release policy for executable and DLL link steps.
    add_library(pmon_release_link_policy INTERFACE)
    add_library(pmon::release_link_policy ALIAS pmon_release_link_policy)
    target_link_options(
        pmon_release_link_policy
        INTERFACE
            $<$<CONFIG:Release>:/DEBUG>
            $<$<CONFIG:Release>:/OPT:REF>
            $<$<CONFIG:Release>:/OPT:ICF>
    )
    set_property(TARGET pmon_release_link_policy PROPERTY FOLDER "Build")
endfunction()
