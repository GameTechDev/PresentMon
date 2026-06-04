// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "UciWrapper.h"

#if PMON_HAS_UCI_SDK

namespace pmon::tel::uci
{
    UciWrapper::UciWrapper()
    {
        const auto pInitialize = static_cast<uc_result_t(*)(uciSession**)>(
            dll.GetProcAddress("uciInitialize"));
        if (!pInitialize) {
            throw Except<TelemetrySubsystemAbsent>("Failed to get uciInitialize proc");
        }

#define X_(name, ...) p##name = static_cast<decltype(p##name)>(dll.GetProcAddress("uci"#name));
        PMON_UCI_ENDPOINT_LIST_WITH_ARGS
#undef X_
#define X_(name) p##name = static_cast<decltype(p##name)>(dll.GetProcAddress("uci"#name));
        PMON_UCI_SESSION_ENDPOINT_LIST
#undef X_
#define X_(name, ...) p##name = static_cast<decltype(p##name)>(dll.GetProcAddress("uci"#name));
        PMON_UCI_RECORD_ENDPOINT_LIST
#undef X_
        pDestroy = static_cast<decltype(pDestroy)>(dll.GetProcAddress("uciDestroy"));

        if (!Ok(pInitialize(&session_)) || session_ == nullptr) {
            throw Except<TelemetrySubsystemAbsent>("uciInitialize call failed");
        }
    }

    UciWrapper::~UciWrapper()
    {
        if (pDestroy && session_) {
            pDestroy(session_);
        }
    }

#define X_(name, ...) \
    uc_result_t UciWrapper::name(NVW_ARGS(__VA_ARGS__)) const noexcept \
    { \
        if (!p##name) { \
            return UC_ERROR_FUNCTION_NOT_SUPPORTED; \
        } \
        return p##name(session_, NVW_NAMES(__VA_ARGS__)); \
    }
    PMON_UCI_ENDPOINT_LIST_WITH_ARGS
#undef X_

#define X_(name) \
    uc_result_t UciWrapper::name() const noexcept \
    { \
        if (!p##name) { \
            return UC_ERROR_FUNCTION_NOT_SUPPORTED; \
        } \
        return p##name(session_); \
    }
    PMON_UCI_SESSION_ENDPOINT_LIST
#undef X_

#define X_(name, ...) \
    uc_result_t UciWrapper::name(NVW_ARGS(__VA_ARGS__)) const noexcept \
    { \
        if (!p##name) { \
            return UC_ERROR_FUNCTION_NOT_SUPPORTED; \
        } \
        return p##name(NVW_NAMES(__VA_ARGS__)); \
    }
    PMON_UCI_RECORD_ENDPOINT_LIST
#undef X_
}

#endif
