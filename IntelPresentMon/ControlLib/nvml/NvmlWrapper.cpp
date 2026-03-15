// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NvmlWrapper.h"

namespace pmon::tel::nvml
{
	NvmlWrapper::NvmlWrapper()
	{
		// get init proc, throw if not found
		const auto pInit_v2 = static_cast<nvmlReturn_t(*)()>(dll.GetProcAddress("nvmlInit_v2"));
		if (!pInit_v2)
		{
			throw std::runtime_error{ "Failed to get init proc for nvml" };
		}

		// get shutdown proc, but don't throw if not found (non-critical)
		// TODO: log error if not found
		pShutdown = static_cast<decltype(pShutdown)>(dll.GetProcAddress("nvmlShutdown"));

		// try and get all other procs, but don't throw if not found
		// TODO: log error for any not found
#define X_(name, ...) p##name = static_cast<decltype(p##name)>(dll.GetProcAddress("nvml"#name));
		NVW_NVML_ENDPOINT_LIST
#undef X_

		// initialize nvml
		if (!Ok(pInit_v2()))
		{
			throw std::runtime_error{ "nvml init call failed" };
		}
	}

	NvmlWrapper::~NvmlWrapper()
	{
		if (pShutdown)
		{
			// TODO: log failure of this function
			pShutdown();
		}
	}

	// definition of wrapper functions
	// calls function pointer if exists, otherwise return NVML error [NVML_ERROR_FUNCTION_NOT_FOUND]
#define X_(name, ...) nvmlReturn_t NvmlWrapper::name(NVW_ARGS(__VA_ARGS__)) const noexcept \
{ \
	if (!p##name) { return nvmlReturn_t::NVML_ERROR_FUNCTION_NOT_FOUND; } \
	return p##name(NVW_NAMES(__VA_ARGS__)); \
}
	NVW_NVML_ENDPOINT_LIST
#undef X_
}
