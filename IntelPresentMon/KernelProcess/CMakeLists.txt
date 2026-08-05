find_package(boost_process CONFIG REQUIRED)

add_executable(
    KernelProcess
    WIN32
        kact/KernelActionRegistration.cpp
        MakeOverlaySpec.cpp
        winmain.cpp
        KernelProcess.rc
)
# Matches the CefNano and PresentMonService ProjectReferences in
# KernelProcess.vcxproj: KernelProcess launches both by name at runtime and
# does not link either.
add_dependencies(KernelProcess PresentMonUI PresentMonService)

target_link_libraries(
    KernelProcess
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        pmon::version_resource
        pmon::release_link_policy
        pmon::common_utilities
        pmon::core
        pmon::interprocess
        pmon::api_wrapper
        pmon::versioning
        Boost::process
        ntdll
)
target_compile_definitions(
    KernelProcess
    PRIVATE
        _CONSOLE
        PM_KERNEL_PROCESS_BUILD=1
)
target_include_directories(
    KernelProcess
    PRIVATE
        # RC.exe resolves the ICON statement in KernelProcess.rc against its own
        # working directory, which for a CMake-generated Visual Studio project is
        # not this source directory.
        $<$<COMPILE_LANGUAGE:RC>:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_compile_options(
    KernelProcess
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    KernelProcess
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
        # Reproduces KernelProcess.vcxproj's Release-only UACUIAccess=true.
        $<$<CONFIG:Release>:/MANIFESTUAC:level='asInvoker'>
        $<$<CONFIG:Release>:/MANIFESTUAC:uiAccess='true'>
)
set_target_properties(
    KernelProcess
    PROPERTIES
        OUTPUT_NAME PresentMon
        FOLDER "UI and Capture"
        VS_DEBUGGER_WORKING_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>"
)

pmon_target_uses_kernel_payload(KernelProcess)
