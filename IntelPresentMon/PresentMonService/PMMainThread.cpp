// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Logging.h"
#include "Service.h"
#include "ActionServer.h"
#include "PresentMon.h"
#include "PowerTelemetryContainer.h"
#include "FrameBroadcaster.h"
#include "..\ControlLib\WmiCpu.h"
#include "..\PresentMonUtils\StringUtils.h"
#include <filesystem>
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/ShmNamer.h"
#include "../Interprocess/source/MetricCapabilitiesShim.h"
#include "CliOptions.h"
#include "GlobalIdentifiers.h"
#include <ranges>
#include "../CommonUtilities/IntervalWaiter.h"
#include "../CommonUtilities/PrecisionWaiter.h"
#include "../CommonUtilities/win/Event.h"

#include "../CommonUtilities/log/GlogShim.h"
#include "testing/TestControl.h"


using namespace std::literals;
using namespace pmon;
using namespace svc;
using namespace util;
using v = log::V;
namespace vi = std::views;


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
        if (*win::WaitAnyEvent(srv->GetServiceStopHandle(), pm->GetStreamingStartHandle()) == 0) {
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
            if (pm->GetActiveStreams() == 0) {
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

// Translate a single power-telemetry sample into the rings for one GPU store.
// This mirrors the CPU placeholder approach but handles double/uint64/bool rings.
static void PopulateGpuTelemetryRings_(
    ipc::GpuDataStore& store,
    const PresentMonPowerTelemetryInfo& s) noexcept
{
    for (auto&& [metric, ringVariant] : store.telemetryData.Rings()) {
        switch (metric) {

            // -------- double metrics --------

        case PM_METRIC_GPU_POWER:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_power_w, s.qpc);
            break;

        case PM_METRIC_GPU_VOLTAGE:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_voltage_v, s.qpc);
            break;

        case PM_METRIC_GPU_FREQUENCY:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_frequency_mhz, s.qpc);
            break;

        case PM_METRIC_GPU_EFFECTIVE_FREQUENCY:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_effective_frequency_mhz, s.qpc);
            break;

        case PM_METRIC_GPU_TEMPERATURE:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_temperature_c, s.qpc);
            break;

        case PM_METRIC_GPU_VOLTAGE_REGULATOR_TEMPERATURE:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_voltage_regulator_temperature_c, s.qpc);
            break;

        case PM_METRIC_GPU_UTILIZATION:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_utilization, s.qpc);
            break;

        case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_render_compute_utilization, s.qpc);
            break;

        case PM_METRIC_GPU_MEDIA_UTILIZATION:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_media_utilization, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_EFFECTIVE_BANDWIDTH:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_mem_effective_bandwidth_gbps, s.qpc);
            break;

        case PM_METRIC_GPU_OVERVOLTAGE_PERCENT:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_overvoltage_percent, s.qpc);
            break;

        case PM_METRIC_GPU_TEMPERATURE_PERCENT:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_temperature_percent, s.qpc);
            break;

        case PM_METRIC_GPU_POWER_PERCENT:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_power_percent, s.qpc);
            break;

        case PM_METRIC_GPU_CARD_POWER:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_card_power_w, s.qpc);
            break;

        case PM_METRIC_GPU_FAN_SPEED:
        {
            auto& ringVect =
                std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant);

            const size_t n = std::min(ringVect.size(), s.fan_speed_rpm.size());
            for (size_t i = 0; i < n; ++i) {
                ringVect[i].Push(s.fan_speed_rpm[i], s.qpc);
            }
            break;
        }

        // VRAM-related doubles
        case PM_METRIC_GPU_MEM_POWER:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.vram_power_w, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_VOLTAGE:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.vram_voltage_v, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_FREQUENCY:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.vram_frequency_mhz, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.vram_effective_frequency_gbps, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_TEMPERATURE:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.vram_temperature_c, s.qpc);
            break;

            // Memory bandwidth doubles
        case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_mem_write_bandwidth_bps, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
            std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant)[0]
                .Push(s.gpu_mem_read_bandwidth_bps, s.qpc);
            break;

            // -------- uint64 metrics --------
        case PM_METRIC_GPU_MEM_USED:
            std::get<ipc::TelemetryMap::HistoryRingVect<uint64_t>>(ringVariant)[0]
                .Push(s.gpu_mem_used_b, s.qpc);
            break;

            // -------- bool metrics --------
        case PM_METRIC_GPU_POWER_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.gpu_power_limited, s.qpc);
            break;

        case PM_METRIC_GPU_TEMPERATURE_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.gpu_temperature_limited, s.qpc);
            break;

        case PM_METRIC_GPU_CURRENT_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.gpu_current_limited, s.qpc);
            break;

        case PM_METRIC_GPU_VOLTAGE_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.gpu_voltage_limited, s.qpc);
            break;

        case PM_METRIC_GPU_UTILIZATION_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.gpu_utilization_limited, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_POWER_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.vram_power_limited, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.vram_temperature_limited, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_CURRENT_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.vram_current_limited, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_VOLTAGE_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.vram_voltage_limited, s.qpc);
            break;

        case PM_METRIC_GPU_MEM_UTILIZATION_LIMITED:
            std::get<ipc::TelemetryMap::HistoryRingVect<bool>>(ringVariant)[0]
                .Push(s.vram_utilization_limited, s.qpc);
            break;

        default:
            pmlog_warn("Unhandled metric").pmwatch((int)metric);
            break;
        }
    }
}


