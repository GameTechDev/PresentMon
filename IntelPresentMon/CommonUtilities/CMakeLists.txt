set(common_utilities_sources
    cli/CliFramework.cpp
    Env.cpp
    Exception.cpp
    file/FileUtils.cpp
    file/PathUtils.cpp
    file/SecureSubdirectory.cpp
    file/TempFile.cpp
    Hash.cpp
    IntervalWaiter.cpp
    log/BasicFileDriver.cpp
    log/Channel.cpp
    log/ChannelFlusher.cpp
    log/CopyDriver.cpp
    log/DiagnosticDriver.cpp
    log/EntryBuilder.cpp
    log/EntryMarshallInjector.cpp
    log/ErrorCode.cpp
    log/ErrorCodeResolvePolicy.cpp
    log/ErrorCodeResolver.cpp
    log/GlobalPolicy.cpp
    log/HrLogger.cpp
    log/IdentificationTable.cpp
    log/Level.cpp
    log/LinePolicy.cpp
    log/LineTable.cpp
    log/Log.cpp
    log/MarshallDriver.cpp
    log/MsvcDebugDriver.cpp
    log/NamedPipeMarshallReceiver.cpp
    log/NamedPipeMarshallSender.cpp
    log/PanicLogger.cpp
    log/SimpleFileStrategy.cpp
    log/StackTrace.cpp
    log/StdioDriver.cpp
    log/Subsystem.cpp
    log/TextFormatter.cpp
    log/TimePoint.cpp
    log/Verbose.cpp
    Memory.cpp
    mt/Thread.cpp
    pipe/CoroMutex.cpp
    pipe/Pipe.cpp
    PrecisionWaiter.cpp
    Qpc.cpp
    ref/GeneratedReflection.cpp
    str/String.cpp
    win/com/ComManager.cpp
    win/com/ProcessSpawnSink.cpp
    win/com/WbemConnection.cpp
    win/com/WbemListener.cpp
    win/com/WbemSink.cpp
    win/Event.cpp
    win/Handle.cpp
    win/HrError.cpp
    win/HrErrorCodeProvider.cpp
    win/MessageBox.cpp
    win/Privileges.cpp
    win/ProcessMapBuilder.cpp
    win/Security.cpp
    win/Utilities.cpp
)

find_package(boost_asio CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(concurrentqueue CONFIG REQUIRED)
find_path(WINREG_INCLUDE_DIRS "winreg/WinReg.hpp" REQUIRED)

add_library(pmon_common_utilities STATIC ${common_utilities_sources})
add_library(pmon::common_utilities ALIAS pmon_common_utilities)
target_compile_definitions(
    pmon_common_utilities
    PUBLIC
        CEREAL_THREAD_SAFE=1
    PRIVATE
        _LIB
)
target_include_directories(
    pmon_common_utilities
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
        "${WINREG_INCLUDE_DIRS}"
)
target_link_libraries(
    pmon_common_utilities
    PUBLIC
        Boost::asio
        cereal::cereal
        CLI11::CLI11
        concurrentqueue::concurrentqueue
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        advapi32
        dbghelp
        ole32
        rpcrt4
        shell32
        user32
        wbemuuid
)
target_compile_options(pmon_common_utilities PRIVATE /EHa)
target_compile_options(
    pmon_common_utilities
    PRIVATE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)
target_link_options(
    pmon_common_utilities
    INTERFACE
        $<$<AND:$<CONFIG:Release>,$<STREQUAL:${PMON_ARCHITECTURE},x64>>:/guard:cf>
)
set_target_properties(
    pmon_common_utilities
    PROPERTIES
        OUTPUT_NAME IPMCommonUtilities
        ARCHIVE_OUTPUT_DIRECTORY
            "${PMON_OUTPUT_ROOT}/lib/CommonUtilities-${PMON_ARCHITECTURE}-$<CONFIG>"
        FOLDER "Foundation"
)
