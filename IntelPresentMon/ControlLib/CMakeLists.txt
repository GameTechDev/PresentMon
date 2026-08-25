find_package(nlohmann_json CONFIG REQUIRED)

add_library(
    pmon_control
    STATIC
        adl/Adl2Wrapper.cpp
        adl/AdlTelemetryProvider.cpp
        igcl/cApiWrapper.cpp
        igcl/IgclErrorCodeProvider.cpp
        igcl/IgclTelemetryProvider.cpp
        nvapi/NvapiWrapper.cpp
        nvapi/NvapiTelemetryProvider.cpp
        nvml/NvmlWrapper.cpp
        nvml/NvmlTelemetryProvider.cpp
        TelemetryAvailabilityMerge.cpp
        TelemetryMetricDiscovery.cpp
        TelemetryCoordinator.cpp
        TelemetryDeviceFingerprint.cpp
        uci/UciTelemetryProvider.cpp
        uci/UciWrapper.cpp
        mock/MockTelemetryProvider.cpp
        wmi/WmiTelemetryProvider.cpp
)
add_library(pmon::control ALIAS pmon_control)

target_include_directories(
    pmon_control
    PUBLIC
        "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(
    pmon_control
    PUBLIC
        pmon::interprocess
        # UciSdk.h selects PMON_HAS_UCI_SDK with __has_include, so the include
        # path alone decides whether UCI support is compiled in.
        pmon::uci_sdk
    PRIVATE
        pmon::build_options
        pmon::warnings_default
        nlohmann_json::nlohmann_json
        wbemuuid
)
target_compile_options(
    pmon_control
    PRIVATE
        /Zc:preprocessor
        $<$<CONFIG:Release>:/guard:cf>
)
target_link_options(
    pmon_control
    INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
)
set_target_properties(
    pmon_control
    PROPERTIES
        OUTPUT_NAME ControlLib
        FOLDER "Service and SDK"
)
