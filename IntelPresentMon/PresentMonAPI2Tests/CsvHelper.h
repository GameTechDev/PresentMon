// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../CommonUtilities/str/String.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <optional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

enum Header {
    Header_Application,
    Header_ProcessID,
    Header_SwapChainAddress,
    Header_PresentRuntime,
    Header_SyncInterval,
    Header_PresentFlags,
    Header_AllowsTearing,
    Header_PresentMode,
    Header_FrameType,
    Header_PresentStartQPC,
    Header_MsBetweenSimulationStart,
    Header_MsPCLatency,
    Header_CPUStartTime,
    Header_CPUStartQPC,
    Header_CPUStartQPCTime,
    Header_CPUStartDateTime,
    Header_MsBetweenAppStart,
    Header_MsCPUBusy,
    Header_MsCPUWait,
    Header_MsGPULatency,
    Header_MsGPUTime,
    Header_MsGPUBusy,
    Header_MsGPUWait,
    Header_MsVideoBusy,
    Header_MsAnimationError,
    Header_AnimationTime,
    Header_MsClickToPhotonLatency,
    Header_MsAllInputToPhotonLatency,

    // App Provided Metrics
    Header_MsInstrumentedLatency,

    // --v1_metrics
    Header_Runtime,
    Header_Dropped,
    Header_TimeInSeconds,
    Header_MsBetweenPresents,
    Header_MsInPresentAPI,
    Header_MsBetweenDisplayChange,
    Header_MsUntilRenderComplete,
    Header_MsUntilDisplayed,
    Header_MsUntilRenderStart,
    Header_MsGPUActive,
    Header_MsGPUVideoActive,
    Header_MsSinceInput,
    Header_QPCTime,

    // Deprecated
    Header_WasBatched,
    Header_DwmNotified,

    // Special values:
    KnownHeaderCount,
    UnknownHeader,
};

struct v2Metrics {
    std::string appName;
    uint32_t processId = 0;
    uint64_t swapChain = 0;
    PM_GRAPHICS_RUNTIME runtime = PM_GRAPHICS_RUNTIME_UNKNOWN;
    int32_t syncInterval = 0;
    uint32_t presentFlags = 0;
    uint32_t allowsTearing = 0;
    PM_PRESENT_MODE presentMode = PM_PRESENT_MODE_UNKNOWN;
    PM_FRAME_TYPE frameType = PM_FRAME_TYPE_NOT_SET;
    uint64_t presentStartQPC = 0;
    double msBetweenPresents = 0.;
    double msInPresentAPI = 0.;
    double msRenderPresentLatency = 0.;
    uint64_t cpuFrameQpc = 0;
    double msBetweenAppStart = 0.;
    double msCpuBusy = 0.;
    double msCpuWait = 0.;
    double msGpuLatency = 0.;
    double msGpuTime = 0.;
    double msGpuBusy = 0.;
    double msGpuWait = 0.;
    double msVideoBusy = 0.;
    std::optional<double> msBetweenSimStart;
    std::optional<double> msUntilDisplayed;
    std::optional<double> msBetweenDisplayChange;
    std::optional<double> msPcLatency;
    std::optional<double> msAnimationError;
    std::optional<double> animationTime;
    std::optional<double> msClickToPhotonLatency;
    std::optional<double> msAllInputToPhotonLatency;
    std::optional<double> msInstrumentedLatency;
    std::optional<double> msInstrumentedRenderLatency;
    std::optional<double> msInstrumentedSleep;
    std::optional<double> msInstrumentedGPULatency;
    std::optional<double> msReadyTimeToDisplayLatency;
    std::optional<double> msReprojectedLatency;
};

constexpr char const* GetHeaderString(Header h)
{
    switch (h) {
    case Header_Application:                return "Application";
    case Header_ProcessID:                  return "ProcessID";
    case Header_SwapChainAddress:           return "SwapChainAddress";
    case Header_PresentRuntime:             return "PresentRuntime";
    case Header_SyncInterval:               return "SyncInterval";
    case Header_PresentFlags:               return "PresentFlags";
    case Header_AllowsTearing:              return "AllowsTearing";
    case Header_PresentMode:                return "PresentMode";
    case Header_FrameType:                  return "FrameType";
    case Header_PresentStartQPC:            return "PresentStartQPC";
    case Header_MsBetweenSimulationStart:   return "MsBetweenSimulationStart";
    case Header_MsPCLatency:                return "MsPCLatency";
    case Header_CPUStartTime:               return "CPUStartTime";
    case Header_CPUStartQPC:                return "CPUStartQPC";
    case Header_CPUStartQPCTime:            return "CPUStartQPCTime";
    case Header_CPUStartDateTime:           return "CPUStartDateTime";
    case Header_MsBetweenAppStart:          return "MsBetweenAppStart";
    case Header_MsCPUBusy:                  return "MsCPUBusy";
    case Header_MsCPUWait:                  return "MsCPUWait";
    case Header_MsGPULatency:               return "MsGPULatency";
    case Header_MsGPUTime:                  return "MsGPUTime";
    case Header_MsGPUBusy:                  return "MsGPUBusy";
    case Header_MsVideoBusy:                return "MsVideoBusy";
    case Header_MsGPUWait:                  return "MsGPUWait";
    case Header_MsAnimationError:           return "MsAnimationError";
    case Header_AnimationTime:              return "AnimationTime";
    case Header_MsClickToPhotonLatency:     return "MsClickToPhotonLatency";
    case Header_MsAllInputToPhotonLatency:  return "MsAllInputToPhotonLatency";

    case Header_Runtime:                    return "Runtime";
    case Header_Dropped:                    return "Dropped";
    case Header_TimeInSeconds:              return "TimeInQPC";
    case Header_MsBetweenPresents:          return "MsBetweenPresents";
    case Header_MsInPresentAPI:             return "MsInPresentAPI";
    case Header_MsBetweenDisplayChange:     return "MsBetweenDisplayChange";
    case Header_MsUntilRenderComplete:      return "MsRenderPresentLatency";
    case Header_MsUntilDisplayed:           return "MsUntilDisplayed";
    case Header_MsUntilRenderStart:         return "msUntilRenderStart";
    case Header_MsGPUActive:                return "msGPUActive";
    case Header_MsGPUVideoActive:           return "msGPUVideoActive";
    case Header_MsSinceInput:               return "msSinceInput";
    case Header_QPCTime:                    return "QPCTime";

    case Header_WasBatched:                 return "WasBatched";
    case Header_DwmNotified:                return "DwmNotified";

    case Header_MsInstrumentedLatency:      return "MsInstrumentedLatency";

    default:                                return "<unknown>";
    }
}

