// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include <format>
#include "WmiCpu.h"

namespace pwr::cpu::wmi {

std::wstring kProcessorFrequency =
    L"\\Processor Information(_Total)\\Processor Frequency";
std::wstring kProcessorPerformance =
    L"\\Processor Information(_Total)\\% Processor Performance";
std::wstring kProcessorIdleTime = L"\\Processor(_Total)\\% Idle Time";

WmiCpu::WmiCpu() {
  HQUERY temp_query = nullptr;
  if (const auto result = PdhOpenQuery(NULL, NULL, &temp_query);
      result != ERROR_SUCCESS) {
    throw std::runtime_error{
        std::format("PdhOpenQuery failed. Result:{}", result).c_str()};
  }
  query_.reset(temp_query);

  if (const auto result =
          PdhAddEnglishCounterW(query_.get(), kProcessorFrequency.c_str(), 0,
                                        &processor_frequency_counter_);
      result != ERROR_SUCCESS) {
    throw std::runtime_error{
        std::format("PdhAddEnglishCounter failed when adding processor frequency "
                    "counter. Result: {}",
                    result)
            .c_str()};
  }

  if (const auto result =
          PdhAddEnglishCounterW(query_.get(), kProcessorPerformance.c_str(),
                                        0, &processor_performance_counter_);
      result != ERROR_SUCCESS) {
    throw std::runtime_error{
        std::format("PdhAddEnglishCounter failed when adding processor performance "
                    "counter. Result: {}",
                    result)
            .c_str()};
  }

  if (const auto result = PdhAddEnglishCounterW(query_.get(), kProcessorIdleTime.c_str(), 0,
                                &processor_idle_time_counter_);
      result != ERROR_SUCCESS) {
    throw std::runtime_error{
        std::format("PdhAddEnglishCounter failed when adding "
                    "processor time counter. Result: {}",
                    result)
            .c_str()};
  }

  // Most counters require two sample values to display a formatted value.
  // PDH stores the current sample value and the previously collected
  // sample value. This call retrieves the first value that will be used
  // by PdhGetFormattedCounterValue in the Sample() call.
  if (const auto result = PdhCollectQueryData(query_.get());
      result != ERROR_SUCCESS) {
    throw std::runtime_error{
        std::format("PdhCollectQueryData failed. Result: {}", result).c_str()};
  }

  // WMI specifies that it should not be sampled faster than once every
  // second. We however allow the user to specify the sample rate for
  // telemetry. Through testing it was observed that allowing a
  // sample rate of faster than one second will cause the CPU utilization
  // number to become inaccurate. Because of this we will impose
  // a one second wait for WMI sampling.

  // Grab the current QPC frequency which returns the current performance-
  // counter frequency in counts per SECOND.
  QueryPerformanceFrequency(&frequency_);
  // Now grab the current value of the performance counter
  QueryPerformanceCounter(&next_sample_qpc_);
  // To calculate the next time we should sample take the just sampled
  // performance counter and the frequency.
  next_sample_qpc_.QuadPart += frequency_.QuadPart;
}

bool WmiCpu::Sample() noexcept {
  DWORD counter_type;

  LARGE_INTEGER qpc;
  QueryPerformanceCounter(&qpc);
  if (qpc.QuadPart < next_sample_qpc_.QuadPart) {
    return true;
  }

  CpuTelemetryInfo info{
      .qpc = (uint64_t)qpc.QuadPart,
  };

  if (const auto result = PdhCollectQueryData(query_.get());
      result != ERROR_SUCCESS) {
    return false;
  }

  // Sample cpu clock. This is an approximation using the frequency and then
  // the current percentage.
  PDH_FMT_COUNTERVALUE counter_value;
  {
    if (const auto result = PdhGetFormattedCounterValue(
            processor_frequency_counter_, PDH_FMT_DOUBLE, &counter_type,
            &counter_value);
        result == ERROR_SUCCESS) {
      info.cpu_frequency = counter_value.doubleValue;

      if (const auto result2 = PdhGetFormattedCounterValue(
              processor_performance_counter_, PDH_FMT_DOUBLE, &counter_type,
              &counter_value);
          result2 == ERROR_SUCCESS) {
        info.cpu_frequency =
            info.cpu_frequency * (counter_value.doubleValue / 100.);
        SetTelemetryCapBit(CpuTelemetryCapBits::cpu_frequency);
      }
    }
  }

  // Sample cpu idle time, and compute cpu utilization using it (Windows 11 Fix)
  //
  // Beginning with Windows 11 22H2, the performance counters for CPU idle time
  // in SystemProcessorPerformanceInformation are broken and statistics derived
  // from those counters will always indicate single-digit cpu utilization %.
  //
  // Idle time reported in SystemProcessorIdleInformation is still consistent on
  // all versions of Windows, as well as WMI provisioned "Processor\% Idle Time".
  //
  // To measure CPU utilization accurately on all systems, it must be calculated:
  // 
  //    100.0 - "Processor(_Total)\% Idle Time"
  {
    if (const auto result =
            PdhGetFormattedCounterValue(processor_idle_time_counter_, PDH_FMT_DOUBLE,
                                        &counter_type, &counter_value);
        result == ERROR_SUCCESS) {
      info.cpu_utilization = 100.0 - counter_value.doubleValue;
      SetTelemetryCapBit(CpuTelemetryCapBits::cpu_utilization);
    }
  }

  // insert telemetry into history
  std::lock_guard lock{history_mutex_};
  history_.Push(info);

  // Update the next sample qpc based on the current sample qpc
  // and adding in the frequency
  next_sample_qpc_.QuadPart = qpc.QuadPart + frequency_.QuadPart;

  return true;
}

std::optional<CpuTelemetryInfo> WmiCpu::GetClosest(uint64_t qpc)
      const noexcept {
  std::lock_guard lock{history_mutex_};
  return history_.GetNearest(qpc);
}

}