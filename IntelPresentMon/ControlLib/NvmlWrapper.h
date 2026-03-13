// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "nvml.h"
#include "DllModule.h"
#include "MacroHelpers.h"

// goals: single source of truth, automatic id lookup, parameter names in intellisense, easy updates
// means: x-macros, macro length overload w/ pairwise operation (up to 6 params)

#define NVW_NVML_ENDPOINT_LIST \
X_(DeviceGetCount_v2, unsigned int*, deviceCount) \
X_(DeviceGetHandleByIndex_v2, unsigned int, index, nvmlDevice_t*, device) \
X_(DeviceGetName, nvmlDevice_t, device, char*, name, unsigned int, length) \
X_(DeviceGetPciInfo_v3, nvmlDevice_t, device, nvmlPciInfo_t*, pci) \
X_(DeviceGetClock, nvmlDevice_t, device, nvmlClockType_t, clockType, nvmlClockId_t, clockId, unsigned int*, clockMHz) \
X_(DeviceGetMemoryInfo, nvmlDevice_t, device, nvmlMemory_t*, memory) \
X_(DeviceGetPowerUsage, nvmlDevice_t, device, unsigned int*, power) \
X_(DeviceGetUtilizationRates, nvmlDevice_t, device, nvmlUtilization_t*, utilization) \
X_(DeviceGetTemperature, nvmlDevice_t, device, nvmlTemperatureSensors_t, sensorType, unsigned int*, temp) \
X_(DeviceGetSamples, nvmlDevice_t, device, nvmlSamplingType_t, type, unsigned long long, lastSeenTimeStamp, nvmlValueType_t*, sampleValType, unsigned int*, sampleCount, nvmlSample_t*, samples) \
X_(DeviceGetEnforcedPowerLimit, nvmlDevice_t, device, unsigned int*, limit) \
X_(DeviceGetPowerManagementLimit, nvmlDevice_t, device, unsigned int*, limit) \
X_(DeviceGetViolationStatus, nvmlDevice_t, device, nvmlPerfPolicyType_t, perfPolicyType, nvmlViolationTime_t*, violTime)

namespace pwr::nv
{
	struct NvmlAdapterSignature
	{
		uint32_t busIdDomain;
		uint32_t busIdBus;
		uint32_t pciDeviceId;
		uint32_t pciSubSystemId;
	};

	class NvmlWrapper
	{
	public:
		NvmlWrapper();
		~NvmlWrapper();
		NvmlAdapterSignature GetAdapterSignature(nvmlDevice_t adapter) const;
		static bool Ok(nvmlReturn_t sta) noexcept { return sta == nvmlReturn_t::NVML_SUCCESS; }
		// endpoint wrapper functions
#define X_(name, ...) nvmlReturn_t name(NVW_ARGS(__VA_ARGS__)) const noexcept;
		NVW_NVML_ENDPOINT_LIST
#undef X_

	private:
		// data
		DllModule dll{ { "nvml.dll" } };
		// endpoint pointers
#define X_(name, ...) nvmlReturn_t (*p##name)(NVW_ARGS(__VA_ARGS__)) = nullptr;
		NVW_NVML_ENDPOINT_LIST
#undef X_
		// private endpoint pointer
		nvmlReturn_t(*pShutdown)() = nullptr;
	};
}