void PowerTelemetryThreadEntry_(Service* const srv, PresentMon* const pm,
	PowerTelemetryContainer* const ptc, ipc::ServiceComms* const pComms)
{
	if (srv == nullptr || pm == nullptr || ptc == nullptr) {
		// TODO: log error here
		return;
	}

    // we first wait for a client control connection before populating telemetry container
    // after populating, we sample each adapter to gather availability information
    // this is deferred until client connection in order to increase the probability that
    // telemetry metric availability is accurately assessed
    {
        const HANDLE events[]{
              srv->GetClientSessionHandle(),
              srv->GetServiceStopHandle(),
        };
        const auto waitResult = WaitForMultipleObjects((DWORD)std::size(events), events, FALSE, INFINITE);
        // TODO: check for wait result error
        // if events[1] was signalled, that means service is stopping so exit thread
        if ((waitResult - WAIT_OBJECT_0) == 1) {
            return;
        }
        pmon::util::QpcTimer timer;
        ptc->Repopulate();
        for (auto&& [i, adapter] : ptc->GetPowerTelemetryAdapters() | vi::enumerate) {
            // sample 2x here as workaround/kludge because Intel provider misreports 1st sample
            adapter->Sample();
            adapter->Sample();
            pComms->RegisterGpuDevice(adapter->GetVendor(), adapter->GetName(),
                ipc::intro::ConvertBitset(adapter->GetPowerTelemetryCapBits()));
            // after registering, we know that at least the store is available even
            // if the introspection itself is not complete
            auto& gpuStore = pComms->GetGpuDataStore(uint32_t(i + 1));
            // TODO: replace this placeholder routine for populating statics
            gpuStore.statics.name = adapter->GetName().c_str();
            gpuStore.statics.vendor = adapter->GetVendor();
            gpuStore.statics.memSize = adapter->GetDedicatedVideoMemory();
            gpuStore.statics.maxMemBandwidth = adapter->GetVideoMemoryMaxBandwidth();
            gpuStore.statics.sustainedPowerLimit = adapter->GetSustainedPowerLimit();
            // max fanspeed is polled in old system but static in new system, shim here
            // TODO: make this fully static
            // infer number of fans by the size of the telemetry ring array for fan speed
            const auto nFans = gpuStore.telemetryData.ArraySize(PM_METRIC_GPU_FAN_SPEED);
            auto& sample = adapter->GetNewest();
            for (size_t i = 0; i < nFans; i++) {
                gpuStore.statics.maxFanSpeedRpm.push_back(sample.max_fan_speed_rpm[i]);
            }
        }
        pComms->FinalizeGpuDevices();
        pmlog_info(std::format("Finished populating GPU telemetry introspection, {} seconds elapsed", timer.Mark()));
    }

	// only start periodic polling when streaming starts
    // exit polling loop and this thread when service is stopping
    {
        IntervalWaiter waiter{ 0.016 };
        const HANDLE events[]{
          pm->GetStreamingStartHandle(),
          srv->GetServiceStopHandle(),
        };
        while (1) {
            auto waitResult = WaitForMultipleObjects((DWORD)std::size(events), events, FALSE, INFINITE);
            // TODO: check for wait result error
            // if events[1] was signalled, that means service is stopping so exit thread
            if ((waitResult - WAIT_OBJECT_0) == 1) {
                return;
            }
            // otherwise we assume streaming has started and we begin the polling loop
            while (WaitForSingleObject(srv->GetServiceStopHandle(), 0) != WAIT_OBJECT_0) {
                // if device was reset (driver installed etc.) we need to repopulate telemetry
                if (WaitForSingleObject(srv->GetResetPowerTelemetryHandle(), 0) == WAIT_OBJECT_0) {
                    // TODO: log error here or inside of repopulate
                    ptc->Repopulate();
                }
                auto& adapters = ptc->GetPowerTelemetryAdapters();
                for (size_t idx = 0; idx < adapters.size(); ++idx) {
                    auto& adapter = adapters[idx];
                    adapter->Sample();

                    // Get the newest sample from the provider
                    const auto& sample = adapter->GetNewest();

                    // Retrieve the matching GPU store.
                    auto& store = pComms->GetGpuDataStore(uint32_t(idx + 1));

                    PopulateGpuTelemetryRings_(store, sample);
                }
                // Convert from the ms to seconds as GetTelemetryPeriod returns back
                // ms and SetInterval expects seconds.
                waiter.SetInterval(pm->GetGpuTelemetryPeriod()/1000.);
                waiter.Wait();
                // go dormant if there are no active streams left
                // TODO: consider race condition here if client stops and starts streams rapidly
                if (pm->GetActiveStreams() == 0) {
                    break;
                }
            }
        }
    }
}

