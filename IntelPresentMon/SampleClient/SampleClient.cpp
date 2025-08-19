// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include <string>
#include "Console.h"
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <TlHelp32.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <format>
#include <chrono>
#include <conio.h>
#include <boost/process.hpp>
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "CliOptions.h"

#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapper/FixedQuery.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "Utils.h"
#include "DynamicQuerySample.h"
#include "FrameQuerySample.h"
#include "IntrospectionSample.h"
#include "CheckMetricSample.h"
#include "WrapperStaticQuery.h"
#include "MetricListSample.h"
#include "LogDemo.h"
#include "DiagnosticDemo.h"
#include "LogSetup.h"

#include "../CommonUtilities/IntervalWaiter.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../CommonUtilities/win/ProcessMapBuilder.h"

void RunPlaybackFrameQuery()
{
    auto& opt = clio::Options::Get();

    // launch the service, getting it to process an ETL gold file (custom object names)
    namespace bp = boost::process;
    using namespace std::literals;
    const auto pipeName = R"(\\.\pipe\pmsvc-ctl-pipe-tt)"s;
    std::vector<std::string> dargs;
    if (opt.servicePacePlayback) {
        dargs.push_back("--pace-playback");
    }
    if (opt.serviceEtlPath) {
        dargs.append_range(std::vector{ "--etl-test-file"s, *opt.serviceEtlPath });
    }
    bp::child svc{
        "PresentMonService.exe"s,
        // "--timed-stop"s, "10000"s,
        "--control-pipe"s, pipeName,
        "--nsm-prefix"s, "pmon_nsm_tt_"s,
        "--intro-nsm"s, "svc-intro-tt"s,
        "--etw-session-name"s, "svc-sesh-tt"s,
        bp::args(dargs),
    };

    // connect to the service with custom control pipe name
    pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName, 500);
    auto api = pmapi::Session{ pipeName };

    if (!opt.serviceEtlPath) {
        // set ETW flush to realtime
        api.SetEtwFlushPeriod(50);
    }

    // setup basic fixed frame query
    PM_BEGIN_FIXED_FRAME_QUERY(MyFrameQuery)
        pmapi::FixedQueryElement frameTime{ this, PM_METRIC_CPU_FRAME_TIME, PM_STAT_NONE };
        pmapi::FixedQueryElement startTime{ this, PM_METRIC_CPU_START_TIME, PM_STAT_NONE };
    PM_END_FIXED_QUERY query{ api, 50 };

    uint32_t pid = 0;
    if (opt.processName && !opt.processId) {
         const auto map = pmon::util::win::ProcessMapBuilder{}.AsNameMap(true);
         pid = map.at(pmon::util::str::ToWide(*opt.processName)).pid;
    }
    else {
        // track the pid we know to be active in the ETL (1268 for dwm in gold_0)
        pid = opt.processId.AsOptional().value_or(1268);
    }
    auto tracker = api.TrackProcess(pid);
    std::this_thread::sleep_for(500ms);

    uint32_t frameCount = 0;
    uint32_t emptyCount = 0;
    try {
        // output frame events as they are received
        while (true) {
            const auto n = query.ForEachConsume(tracker, [&] {
                std::cout
                    << "(" << query.PeekBlobContainer().GetNumBlobsPopulated() << ") "
                    << "Start: " << query.startTime.As<double>()
                    << " x FrameTime: " << query.frameTime.As<double>()
                    << "  (" << ++frameCount << ")\n";
            });
            if (n) {
                emptyCount = 0;
            }
            else {
                std::this_thread::yield();
                emptyCount++;
            }

            if (emptyCount >= 10) {
                pmStopPlayback_(api.GetHandle());
            }
        }
    }
    catch (const pmapi::ApiErrorException& ex) {
        if (ex.GetCode() != PM_STATUS_INVALID_PID) {
            std::cout << "Unexpected Error (" << frameCount << " frames processed).\n";
            throw;
        }
        std::cout << "Process exit detected, ending frame processing (" << frameCount << " frames processed).\n";
    }
}

