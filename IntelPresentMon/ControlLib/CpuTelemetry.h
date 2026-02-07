// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <bitset>
#include <vector>
#include <Wbemidl.h>
#include <comdef.h>
#include <wrl/client.h>
#include <stdexcept>
#include "CpuTelemetryInfo.h"

namespace pwr::cpu {

class CpuTelemetry {
 public:
  virtual ~CpuTelemetry() = default;
  virtual CpuTelemetryInfo Sample() noexcept = 0;
  void SetTelemetryCapBit(CpuTelemetryCapBits telemetryCapBit) noexcept
  {
      cpuTelemetryCapBits_.set(static_cast<size_t>(telemetryCapBit));
  }
  std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)> GetCpuTelemetryCapBits()
  {
      return cpuTelemetryCapBits_;
  }

  std::string GetCpuName();
  double GetCpuPowerLimit() { return 0.; }
  
 private:

  bool ExecuteWQLProcessorNameQuery(std::wstring& processor_name);

  std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
      cpuTelemetryCapBits_{};
  std::string cpu_name_;
};
}