std::wstring CreateErrorString(Header columnId, size_t line)
{
    std::wstring errorMessage = L"Invalid ";
    errorMessage += pmon::util::str::ToWide(GetHeaderString(columnId));
    errorMessage += L" at line: ";
    errorMessage += std::to_wstring(line);

    return errorMessage;
}

template <typename T>
class CharConvert {
public:
    void Convert(const std::string data, T& convertedData, Header columnId, size_t line);
};

template <typename T>
void CharConvert<T>::Convert(const std::string data, T& convertedData, Header columnId, size_t line) {
    if constexpr (std::is_same<T, double>::value) {
        try
        {
            convertedData = std::stod(data);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, uint32_t>::value) {
        try
        {
            convertedData = std::stoul(data);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, int32_t>::value) {
        try
        {
            convertedData = std::stoi(data);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, uint64_t>::value) {
        try
        {
            char* endptr;
            unsigned long long result1;

            if (data.size() > 2 && data[0] == '0' && (data[1] == 'x' || data[1] == 'X')) {
                // Convert hexadecimal
                result1 = std::strtoull(data.c_str(), &endptr, 16);
            }
            else {
                // Convert decimal
                result1 = std::strtoull(data.c_str(), &endptr, 10);
            }
            convertedData = static_cast<uint64_t>(result1);
        }
        catch (...) {
            Assert::Fail(CreateErrorString(columnId, line).c_str());

        }
    }
    else if constexpr (std::is_same<T, PM_GRAPHICS_RUNTIME>::value) {
        if (data == "DXGI") {
            convertedData = PM_GRAPHICS_RUNTIME_DXGI;
        }
        else if (data == "D3D9") {
            convertedData = PM_GRAPHICS_RUNTIME_D3D9;
        }
        else if (data == "Other") {
            convertedData = PM_GRAPHICS_RUNTIME_UNKNOWN;
        }
        else {
            Assert::Fail(CreateErrorString(columnId, line).c_str());
        }

    }
    else if constexpr (std::is_same<T, PM_PRESENT_MODE>::value) {
        if (data == "Hardware: Legacy Flip") {
            convertedData = PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP;
        }
        else if (data == "Hardware: Legacy Copy to Front Buffer") {
            convertedData = PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER;
        }
        else if (data == "Hardware: Independent Flip") {
            convertedData = PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP;
        }
        else if (data == "Composed: Flip") {
            convertedData = PM_PRESENT_MODE_COMPOSED_FLIP;
        }
        else if (data == "Composed: Copy with GPU GDI") {
            convertedData = PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI;
        }
        else if (data == "Composed: Copy with CPU GDI") {
            convertedData = PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI;
        }
        else if (data == "Hardware Composed: Independent Flip") {
            convertedData = PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP;
        }
        else if (data == "Other") {
            convertedData = PM_PRESENT_MODE_UNKNOWN;
        }
        else {
            Assert::Fail(CreateErrorString(Header_PresentMode, line).c_str());
        }
    }
    else if constexpr (std::is_same<T, PM_FRAME_TYPE>::value) {
        if (data == "NotSet") {
            convertedData = PM_FRAME_TYPE_NOT_SET;
        }
        else if (data == "Unspecified") {
            convertedData = PM_FRAME_TYPE_UNSPECIFIED;
        }
        else if (data == "Application") {
            convertedData = PM_FRAME_TYPE_APPLICATION;
        }
        else if (data == "Repeated") {
            convertedData = PM_FRAME_TYPE_REPEATED;
        }
        else if (data == "AMD_AFMF") {
            convertedData = PM_FRAME_TYPE_AMD_AFMF;
        }
        else if (data == "Intel XeSS-FG") {
            convertedData = PM_FRAME_TYPE_INTEL_XEFG;
        }
        else {
            Assert::Fail(CreateErrorString(Header_FrameType, line).c_str());
        }
    }
    else
    {
        Assert::Fail(CreateErrorString(UnknownHeader, line).c_str());
    }
}

size_t countDecimalPlaces(double value) {
    std::string str = std::to_string(value);
    auto dotPos = str.find('.');
    if (dotPos == std::string::npos) return 0;

    // Get the decimal part
    std::string decimals = str.substr(dotPos + 1);

    // Remove trailing zeros
    decimals.erase(decimals.find_last_not_of('0') + 1);

    // Count non-zero decimal digits
    size_t count = 0;
    for (char c : decimals) {
        if (c != '0') ++count;
    }
    return count;
}

template<typename T>
bool Validate(const T& param1, const T& param2) {
    if constexpr (std::is_same<T, double>::value) {
        auto min = std::min(countDecimalPlaces(param1), countDecimalPlaces(param2));
        double threshold = pow(0.1, min);
        double difference = param1 - param2;
        if (difference > -threshold && difference < threshold) {
            return true;
        }
        else
        {
            return false;
        }
    }
    else {
        return param1 == param2;
    }

}

std::optional<std::ofstream> CreateCsvFile(std::string& output_dir, std::string& processName)
{
    // Setup csv file
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm local_time;
    localtime_s(&local_time, &now);
    std::ofstream csvFile;
    std::string csvFileName = output_dir + processName;
    try {
        csvFile.open(csvFileName);
        csvFile <<
            "Application,ProcessID,SwapChainAddress,PresentRuntime"
            ",SyncInterval,PresentFlags,AllowsTearing,PresentMode"
            ",FrameType,TimeInSec,MsBetweenSimulationStart,MsBetweenPresents"
            ",MsBetweenDisplayChange,MsInPresent,MsRenderPresentLatency"
            ",MsUntilDisplayed,MsPCLatency,CPUStartQPC,MsBetweenAppStart"
            ",MsCPUBusy,MsCPUWait,MsGPULatency,MsGPUTime,MsGPUBusy,MsGPUWait"
            ",MsVideoBusy,MsAnimationError,AnimationTime,MsFlipDelay,MsAllInputToPhotonLatency"
            ",MsClickToPhotonLatency,MsInstrumentedLatency";
        csvFile << std::endl;
        return csvFile;
    }
    catch (...) {
        return std::nullopt;
    }
}

std::string TranslatePresentMode(PM_PRESENT_MODE present_mode) {
    switch (present_mode) {
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP:
        return "Hardware: Legacy Flip";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER:
        return "Hardware: Legacy Copy to front buffer";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP:
        return "Hardware: Independent Flip";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_FLIP:
        return "Composed: Flip";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP:
        return "Hardware Composed: Independent Flip";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI:
        return "Composed: Copy with GPU GDI";
    case PM_PRESENT_MODE::PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI:
        return "Composed: Copy with CPU GDI";
    default:
        return("Other");
    }
}
std::string TranslateGraphicsRuntime(PM_GRAPHICS_RUNTIME graphicsRuntime) {
    switch (graphicsRuntime) {
    case PM_GRAPHICS_RUNTIME_UNKNOWN:
        return "UNKNOWN";
    case PM_GRAPHICS_RUNTIME_DXGI:
        return "DXGI";
    case PM_GRAPHICS_RUNTIME_D3D9:
        return "D3D9";
    default:
        return "UNKNOWN";
    }
}

std::string TranslateFrameType(PM_FRAME_TYPE frameType) {
    switch (frameType) {
    case PM_FRAME_TYPE_NOT_SET:
    case PM_FRAME_TYPE_UNSPECIFIED:
    case PM_FRAME_TYPE_APPLICATION:
        return "Application";
    case PM_FRAME_TYPE_AMD_AFMF:
        return "AMD_AFMF";
    case PM_FRAME_TYPE_INTEL_XEFG:
        return "Intel XeSS-FG";
    default:
        return "<Unknown>";
    }
}

void WriteToCSV(std::optional<std::ofstream>& debugCsvFile, const std::string& processName, const unsigned int& processId,
    PM_QUERY_ELEMENT(&queryElements)[29], pmapi::BlobContainer& blobs)
{

    if (!debugCsvFile.has_value()) {
        return;
    }
    try {
        for (auto pBlob : blobs) {
            const auto swapChain = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[0].dataOffset]);
            const auto graphicsRuntime = *reinterpret_cast<const PM_GRAPHICS_RUNTIME*>(&pBlob[queryElements[1].dataOffset]);
            const auto syncInterval = *reinterpret_cast<const int32_t*>(&pBlob[queryElements[2].dataOffset]);
            const auto presentFlags = *reinterpret_cast<const uint32_t*>(&pBlob[queryElements[3].dataOffset]);
            const auto allowsTearing = *reinterpret_cast<const bool*>(&pBlob[queryElements[4].dataOffset]);
            const auto presentMode = *reinterpret_cast<const PM_PRESENT_MODE*>(&pBlob[queryElements[5].dataOffset]);
            const auto frameType = *reinterpret_cast<const PM_FRAME_TYPE*>(&pBlob[queryElements[6].dataOffset]);
            const auto timeQpc = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[7].dataOffset]);
            const auto msBetweenSimStartTime = *reinterpret_cast<const double*>(&pBlob[queryElements[8].dataOffset]);
            const auto msBetweenPresents = *reinterpret_cast<const double*>(&pBlob[queryElements[9].dataOffset]);
            const auto msBetweenDisplayChange = *reinterpret_cast<const double*>(&pBlob[queryElements[10].dataOffset]);
            const auto msInPresentApi = *reinterpret_cast<const double*>(&pBlob[queryElements[11].dataOffset]);
            const auto msRenderPresentLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[12].dataOffset]);
            const auto msUntilDisplayed = *reinterpret_cast<const double*>(&pBlob[queryElements[13].dataOffset]);
            const auto msPcLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[14].dataOffset]);
            const auto cpuStartQpc = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[15].dataOffset]);
            const auto msBetweenAppStart = *reinterpret_cast<const double*>(&pBlob[queryElements[16].dataOffset]);
            const auto msCpuBusy = *reinterpret_cast<const double*>(&pBlob[queryElements[17].dataOffset]);
            const auto msCpuWait = *reinterpret_cast<const double*>(&pBlob[queryElements[18].dataOffset]);
            const auto msGpuLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[19].dataOffset]);
            const auto msGpuTime = *reinterpret_cast<const double*>(&pBlob[queryElements[20].dataOffset]);
            const auto msGpuBusy = *reinterpret_cast<const double*>(&pBlob[queryElements[21].dataOffset]);
            const auto msGpuWait = *reinterpret_cast<const double*>(&pBlob[queryElements[22].dataOffset]);
            const auto msAnimationError = *reinterpret_cast<const double*>(&pBlob[queryElements[23].dataOffset]);
            const auto animationTime = *reinterpret_cast<const double*>(&pBlob[queryElements[24].dataOffset]);
            const auto msFlipDelay = *reinterpret_cast<const double*>(&pBlob[queryElements[25].dataOffset]);
            const auto msAllInputToPhotonLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[26].dataOffset]);
            const auto msClickToPhotonLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[27].dataOffset]);
            const auto msInstrumentedLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[28].dataOffset]);
            *debugCsvFile << processName << ",";
            *debugCsvFile << processId << ",";
            *debugCsvFile << std::hex << "0x" << std::dec << swapChain << ",";
            *debugCsvFile << TranslateGraphicsRuntime(graphicsRuntime) << ",";
            *debugCsvFile << syncInterval << ",";
            *debugCsvFile << presentFlags << ",";
            *debugCsvFile << allowsTearing << ",";
            *debugCsvFile << TranslatePresentMode(presentMode) << ",";
            *debugCsvFile << TranslateFrameType(frameType) << ",";
            *debugCsvFile << timeQpc << ","; // Time in QPC
            *debugCsvFile << msBetweenSimStartTime << ",";
            *debugCsvFile << msBetweenPresents << ",";
            *debugCsvFile << msBetweenDisplayChange << ",";
            *debugCsvFile << msInPresentApi << ",";
            *debugCsvFile << msRenderPresentLatency << ",";
            *debugCsvFile << msUntilDisplayed << ",";
            *debugCsvFile << msPcLatency << ",";
            *debugCsvFile << cpuStartQpc << ",";
            *debugCsvFile << msBetweenAppStart << ",";
            *debugCsvFile << msCpuBusy << ",";
            *debugCsvFile << msCpuWait << ",";
            *debugCsvFile << msGpuLatency << ",";
            *debugCsvFile << msGpuTime << ",";
            *debugCsvFile << msGpuBusy << ",";
            *debugCsvFile << msGpuWait << ",";
            *debugCsvFile << 0 << ",";
            *debugCsvFile << msAnimationError << ",";
            *debugCsvFile << animationTime << ",";
            *debugCsvFile << msFlipDelay << ",";
            *debugCsvFile << msAllInputToPhotonLatency << ",";
            *debugCsvFile << msClickToPhotonLatency << ",";
            *debugCsvFile << msInstrumentedLatency << std::endl;
        }
    }
    catch (...) {
        return;
    }
}