void RunPlaybackDynamicQuery()
{
    auto& opt = clio::Options::Get();

    // launch the service, getting it to process an ETL gold file (custom object names)
    namespace bp = boost::process;
    using namespace std::literals;
    const auto pipeName = R"(\\.\pipe\pmsvc-ctl-pipe-tt)"s;
    std::vector<std::string> dargs;
    if (opt.servicePacePlayback) {
        dargs.push_back("--pace-playback");
    }
    if (opt.serviceEtlPath) {
        dargs.append_range(std::vector{ "--etl-test-file"s, *opt.serviceEtlPath });
    }
    bp::child svc{
        "PresentMonService.exe"s,
        // "--timed-stop"s, "10000"s,
        "--control-pipe"s, pipeName,
        "--nsm-prefix"s, "pmon_nsm_tt_"s,
        "--intro-nsm"s, "svc-intro-tt"s,
        "--etw-session-name"s, "svc-sesh-tt"s,
        bp::args(dargs),
    };

    // connect to the service with custom control pipe name
    pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName, 500);
    auto api = pmapi::Session{ pipeName };

    std::ofstream csv{ "polled.csv" };
    csv << "time,fps\n";

    if (!opt.serviceEtlPath) {
        // set ETW flush to realtime
        api.SetEtwFlushPeriod(50);
    }

    // setup fixed dynamic query for basic metrics (FPS presented)
    PM_BEGIN_FIXED_DYNAMIC_QUERY(MyDynamicQuery)
        pmapi::FixedQueryElement fpsAvg{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_AVG };
    PM_END_FIXED_QUERY query{ api, 200., 50., 1, 1 };

    // track the pid we know to be active in the ETL (1268 for dwm in gold_0)
    auto tracker = api.TrackProcess(opt.processId.AsOptional().value_or(1268));

    pmon::util::IntervalWaiter waiter{ 0.1 };

    using Clock = std::chrono::high_resolution_clock;
    const auto start = Clock::now();

    try {
        // output realtime samples to console at a steady interval
        while (true) {
            std::chrono::duration<double> elapsed = Clock::now() - start;
            if (elapsed.count() > 20.) {
                std::cout << "Fixed time elapsed, exiting...." << std::endl;
                break;
            }
            query.Poll(tracker);
            std::cout << "TIME: " << elapsed.count() << "  FPS: " << query.fpsAvg.As<float>() << "\n";
            csv << elapsed.count() << "," << query.fpsAvg.As<float>() << std::endl;
            waiter.Wait();
        }
    }
    catch (const pmapi::ApiErrorException& ex) {
        if (ex.GetCode() != PM_STATUS_INVALID_PID) {
            std::cout << "Unexpected Error\n";
            throw;
        }
        std::cout << "Process exit detected, ending frame processing.\n";
    }
}

void RunPlaybackDynamicQueryN()
{
    auto& opt = clio::Options::Get();

    // launch the service, getting it to process an ETL gold file (custom object names)
    namespace bp = boost::process;
    using namespace std::literals;
    const auto pipeName = R"(\\.\pipe\pmsvc-ctl-pipe-tt)"s;
    std::vector<std::string> dargs;
    if (opt.servicePacePlayback) {
        dargs.push_back("--pace-playback");
    }
    if (opt.serviceEtlPath) {
        dargs.append_range(std::vector{ "--etl-test-file"s, *opt.serviceEtlPath });
    }

    for (int x = 0; x < 10; x++) {
        bp::child svc{
            "PresentMonService.exe"s,
            // "--timed-stop"s, "10000"s,
            "--control-pipe"s, pipeName,
            "--nsm-prefix"s, "pmon_nsm_tt_"s,
            "--intro-nsm"s, "svc-intro-tt"s,
            "--etw-session-name"s, "svc-sesh-tt"s,
            bp::args(dargs),
        };

        // connect to the service with custom control pipe name
        pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500);
        auto api = pmapi::Session{ pipeName };

        std::ofstream csv{ std::format("polled_{}.csv", x)};
        csv << "time,fps\n";

        if (!opt.serviceEtlPath) {
            // set ETW flush to realtime
            api.SetEtwFlushPeriod(50);
        }

        // setup fixed dynamic query for basic metrics (FPS presented)
        PM_BEGIN_FIXED_DYNAMIC_QUERY(MyDynamicQuery)
            pmapi::FixedQueryElement fpsAvg{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_AVG };
        PM_END_FIXED_QUERY query{ api, 200., 50., 1, 1 };

        // track the pid we know to be active in the ETL (1268 for dwm in gold_0)
        auto tracker = api.TrackProcess(opt.processId.AsOptional().value_or(1268));

        pmon::util::IntervalWaiter waiter{ 0.1 };

        using Clock = std::chrono::high_resolution_clock;
        const auto start = Clock::now();

        try {
            // output realtime samples to console at a steady interval
            while (true) {
                std::chrono::duration<double> elapsed = Clock::now() - start;
                if (elapsed.count() > 20.) {
                    std::cout << "Fixed time elapsed, exiting...." << std::endl;
                    pmStopPlayback_(api.GetHandle());
                    break;
                }
                query.Poll(tracker);
                std::cout << "TIME: " << elapsed.count() << "  FPS: " << query.fpsAvg.As<float>() << "\n";
                csv << elapsed.count() << "," << query.fpsAvg.As<float>() << std::endl;
                waiter.Wait();
            }
        }
        catch (const pmapi::ApiErrorException& ex) {
            if (ex.GetCode() != PM_STATUS_INVALID_PID) {
                std::cout << "Unexpected Error\n";
                throw;
            }
            std::cout << "Process exit detected, ending frame processing.\n";
        }
    }
}

