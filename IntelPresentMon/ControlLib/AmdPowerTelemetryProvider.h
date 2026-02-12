// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#define NOMINMAX
#include <Windows.h>
#include <memory>
#include <mutex>
#include <vector>
#include "PresentMonPowerTelemetry.h"
#include "PowerTelemetryProvider.h"
#include "Adl2Wrapper.h"
#include "DeviceIdAllocator.h"

namespace pwr::amd {
class AmdPowerTelemetryProvider : public PowerTelemetryProvider {
 public:
  explicit AmdPowerTelemetryProvider(DeviceIdAllocator& allocator);
  AmdPowerTelemetryProvider(const AmdPowerTelemetryProvider& t) = delete;
  AmdPowerTelemetryProvider& operator=(const AmdPowerTelemetryProvider& t) = delete;
  ~AmdPowerTelemetryProvider() override;
  const std::vector<std::shared_ptr<PowerTelemetryAdapter>>&
  GetAdapters() noexcept override;
  uint32_t GetAdapterCount() const noexcept override;

 private:
  Adl2Wrapper adl_;
  std::vector<AdapterInfo> adl_adapter_infos_;
  std::vector<std::shared_ptr<PowerTelemetryAdapter>> adapter_ptrs_;
};
}  // namespace pwr::amd
