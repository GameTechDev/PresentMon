// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ComManager.h"
#include "../../log/Log.h"
#include "../../Exception.h"
#include "Comdef.h"


namespace pmon::util::win::com
{
	using namespace ::pmon::util;

	ComManager::ComManager()
	{
        if (auto hr = CoInitializeEx(0, COINIT_MULTITHREADED); FAILED(hr))
        {
			pmlog_error("Failed to init COM").hr(hr);
			throw Except<Exception>();
		}

		if (auto hr = CoInitializeSecurity(
            nullptr,
            -1,                          // COM negotiates service
            nullptr,                     // Authentication services
            nullptr,                     // Reserved
            RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
            RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
            nullptr,                     // Authentication info
            EOAC_NONE,                   // Additional capabilities 
            nullptr                      // Reserved
        ); FAILED(hr) && hr != RPC_E_TOO_LATE)
		{
            CoUninitialize();
			pmlog_error("Failed to init COM security").hr(hr);
			throw Except<Exception>();
		}
	}

	ComManager::~ComManager()
	{
        CoUninitialize();
	}
}