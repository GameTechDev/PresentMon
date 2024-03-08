// Copyright (C) 2020 Intel Corporation
// SPDX-License-Identifier: MIT

struct PMTraceConsumer;
struct MRTraceConsumer;

struct TraceSession {
    enum TimestampType {
        TIMESTAMP_TYPE_QPC = 1,
        TIMESTAMP_TYPE_SYSTEM_TIME = 2,
        TIMESTAMP_TYPE_CPU_CYCLE_COUNTER = 3,
    };

    LARGE_INTEGER mStartTimestamp = {};
    LARGE_INTEGER mTimestampFrequency = {};
    uint64_t mStartFileTime = 0;
    TimestampType mTimestampType = TIMESTAMP_TYPE_QPC;

    PMTraceConsumer* mPMConsumer = nullptr;
    MRTraceConsumer* mMRConsumer = nullptr;
    TRACEHANDLE mSessionHandle = 0;                         // invalid session handles are 0
    TRACEHANDLE mTraceHandle = INVALID_PROCESSTRACE_HANDLE; // invalid trace handles are INVALID_PROCESSTRACE_HANDLE
    ULONG mContinueProcessingBuffers = TRUE;

    ULONG Start(
        PMTraceConsumer* pmConsumer,  // Required PMTraceConsumer instance
        MRTraceConsumer* mrConsumer,  // If nullptr, no WinMR tracing
        wchar_t const* etlPath,       // If nullptr, live/realtime tracing session
        wchar_t const* sessionName);  // Required session name

    void Stop();

    ULONG CheckLostReports(ULONG* eventsLost, ULONG* buffersLost) const;
    static ULONG StopNamedSession(wchar_t const* sessionName);
};
