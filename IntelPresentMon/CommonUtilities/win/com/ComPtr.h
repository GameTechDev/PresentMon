// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../WinAPI.h"
#include <wrl.h>

// TODO: implement this ourselves so that we don't have to include winapi everywhere we use comptr

namespace pmon::util::win::com
{
    template<class T>
    class ComPtr : public Microsoft::WRL::ComPtr<T>
    {
    public:
        T& operator*()
        {
            return *this->Get();
        }
        const T& operator*() const
        {
            return *this->Get();
        }
    };
}