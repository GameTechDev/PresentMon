// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <format>
#include "igcl_api.h"
#include "../CommonUtilities/log/Log.h"

namespace pwr::log
{
    inline std::string MakeErrorLocationString(int line, const char* file, const char* function)
    {
        return std::format("{} at {}:{}", function, file, line);
    }

    inline std::string MakeTelemetryDebugErrorString(const char* msg, int line, const char* file, const char* function)
    {
        return std::format("(TEL) {} in {}\n", msg, MakeErrorLocationString(line, file, function));
    }
}

#define IGCL_ERR(ec) pmlog_error("IGCL").code(ec);
#define TELE_ERR(msg) OutputDebugStringA(pwr::log::MakeTelemetryDebugErrorString((msg), __LINE__, __FILE__, __FUNCTION__).c_str())