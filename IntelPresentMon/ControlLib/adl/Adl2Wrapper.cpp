// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Adl2Wrapper.h"

namespace pmon::tel::adl {

  void* __stdcall ADL_Main_Memory_Alloc(int iSize) {
    void* lpBuffer = malloc(iSize);
    return lpBuffer;
  }

  Adl2Wrapper::Adl2Wrapper() {
    // get init proc, throw if not found
    const auto adl2_main_control_create_ptr =
        static_cast<int (*)(ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE*)>(
            dll.GetProcAddress("ADL2_Main_Control_Create"));
    if (!adl2_main_control_create_ptr) {
      throw Except<TelemetrySubsystemAbsent>(
          "Failed to get ADL2 main control create proc");
    }

    // get shutdown proc, but don't throw if not found (non-critical)
    // TODO: log error if not found
    ADL2_Main_Control_Destroy_ptr_ = static_cast<int (*)(ADL_CONTEXT_HANDLE)>(
            dll.GetProcAddress("ADL2_Main_Control_Destroy"));

    // try and get all other procs, but don't throw if not found
    // TODO: log error for any not found
#define X_(name, ...) p##name = static_cast<decltype(p##name)>(dll.GetProcAddress("ADL2_" #name));
    AMD_ADL2_ENDPOINT_LIST
#undef X_

    // Initialize ADL2. The second parameter is 1, which means:
    // retrieve adapter information only for adapters that are physically
    // present and enabled in the system
    if (!Ok(adl2_main_control_create_ptr(ADL_Main_Memory_Alloc, 1,
                                         &adl_context_))) {
      throw Except<TelemetrySubsystemAbsent>("ADL2 initialization call failed");
    }
  }

  Adl2Wrapper::~Adl2Wrapper() {
    if (ADL2_Main_Control_Destroy_ptr_ && adl_context_) {
      // TODO: log failure of this function
      ADL2_Main_Control_Destroy_ptr_(adl_context_);
    }
  }

  // Definition of wrapper functions
#define X_(name, ...)                                           \
  int Adl2Wrapper::name(NVW_ARGS(__VA_ARGS__)) const noexcept { \
    if (!p##name) {                                             \
      return ADL_ERR;                                           \
    }                                                           \
    return p##name(adl_context_,NVW_NAMES(__VA_ARGS__));        \
  }
  AMD_ADL2_ENDPOINT_LIST
#undef X_
} // namespace pmon::tel::adl
