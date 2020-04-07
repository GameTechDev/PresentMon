/*
Copyright 2017-2020 Intel Corporation

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

#include "PresentMon.hpp"

#include "../PresentData/TraceSession.hpp"
#include <VersionHelpers.h>

namespace {

TraceSession gSession;
static PMTraceConsumer* gPMConsumer = nullptr;
static MRTraceConsumer* gMRConsumer = nullptr;

}

bool StartTraceSession()
{
    auto const& args = GetCommandLineArgs();
    auto simple = args.mVerbosity == Verbosity::Simple;
    auto includeWinMR = args.mIncludeWindowsMixedReality;
    auto expectFilteredEvents =
        args.mEtlFileName == nullptr && // Scope filtering based on event ID only works for realtime collection
        IsWindows8Point1OrGreater();    // and requires Win8.1+

    // Create consumers
    gPMConsumer = new PMTraceConsumer(expectFilteredEvents, simple);
    if (includeWinMR) {
        gMRConsumer = new MRTraceConsumer(simple);
    }

    // Start the session;
    // If a session with this same name is already running, we either exit or
    // stop it and start a new session.  This is useful if a previous process
    // failed to properly shut down the session for some reason.
    auto status = gSession.Start(gPMConsumer, gMRConsumer, args.mEtlFileName, args.mSessionName);

    if (status == ERROR_ALREADY_EXISTS) {
        if (args.mStopExistingSession) {
            fprintf(stderr,
                "warning: a trace session named \"%s\" is already running and it will be stopped.\n"
                "         Use -session_name with a different name to start a new session.\n",
                args.mSessionName);
        } else {
            fprintf(stderr,
                "error: a trace session named \"%s\" is already running. Use -stop_existing_session\n"
                "       to stop the existing session, or use -session_name with a different name to\n"
                "       start a new session.\n",
                args.mSessionName);
            delete gPMConsumer;
            delete gMRConsumer;
            gPMConsumer = nullptr;
            gMRConsumer = nullptr;
            return false;
        }

        status = TraceSession::StopNamedSession(args.mSessionName);
        if (status == ERROR_SUCCESS) {
            status = gSession.Start(gPMConsumer, gMRConsumer, args.mEtlFileName, args.mSessionName);
        }
    }

    // Report error if we failed to start a new session
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "error: failed to start trace session");
        switch (status) {
        case ERROR_FILE_NOT_FOUND:    fprintf(stderr, " (file not found)"); break;
        case ERROR_PATH_NOT_FOUND:    fprintf(stderr, " (path not found)"); break;
        case ERROR_INVALID_PARAMETER: fprintf(stderr, " (Logfile is NULL)"); break;
        case ERROR_BAD_PATHNAME:      fprintf(stderr, " (invalid LoggerName)"); break;
        case ERROR_ACCESS_DENIED:     fprintf(stderr, " (access denied)"); break;
        default:                      fprintf(stderr, " (error=%u)", status); break;
        }
        fprintf(stderr, ".\n");

        delete gPMConsumer;
        delete gMRConsumer;
        gPMConsumer = nullptr;
        gMRConsumer = nullptr;
        return false;
    }

    // -------------------------------------------------------------------------
    // Start the consumer and output threads
    StartConsumerThread(gSession.mTraceHandle);
    StartOutputThread();

    return true;
}

void StopTraceSession()
{
    // Stop the trace session.
    gSession.Stop();

    // Wait for the consumer and output threads to end (which are using the
    // consumers).
    WaitForConsumerThreadToExit();
    StopOutputThread();

    // Destruct the consumers
    delete gMRConsumer;
    delete gPMConsumer;
    gMRConsumer = nullptr;
    gPMConsumer = nullptr;
}

void CheckLostReports(ULONG* eventsLost, ULONG* buffersLost)
{
    auto status = gSession.CheckLostReports(eventsLost, buffersLost);
    (void) status;
}

void DequeueAnalyzedInfo(
    std::vector<NTProcessEvent>* ntProcessEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presents,
    std::vector<std::shared_ptr<LateStageReprojectionEvent>>* lsrs)
{
    gPMConsumer->DequeueProcessEvents(*ntProcessEvents);
    gPMConsumer->DequeuePresents(*presents);
    if (gMRConsumer != nullptr) {
        gMRConsumer->DequeueLSRs(*lsrs);
    }
}

double QpcDeltaToSeconds(uint64_t qpcDelta)
{
    return (double) qpcDelta / gSession.mQpcFrequency.QuadPart;
}

uint64_t SecondsDeltaToQpc(double secondsDelta)
{
    return (uint64_t) (secondsDelta * gSession.mQpcFrequency.QuadPart);
}

double QpcToSeconds(uint64_t qpc)
{
    return QpcDeltaToSeconds(qpc - gSession.mStartQpc.QuadPart);
}
