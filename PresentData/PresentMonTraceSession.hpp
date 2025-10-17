// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT
#include "../IntelPresentMon/CommonUtilities/PrecisionWaiter.h"
#include "IFilterBuildListener.h"

struct PMTraceConsumer;

struct PMTraceSession {
    enum TimestampType {
        TIMESTAMP_TYPE_QPC = 1,
        TIMESTAMP_TYPE_SYSTEM_TIME = 2,
        TIMESTAMP_TYPE_CPU_CYCLE_COUNTER = 3,
    };

    PMTraceConsumer* mPMConsumer = nullptr; // Required PMTraceConsumer instance

    // event pacing-specific members
    //
    int64_t mPacingActualLogStartTimestamp = 0;
    int64_t mPacingRealtimeStartTimestamp = 0;
    int64_t mPacingQpcOffset = 0;
    double mPacingQpcPeriod = 0;
    pmon::util::PrecisionWaiter mPacingWaiter{ 0.000'5 };

    LARGE_INTEGER mStartTimestamp = {};
    LARGE_INTEGER mTimestampFrequency = {};
    uint64_t mStartFileTime = 0;
    TimestampType mTimestampType = TIMESTAMP_TYPE_QPC;

    TRACEHANDLE mSessionHandle = 0;                         // invalid session handles are 0
    TRACEHANDLE mTraceHandle = INVALID_PROCESSTRACE_HANDLE; // invalid trace handles are INVALID_PROCESSTRACE_HANDLE

    ULONG mContinueProcessingBuffers = TRUE;

    ULONG mNumEventsLost = 0;
    ULONG mNumBuffersLost = 0;

    bool mIsRealtimeSession = false;

    ULONG Start(wchar_t const* etlPath,      // If nullptr, start a live/realtime tracing session
                wchar_t const* sessionName); // Required session name
    void Stop();

    double TimestampDeltaToMilliSeconds(uint64_t timestampDelta) const;
    double TimestampDeltaToMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo) const;
    double TimestampDeltaToUnsignedMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo) const;
    double TimestampToMilliSeconds(uint64_t timestamp) const;
    void TimestampToLocalSystemTime(uint64_t timestamp, SYSTEMTIME* st, uint64_t* ns) const;
    uint64_t MilliSecondsDeltaToTimestamp(double millisecondsDelta) const;
};

ULONG StopNamedTraceSession(wchar_t const* sessionName);

// this is called by EnableProviders, does the actual provider enabling and filter settings
// can be called without session or consumer to just extract out the filter parameters for use elsewhere
ULONG EnableProvidersListing(
    TRACEHANDLE sessionHandle,
    const GUID* pSessionGuid,
    const PMTraceConsumer* pmConsumer,
    bool filterEventIds,
    bool isWin11OrGreater,
    std::shared_ptr<IFilterBuildListener> pListener = {});
