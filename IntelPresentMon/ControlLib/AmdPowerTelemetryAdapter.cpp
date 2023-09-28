// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AmdPowerTelemetryAdapter.h"
#include "Logging.h"

#define GOOGLE_GLOG_DLL_DECL
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#define MAX_OD5_THERMAL_DEVICES 5;

namespace pwr::amd {
AmdCheckerToken chk;

AmdResultGrabber::AmdResultGrabber(int result,
                                   std::source_location loc) noexcept
    : result_(result), loc_(loc) {}
int operator>>(AmdResultGrabber g, AmdCheckerToken) {
  if (g.result_ != ADL_OK) {
    LOG(INFO) << "Failed Telemetry Query: " << g.loc_.file_name() << " "
              << g.loc_.line() << " " << g.result_;
  }
  return g.result_;
}

AmdPowerTelemetryAdapter::AmdPowerTelemetryAdapter(
    const Adl2Wrapper* adl2_wrapper, std::string adl_adapter_name,
    int adl_adapter_index, int overdrive_version)
    : adl2_(adl2_wrapper),
      name_(std::move(adl_adapter_name)),
      adl_adapter_index_(adl_adapter_index),
      overdrive_version_(overdrive_version) {}

uint64_t AmdPowerTelemetryAdapter::GetDedicatedVideoMemory() const noexcept {
  uint64_t video_mem_size = 0;
  ADLMemoryInfoX4 memory_info;
  if (adl2_->Ok(adl2_->Adapter_MemoryInfoX4_Get(adl_adapter_index_, &memory_info))) {
    // iMemorySize is the memory size in bytes
    video_mem_size = static_cast<uint64_t>(memory_info.iMemorySize);
  }
  return video_mem_size;
}

bool AmdPowerTelemetryAdapter::Sample() noexcept {
  LARGE_INTEGER qpc;
  QueryPerformanceCounter(&qpc);

  PresentMonPowerTelemetryInfo info{
      .qpc = (uint64_t)qpc.QuadPart,
  };

  // Sample metrics based on supported overdrive version
  bool sample_return = false;
  if (overdrive_version_ == 5) {
    sample_return = Overdrive5Sample(info);
  } else if (overdrive_version_ == 6) {
    sample_return = Overdrive6Sample(info);
  } else if (overdrive_version_ == 7) {
    sample_return = Overdrive7Sample(info);
  } else {
    sample_return = Overdrive8Sample(info);
  }

  // Next sample telemery data that is common. Starting with VRAM usage
  {
    int vram_usage = 0;
    auto result =
        adl2_->Adapter_VRAMUsage_Get(adl_adapter_index_, &vram_usage) >> chk;
    // ADL Adapter_VRAMUsage_Get returns VRAM memory usage in MB
    if (adl2_->Ok(result)) {
      info.gpu_mem_used_b = static_cast<uint64_t>(vram_usage) * 1000000;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_used);
    }
  }

  // Next sample memory usage
  { 
    ADLMemoryInfoX4 memory_info;
    auto result =
        adl2_->Adapter_MemoryInfoX4_Get(adl_adapter_index_, &memory_info) >>
        chk;
    if (adl2_->Ok(result)) {
      // iMemoryBandwidthX2 does not specify size but iMemoryBandwith
      // returns megabytes per second. Assuming they are the same.
      info.gpu_mem_max_bandwidth_bps =
          static_cast<uint64_t>(memory_info.iMemoryBandwidthX2) * 1000000;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_max_bandwidth);
      // iMemorySize is the memory size in bytes
      info.gpu_mem_total_size_b = static_cast<uint64_t>(memory_info.iMemorySize);
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_size);
    }
  }

  // Insert telemetry into history
  std::lock_guard lock{history_mutex_};
  history_.Push(info);

  return sample_return;
}

