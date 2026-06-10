// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NvapiWrapper.h"
#include "nvapi_interface_table.h"
#include <algorithm>
#include <string>

namespace pmon::tel::nvapi
{
	NvapiWrapper::NvapiWrapper()
	{
		// Nvapi has it's own method for finding interfaces, so we need to get the proc first
		const auto QueryInterface = static_cast<void*(*)(unsigned int)>(dll.GetProcAddress("nvapi_QueryInterface"));
		if (!QueryInterface)
		{
			throw Except<TelemetrySubsystemAbsent>("Failed to get nvapi_QueryInterface proc");
		}

		// Query for all endpoints in NVW_ENDPOINT_LIST using the QueryInterface endpoint
		// do compile-time lookup into constexpr array to get the enpoint ids
		const auto LoadEndpoint = [QueryInterface](const std::string& endpointName) -> void* {
			if (const auto i = std::ranges::find(nvapi_interface_table, endpointName, &NVAPI_INTERFACE_TABLE_ENTRY::func);
				i != std::end(nvapi_interface_table))
			{
				// TODO: log error that endpoint not found after query (if Query returns nullptr)
				return QueryInterface(i->id);
			}
			else
			{
				// TODO: log error that endpoint not found in ID lookup table
				return nullptr;
			}
		};
#define X_(name, ...) p##name = static_cast<decltype(p##name)>(LoadEndpoint("NvAPI_"#name));
		NVW_NVAPI_ENDPOINT_LIST
#undef X_

		// try to initialize the api
		// if we are unable, abort wrapper construction with exception
		if (const auto pInitialize = static_cast<NvAPI_Status(*)()>(LoadEndpoint("NvAPI_Initialize")))
		{
			if (!Ok(pInitialize()))
			{
				throw Except<TelemetrySubsystemAbsent>("NvAPI_Initialize call failed");
			}
		}
		else
		{
			throw Except<TelemetrySubsystemAbsent>("Failed to query NvAPI_Initialize endpoint");
		}

		// load the private endpoint for unloading api
		// TODO: log if we cannot find this endpoint, but don't throw
		pUnload = static_cast<NvAPI_Status(*)()>(LoadEndpoint("NvAPI_Unload"));
	}

	NvapiWrapper::~NvapiWrapper()
	{
		if (pUnload)
		{
			// TODO: log failure of this function
			pUnload();
		}
	}

	// definition of wrapper functions
	// calls function pointer if exists, otherwise return NvAPI error [NVAPI_FUNCTION_NOT_FOUND]
#define X_(name, ...) NvAPI_Status NvapiWrapper::name(NVW_ARGS(__VA_ARGS__)) const noexcept \
{ \
	if (!p##name) { return NVAPI_FUNCTION_NOT_FOUND; } \
	return p##name(NVW_NAMES(__VA_ARGS__)); \
}
	NVW_NVAPI_ENDPOINT_LIST
#undef X_
}