void CpuTelemetryThreadEntry_(Service* const srv, PresentMon* const pm, ipc::ServiceComms* pComms,
	pwr::cpu::CpuTelemetry* const cpu) noexcept
{
    // we don't expect any exceptions in this system during normal operation
    // (maybe during initialization at most, not during polling)
    // but if they do happen, it is a halting condition for the system telemetry
    // don't let this thread crash the process, just exit with an error for later
    // diagnosis
    try {
        IntervalWaiter waiter{ 0.016 };
        if (srv == nullptr || pm == nullptr) {
            // TODO: log error on this condition
            return;
        }

        const HANDLE events[]{
            pm->GetStreamingStartHandle(),
            srv->GetServiceStopHandle(),
        };

        while (1) {
            auto waitResult = WaitForMultipleObjects((DWORD)std::size(events), events, FALSE, INFINITE);
            auto i = waitResult - WAIT_OBJECT_0;
            if (i == 1) {
                return;
            }
            while (WaitForSingleObject(srv->GetServiceStopHandle(), 0) != WAIT_OBJECT_0) {
                // TODO:streamer replace this flow with a call that populates rings of a store
                cpu->Sample();
                // placeholder routine shim to translate cpu tranfer struct into rings
                // replace with a direct mapping on PM_METRIC that obviates the switch
                auto& store = pComms->GetSystemDataStore();
                auto& sample = cpu->GetNewest();
                for (auto&& [metric, ringVariant] : store.telemetryData.Rings()) {
                    // all cpu telemetry is double
                    auto& ringVect = std::get<ipc::TelemetryMap::HistoryRingVect<double>>(ringVariant);
                    switch (metric) {
                    case PM_METRIC_CPU_FREQUENCY:
                        ringVect[0].Push(sample.cpu_frequency, sample.qpc);
                        break;
                    case PM_METRIC_CPU_UTILIZATION:
                        ringVect[0].Push(sample.cpu_utilization, sample.qpc);
                        break;
                    case PM_METRIC_CPU_POWER:
                        ringVect[0].Push(sample.cpu_power_w, sample.qpc);
                        break;
                    case PM_METRIC_CPU_POWER_LIMIT:
                        ringVect[0].Push(sample.cpu_power_limit_w, sample.qpc);
                        break;
                    case PM_METRIC_CPU_TEMPERATURE:
                        ringVect[0].Push(sample.cpu_temperature, sample.qpc);
                        break;
                    default:
                        pmlog_warn("Unhandled metric ring").pmwatch((int)metric);
                        break;
                    }
                }
                // Convert from the ms to seconds as GetTelemetryPeriod returns back
                // ms and SetInterval expects seconds.
                waiter.SetInterval(pm->GetGpuTelemetryPeriod() / 1000.);
                waiter.Wait();
                // Get the number of currently active streams
                auto num_active_streams = pm->GetActiveStreams();
                if (num_active_streams == 0) {
                    break;
                }
            }
        }
    }
    catch (...) {
        pmlog_error(util::ReportException("Failure in telemetry loop"));
    }
}



