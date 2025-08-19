// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Wbemidl.h>
#include <string>

namespace pmon::util::win::com
{
    class WbemSink : public IWbemObjectSink
    {
    public:
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;
        HRESULT STDMETHODCALLTYPE
            QueryInterface(REFIID riid, void** ppv) override;

        // HRESULT STDMETHODCALLTYPE Indicate(LONG count, IWbemClassObject 
        //    __RPC_FAR* __RPC_FAR* pObjArr) = 0 -> WBEM_S_NO_ERROR;

        HRESULT STDMETHODCALLTYPE SetStatus(
            LONG lFlags,
            HRESULT hResult,
            BSTR strParam,
            IWbemClassObject __RPC_FAR* pObjParam
        ) override;

        virtual ~WbemSink() = default;
        virtual std::string GetQueryString() const = 0;
    private:
        LONG refCount = 0;
    };
}