void IntrospectAllDynamicOptions()
{
    namespace bp = boost::process;
    using namespace std::literals;

    const auto pipeName = R"(\\.\pipe\pmsvc-ctl-pipe-tt)"s;
    bp::child svc{
        "PresentMonService.exe"s,
        "--control-pipe"s, pipeName,
        "--nsm-prefix"s, "pmon_nsm_tt_"s,
        "--intro-nsm"s, "svc-intro-tt"s,
        "--etw-session-name"s, "svc-sesh-tt"s,
    };

    // connect to the service with custom control pipe name
    pmon::util::pipe::DuplexPipe::WaitForAvailability(pipeName + "-in", 500);
    auto api = pmapi::Session{ pipeName };

    auto pIntro = api.GetIntrospectionRoot();

    for (const auto& m : pIntro->GetMetrics()) {
        if (m.GetType() != PM_METRIC_TYPE_DYNAMIC && m.GetType() != PM_METRIC_TYPE_DYNAMIC_FRAME) {
            continue;
        }
        auto dmi = m.GetDeviceMetricInfo();
        if (dmi.size() != 1) {
            continue;
        }
        if (!dmi.front().IsAvailable() || dmi.front().GetDevice().GetId() != 0) {
            continue;
        }
        std::cout << m.Introspect().GetSymbol() << ": " << m.Introspect().GetDescription() << "\n";
        for (const auto& s : m.GetStatInfo()) {
            std::cout << "   " << s.IntrospectStat().GetSymbol() << "\n";
        }
    }
}


int main(int argc, char* argv[])
{
    try {
        // command line options initialization
        if (auto e = clio::Options::Init(argc, argv)) {
            return *e;
        }
        auto& opt = clio::Options::Get();

        // use the middleware in the dev path esp. if we are running debug build
        // important to do this before any intra-process log linking occurs
        if (opt.middlewareDllPath) {
            pmLoaderSetPathToMiddlewareDll_(opt.middlewareDllPath->c_str());
        }

        // setup logging, including middleware intra-process cross-module link
        p2sam::LogChannelManager zLogMan_;
        p2sam::ConfigureLogging();

        // helper for connecting a pmapi session
        const auto ConnectSession = [&] {
            if (opt.controlPipe) {
                return std::make_unique<pmapi::Session>(*opt.controlPipe);
            }
            else {
                return std::make_unique<pmapi::Session>();
            }
        };

        // determine requested mode to run the sample app in
        switch (*opt.mode) {
        case clio::Mode::LogDemo:
            RunLogDemo(*opt.submode); break;
        case clio::Mode::DiagnosticsDemo:
            RunDiagnosticDemo(*opt.submode); break;
        case clio::Mode::Introspection:
            return IntrospectionSample(ConnectSession());
        case clio::Mode::CheckMetric:
            return CheckMetricSample(ConnectSession());
        case clio::Mode::DynamicQuery:
            return DynamicQuerySample(ConnectSession(), *opt.windowSize, *opt.metricOffset, false);
        case clio::Mode::AddGpuMetric:
            return DynamicQuerySample(ConnectSession(), *opt.windowSize, *opt.metricOffset, true);
        case clio::Mode::WrapperStaticQuery:
            return WrapperStaticQuerySample(ConnectSession());
        case clio::Mode::MetricList:
            return MetricListSample(ConnectSession());
        case clio::Mode::FrameQuery:
            return FrameQuerySample(ConnectSession(), false);
        case clio::Mode::CsvFrameQuery:
            return FrameQuerySample(ConnectSession(), true);
        case clio::Mode::PlaybackDynamicQuery:
            RunPlaybackDynamicQueryN(); break;
        case clio::Mode::PlaybackFrameQuery:
            RunPlaybackFrameQuery(); break;
        case clio::Mode::IntrospectAllDynamicOptions:
            IntrospectAllDynamicOptions(); break;
        default:
            throw std::runtime_error{ "unknown sample client mode" };
        }

        // exit code
        return 0;
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cout << "Unknown Error" << std::endl;
        return -1;
    }

}