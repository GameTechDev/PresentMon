/*
Copyright 2020 Intel Corporation

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

struct PMTraceConsumer;
struct MRTraceConsumer;

struct TraceSession {
    LARGE_INTEGER mStartQpc = {};
    LARGE_INTEGER mQpcFrequency = {};
    PMTraceConsumer* mPMConsumer = nullptr;
    MRTraceConsumer* mMRConsumer = nullptr;
    TRACEHANDLE mHandle = 0;                                // invalid session handles are 0
    TRACEHANDLE mTraceHandle = INVALID_PROCESSTRACE_HANDLE; // invalid trace handles are INVALID_PROCESSTRACE_HANDLE
    ULONG mContinueProcessingBuffers = TRUE;

    ULONG Start(
        PMTraceConsumer* pmConsumer, // Required PMTraceConsumer instance
        MRTraceConsumer* mrConsumer, // If nullptr, no WinMR tracing
        char const* etlPath,         // If nullptr, live/realtime tracing session
        char const* sessionName);    // Required session name

    void Stop();

    ULONG CheckLostReports(ULONG* eventsLost, ULONG* buffersLost) const;
    static ULONG StopNamedSession(char const* sessionName);
};