class CsvParser {
public:
    CsvParser();

    bool Open(std::wstring const& path, uint32_t processId);
    void Close();
    bool VerifyBlobAgainstCsv(const std::string& processName, const unsigned int& processId,
        PM_QUERY_ELEMENT(&queryElements)[29], pmapi::BlobContainer& blobs, std::optional<std::ofstream>& debugCsvFile);
    bool ResetCsv();

private:
    bool FindFirstRowWithPid(const unsigned int& processId);
    bool ReadRow(bool gatherMetrics = false);
    size_t GetColumnIndex(char const* header);

    Header FindHeader(char const* header);
    void CheckAll(size_t const* columnIndex, bool* ok, std::initializer_list<Header> const& headers);

    void ConvertToMetricDataType(const char* data, Header columnId);

    FILE* fp_ = nullptr;

    size_t headerColumnIndex_[KnownHeaderCount] = {0};

    char row_[1024] = {0};
    size_t line_ = 0;
    std::vector<char const*> cols_;
    v2Metrics v2MetricRow_;
    uint32_t processId_ = 0;
    std::map<size_t, Header> activeColHeadersMap_;
};

CsvParser::CsvParser()
{}

bool CsvParser::VerifyBlobAgainstCsv(const std::string& processName, const unsigned int& processId,
    PM_QUERY_ELEMENT(&queryElements)[29], pmapi::BlobContainer& blobs, std::optional<std::ofstream>& debugCsvFile)
{
    if (debugCsvFile.has_value()) {
        WriteToCSV(debugCsvFile, processName, processId, queryElements, blobs);
    }
    for (auto pBlob : blobs) {
        // Read a row of blob data
        //const auto appName = *reinterpret_cast<const char*>(&pBlob[queryElements[0].dataOffset]);
        const auto swapChain = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[0].dataOffset]);
        const auto graphicsRuntime = *reinterpret_cast<const PM_GRAPHICS_RUNTIME*>(&pBlob[queryElements[1].dataOffset]);
        const auto syncInterval = *reinterpret_cast<const int32_t*>(&pBlob[queryElements[2].dataOffset]);
        const auto presentFlags = *reinterpret_cast<const uint32_t*>(&pBlob[queryElements[3].dataOffset]);
        const auto allowsTearing = *reinterpret_cast<const bool*>(&pBlob[queryElements[4].dataOffset]);
        const auto presentMode = *reinterpret_cast<const PM_PRESENT_MODE*>(&pBlob[queryElements[5].dataOffset]);
        const auto frameType = *reinterpret_cast<const PM_FRAME_TYPE*>(&pBlob[queryElements[6].dataOffset]);
        const auto timeQpc = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[7].dataOffset]);
        const auto msBetweenSimStartTime = *reinterpret_cast<const double*>(&pBlob[queryElements[8].dataOffset]);
        const auto msBetweenPresents = *reinterpret_cast<const double*>(&pBlob[queryElements[9].dataOffset]);
        const auto msBetweenDisplayChange = *reinterpret_cast<const double*>(&pBlob[queryElements[10].dataOffset]);
        const auto msInPresentApi = *reinterpret_cast<const double*>(&pBlob[queryElements[11].dataOffset]);
        const auto msRenderPresentLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[12].dataOffset]);
        const auto msUntilDisplayed = *reinterpret_cast<const double*>(&pBlob[queryElements[13].dataOffset]);
        const auto msPcLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[14].dataOffset]);
        const auto cpuStartQpc = *reinterpret_cast<const uint64_t*>(&pBlob[queryElements[15].dataOffset]);
        const auto msBetweenAppStart = *reinterpret_cast<const double*>(&pBlob[queryElements[16].dataOffset]);
        const auto msCpuBusy = *reinterpret_cast<const double*>(&pBlob[queryElements[17].dataOffset]);
        const auto msCpuWait = *reinterpret_cast<const double*>(&pBlob[queryElements[18].dataOffset]);
        const auto msGpuLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[19].dataOffset]);
        const auto msGpuTime = *reinterpret_cast<const double*>(&pBlob[queryElements[20].dataOffset]);
        const auto msGpuBusy = *reinterpret_cast<const double*>(&pBlob[queryElements[21].dataOffset]);
        const auto msGpuWait = *reinterpret_cast<const double*>(&pBlob[queryElements[22].dataOffset]);
        const auto msAnimationError = *reinterpret_cast<const double*>(&pBlob[queryElements[23].dataOffset]);
        const auto animationTime = *reinterpret_cast<const double*>(&pBlob[queryElements[24].dataOffset]);
        const auto msFrameDelay = *reinterpret_cast<const double*>(&pBlob[queryElements[25].dataOffset]);
        const auto msAllInputToPhotonLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[26].dataOffset]);
        const auto msClickToPhotonLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[27].dataOffset]);
        const auto msInstrumentedLatency = *reinterpret_cast<const double*>(&pBlob[queryElements[28].dataOffset]);
        
        // Read rows until we find one with the process we are interested in
        // or we are out of data.
        for (;;) {
            if (ReadRow(true)) {
                if (v2MetricRow_.processId == processId_) {
                    break;
                }
            }
            else
            {
                return true;
            }
        }

        assert(v2MetricRow_.processId == processId_);

        // Go through all of the active headers and validate results
        for (const auto& pair : activeColHeadersMap_) {

            bool columnsMatch = false;
            switch (pair.second)
            {
            case Header_Application:
                columnsMatch = true;
                break;
            case Header_ProcessID:
                columnsMatch = Validate(v2MetricRow_.processId, processId_);
                break;
            case Header_SwapChainAddress:
                    columnsMatch = Validate(v2MetricRow_.swapChain, swapChain);
                break;
            case Header_Runtime:
                columnsMatch = Validate(v2MetricRow_.runtime, graphicsRuntime);
                break;
            case Header_SyncInterval:
                columnsMatch = Validate(v2MetricRow_.syncInterval, syncInterval);
                break;
            case Header_PresentFlags:
                columnsMatch = Validate(v2MetricRow_.presentFlags, presentFlags);
                break;
            case Header_AllowsTearing:
                columnsMatch = Validate(v2MetricRow_.allowsTearing, (uint32_t)allowsTearing);
                break;
            case Header_PresentMode:
                columnsMatch = Validate(v2MetricRow_.presentMode, presentMode);
                break;
            case Header_FrameType:
                columnsMatch = Validate(v2MetricRow_.frameType, frameType);
                break;
            case Header_TimeInSeconds:
                columnsMatch = Validate(v2MetricRow_.presentStartQPC, timeQpc);
                break;
            case Header_CPUStartQPC:
                columnsMatch = Validate(v2MetricRow_.cpuFrameQpc, cpuStartQpc);
                break;
            case Header_MsBetweenAppStart:
                columnsMatch = Validate(v2MetricRow_.msBetweenAppStart, msBetweenAppStart);
                break;
            case Header_MsCPUBusy:
                columnsMatch = Validate(v2MetricRow_.msCpuBusy, msCpuBusy);
                break;
            case Header_MsCPUWait:
                columnsMatch = Validate(v2MetricRow_.msCpuWait, msCpuWait);
                break;
            case Header_MsGPULatency:
                columnsMatch = Validate(v2MetricRow_.msGpuLatency, msGpuLatency);
                break;
            case Header_MsGPUTime:
                columnsMatch = Validate(v2MetricRow_.msGpuTime, msGpuTime);
                break;
            case Header_MsGPUBusy:
                columnsMatch = Validate(v2MetricRow_.msGpuBusy, msGpuBusy);
                break;
            case Header_MsGPUWait:
                columnsMatch = Validate(v2MetricRow_.msGpuWait, msGpuWait);
                break;
            case Header_MsBetweenSimulationStart:
                if (v2MetricRow_.msBetweenSimStart.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msBetweenSimStart.value(), msBetweenSimStartTime);
                }
                else
                {
                    if (std::isnan(msBetweenSimStartTime)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_MsUntilDisplayed:
                if (v2MetricRow_.msUntilDisplayed.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msUntilDisplayed.value(), msUntilDisplayed);
                }
                else
                {
                    if (std::isnan(msUntilDisplayed)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_MsBetweenDisplayChange:
                if (v2MetricRow_.msBetweenDisplayChange.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msBetweenDisplayChange.value(), msBetweenDisplayChange);
                }
                else
                {
                    if (std::isnan(msBetweenDisplayChange)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_MsPCLatency:
                if (v2MetricRow_.msPcLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msPcLatency.value(), msPcLatency);
                    if (!columnsMatch) {
                        OutputDebugStringA("What!?\n");
                    }
                }
                else
                {
                    if (std::isnan(msPcLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_MsAnimationError:
                if (v2MetricRow_.msAnimationError.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msAnimationError.value(), msAnimationError);
                }
                else
                {
                    if (std::isnan(msAnimationError)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_AnimationTime:
                if (v2MetricRow_.animationTime.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.animationTime.value(), animationTime);
                }
                else
                {
                    if (std::isnan(animationTime)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_MsClickToPhotonLatency:
                if (v2MetricRow_.msClickToPhotonLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msClickToPhotonLatency.value(), msClickToPhotonLatency);
                }
                else
                {
                    if (std::isnan(msClickToPhotonLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_MsAllInputToPhotonLatency:
                if (v2MetricRow_.msAllInputToPhotonLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msAllInputToPhotonLatency.value(), msAllInputToPhotonLatency);
                }
                else
                {
                    if (std::isnan(msAllInputToPhotonLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            case Header_MsInstrumentedLatency:
                if (v2MetricRow_.msInstrumentedLatency.has_value()) {
                    columnsMatch = Validate(v2MetricRow_.msInstrumentedLatency.value(), msInstrumentedLatency);
                }
                else
                {
                    if (std::isnan(msInstrumentedLatency)) {
                        columnsMatch = true;
                    }
                    else
                    {
                        columnsMatch = false;
                    }
                }
                break;
            default:
                columnsMatch = true;
                break;
            }
            if (columnsMatch == false) {
                // If the columns do not match, create an error string
                // and assert failure.
                Assert::Fail(CreateErrorString(pair.second, line_).c_str());
            }
            Assert::IsTrue(columnsMatch, CreateErrorString(pair.second, line_).c_str());
        }
    }

    return true;
}

bool CsvParser::FindFirstRowWithPid(const unsigned int& searchProcessId)
{
    for (;;) {
        if (!ReadRow(true)) {
            break;
        }
        if (headerColumnIndex_[Header_ProcessID] != SIZE_MAX) {
            auto processColIdx = headerColumnIndex_[Header_ProcessID];
            char const* processIdString = nullptr;
            if (processColIdx < cols_.size()) {
                processIdString = cols_[Header_ProcessID];
                unsigned int currentProcessId = 0;
                int succeededCount = sscanf_s(processIdString, "%ud", &currentProcessId);
                if (searchProcessId == currentProcessId) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool CsvParser::ResetCsv()
{
    // Rewind the file pointer and then read the header to get
    // to the data
    rewind(fp_);
    ReadRow();

    return true;
}

bool CsvParser::Open(std::wstring const& path, uint32_t processId) {
    for (uint32_t i = 0; i < _countof(headerColumnIndex_); ++i) {
        headerColumnIndex_[i] = SIZE_MAX;
    }
    cols_.clear();

    if (_wfopen_s(&fp_, path.c_str(), L"r")) {
        return false;
    }

    // Remove UTF-8 marker if there is one.
    if (fread(row_, 3, 1, fp_) != 1 ||
        row_[0] != -17 || // 0xef
        row_[1] != -69 || // 0xbb
        row_[2] != -65) { // 0xbf
        fseek(fp_, 0, SEEK_SET);
    }

    // Read the header and ensure required columns are present
    ReadRow();

    for (size_t i = 0, n = cols_.size(); i < n; ++i) {
        auto h = FindHeader(cols_[i]);
        switch (h) {
        case KnownHeaderCount:
        case UnknownHeader:
            // This is fine for when processing ETLs for testing the middleware
            // as there are columns in the CSV that do not have corresponding
            // metrics so we can just ignore them.
            break;
        default:
            if ((size_t)h < KnownHeaderCount) {
                if (headerColumnIndex_[(size_t)h] != SIZE_MAX) {
                    std::wstring errorMessage = L"Duplicate column: ";
                    errorMessage += pmon::util::str::ToWide(cols_[i]);
                    Assert::Fail(errorMessage.c_str());
                }
                else {
                    headerColumnIndex_[(size_t)h] = i;
                }
                break;
            }
            else {
                std::wstring errorMessage = L"Index outside of known headers.";
                Assert::Fail(errorMessage.c_str());
            }
        }
    }

    bool columnsOK = true;
    CheckAll(headerColumnIndex_, &columnsOK, { Header_Application,
                                               Header_ProcessID,
                                               Header_SwapChainAddress,
                                               Header_PresentRuntime,
                                               Header_SyncInterval,
                                               Header_PresentFlags,
                                               Header_AllowsTearing,
                                               Header_PresentMode,
                                               Header_FrameType,
                                               Header_CPUStartQPC,
                                               Header_MsBetweenAppStart,
                                               Header_MsCPUBusy,
                                               Header_MsCPUWait,
                                               Header_MsGPULatency,
                                               Header_MsGPUTime,
                                               Header_MsGPUBusy,
                                               Header_MsGPUWait,
                                               Header_MsVideoBusy,
                                               Header_MsUntilDisplayed,
                                               Header_MsBetweenDisplayChange,
                                               Header_MsAnimationError,
                                               Header_AnimationTime,
                                               Header_MsClickToPhotonLatency,
                                               Header_MsAllInputToPhotonLatency,
                                               Header_MsBetweenSimulationStart,
                                               Header_MsPCLatency});

    if (!columnsOK) {
        Assert::Fail(L"Missing required columns");
    }

    // Create a vector of active headers to be used when reading
    // metric data
    for (uint32_t i = 0; i < KnownHeaderCount; ++i) {
        if (headerColumnIndex_[i] != SIZE_MAX) {
            activeColHeadersMap_[headerColumnIndex_[i]] = (Header)i;
        }
    }

    // Set the process id for this CSV parser
    processId_ = processId;

    return true;
}

void CsvParser::CheckAll(size_t const* columnIndex, bool* ok, std::initializer_list<Header> const& headers)
{
    for (auto const& h : headers) {
        if (columnIndex[h] == SIZE_MAX) {
            *ok = false;
            return;
        }
    }
}

void CsvParser::Close()
{
    fclose(fp_);
    fp_ = nullptr;
}

void CsvParser::ConvertToMetricDataType(const char* data, Header columnId)
{
    switch (columnId)
    {
    case Header_Application:
    {
        v2MetricRow_.appName = data;
    }
    break;
    case Header_ProcessID:
    {
        CharConvert<uint32_t> converter;
        converter.Convert(data, v2MetricRow_.processId, columnId, line_);
    }
    break;
    case Header_SwapChainAddress:
    {
        CharConvert<uint64_t> converter;
        converter.Convert(data, v2MetricRow_.swapChain, columnId, line_);
    }
    break;
    case Header_PresentRuntime:
    {
        CharConvert<PM_GRAPHICS_RUNTIME> converter;
        converter.Convert(data, v2MetricRow_.runtime, columnId, line_);
    }
    break;
    case Header_SyncInterval:
    {
        CharConvert<int32_t> converter;
        converter.Convert(data, v2MetricRow_.syncInterval, columnId, line_);
    }
    break;
    case Header_PresentFlags:
    {
        CharConvert<uint32_t> converter;
        converter.Convert(data, v2MetricRow_.presentFlags, columnId, line_);
    }
    break;
    case Header_AllowsTearing:
    {
        CharConvert<uint32_t> converter;
        converter.Convert(data, v2MetricRow_.allowsTearing, columnId, line_);
    }
    break;
    case Header_PresentMode:
    {
        CharConvert<PM_PRESENT_MODE> converter;
        converter.Convert(data, v2MetricRow_.presentMode, columnId, line_);
    }
    break;
    case Header_FrameType:
    {
        CharConvert<PM_FRAME_TYPE> converter;
        converter.Convert(data, v2MetricRow_.frameType, columnId, line_);
    }
    break;
    case Header_TimeInSeconds:
    {
        CharConvert<uint64_t> converter;
        converter.Convert(data, v2MetricRow_.presentStartQPC, columnId, line_);
    }
    break;
    case Header_CPUStartQPC:
    {
        CharConvert<uint64_t> converter;
        converter.Convert(data, v2MetricRow_.cpuFrameQpc, columnId, line_);
    }
    break;
    case Header_MsBetweenAppStart:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msBetweenAppStart, columnId, line_);
    }
    break;
    case Header_MsCPUBusy:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msCpuBusy, columnId, line_);
    }
    break;
    case Header_MsCPUWait:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msCpuWait, columnId, line_);
    }
    break;
    case Header_MsGPULatency:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msGpuLatency, columnId, line_);
    }
    break;
    case Header_MsGPUTime:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msGpuTime, columnId, line_);
    }
    break;
    case Header_MsGPUBusy:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msGpuBusy, columnId, line_);
    }
    break;
    case Header_MsGPUWait:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msGpuWait, columnId, line_);
    }
    break;
    case Header_MsVideoBusy:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msVideoBusy, columnId, line_);
    }
    break;
    case Header_MsBetweenSimulationStart:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msBetweenSimStart = convertedData;
        }
        else
        {
            v2MetricRow_.msBetweenSimStart.reset();
        }
    }
    break;
    case Header_MsBetweenPresents:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msBetweenPresents, columnId, line_);
    }
    break;
    case Header_MsBetweenDisplayChange:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msBetweenDisplayChange = convertedData;
        }
        else {
            v2MetricRow_.msBetweenDisplayChange.reset();
        }
    }
    break;
    case Header_MsInPresentAPI: {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msInPresentAPI, columnId, line_);
    }
    break;
    case Header_MsUntilRenderComplete:
    {
        CharConvert<double> converter;
        converter.Convert(data, v2MetricRow_.msRenderPresentLatency, columnId, line_);
    }
    break;
    case Header_MsUntilDisplayed:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msUntilDisplayed = convertedData;
        }
        else
        {
            v2MetricRow_.msUntilDisplayed.reset();
        }
    }
    break;
    case Header_MsPCLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msPcLatency = convertedData;
        }
        else {
            v2MetricRow_.msPcLatency.reset();
        }
    }
    break;
    case Header_MsAnimationError:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msAnimationError = convertedData;
        }
        else {
            v2MetricRow_.msAnimationError.reset();
        }
    }
    break;
    case Header_AnimationTime:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.animationTime = convertedData;
        }
        else {
            v2MetricRow_.animationTime.reset();
        }
    }
    break;
    case Header_MsClickToPhotonLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msClickToPhotonLatency = convertedData;
        }
        else {
            v2MetricRow_.msClickToPhotonLatency.reset();
        }
    }
    break;
    case Header_MsAllInputToPhotonLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msAllInputToPhotonLatency = convertedData;
        }
        else {
            v2MetricRow_.msAllInputToPhotonLatency.reset();
        }
    }
    break;
    case Header_MsInstrumentedLatency:
    {
        if (strncmp(data, "NA", 2) != 0) {
            double convertedData = 0.;
            CharConvert<double> converter;
            converter.Convert(data, convertedData, columnId, line_);
            v2MetricRow_.msInstrumentedLatency = convertedData;
        }
        else
        {
            v2MetricRow_.msInstrumentedLatency.reset();
        }
    }
    break;
    default:
        Assert::Fail(CreateErrorString(UnknownHeader, line_).c_str());
    }

    return;
}

