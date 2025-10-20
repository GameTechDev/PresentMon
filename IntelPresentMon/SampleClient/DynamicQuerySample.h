// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

bool FindFirstAvailableGpuMetric(pmapi::Session& pSession, PM_METRIC& gpuMetric, uint32_t& gpuMetricDeviceId, std::string& gpuMetricName)
{
    // Search through introspection for first available GPU
    // metric. If we happen to be on a machine with an unsupported
    // GPU return false
    auto pIntrospectionRoot = pSession.GetIntrospectionRoot();
    auto metricEnums = pIntrospectionRoot->FindEnum(PM_ENUM_METRIC);

    // Loop through ALL PresentMon metrics
    for (auto metric : pIntrospectionRoot->GetMetrics())
    {
        // Go through the device metric info to determine the metric's availability
        auto metricInfo = metric.GetDeviceMetricInfo();
        for (auto mi : metricInfo)
        {
            // Check to see if the returned device is non-zero and if it's
            // available on the system. Non-zero device id's indicate this
            // metric is associated with a device
            if (mi.GetDevice().GetId() != 0 && mi.IsAvailable())
            {
                // Look through PM_ENUM_METRIC enums to gather the metric symbol
                for (auto key : metricEnums.GetKeys())
                {
                    if (key.GetId() == metric.GetId())
                    {
                        // Save off the PM_METRIC id
                        gpuMetric = metric.GetId();
                        // Save off the device id associated with the PM_METRIC
                        gpuMetricDeviceId = mi.GetDevice().GetId();
                        gpuMetricName = key.GetName();
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

int AddGpuMetric(pmapi::Session& pSession, unsigned int processId, double windowSize, double metricOffset)
{
    try {
        auto processTracker = pSession.TrackProcess(processId);

        std::vector<PM_QUERY_ELEMENT> elements;
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_APPLICATION, .stat = PM_STAT_MID_POINT, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_95, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_PERCENTILE_90, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MAX, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_PRESENTED_FPS, .stat = PM_STAT_MIN, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_CPU_FRAME_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_CPU_WAIT, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_GPU_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_GPU_BUSY, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_DISPLAY_LATENCY, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_DISPLAYED_TIME, .stat = PM_STAT_AVG, .deviceId = 0, .arrayIndex = 0 });
        elements.push_back(PM_QUERY_ELEMENT{ .metric = PM_METRIC_CLICK_TO_PHOTON_LATENCY, .stat = PM_STAT_NON_ZERO_AVG, .deviceId = 0, .arrayIndex = 0 });

        PM_METRIC gpuMetric;
        uint32_t gpuDeviceId;
        std::string gpuMetricName;
        if (FindFirstAvailableGpuMetric(pSession, gpuMetric, gpuDeviceId, gpuMetricName)) {
            elements.push_back(PM_QUERY_ELEMENT{ .metric = gpuMetric, .stat = PM_STAT_AVG, .deviceId = gpuDeviceId, .arrayIndex = 0 });
        }

        auto dynamicQuery = pSession.RegisterDynamicQuery(elements, windowSize, metricOffset);
        auto blobs = dynamicQuery.MakeBlobContainer(1u);

        if (InitializeConsole() == false) {
            std::cout << "\nFailed to initialize console.\n";
            return -1;
        }

        while (!_kbhit()) {
            dynamicQuery.Poll(processTracker, blobs);

            for (auto pBlob : blobs) {
                ConsolePrintLn("Process Name = %s", *reinterpret_cast<const std::string*>(&pBlob[elements[0].dataOffset]));
                ConsolePrintLn("Presented FPS Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[1].dataOffset]));
                ConsolePrintLn("Presented FPS 90% = %f", *reinterpret_cast<const double*>(&pBlob[elements[2].dataOffset]));
                ConsolePrintLn("Presented FPS 95% = %f", *reinterpret_cast<const double*>(&pBlob[elements[3].dataOffset]));
                ConsolePrintLn("Presented FPS 99% = %f", *reinterpret_cast<const double*>(&pBlob[elements[4].dataOffset]));
                ConsolePrintLn("Presented FPS Max = %f", *reinterpret_cast<const double*>(&pBlob[elements[5].dataOffset]));
                ConsolePrintLn("Presented FPS Min = %f", *reinterpret_cast<const double*>(&pBlob[elements[6].dataOffset]));
                ConsolePrintLn("Frame Duration Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[7].dataOffset]));
                ConsolePrintLn("Frame Pacing Stall Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[8].dataOffset]));
                ConsolePrintLn("GPU Duration Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[9].dataOffset]));
                ConsolePrintLn("GPU Busy Time Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[10].dataOffset]));
                ConsolePrintLn("Display Latency Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[11].dataOffset]));
                ConsolePrintLn("Display Duration Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[12].dataOffset]));
                ConsolePrintLn("Input Latency Average = %f", *reinterpret_cast<const double*>(&pBlob[elements[13].dataOffset]));
                if (gpuMetricName.length() != 0)
                {
                    ConsolePrintLn(
                        std::format("{} Average = {}",
                            gpuMetricName, *reinterpret_cast<const double*>(&pBlob[elements[14].dataOffset])).c_str()
                    );
                }

            }
            CommitConsole();
            Sleep(10);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cout << "Unknown Error" << std::endl;
        return -1;
    }

    return 0;
}

int DynamicQuerySample(std::unique_ptr<pmapi::Session>&& pSession, double windowSize, double metricOffset, bool addGPUMetric)
{
    using namespace std::chrono_literals;
    using namespace pmapi;

    try {
        std::optional<unsigned int> processId;
        std::optional<std::string> processName;
        
        GetProcessInformation(processName, processId);
        if (!processId.has_value())
        {
            std::cout << "Must set valid process name or process id:\n";
            std::cout << "--dynamic-query-sample [--process-id id | --process-name name.exe] [--add-gpu-metric]\n";
            return -1;
        }

        auto& opt = clio::Options::Get();
        if (addGPUMetric) {
            return AddGpuMetric(*pSession, processId.value(), windowSize, metricOffset);
        }

        auto proc = pSession->TrackProcess(processId.value());

        PM_BEGIN_FIXED_DYNAMIC_QUERY(MyDynamicQuery)
            FixedQueryElement appName{ this, PM_METRIC_APPLICATION, PM_STAT_NONE };
            FixedQueryElement fpsAvg{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_AVG };
            FixedQueryElement fps90{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_PERCENTILE_90 };
            FixedQueryElement fps95{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_PERCENTILE_95 };
            FixedQueryElement fps99{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_PERCENTILE_99 };
            FixedQueryElement fpsMax{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_MAX };
            FixedQueryElement fpsMin{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_MIN };
            FixedQueryElement frameDurationAvg{ this, PM_METRIC_CPU_FRAME_TIME, PM_STAT_AVG };
            FixedQueryElement fpStallAvg{ this, PM_METRIC_CPU_WAIT, PM_STAT_AVG };
            FixedQueryElement gpuDurationAvg{ this, PM_METRIC_GPU_TIME, PM_STAT_AVG };
            FixedQueryElement gpuBusyTimeAvg{ this, PM_METRIC_GPU_BUSY, PM_STAT_AVG };
            FixedQueryElement gpuDisplayLatencyAvg{ this, PM_METRIC_DISPLAY_LATENCY, PM_STAT_AVG };
            FixedQueryElement gpuDisplayDurationAvg{ this, PM_METRIC_DISPLAYED_TIME, PM_STAT_AVG };
            FixedQueryElement gpuPhotonLatencyAvg{ this, PM_METRIC_CLICK_TO_PHOTON_LATENCY, PM_STAT_NON_ZERO_AVG };
            FixedQueryElement gpuPower{ this, PM_METRIC_GPU_POWER, PM_STAT_AVG, 1 };
            FixedQueryElement presentMode{ this, PM_METRIC_PRESENT_MODE, PM_STAT_MID_POINT };
            FixedQueryElement gpuName{ this, PM_METRIC_GPU_NAME, PM_STAT_NONE, 1 };
            FixedQueryElement fanSpeed{ this, PM_METRIC_GPU_FAN_SPEED, PM_STAT_AVG, 1 };
        PM_END_FIXED_QUERY dq{ *pSession, windowSize, metricOffset, 1, 1 };

        if (InitializeConsole() == false) {
            std::cout << "\nFailed to initialize console.\n";
            return -1;
        }

        while (!_kbhit()) {
            dq.Poll(proc);
            ConsolePrintLn("App Name = %s", dq.appName.As<std::string>().c_str());
            ConsolePrintLn("Presented FPS Average = %f", dq.fpsAvg.As<double>());
            ConsolePrintLn("Presented FPS 90% = %f", dq.fps90.As<double>());
            ConsolePrintLn("Presented FPS 95% = %f", dq.fps95.As<double>());
            ConsolePrintLn("Presented FPS 99% = %f", dq.fps99.As<double>());
            ConsolePrintLn("Presented FPS Max = %f", dq.fpsMax.As<double>());
            ConsolePrintLn("Presented FPS Min = %f", dq.fpsMin.As<double>());
            ConsolePrintLn("Frame Duration Average = %f", dq.frameDurationAvg.As<double>());
            ConsolePrintLn("Frame Pacing Stall Average = %f", dq.fpStallAvg.As<double>());
            ConsolePrintLn("GPU Duration Average = %f", dq.gpuDisplayDurationAvg.As<double>());
            ConsolePrintLn("GPU Busy Time Average = %f", dq.gpuBusyTimeAvg.As<double>());
            ConsolePrintLn("Display Latency Average = %f", dq.gpuDisplayLatencyAvg.As<double>());
            ConsolePrintLn("Display Duration Average = %f", dq.gpuDisplayDurationAvg.As<double>());
            ConsolePrintLn("Input Latency Average = %f", dq.gpuPhotonLatencyAvg.As<double>());
            ConsolePrintLn("GPU Power Average = %f", dq.gpuPower.As<double>());
            ConsolePrintLn("Present Mode = %s", dq.presentMode.As<std::string>().c_str());
            ConsolePrintLn("GPU Name = %s", dq.gpuName.As<std::string>().c_str());
            if (dq.fanSpeed.IsAvailable()) {
                ConsolePrintLn("GPU Fan Speed = %f", dq.fanSpeed.As<double>());
            }
            else {
                ConsolePrintLn("GPU Fan Speed = Unavailable");
            }
            CommitConsole();
            std::this_thread::sleep_for(20ms);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cout << "Unknown Error" << std::endl;
        return -1;
    }

    return 0;
}
