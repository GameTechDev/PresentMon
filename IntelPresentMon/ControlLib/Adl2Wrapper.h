// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "adl_sdk.h"
#include "DllModule.h"
#include "MacroHelpers.h"


// goals: single source of truth, automatic id lookup, parameter names in
// intellisense, easy updates means: x-macros, macro length overload w/ pairwise
// operation (up to 6 params)
#define AMD_ADL2_ENDPOINT_LIST                                                 \
  X_(Adapter_NumberOfAdapters_Get, int*, lpNumAdapters)                        \
  X_(Adapter_AdapterInfo_Get, LPAdapterInfo, lpInfo, int, iInputSize)          \
  X_(Adapter_Active_Get, int, iAdapterIndex, int*, lpStatus)                   \
  X_(Adapter_VRAMUsage_Get, int, iAdapterIndex, int*, iVRAMUsageInMB)          \
  X_(Adapter_MemoryInfoX4_Get, int, iAdapterIndex, ADLMemoryInfoX4*,           \
     lpMemoryInfoX4)                                                           \
  X_(Overdrive_Caps, int, iAdapterIndex, int*, iSupported, int*, iEnabled,     \
     int*, iVersion)                                                           \
  X_(Overdrive5_ThermalDevices_Enum, int, iAdapterIndex, int,                  \
     iThermalControllerIndex, ADLThermalControllerInfo*,                       \
     lpThermalControllerInfo)                                                  \
  X_(Overdrive5_ODParameters_Get, int, iAdapterIndex, ADLODParameters*,        \
     lpOdParameters)                                                           \
  X_(Overdrive5_Temperature_Get, int, iAdapterIndex, int,                      \
     iThermalControllerIndex, ADLTemperature*, lpTemperature)                  \
  X_(Overdrive5_FanSpeed_Get, int, iAdapterIndex, int,                         \
     iThermalControllerIndex, ADLFanSpeedValue*, lpFanSpeedValue)              \
  X_(Overdrive5_FanSpeedInfo_Get, int, iAdapterIndex, int,                     \
     iThermalControllerIndex, ADLFanSpeedInfo*, lpFanSpeedInfo)                \
  X_(Overdrive5_ODPerformanceLevels_Get, int, iAdapterIndex, int, iDefault,    \
     ADLODPerformanceLevels*, lpOdPerformanceLevels)                           \
  X_(Overdrive5_CurrentActivity_Get, int, iAdapterIndex, ADLPMActivity*,       \
     lpActivity)                                                               \
  X_(Overdrive5_PowerControl_Caps, int, iAdapterIndex, int*, lpSupported)      \
  X_(Overdrive5_PowerControlInfo_Get, int, iAdapterIndex,                      \
     ADLPowerControlInfo*, lpPowerControlInfo)                                 \
  X_(Overdrive5_PowerControl_Get, int, iAdapterIndex, int*, lpCurrentValue,    \
     int*, lpDefaultValue)                                                     \
  X_(Overdrive6_FanSpeed_Get, int, iAdapterIndex, ADLOD6FanSpeedInfo*,         \
     lpFanSpeedInfo)                                                           \
  X_(Overdrive6_ThermalController_Caps, int, iAdapterIndex,                    \
     ADLOD6ThermalControllerCaps*, lpThermalControllerCaps)                    \
  X_(Overdrive6_Temperature_Get, int, iAdapterIndex, int*, lpTemperature)      \
  X_(Overdrive6_Capabilities_Get, int, iAdapterIndex, ADLOD6Capabilities*,     \
     lpODCapabilities)                                                         \
  X_(Overdrive6_StateInfo_Get, int, iAdapterIndex, int, iStateType,            \
     ADLOD6StateInfo*, lpStateInfo)                                            \
  X_(Overdrive6_CurrentStatus_Get, int, iAdapterIndex, ADLOD6CurrentStatus*,   \
     lpCurrentStatus)                                                          \
  X_(Overdrive6_PowerControl_Caps, int, iAdapterIndex, int*, lpSupported)      \
  X_(Overdrive6_PowerControlInfo_Get, int, iAdapterIndex,                      \
     ADLOD6PowerControlInfo*, lpPowerControlInfo)                              \
  X_(Overdrive6_PowerControl_Get, int, iAdapterIndex, int*, lpCurrentValue,    \
     int*, lpDefaultValue)                                                     \
  X_(Overdrive6_CurrentPower_Get, int, iAdapterIndex, int, iPowerType, int*,   \
     lpCurrentValue)                                                           \
  X_(New_QueryPMLogData_Get, int, iAdapterIndex, ADLPMLogDataOutput*,          \
     lpDataOutput)                                                             \
  X_(OverdriveN_CapabilitiesX2_Get, int, iAdapterIndex, ADLODNCapabilitiesX2*, \
     lpOdCapabilitiesX2)                                                       \
  X_(OverdriveN_PerformanceStatus_Get, int, iAdapterIndex,                     \
     ADLODNPerformanceStatus*, lpODPerformanceStatus)                          \
  X_(OverdriveN_Temperature_Get, int, iAdapterIndex, int, iTempType, int*,     \
     lpTemperature)                                                            \
  X_(OverdriveN_FanControl_Get, int, iAdapterIndex, ADLODNFanControl*,         \
     lpOdFanControl)

namespace pwr::amd {

class Adl2Wrapper {
 public:
  Adl2Wrapper();
  Adl2Wrapper(const Adl2Wrapper& t) = delete;
  Adl2Wrapper& operator=(const Adl2Wrapper& t) = delete;
  ~Adl2Wrapper();

  static bool Ok(int sta) noexcept { return sta == ADL_OK; }
  // endpoint wrapper functions
#define X_(name, ...) int name(NVW_ARGS(__VA_ARGS__)) const noexcept;
  AMD_ADL2_ENDPOINT_LIST
#undef X_
 private:

  DllModule dll{{"atiadlxx.dll", "atiadlxy.dll"}};
#define X_(name, ...) \
  int (*p##name)(ADL_CONTEXT_HANDLE context, NVW_ARGS(__VA_ARGS__)) = nullptr;
  AMD_ADL2_ENDPOINT_LIST
#undef X_
  // Private endpoint pointer to shutdown ADL2
  int (*ADL2_Main_Control_Destroy_ptr_)(ADL_CONTEXT_HANDLE) = nullptr;
  ADL_CONTEXT_HANDLE adl_context_ = nullptr;
  // Test function
};

}  // namespace pwr::amd