// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <mutex>
#include <source_location>
#include "PowerTelemetryAdapter.h"
#include "TelemetryHistory.h"
#include "Adl2Wrapper.h"

namespace pwr::amd {
struct AmdCheckerToken {};
extern AmdCheckerToken chk;
struct AmdResultGrabber {
  AmdResultGrabber(int result,
            std::source_location = std::source_location::current()) noexcept;
  int result_;
  std::source_location loc_;
};
int operator>>(AmdResultGrabber, AmdCheckerToken) noexcept;

class AmdPowerTelemetryAdapter : public PowerTelemetryAdapter {
 public:
  AmdPowerTelemetryAdapter(const Adl2Wrapper* adl_wrapper, std::string adl_adapter_name, 
                           int adl_adapter_index, int overdrive_version);
  bool Sample() noexcept override;
  std::optional<PresentMonPowerTelemetryInfo> GetClosest(
      uint64_t qpc) const noexcept override;
  PM_DEVICE_VENDOR GetVendor() const noexcept override;
  std::string GetName() const noexcept override;
  uint64_t GetDedicatedVideoMemory() const noexcept override;
  uint64_t GetVideoMemoryMaxBandwidth() const noexcept override;
  double GetSustainedPowerLimit() const noexcept override;

 private:
  bool Overdrive5Sample(PresentMonPowerTelemetryInfo& info) noexcept;
  bool Overdrive6Sample(PresentMonPowerTelemetryInfo& info) noexcept;
  bool Overdrive7Sample(PresentMonPowerTelemetryInfo& info) noexcept;
  bool Overdrive8Sample(PresentMonPowerTelemetryInfo& info) noexcept;
  bool GetVideoMemoryInfo(uint64_t& gpu_mem_size, uint64_t& gpu_mem_max_bandwidth) const noexcept;
  bool GetSustainedPowerLimit(double& sustainedPowerLimit) const noexcept;

  const Adl2Wrapper* adl2_;
  int adl_adapter_index_ = 0;
  int overdrive_version_ = 0;
  std::string name_ = "Unknown Adapter Name";
  mutable std::mutex history_mutex_;
  TelemetryHistory<PresentMonPowerTelemetryInfo> history_{
      PowerTelemetryAdapter::defaultHistorySize};
};
}