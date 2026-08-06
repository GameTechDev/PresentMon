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
# does not link either. PresentMonAPI2.dll is loaded at runtime (see
# --middleware-dll-path); build it into the output dir before F5.
add_dependencies(KernelProcess PresentMonUI PresentMonService PresentMonAPI2)

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
)
pmon_apply_kernel_process_manifest(KernelProcess)
pmon_attach_kernel_process_developer_signing(KernelProcess)
set_target_properties(
    KernelProcess
    PROPERTIES
        OUTPUT_NAME PresentMon
        FOLDER "UI and Capture"
        VS_DEBUGGER_WORKING_DIRECTORY "${PMON_OUTPUT_ROOT}/$<CONFIG>"
        # Matches KernelProcess.args.json preset VS-Debug-Run for F5 from the
        # CMake-generated Visual Studio solution.
        VS_DEBUGGER_COMMAND_ARGUMENTS "--svc-as-child --files-working --log-level verbose --middleware-dll-path .\\PresentMonAPI2.dll --log-middleware-copy"
)

pmon_target_uses_kernel_payload(KernelProcess)
