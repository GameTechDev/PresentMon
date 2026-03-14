// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Logging.h"
#include "Service.h"
#include "ActionServer.h"
#include "PresentMon.h"
#include "FrameBroadcaster.h"
#include "..\ControlLib\TelemetryCoordinator.h"
#include "../Interprocess/source/Interprocess.h"
#include "CliOptions.h"
#include "Registry.h"
#include "GlobalIdentifiers.h"
#include "../CommonUtilities/IntervalWaiter.h"
#include "../CommonUtilities/win/Event.h"
#include "../CommonUtilities/log/IdentificationTable.h"

#include "../CommonUtilities/log/GlogShim.h"
#include "testing/TestControl.h"


using namespace std::literals;
using namespace pmon;
using namespace svc;
using namespace util;
using v = log::V;


void EventFlushThreadEntry_(Service* const srv, PresentMon* const pm)
{
    if (srv == nullptr || pm == nullptr) {
        pmlog_error();
        return;
    }

    // this is the interval to wait when manual flush is disabled
    // we still want to run the inner loop to poll in case it gets enabled
    const uint32_t disabledIntervalMs = 250u;
    // waiter interval will be set later before it is actually used; set 0 placeholder
    IntervalWaiter waiter{ 0. };
    // outer dormant loop waits for either start of process tracking or service exit
    while (true) {
        pmlog_verb(v::etwq)("Begin idle ETW flush wait");
        // if event index 0 is signalled that means we are stopping
        if (win::WaitAnyEvent(srv->GetServiceStopHandle(), pm->GetStreamingStartHandle()) == 0) {
            pmlog_dbg("exiting ETW flush thread due to stop handle");
            return;
        }
        pmlog_verb(v::etwq)("Entering ETW flush inner active loop");
        // read flush period here right before first wait
        auto currentInterval = (double)pm->GetEtwFlushPeriod().value_or(disabledIntervalMs) / 1000.;
        // otherwise we assume streaming has started and we begin the flushing loop, checking for stop signal
        while (!win::WaitAnyEventFor(0s, srv->GetServiceStopHandle())) {
            // use interval wait to time flushes as a fixed cadence
            pmlog_verb(v::etwq)("Wait on ETW flush interval (period)").pmwatch(currentInterval);
            waiter.SetInterval(currentInterval);
            waiter.Wait();
            // go dormant if there are no active streams left
            // TODO: GetActiveStreams is not technically thread-safe, reconsider fixing this stuff in Service
            if (!pm->HasLiveTargets()) {
                pmlog_dbg("ETW flush loop entering dormancy due to 0 active streams");
                break;
            }
            if (auto flushPeriodMs = pm->GetEtwFlushPeriod()) {
                // flush events manually to reduce latency
                pmlog_verb(v::etwq)("Manual ETW flush").pmwatch(*flushPeriodMs);
                pm->FlushEvents();
                // update flush period
                currentInterval = *flushPeriodMs / 1000.;
            }
            else {
                pmlog_verb(v::etwq)("Detected disabled ETW flush, using idle poll period");
                // set a period here (low rate) to check for changes to the etw flush setting
                currentInterval = disabledIntervalMs / 1000.;
            }
        }
    }
}

void TelemetryThreadEntry_(Service* const srv, PresentMon* const pm, ipc::ServiceComms* const pComms)
{
    using util::win::WaitAnyEvent;
    using util::win::WaitAnyEventFor;

    try {
        util::log::IdentificationTable::AddThisThread("telemetry");
        pmlog_dbg("Starting telemetry thread");
        if (srv == nullptr || pm == nullptr || pComms == nullptr) {
            pmlog_error("Required parameter was null");
            return;
        }

        if (WaitAnyEvent(srv->GetClientSessionHandle(), srv->GetServiceStopHandle()) == 1) {
            pmlog_dbg("Exiting telemetry thread before initialization");
            return;
        }

        pmon::util::QpcTimer timer;
        pmon::tel::TelemetryCoordinator coordinator;
        coordinator.RegisterDevicesToIpc(*pComms);
        coordinator.PopulateStaticsToIpc(*pComms);
        pmlog_info(std::format(
            "Finished populating telemetry introspection, {} seconds elapsed",
            timer.Mark()));

        IntervalWaiter waiter{ 0.016 };
        while (true) {
            pmlog_dbg("(re)starting telemetry idle wait");
            if (WaitAnyEvent(pm->GetDeviceUsageEvent(), srv->GetServiceStopHandle()) == 1) {
                pmlog_dbg("telemetry received exit code, thread exiting");
                return;
            }

            auto pMetricUse = pm->GetDeviceMetricUsageSnapshot();
            if (!pMetricUse || pMetricUse->empty()) {
                pmlog_dbg("received device usage event, but no telemetry device metrics were active");
                continue;
            }

            pmlog_dbg("entering telemetry active poll loop");
            while (!WaitAnyEventFor(0ms, srv->GetServiceStopHandle())) {
                if (WaitAnyEventFor(0ms, srv->GetResetPowerTelemetryHandle())) {
                    pmlog_warn("Telemetry provider reset requested; live telemetry reprobe is not yet implemented");
                }

                pMetricUse = pm->GetDeviceMetricUsageSnapshot();
                if (!pMetricUse || pMetricUse->empty()) {
                    break;
                }

                coordinator.PollToIpc(*pMetricUse, *pComms);

                waiter.SetInterval(pm->GetGpuTelemetryPeriod() / 1000.);
                waiter.Wait();
            }
        }
    }
    catch (...) {
        pmlog_error(util::ReportException("Failure in telemetry loop"));
    }
}



