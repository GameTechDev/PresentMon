// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <optional>
#include <bitset>
#include <vector>
#include <Wbemidl.h>
#include <comdef.h>
#include <wrl/client.h>
#include <stdexcept>
#include "CpuTelemetryInfo.h"
#include "..\PresentMonUtils\StringUtils.h"

namespace pwr::cpu {

class CpuTelemetry {
 public:
  virtual ~CpuTelemetry() = default;
  virtual bool Sample() noexcept = 0;
  virtual std::optional<CpuTelemetryInfo> GetClosest(
      uint64_t qpc) const noexcept = 0;
  void SetTelemetryCapBit(CpuTelemetryCapBits telemetryCapBit) noexcept
  {
      cpuTelemetryCapBits_.set(static_cast<size_t>(telemetryCapBit));
  }
  std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)> GetCpuTelemetryCapBits()
  {
      return cpuTelemetryCapBits_;
  }

  std::string GetCpuName();

  // constants
  static constexpr size_t defaultHistorySize = 300;
  // data
 private:

  bool ExecuteWQLProcessorNameQuery(std::wstring& processor_name);

  std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
      cpuTelemetryCapBits_{};
  std::string cpu_name_;
};
}