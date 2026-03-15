// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#define NOMINMAX
#include <Windows.h>
#include "../DllModule.h"
#include "../MacroHelpers.h"
#include "nvapi.h"

// goals: single source of truth, automatic id lookup, parameter names in intellisense, easy updates
// means: x-macros, macro length overload w/ pairwise operation (up to 6 params)

#define NVW_NVAPI_ENDPOINT_LIST \
X_(EnumNvidiaDisplayHandle, NvU32, thisEnum, NvDisplayHandle*, pNvDispHandle) \
X_(EnumPhysicalGPUs, NvPhysicalGpuHandle*, pNvGPUHandleArray, NvU32*, pCount) \
X_(GPU_GetPstatesInfoEx, NvPhysicalGpuHandle, hPhysicalGpu, NV_GPU_PERF_PSTATES_INFO*, pPerfPstatesInfo, NvU32, inputFlags) \
X_(GPU_GetAllClockFrequencies, NvPhysicalGpuHandle, hPhysicalGPU, NV_GPU_CLOCK_FREQUENCIES*, pClkFreqs) \
X_(GPU_GetDynamicPstatesInfoEx, NvPhysicalGpuHandle, hPhysicalGpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX*, pDynamicPstatesInfoEx) \
X_(GPU_GetThermalSettings, NvPhysicalGpuHandle, hPhysicalGpu, NvU32, sensorIndex, NV_GPU_THERMAL_SETTINGS*, pThermalSettings) \
X_(GPU_GetMemoryInfo, NvPhysicalGpuHandle, hPhysicalGpu, NV_DISPLAY_DRIVER_MEMORY_INFO*, pMemoryInfo) \
X_(GPU_GetTachReading, NvPhysicalGpuHandle, hPhysicalGPU, NvU32*, pValue) \
X_(GPU_GetFullName, NvPhysicalGpuHandle, hPhysicalGpu, NvAPI_ShortString, szName) \
X_(GPU_GetPCIIdentifiers, NvPhysicalGpuHandle, hPhysicalGpu, NvU32*, pDeviceId, NvU32*, pSubSystemId, NvU32*, pRevisionId, NvU32*, pExtDeviceId) \
X_(GPU_GetBusId, NvPhysicalGpuHandle, hPhysicalGpu, NvU32*, pBusId)


namespace pmon::tel::nvapi
{
	// utilization domains are not defined in the public nvapi header
	enum NVAPI_GPU_UTILIZATION_DOMAIN
	{
		NVAPI_GPU_UTILIZATION_DOMAIN_GPU,
		NVAPI_GPU_UTILIZATION_DOMAIN_FB,
		NVAPI_GPU_UTILIZATION_DOMAIN_VID,
		NVAPI_GPU_UTILIZATION_DOMAIN_BUS,
	};

	class NvapiWrapper
	{
	public:
		NvapiWrapper();
        NvapiWrapper(const NvapiWrapper& t) = delete;
        NvapiWrapper& operator=(const NvapiWrapper& t) = delete;
		~NvapiWrapper();
		// endpoint wrapper functions
#define X_(name, ...) NvAPI_Status name(NVW_ARGS(__VA_ARGS__)) const noexcept;
		NVW_NVAPI_ENDPOINT_LIST
#undef X_
		static bool Ok(NvAPI_Status sta) noexcept { return sta == NVAPI_OK; }
		
	private:
		DllModule dll{ { "nvapi64.dll", "nvapi32.dll" } };
		// endpoint pointers
#define X_(name, ...) NvAPI_Status (*p##name)(NVW_ARGS(__VA_ARGS__)) = nullptr;
		NVW_NVAPI_ENDPOINT_LIST
#undef X_
		// private endpoint pointer
		NvAPI_Status (*pUnload)() = nullptr;
	};
}