bool CsvParser::ReadRow(bool gatherMetrics)
{
    row_[0] = '\0';
    cols_.clear();

    // Read a line
    if (fgets(row_, _countof(row_), fp_) == nullptr) {
        if (ferror(fp_) != 0) {
            std::wstring errorMessage = L"File read error at line: ";
            errorMessage += std::to_wstring(line_);
            Assert::Fail(errorMessage.c_str());
        }
        return false;
    }

    line_ += 1;

    // Split line into columns, skipping leading/trailing whitespace
    auto p0 = row_;
    for (; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
    uint32_t columnId = 0;
    for (auto p = p0; ; ++p) {
        auto ch = *p;
        if (ch == ',' || ch == '\0') {
            *p = '\0';
            cols_.push_back(p0);
            if (gatherMetrics) {
                ConvertToMetricDataType(p0, activeColHeadersMap_[columnId++]);
            }
            for (p0 = p + 1; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
            for (auto q = p - 1; *q == ' ' || *q == '\t' || *q == '\n' || *q == '\r'; --q) *q = '\0';
            if (ch == '\0') break;
        }
    }

    return true;
}

size_t CsvParser::GetColumnIndex(char const* header)
{
    auto h = FindHeader(header);
    return h < KnownHeaderCount ? headerColumnIndex_[h] : SIZE_MAX;
}

Header CsvParser::FindHeader(char const* header)
{
    for (uint32_t i = 0; i < KnownHeaderCount; ++i) {
        auto h = (Header)i;
        if (strcmp(header, GetHeaderString(h)) == 0) {
            return h;
        }
    }
    return UnknownHeader;
}