void PresentMonMainThread(Service* const pSvc)
{
    assert(pSvc);

    // these thread containers need to be created outside of the try scope
    // so that if an exception happens, it won't block during unwinding,
    // trying to join threads that are waiting for a stop signal
    std::jthread telemetryThread;

    try {
        // alias for options
        auto& opt = clio::Options::Get();
        const auto& reg = Reg::Get();
        const auto frameRingSamples = opt.frameRingSamples.AsOptional().value_or(
            reg.frameRingSamples.AsOptional()
                .transform([](auto val) { return (size_t)val; })
                .value_or(*opt.frameRingSamples));
        const auto telemetryRingSamples = opt.telemetryRingSamples.AsOptional().value_or(
            reg.telemetryRingSamples.AsOptional()
                .transform([](auto val) { return (size_t)val; })
                .value_or(*opt.telemetryRingSamples));

        // spin here waiting for debugger to attach, after which debugger should set
        // debug_service to false in order to proceed
        for (auto debug_service = opt.debug; debug_service;) {
            if (WaitForSingleObject(pSvc->GetServiceStopHandle(), 0) != WAIT_OBJECT_0) {
                Sleep(100);
            }
            else {
                return;
            }
        }

        if (opt.timedStop) {
            const auto hTimer = CreateWaitableTimerA(NULL, FALSE, NULL);
            const LARGE_INTEGER liDueTime{
                // timedStop in ms, we need to express in units of 100ns
                // and making it negative makes the timeout relative to now
                // (positive value indicates an absolute timepoint)
                .QuadPart = -10'000LL * *opt.timedStop,
            };
            struct Completion {
                static void CALLBACK Routine(LPVOID pSvc, DWORD dwTimerLowValue, DWORD dwTimerHighValue) {
                    static_cast<Service*>(pSvc)->SignalServiceStop();
                }
            };
            if (hTimer) {
                SetWaitableTimer(hTimer, &liDueTime, 0, &Completion::Routine, pSvc, FALSE);
            }
        }

        // create service-side comms object for transmitting introspection data to clients
        std::unique_ptr<ipc::ServiceComms> pComms;
        try {
            pmlog_dbg("Creating comms with shm prefix: ").pmwatch(*opt.shmNamePrefix);
            pComms = ipc::MakeServiceComms(*opt.shmNamePrefix,
                frameRingSamples,
                telemetryRingSamples);
            pmlog_info("Created comms with introspection shm name: ")
                .pmwatch(pComms->GetNamer().MakeIntrospectionName());
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "Failed making service comms> " << e.what() << std::endl;
            pSvc->SignalServiceStop(-1);
            return;
        }
        // create comms wrapper for managing frame data segments
        FrameBroadcaster frameBroadcaster{ *pComms };

        // container for session object
        PresentMon pm{ frameBroadcaster, !opt.etlTestFile };

        // Start named pipe action RPC server (active threaded)
        auto pActionServer = std::make_unique<ActionServer>(pSvc, &pm, opt.controlPipe.AsOptional());

        try {
            telemetryThread = std::jthread{ TelemetryThreadEntry_, pSvc, &pm, pComms.get() };
        }
        catch (...) {
            LOG(ERROR) << "failed creating telemetry thread" << std::endl;
        }

        // start thread for manual ETW event buffer flushing
        std::jthread flushThread{ EventFlushThreadEntry_, pSvc, &pm };

        // communication controller for testing purposes
        std::unique_ptr<pmon::svc::testing::TestControlModule> pTcm;
        if (opt.enableTestControl) {
            pTcm = std::make_unique<pmon::svc::testing::TestControlModule>(&pm, pSvc);
        }

        // periodically check trace sessions while waiting for service stop event
        while (!util::win::WaitAnyEventFor(250ms, pSvc->GetServiceStopHandle())) {
            pm.CheckTraceSessions();
        }

        // Stop the PresentMon sessions
        pm.StopTraceSessions();
        // wait for the telemetry threads to exit
        if (telemetryThread.joinable()) {
            telemetryThread.join();
        }
    }
    catch (...) {
        LOG(ERROR) << "Exception in PMMainThread, bailing" << std::endl;
        if (pSvc) {
            pSvc->SignalServiceStop(-1);
        }
    }
}
