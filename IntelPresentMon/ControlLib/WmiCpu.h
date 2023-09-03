// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#define NOMINMAX
#include <Windows.h>
#include <pdh.h>
#include "CpuTelemetry.h"
#include "TelemetryHistory.h"
#include <mutex>
#include <optional>

namespace pwr::cpu::wmi {

struct PDHQueryDeleter {
  void operator()(PDH_HQUERY query) const { PdhCloseQuery(query); }
};

class WmiCpu : public CpuTelemetry {
 public:
  WmiCpu();
  bool Sample() noexcept override;
  std::optional<CpuTelemetryInfo> GetClosest(
      uint64_t qpc) const noexcept override;
  // types
  class NonGraphicsDeviceException : public std::exception {};

 private:

  // data
  std::unique_ptr<std::remove_pointer_t<PDH_HQUERY>, PDHQueryDeleter> query_;
  HCOUNTER processor_frequency_counter_;
  HCOUNTER processor_performance_counter_;
  HCOUNTER processor_idle_time_counter_;
  LARGE_INTEGER next_sample_qpc_ = {};
  LARGE_INTEGER frequency_ = {};
  std::string cpu_name_;

  mutable std::mutex history_mutex_;
  TelemetryHistory<CpuTelemetryInfo> history_{CpuTelemetry::defaultHistorySize};
};

}  // namespace pwr::cpu::wmi