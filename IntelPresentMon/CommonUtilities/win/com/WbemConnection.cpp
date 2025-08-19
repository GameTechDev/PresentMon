// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WbemConnection.h"
#include "Comdef.h"
#include <Wbemidl.h>
#include "../../log/Log.h"
#include "../../Exception.h"
#include "WbemListener.h"

#pragma comment(lib, "wbemuuid.lib")

namespace pmon::util::win::com
{
	using namespace ::pmon::util;

	WbemConnection::WbemConnection()
	{
		// Obtain the initial locator to WMI
		ComPtr<IWbemLocator> pLocator;
		if (auto hr = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pLocator)
		); FAILED(hr))
		{
			pmlog_error("Failed to create wbem locator").hr(hr);
			throw Except<Exception>();
		}

		// Connect to WMI through the IWbemLocator::ConnectServer method
		// Connect to the local root\cimv2 namespace
		// and obtain pointer pConnection to make IWbemServices calls.
		if (auto hr = pLocator->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"),
			nullptr,
			nullptr,
			0,
			0,
			0,
			0,
			&pConnection
		); FAILED(hr))
		{
			pmlog_error("Failed to connect to wbem connection").hr(hr);
			throw Except<Exception>();
		}

		// Set security levels on the proxy (pConnection)
		if (auto hr = CoSetProxyBlanket(
			pConnection.Get(),           // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx 
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx 
			nullptr,                     // Server principal name 
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			nullptr,                     // client identity
			EOAC_NONE                    // proxy capabilities 
		); FAILED(hr))
		{
			pmlog_error("Failed to set proxy security blanket on wbem connection").hr(hr);
			throw Except<Exception>();
		}
	}
	std::unique_ptr<WbemListener> WbemConnection::CreateListener_(WbemSink* pSink)
	{
		return std::make_unique<WbemListener>(pSink, pConnection);
	}
}