void PresentMonMainThread(Service* const pSvc)
{
    namespace rn = std::ranges; namespace vi = rn::views;

    assert(pSvc);

    // these thread containers need to be created outside of the try scope
    // so that if an exception happens, it won't block during unwinding,
    // trying to join threads that are waiting for a stop signal
    std::jthread gpuTelemetryThread;
    std::jthread cpuTelemetryThread;

    try {
        // alias for options
        auto& opt = clio::Options::Get();

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
            pComms = ipc::MakeServiceComms(*opt.shmNamePrefix);
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
        // container for all GPU telemetry providers
        PowerTelemetryContainer ptc;

        // Set the created power telemetry container 
        pm.SetPowerTelemetryContainer(&ptc);

        // Start named pipe action RPC server (active threaded)
        auto pActionServer = std::make_unique<ActionServer>(pSvc, &pm, opt.controlPipe.AsOptional());

        try {
            gpuTelemetryThread = std::jthread{ PowerTelemetryThreadEntry_, pSvc, &pm, &ptc, pComms.get() };
        }
        catch (...) {
            LOG(ERROR) << "failed creating gpu(power) telemetry thread" << std::endl;
        }

        // Create CPU telemetry
        std::shared_ptr<pwr::cpu::CpuTelemetry> cpu;
        try {
            // Try to use WMI for metrics sampling
            cpu = std::make_shared<pwr::cpu::wmi::WmiCpu>();
        }
        catch (const std::runtime_error& e) {
            LOG(ERROR) << "failed creating wmi cpu telemetry thread; Status: " << e.what() << std::endl;
        }
        catch (...) {
            LOG(ERROR) << "failed creating wmi cpu telemetry thread" << std::endl;
        }

        if (cpu) {
            cpuTelemetryThread = std::jthread{ CpuTelemetryThreadEntry_, pSvc, &pm, pComms.get(), cpu.get()};
            pm.SetCpu(cpu);
            // sample once to populate the cap bits
            cpu->Sample();
            // determine vendor based on device name
            // TODO: move this logic either into system (CPU) provider or
            // into the ipc components
            const auto vendor = [&] {
                const auto lowerNameRn = cpu->GetCpuName() | vi::transform(tolower);
                const std::string lowerName{ lowerNameRn.begin(), lowerNameRn.end() };
                if (lowerName.contains("intel")) {
                    return PM_DEVICE_VENDOR_INTEL;
                }
                else if (lowerName.contains("amd")) {
                    return PM_DEVICE_VENDOR_AMD;
                }
                else {
                    return PM_DEVICE_VENDOR_UNKNOWN;
                }
            }();
            // register cpu
            pComms->RegisterCpuDevice(vendor, cpu->GetCpuName(), 
                ipc::intro::ConvertBitset(cpu->GetCpuTelemetryCapBits()));
            // after registering, we know that at least the store is available even
            // if the introspection itself is not complete
            auto& systemStore = pComms->GetSystemDataStore();
            // TODO: replace this placeholder routine for populating statics
            systemStore.statics.cpuName = cpu->GetCpuName().c_str();
            systemStore.statics.cpuPowerLimit = cpu->GetCpuPowerLimit();
            systemStore.statics.cpuVendor = vendor;
        } else {
            // We were unable to determine the cpu.
            pComms->RegisterCpuDevice(PM_DEVICE_VENDOR_UNKNOWN, "UNKNOWN_CPU",
                ipc::intro::ConvertBitset(CpuTelemetryBitset{}));
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
    }
    catch (...) {
        LOG(ERROR) << "Exception in PMMainThread, bailing" << std::endl;
        if (pSvc) {
            pSvc->SignalServiceStop(-1);
        }
    }
}
