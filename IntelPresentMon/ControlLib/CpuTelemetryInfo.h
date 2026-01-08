// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <bitset>

struct CpuTelemetryInfo {
  uint64_t qpc;

  double cpu_utilization;
  double cpu_power_w;
  double cpu_power_limit_w;
  double cpu_temperature;
  double cpu_frequency;
};

enum class CpuTelemetryCapBits {
  cpu_utilization,
  cpu_power,
  cpu_power_limit,
  cpu_temperature,
  cpu_frequency,
  cpu_core_utility,
  cpu_telemetry_count
};

using CpuTelemetryBitset = std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>;