bool AmdPowerTelemetryAdapter::Overdrive5Sample(
    PresentMonPowerTelemetryInfo& info) noexcept {
  // gpu temperature and fan speed
  {
    int result = 0;
    ADLThermalControllerInfo thermals = {.iSize =
                                             sizeof(ADLThermalControllerInfo)};
    // Call ADL_Overdrive5_ThermalDevices_Enum(). This is an interesting one as
    // the ADL specification states to "Set to 0" for the
    // iThermalControllerIndex but sample pieces of code in the SDK perform loop
    // over this call increasing the index until ADL_WARNING_NO_DATA is
    // returned. For now, follow the sample code and loop up to 5. We
    // will report the LAST successful enumeration for now.
    // TODO (megalvan) test this code out to see if 0 is the correct
    // value to send OR if we are supposed to iterate.
    for (int thermal_controller_index = 0; thermal_controller_index < 10;
         thermal_controller_index++) {
      result =
          adl2_->Overdrive5_ThermalDevices_Enum(
              adl_adapter_index_, thermal_controller_index, &thermals) >>
          chk;
      if (adl2_->Ok(result)) {
        if (thermals.iThermalDomain == ADL_DL_THERMAL_DOMAIN_GPU) {
          ADLTemperature temp = {.iSize = sizeof(ADLTemperature)};
          result = adl2_->Overdrive5_Temperature_Get(
                       adl_adapter_index_, thermal_controller_index, &temp) >>
                   chk;
          if (adl2_->Ok(result)) {
            // Temperature is returned in millidegrees Celsius
            info.gpu_temperature_c = (double)temp.iTemperature / 1000.;
            SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature);
          }
          ADLFanSpeedInfo fan_speed_info = {.iSize = sizeof(ADLFanSpeedInfo)};
          result = adl2_->Overdrive5_FanSpeedInfo_Get(adl_adapter_index_,
                                                      thermal_controller_index,
                                                      &fan_speed_info) >>
                   chk;
          if (adl2_->Ok(result)) {
            // Fan speed can be reported as RPMs or in percent. We only
            // support RPMs. Probably should report both and let user
            // select which they want.
            if ((fan_speed_info.iFlags & ADL_DL_FANCTRL_SUPPORTS_RPM_READ) ==
                ADL_DL_FANCTRL_SUPPORTS_RPM_READ) {
              ADLFanSpeedValue fan_speed_value = {
                  .iSize = sizeof(ADLFanSpeedValue),
                  .iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM};
              result = adl2_->Overdrive5_FanSpeed_Get(adl_adapter_index_,
                                                      thermal_controller_index,
                                                      &fan_speed_value) >>
                       chk;
              if (adl2_->Ok(result)) {
                info.fan_speed_rpm[thermal_controller_index] =
                    (double)fan_speed_value.iFanSpeed;
                // TODO -> create function to fix this!!
                SetTelemetryCapBit(GpuTelemetryCapBits::fan_speed_1);
              }
            }
          }
        }
      }
    }
  }

  // GPU, VRAM clock frequencies and GPU voltage
  {
    ADLPMActivity activity = {.iSize = sizeof(ADLPMActivity)};
    int result =
        adl2_->Overdrive5_CurrentActivity_Get(adl_adapter_index_, &activity) >>
        chk;
    if (adl2_->Ok(result)) {
      info.gpu_utilization = (double)activity.iActivityPercent;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_utilization);
      // It appears iEngineClock and iMemory clock come back as 10 Khz.
      // This is what the Overdrive6 API reports back and appears to be
      // confirmed from the Overdrive sample applications.
      info.gpu_frequency_mhz = (double)activity.iEngineClock / 100;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_frequency);
      info.vram_frequency_mhz = (double)activity.iMemoryClock / 100;
      SetTelemetryCapBit(GpuTelemetryCapBits::vram_frequency);
      // Overdrive sample application treats this as volts.
      info.gpu_voltage_v = (double)activity.iVddc;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_voltage);
    }
  }

  // Current Sustained Power Control limit
  {
    int power_control_supported = 0;
    int result = adl2_->Overdrive5_PowerControl_Caps(
                     adl_adapter_index_, &power_control_supported) >>
        chk;
    if (adl2_->Ok(result)) {
      if (power_control_supported) {
        int power_control_current = 0;
        int power_control_default = 0;
        result = adl2_->Overdrive5_PowerControl_Get(adl_adapter_index_,
                                                    &power_control_current,
                                                    &power_control_default) >>
                 chk;
        if (adl2_->Ok(result)) {
          // Not sure on return type, assuming watts
          info.gpu_sustained_power_limit_w = (double)power_control_current;
          SetTelemetryCapBit(GpuTelemetryCapBits::gpu_sustained_power_limit);
        }
      }
    }
  }

  // How does one make sure the OverdriveN functions are available??
  {
    ADLODNPerformanceStatus performance_status = {};
    int result = adl2_->OverdriveN_PerformanceStatus_Get(adl_adapter_index_,
                                                         &performance_status) >>
                 chk;
    if (adl2_->Ok(result)) {
      // Overdrive sample application treats this as volts.
      info.gpu_voltage_v = (double)performance_status.iVDDC;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_voltage);
    }
  }

  return true;
}

