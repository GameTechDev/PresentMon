find_package(boost_process CONFIG REQUIRED)

add_executable(
    SampleClient
        Console.cpp
        DiagnosticDemo.cpp
        EtlLogger.cpp
        IpcComponentServer.cpp
        LogDemo.cpp
        LogSetup.cpp
        MultiClient.cpp
        PacedFramePlayback.cpp
        PacedPlayback.cpp
        SampleClient.cpp
        ServiceCrashClient.cpp
)

target_link_libraries(
    SampleClient
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        pmon::release_link_policy
        pmon::common_utilities
        pmon::interprocess
        pmon::api_wrapper
        PresentMonAPI2Loader
        Boost::process
        winmm
)
target_include_directories(
    SampleClient
    PRIVATE
        "${PROJECT_SOURCE_DIR}/IntelPresentMon"
)
target_compile_options(
    SampleClient
    PRIVATE
        /EHa
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    SampleClient
    PRIVATE
        /SUBSYSTEM:CONSOLE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    SampleClient
    PROPERTIES
        # SampleClient is the one MultiByte target in this phase.
        PMON_MULTIBYTE ON
        FOLDER "Service and SDK"
)
