// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include <format>
#include <map>
#include "AmdPowerTelemetryProvider.h"
#include "AmdPowerTelemetryAdapter.h"
#include "Logging.h"

#define AMDVENDORID (1002)

namespace pwr::amd {

AmdPowerTelemetryProvider::AmdPowerTelemetryProvider() {
  int count = 0;
  if (!adl_.Ok(adl_.Adapter_NumberOfAdapters_Get(&count))) {
    throw std::runtime_error{"Failed to get number of amd gpus"};
  }
  // Resize the container to the actual count of adl adapter infos
  adl_adapter_infos_.resize(count);

  // Get the adapter infos
  int full_adapter_info_size =
      (int)(adl_adapter_infos_.size() * sizeof(AdapterInfo));
  if (!adl_.Ok(adl_.Adapter_AdapterInfo_Get(adl_adapter_infos_.data(),
                                            full_adapter_info_size))) {
      throw std::runtime_error{"Failed to adapter infos for amd gpus"};
  }

  // Map of bus number to adapter string to track unique adapters
  std::map<int, std::string> adapters;

  for (auto& ai : adl_adapter_infos_) {
    // Following the AMD example code for using Overdrive8 here. Instead of making
    // a call to ADL2_Adapter_Active_Get the sample code checks the bus number. We
    // are following this methodology as well because when making this call from
    // the service ADL2_Adapter_Active_Get never returns an active adapter while
    // the bus number is active and Overdrive caps report support.
    if ((ai.iBusNumber > -1) && ai.iVendorID == AMDVENDORID) {
      int overdrive_supported = 0;
      int overdrive_enabled = 0;
      int overdrive_version = 0;
      // Again following the AMD Overdrive8 sample code. The code does not
      // check for success on the Overdrive_Caps call. Instead just checks
      // the supported overdrive version.
      adl_.Overdrive_Caps(ai.iAdapterIndex, &overdrive_supported,
                          &overdrive_enabled, &overdrive_version);

      // If we don't get updated values for ANY of the passed in parameters
      // bail out.
      if ((overdrive_supported == 0) && (overdrive_enabled == 0) &&
          (overdrive_version == 0)) {
        throw std::runtime_error{"Failed to get overdrive caps for amd gpus"};
      }

      // If it's not a version of overdrive we support, bail out.
      if ((overdrive_version < 5) || (overdrive_version > 8)) {
        throw std::runtime_error{
            std::format(
                "Overdrive version is not supported. Version reported: {}",
                overdrive_version)
                .c_str()};
      }

      auto result =
          adapters.insert(std::pair{ai.iBusNumber, ai.strAdapterName});
      if (result.second) {
        try {
          std::string adl_adapter_name = ai.strAdapterName;
          adapter_ptrs_.push_back(std::make_shared<AmdPowerTelemetryAdapter>(
              &adl_, adl_adapter_name, ai.iAdapterIndex, overdrive_version));
          // If we are able to create a telemetry adapter we are done.
          break;
        } catch (const std::exception& e) {
          TELE_ERR(e.what());
        } catch (...) {
          TELE_ERR("unknown error");
        }
      }
    }
  }
}

AmdPowerTelemetryProvider::~AmdPowerTelemetryProvider() {
  adapter_ptrs_.clear();
}

const std::vector<std::shared_ptr<PowerTelemetryAdapter>>&
AmdPowerTelemetryProvider::GetAdapters() noexcept {
  return adapter_ptrs_;
}

uint32_t AmdPowerTelemetryProvider::GetAdapterCount() const noexcept {
  return (uint32_t)adapter_ptrs_.size();
}

}  // namespace pwr::amd