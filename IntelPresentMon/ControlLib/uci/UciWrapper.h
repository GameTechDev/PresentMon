// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../DllModule.h"
#include "../MacroHelpers.h"
#include "UciSdk.h"

#if PMON_HAS_UCI_SDK

#define PMON_UCI_ENDPOINT_LIST_WITH_ARGS \
X_(GetCapabilities, const char**, capabilitiesJson) \
X_(Configure, const char*, configJson) \
X_(SetDataCallback, UciDataCallback, dataCallback)

#define PMON_UCI_SESSION_ENDPOINT_LIST \
X_(Start) \
X_(Stop) \
X_(AggregateData)

#define PMON_UCI_RECORD_ENDPOINT_LIST \
X_(MetricRecordGetMetricName, uciMetricRecordHandle, record, char**, name) \
X_(MetricRecordGetEntity, uciMetricRecordHandle, record, char**, entity) \
X_(MetricRecordGetDescriptor, uciMetricRecordHandle, record, char**, descriptor) \
X_(MetricRecordGetUnit, uciMetricRecordHandle, record, char**, unit) \
X_(MetricRecordGetSample, uciMetricRecordHandle, record, double*, sample) \
X_(MetricRecordGetRecordTimestamp, uciMetricRecordHandle, record, uint64_t*, timestamp) \
X_(MetricRecordGetRecordDuration, uciMetricRecordHandle, record, uint64_t*, duration)

namespace pmon::tel::uci
{
    using UciDataCallback = void (*)(uciDataBundle*);

    class UciWrapper
    {
    public:
        UciWrapper();
        UciWrapper(const UciWrapper& t) = delete;
        UciWrapper& operator=(const UciWrapper& t) = delete;
        ~UciWrapper();
        static bool Ok(uc_result_t sta) noexcept { return sta == UC_SUCCESS; }

#define X_(name, ...) uc_result_t name(NVW_ARGS(__VA_ARGS__)) const noexcept;
        PMON_UCI_ENDPOINT_LIST_WITH_ARGS
#undef X_
#define X_(name) uc_result_t name() const noexcept;
        PMON_UCI_SESSION_ENDPOINT_LIST
#undef X_
#define X_(name, ...) uc_result_t name(NVW_ARGS(__VA_ARGS__)) const noexcept;
        PMON_UCI_RECORD_ENDPOINT_LIST
#undef X_

    private:
        DllModule dll{ { "unified-collector-interface.dll" }, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS };
        uciSession* session_ = nullptr;

#define X_(name, ...) uc_result_t (*p##name)(uciSession* session, NVW_ARGS(__VA_ARGS__)) = nullptr;
        PMON_UCI_ENDPOINT_LIST_WITH_ARGS
#undef X_
#define X_(name) uc_result_t (*p##name)(uciSession* session) = nullptr;
        PMON_UCI_SESSION_ENDPOINT_LIST
#undef X_
#define X_(name, ...) uc_result_t (*p##name)(NVW_ARGS(__VA_ARGS__)) = nullptr;
        PMON_UCI_RECORD_ENDPOINT_LIST
#undef X_
        uc_result_t (*pDestroy)(uciSession* session) = nullptr;
    };
}

#endif
