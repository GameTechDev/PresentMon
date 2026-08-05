add_executable(
    PresentMonUI
    WIN32
        source/DataBindAccessor.cpp
        source/NanoCefBrowserClient.cpp
        source/NanoCefProcessHandler.cpp
        source/SchemeFileHandler.cpp
        source/SchemeHandlerFactory.cpp
        source/util/ActionClientServer.cpp
        source/util/AsyncEndpoint.cpp
        source/util/AsyncEndpointCollection.cpp
        source/util/AsyncEndpointManager.cpp
        source/util/cact/CefActionRegistration.cpp
        source/util/CefValues.cpp
        source/util/FontEnumerator.cpp
        source/util/IpcInvocationManager.cpp
        source/util/KernelActionRegistration.cpp
        source/util/LogSetup.cpp
        source/util/PathSanitaryCheck.cpp
        source/util/SignalManager.cpp
        source/util/ThreadCompass.cpp
        source/winmain.cpp
        AppCef.rc
        compatibility.manifest
)

target_link_libraries(
    PresentMonUI
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        pmon::version_resource
        pmon::release_link_policy
        pmon::common_utilities
        pmon::core
        pmon::interprocess
        pmon::versioning
        dwrite
        dwmapi
        shcore
)
target_compile_definitions(
    PresentMonUI
    PRIVATE
        _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING
        PM_BUILD_WINSDK_VERSION_STR="10.0"
        "PM_BUILD_CRT_RUNTIME_STR=\"$<IF:$<CONFIG:Debug>,MultiThreadedDebug,MultiThreaded>\""
)
target_include_directories(
    PresentMonUI
    PRIVATE
        # RC.exe resolves the ICON statement in AppCef.rc against its own
        # working directory, which for a CMake-generated Visual Studio
        # project is not this source directory. Scoped to RC only; C++
        # includes resolve through linked targets and quoted sibling
        # includes, never through this directory.
        $<$<COMPILE_LANGUAGE:RC>:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_compile_options(
    PresentMonUI
    PRIVATE
        /EHa
        /fp:fast
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    PresentMonUI
    PRIVATE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    PresentMonUI
    PROPERTIES
        # CefNano is the other MultiByte target alongside SampleClient.
        PMON_MULTIBYTE ON
        FOLDER "UI and Capture"
)

pmon_target_uses_cef(PresentMonUI)
pmon_target_uses_shaders(PresentMonUI)

