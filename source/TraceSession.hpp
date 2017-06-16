/*
Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <windows.h>
#include <evntcons.h> // must be after windows.h
#include <stdint.h>
#include <unordered_map>

typedef void (*EventHandlerFn)(EVENT_RECORD* pEventRecord, void* pContext);
typedef bool (*ShouldStopProcessingEventsFn)();

struct TraceSession {
    // BEGIN trace property block, must be beginning of TraceSession
    EVENT_TRACE_PROPERTIES properties_;
    wchar_t loggerName_[MAX_PATH];
    // END Trace property block

    TRACEHANDLE sessionHandle_;    // Must be first member after trace property block
    TRACEHANDLE traceHandle_;
    ShouldStopProcessingEventsFn shouldStopProcessingEventsFn_;
    uint64_t startTime_;
    uint64_t frequency_;
    uint32_t eventsLostCount_;
    uint32_t buffersLostCount_;

    // Structure to hold the mapping from provider ID to event handler function
    struct GUIDHash { size_t operator()(GUID const& g) const; };
    struct GUIDEqual { bool operator()(GUID const& lhs, GUID const& rhs) const; };
    struct Handler {
        EventHandlerFn fn_;
        void* ctxt_;
        ULONGLONG matchAny_;
        ULONGLONG matchAll_;
        UCHAR level_;
    };
    std::unordered_map<GUID, Handler, GUIDHash, GUIDEqual> eventHandler_;

    TraceSession()
        : sessionHandle_(0)
        , traceHandle_(INVALID_PROCESSTRACE_HANDLE)
        , startTime_(0)
        , frequency_(0)
        , shouldStopProcessingEventsFn_(nullptr)
    {
    }

    // Usage:
    //
    // 1) use TraceSession::AddProvider() to add the IDs and handler functions
    // for all the providers you want to trace.
    //
    // 2) call TraceSession::InitializeRealtime() or
    // TraceSession::InitializeEtlFile(), to start tracing events from
    // real-time collection or from a previously-captured .etl file. At this
    // point, events start to be traced.
    //
    // 3) call ::ProcessTrace() to start collecting the events; provider
    // handler functions will be called as those provider events are collected.
    // ProcessTrace() will exit when shouldStopProcessingEventsFn_ returns
    // true, or when the .etl file is fully consumed.
    //
    // 4) Finalize() to clean up.

    // AddProvider() returns false if the providerId already has a handler.
    // RemoveProvider() returns false if the providerId don't have a handler.
    bool AddProvider(GUID providerId, UCHAR level, ULONGLONG matchAnyKeyword, ULONGLONG matchAllKeyword,
                     EventHandlerFn handlerFn, void* handlerContext);
    bool RemoveProvider(GUID providerId);

    // InitializeRealtime() and InitializeEtlFile() return false if the session
    // could not be created.
    bool InitializeEtlFile(char const* etlPath, ShouldStopProcessingEventsFn shouldStopProcessingEventsFn);
    bool InitializeRealtime(char const* traceSessionName, ShouldStopProcessingEventsFn shouldStopProcessingEventsFn);
    void Finalize();

    // Call CheckLostReports() at any time the session is initialized to query
    // how many events and buffers have been lost while tracing.
    bool CheckLostReports(uint32_t* eventsLost, uint32_t* buffersLost);
};

