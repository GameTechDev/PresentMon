// Copyright (C) 2017,2019-2022 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMon.hpp"

#include "../PresentData/TraceSession.hpp"

namespace {

TraceSession gSession;
static PMTraceConsumer* gPMConsumer = nullptr;
static MRTraceConsumer* gMRConsumer = nullptr;

}

bool StartTraceSession()
{
    auto const& args = GetCommandLineArgs();
    auto filterProcessIds = args.mTargetPid != 0; // Does not support process names at this point

    // Create consumers
    gPMConsumer = new PMTraceConsumer();
    gPMConsumer->mFilteredProcessIds = filterProcessIds;
    gPMConsumer->mTrackDisplay = args.mTrackDisplay;
    gPMConsumer->mTrackGPU = args.mTrackGPU;
    gPMConsumer->mTrackGPUVideo = args.mTrackGPUVideo;
    gPMConsumer->mTrackInput = args.mTrackInput;

    if (filterProcessIds) {
        gPMConsumer->AddTrackedProcessForFiltering(args.mTargetPid);
    }

    if (args.mTrackWMR) {
        gMRConsumer = new MRTraceConsumer(args.mTrackDisplay);
    }

    if (args.mOutputDateTime) {
       gSession.mTimestampType = TraceSession::TIMESTAMP_TYPE_SYSTEM_TIME;
    } else {
       gSession.mTimestampType = TraceSession::TIMESTAMP_TYPE_QPC;
    }

    // Start the session;
    // If a session with this same name is already running, we either exit or
    // stop it and start a new session.  This is useful if a previous process
    // failed to properly shut down the session for some reason.
    auto status = gSession.Start(gPMConsumer, gMRConsumer, args.mEtlFileName, args.mSessionName);
    if (status == ERROR_ALREADY_EXISTS) {
        if (args.mStopExistingSession) {
            PrintWarning(
                L"warning: a trace session named \"%s\" is already running and it will be stopped.\n"
                L"         Use -session_name with a different name to start a new session.\n",
                args.mSessionName);
        } else {
            PrintError(
                L"error: a trace session named \"%s\" is already running. Use -stop_existing_session\n"
                L"       to stop the existing session, or use -session_name with a different name to\n"
                L"       start a new session.\n",
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
        PrintError(L"error: failed to start trace session: ");
        switch (status) {
        case ERROR_FILE_NOT_FOUND: PrintError(L"file not found.\n"); break;
        case ERROR_PATH_NOT_FOUND: PrintError(L"path not found.\n"); break;
        case ERROR_BAD_PATHNAME:   PrintError(L"invalid --session_name.\n"); break;
        case ERROR_ACCESS_DENIED:  PrintError(L"access denied.\n"); break;
        case ERROR_FILE_CORRUPT:   PrintError(L"invalid --etl_file.\n"); break;
        default:                   PrintError(L"error code %lu.\n", status); break;
        }

        if (status == ERROR_ACCESS_DENIED && !InPerfLogUsersGroup()) {
            PrintError(
                L"       PresentMon requires either administrative privileges or to be run by a user in the\n"
                L"       \"Performance Log Users\" user group.  View the readme for more details.\n");
        }

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
    std::vector<ProcessEvent>* processEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
    std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents,
    std::vector<std::shared_ptr<LateStageReprojectionEvent>>* lsrs)
{
    gPMConsumer->DequeueProcessEvents(*processEvents);
    gPMConsumer->DequeuePresentEvents(*presentEvents);
    gPMConsumer->DequeueLostPresentEvents(*lostPresentEvents);
    if (gMRConsumer != nullptr) {
        gMRConsumer->DequeueLSRs(*lsrs);
    }
}

double TimestampDeltaToSeconds(uint64_t timestampDelta)
{
    return (double) timestampDelta / gSession.mTimestampFrequency.QuadPart;
}

double PositiveTimestampDeltaToSeconds(uint64_t timestampFrom, uint64_t timestampTo)
{
    return timestampFrom == 0 || timestampTo <= timestampFrom ? 0.0 : TimestampDeltaToSeconds(timestampTo - timestampFrom);
}

double TimestampDeltaToSeconds(uint64_t timestampFrom, uint64_t timestampTo)
{
    return timestampFrom == 0 || timestampTo == 0 || timestampFrom == timestampTo ? 0.0 :
        timestampTo > timestampFrom ? TimestampDeltaToSeconds(timestampTo - timestampFrom) :
                         -TimestampDeltaToSeconds(timestampFrom - timestampTo);
}

uint64_t SecondsDeltaToTimestamp(double secondsDelta)
{
    return (uint64_t) (secondsDelta * gSession.mTimestampFrequency.QuadPart);
}

double TimestampToSeconds(uint64_t timestamp)
{
    return TimestampDeltaToSeconds(timestamp - gSession.mStartTimestamp.QuadPart);
}

void TimestampToLocalSystemTime(uint64_t timestamp, SYSTEMTIME* st, uint64_t* ns)
{
    if (gSession.mTimestampType != TraceSession::TIMESTAMP_TYPE_SYSTEM_TIME) {
        auto delta100ns = (timestamp - gSession.mStartTimestamp.QuadPart) * 10000000ull / gSession.mTimestampFrequency.QuadPart;
        timestamp = gSession.mStartFileTime + delta100ns;
    }

    FILETIME lft{};
    FileTimeToLocalFileTime((FILETIME*) &timestamp, &lft);
    FileTimeToSystemTime(&lft, st);
    *ns = (timestamp % 10000000) * 100;
}