bool AmdPowerTelemetryAdapter::Overdrive6Sample(
    PresentMonPowerTelemetryInfo& info) noexcept {
  // gpu temperature and fan speed
  {
    ADLOD6ThermalControllerCaps thermals = {};
    int result = adl2_->Overdrive6_ThermalController_Caps(adl_adapter_index_,
                                                          &thermals) >>
                 chk;
    if (adl2_->Ok(result)) {
      // As with overdrive 5 we must check if reading of RPMs is allowed by the
      // gpu.
      if ((thermals.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_RPM_READ) ==
          ADL_OD6_TCCAPS_FANSPEED_RPM_READ) {
        ADLOD6FanSpeedInfo fan_speed_info = {};
        result = adl2_->Overdrive6_FanSpeed_Get(adl_adapter_index_,
                                                &fan_speed_info) >>
                 chk;
        if (adl2_->Ok(result)) {
          if ((fan_speed_info.iSpeedType & ADL_OD6_FANSPEED_TYPE_RPM) ==
              ADL_OD6_FANSPEED_TYPE_RPM) {
            info.fan_speed_rpm[0] = (double)fan_speed_info.iFanSpeedRPM;
            SetTelemetryCapBit(GpuTelemetryCapBits::fan_speed_1);
          }
        }
      }

      // Make sure thermal controller exists on the gpu
      if ((thermals.iCapabilities & ADL_OD6_TCCAPS_THERMAL_CONTROLLER) ==
          ADL_OD6_TCCAPS_THERMAL_CONTROLLER) {
        int temp = 0;
        result =
            adl2_->Overdrive6_Temperature_Get(adl_adapter_index_, &temp) >> chk;
        if (adl2_->Ok(result)) {
          // Temperature is returned in millidegrees Celsius
          info.gpu_temperature_c = (double)temp / 1000.;
          SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature);
        }
      }
    }
  }

  // gpu and vram frequencies, as well as gpu utilization
  {
    ADLOD6CurrentStatus currentStatus = {};
    int result = adl2_->Overdrive6_CurrentStatus_Get(adl_adapter_index_,
                                                     &currentStatus) >>
                 chk;
    if (adl2_->Ok(result)) {
      // iEngineClock and iMemory clock are returned as 10 Khz
      info.gpu_frequency_mhz =
          static_cast<double>(currentStatus.iEngineClock) / 100.;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_frequency);
      info.vram_frequency_mhz =
          static_cast<double>(currentStatus.iMemoryClock) / 100.;
      SetTelemetryCapBit(GpuTelemetryCapBits::vram_frequency);
      ADLOD6Capabilities caps = {};
      result =
          adl2_->Overdrive6_Capabilities_Get(adl_adapter_index_, &caps) >> chk;
      if (adl2_->Ok(result)) {
        if ((caps.iCapabilities & ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR) ==
            ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR) {
          info.gpu_utilization = (double)currentStatus.iActivityPercent;
          SetTelemetryCapBit(GpuTelemetryCapBits::gpu_utilization);
        }
      }
    }
  }

  // gpu power
  {
    int current_power_w = 0;
    int result = adl2_->Overdrive6_CurrentPower_Get(adl_adapter_index_, 0,
                                                    &current_power_w) >>
                 chk;
    if (adl2_->Ok(result)) {
      // The returned int contains the current power in Watts with 8
      // fractional bits
      info.gpu_power_w = (double)(current_power_w >> 8);
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_power);
    }
  }

  // sustained power control limit
  {
    int power_control_supported = 0;
    int result = adl2_->Overdrive6_PowerControl_Caps(
                     adl_adapter_index_, &power_control_supported) >>
                 chk;
    if (adl2_->Ok(result)) {
      if (power_control_supported) {
        int power_control_current = 0;
        int power_control_default = 0;
        result = adl2_->Overdrive6_PowerControl_Get(adl_adapter_index_,
                                                    &power_control_current,
                                                    &power_control_default) >>
                 chk;
        if (adl2_->Ok(result)) {
          info.gpu_sustained_power_limit_w = (double)power_control_current;
          SetTelemetryCapBit(GpuTelemetryCapBits::gpu_sustained_power_limit);
        }
      }
    }
  }

  return true;
}

