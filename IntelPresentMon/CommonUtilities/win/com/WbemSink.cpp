// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "WbemSink.h"

namespace pmon::util::win::com
{
	ULONG STDMETHODCALLTYPE WbemSink::AddRef()
	{
		return InterlockedIncrement(&refCount);
	}
	ULONG STDMETHODCALLTYPE WbemSink::Release()
	{
		const auto refCount_ = InterlockedDecrement(&refCount);
		if (refCount_ == 0) {
			delete this;
		}
		return refCount_;
	}
	HRESULT STDMETHODCALLTYPE WbemSink::QueryInterface(REFIID riid, void** ppv)
	{
		if (riid == IID_IUnknown || riid == IID_IWbemObjectSink) {
			*ppv = static_cast<IWbemObjectSink*>(this);
			AddRef();
			return WBEM_S_NO_ERROR;
		}
		else {
			return E_NOINTERFACE;
		}
	}
	HRESULT STDMETHODCALLTYPE WbemSink::SetStatus(LONG lFlags, HRESULT hResult,
		BSTR strParam, IWbemClassObject* pObjParam)
	{
		return WBEM_S_NO_ERROR;
	}
}