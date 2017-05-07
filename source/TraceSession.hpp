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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <evntcons.h> // must be after windows.h
#include <stdint.h>

#include "PresentMon.hpp"
#include "PresentMonTraceConsumer.hpp"

struct TraceSession {
    // BEGIN trace property block, must be beginning of TraceSession
    EVENT_TRACE_PROPERTIES properties_;
    wchar_t loggerName_[MAX_PATH];
    // END Trace property block

    TRACEHANDLE sessionHandle_;    // Must be first member after trace property block
    TRACEHANDLE traceHandle_;
    uint64_t startTime_;
    uint64_t frequency_;
    uint32_t eventsLostCount_;
    uint32_t buffersLostCount_;

    PresentMonData* pmData_;
    PMTraceConsumer* pmTraceConsumer_;

    TraceSession()
        : sessionHandle_(0)
        , traceHandle_(INVALID_PROCESSTRACE_HANDLE)
        , startTime_(0)
        , pmData_(nullptr)
        , pmTraceConsumer_(nullptr)
    {
    }

    bool Initialize(bool simpleMode, char const* inputEtlPath);
    void Finalize();
    bool CheckLostReports(uint32_t* eventsLost, uint32_t* buffersLost);
};