bool AmdPowerTelemetryAdapter::Overdrive7Sample(
    PresentMonPowerTelemetryInfo& info) noexcept {

  ADLODNCapabilitiesX2 od_caps = {};
  int result =
      adl2_->OverdriveN_CapabilitiesX2_Get(adl_adapter_index_, &od_caps) >>
      chk;
  if (adl2_->Ok(result)) {
    ADLODNPerformanceStatus od_perf_status = {};
    result = adl2_->OverdriveN_PerformanceStatus_Get(adl_adapter_index_,
                                                     &od_perf_status) >>
             chk;
    if (adl2_->Ok(result)) {
      // iEngineClock and iMemory clock are returned as 10 Khz
      info.gpu_frequency_mhz = (double)od_perf_status.iCoreClock / 100.;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_frequency);
      info.vram_frequency_mhz = (double)od_perf_status.iMemoryClock / 100.;
      SetTelemetryCapBit(GpuTelemetryCapBits::vram_frequency);
      info.gpu_voltage_v = (double)od_perf_status.iVDDC / 1000.;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_voltage);
      info.gpu_utilization = (double)od_perf_status.iGPUActivityPercent;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_utilization);
    }
    int temperature = 0;
    result = adl2_->OverdriveN_Temperature_Get(adl_adapter_index_, 1,
                                               &temperature) >>
             chk;
    if (adl2_->Ok(result)) {
      // Temperature is returned in millidegrees Celsius
      info.gpu_temperature_c = (double)temperature / 1000.;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature);
    }
    ADLODNFanControl od_fan_control = {};
    result =
        adl2_->OverdriveN_FanControl_Get(adl_adapter_index_, &od_fan_control) >> chk;
    if (adl2_->Ok(result)) {
      info.fan_speed_rpm[0] = (double)od_fan_control.iCurrentFanSpeed;
      SetTelemetryCapBit(GpuTelemetryCapBits::fan_speed_0);
    }
  }

  // gpu power
  {
    int current_power_w = 0;
    int result = adl2_->Overdrive6_CurrentPower_Get(adl_adapter_index_, 0,
                                                    &current_power_w) >>
                 chk;
    if (adl2_->Ok(result)) {
      // The returned int contains the current power in Watts with 8
      // fractional bits
      info.gpu_power_w = (double)(current_power_w >> 8);
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_power);
    }
  }

  return true;
}

bool AmdPowerTelemetryAdapter::Overdrive8Sample(
    PresentMonPowerTelemetryInfo& info) noexcept {
  ADLPMLogDataOutput data_output = {};
  if (adl2_->Ok(adl2_->New_QueryPMLogData_Get(adl_adapter_index_,
                                              &data_output))) {
    if (data_output.sensors[PMLOG_CLK_GFXCLK].supported) {
      info.gpu_frequency_mhz = (double)data_output.sensors[PMLOG_CLK_GFXCLK].value;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_frequency);
    }
    if (data_output.sensors[PMLOG_CLK_MEMCLK].supported) {
      info.vram_frequency_mhz = (double)data_output.sensors[PMLOG_CLK_MEMCLK].value;
      SetTelemetryCapBit(GpuTelemetryCapBits::vram_frequency);
    }
    if (data_output.sensors[PMLOG_FAN_RPM].supported) {
      info.fan_speed_rpm[0] = (double)data_output.sensors[PMLOG_FAN_RPM].value;
      SetTelemetryCapBit(GpuTelemetryCapBits::fan_speed_0);
    }
    if (data_output.sensors[PMLOG_TEMPERATURE_EDGE].supported) {
      info.gpu_temperature_c =
          (double)data_output.sensors[PMLOG_TEMPERATURE_EDGE].value;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature);
    }
    if (data_output.sensors[PMLOG_INFO_ACTIVITY_GFX].supported) {
      info.gpu_utilization =
          (double)data_output.sensors[PMLOG_INFO_ACTIVITY_GFX].value;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_utilization);
    }
    if (data_output.sensors[PMLOG_GFX_VOLTAGE].supported) {
      info.gpu_voltage_v =
          (double)data_output.sensors[PMLOG_GFX_VOLTAGE].value / 1000.;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_voltage);
    }
    if (data_output.sensors[PMLOG_ASIC_POWER].supported) {
      info.gpu_power_w = (double)data_output.sensors[PMLOG_ASIC_POWER].value;
      SetTelemetryCapBit(GpuTelemetryCapBits::gpu_power);
    }
    return true;
  } else {
    return false;
  }
}

std::optional<PresentMonPowerTelemetryInfo> AmdPowerTelemetryAdapter::GetClosest(
    uint64_t qpc) const noexcept {
  std::lock_guard<std::mutex> lock(history_mutex_);
  return history_.GetNearest(qpc);
}

PM_GPU_VENDOR AmdPowerTelemetryAdapter::GetVendor() const noexcept {
  return PM_GPU_VENDOR::PM_GPU_VENDOR_AMD;
}

std::string AmdPowerTelemetryAdapter::GetName() const noexcept {
  return name_;
}

}  // namespace pwr::amd