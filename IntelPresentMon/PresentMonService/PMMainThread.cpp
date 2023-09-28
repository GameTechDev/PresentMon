// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Service.h"
#include "NamedPipeServer.h"
#include "PresentMon.h"
#include "PowerTelemetryContainer.h"
#include "..\ControlLib\WmiCpu.h"
#include "..\PresentMonUtils\StringUtils.h"
#include <filesystem>

#define GOOGLE_GLOG_DLL_DECL
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

void InitializeLogging(const char* servicename, const char* location, const char* basename,
                       const char* extension, int level) {
  // initialize glog
  if (!google::IsGoogleLoggingInitialized()) {
    google::InitGoogleLogging(servicename);
    if (!basename) {
      basename = "pmon-srv";
    }
    if (location) {
      google::SetLogDestination(
          google::GLOG_INFO,
          std::format("{}\\{}-info-", location, basename).c_str());
      google::SetLogDestination(
          google::GLOG_WARNING,
          std::format("{}\\{}-warn-", location, basename).c_str());
      google::SetLogDestination(
          google::GLOG_ERROR,
          std::format("{}\\{}-err-", location, basename).c_str());
      google::SetLogDestination(
          google::GLOG_FATAL,
          std::format("{}\\{}-fatal-", location, basename).c_str());
    }
    if (extension) {
      google::SetLogFilenameExtension(extension);
    }
    FLAGS_minloglevel = std::clamp(level, 0, 3);
  }
}

bool NanoSleep(int32_t ms) {
  HANDLE timer;
  LARGE_INTEGER li;
  // Convert from ms to 100ns units and negate
  int64_t ns = -10000 * (int64_t)ms;
  // Create a high resolution table
  if (!(timer = CreateWaitableTimerEx(NULL, NULL,
                                      CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                      TIMER_ALL_ACCESS))) {
    return false;
  }
  li.QuadPart = ns;
  if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
    CloseHandle(timer);
    return false;
  }
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
  return true;
}

bool CheckStartParameters(std::vector<std::wstring>& start_parameters,
                          std::wstring parameter) {
  
  for(auto p : start_parameters) {
    if (p.compare(parameter) == 0) {
      return true;
    }
  }
  return false;
}

bool SetupFileLogging(std::vector<std::wstring>& start_parameters) {
  
  // First parameter is executable name
  std::string exe_name = ConvertFromWideString(start_parameters[0]);
  if (exe_name.size() == 0){
    return false;
  }

  // Now search for --enable_file_logging and the path
  bool check_next_parameter = false;
  for (auto p : start_parameters) {
    if (check_next_parameter) {
      std::filesystem::path path(p);
      if (std::filesystem::is_directory(path)) {
        std::string log_path = ConvertFromWideString(p);
        if (log_path.size() == 0) {
          return false;
        }
        InitializeLogging(exe_name.c_str(), log_path.c_str(),
                          "pm-srv", ".log", 0);
        return true;
      } else {
        return false;
      }
    }
    if (p.compare(L"--enable_file_logging") == 0) {
      check_next_parameter = true;
    }
  }
  return false;
}

// Attempt to use a high resolution sleep but if not
// supported use regular Sleep().
void PmSleep(int32_t ms) {
  if (!NanoSleep(ms)) {
    Sleep(ms);
  }
  return;
}

void IPCCommunication(Service* srv, PresentMon* pm)
{
    bool createNamedPipeServer = true;

    if (srv == nullptr) {
        return;
    }

    NamedPipeServer* nps = new NamedPipeServer(srv, pm);
    if (nps == nullptr) {
        return;
    }

    while (createNamedPipeServer) {
            DWORD result = nps->RunServer();
            if (result == ERROR_SUCCESS) {
                createNamedPipeServer = false;
            }
            else {
                // We were unable to start our named pipe server. Sleep for
                // a bit and then try again.
                PmSleep(3000);
            }
    }

    delete nps;

    return;
}

void PowerTelemetry(Service* srv, PresentMon* pm,
                    PowerTelemetryContainer* ptc) {
  if (srv == nullptr || pm == nullptr || ptc == nullptr) {
    return;
  }

  // Grab the initial power telemetry providers right at the start
  // to support client adapter enumeration queries
  try {
    ptc->QueryPowerTelemetrySupport();
  } catch (...) {
  }          

  // Get the streaming start event
  HANDLE events[2];
  events[0] = pm->GetStreamingStartHandle();
  events[1] = srv->GetServiceStopHandle();

  while (1) {
    auto waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);
    auto i = waitResult - WAIT_OBJECT_0;
    if (i == 1) {
      return;
    }
    while (WaitForSingleObject(srv->GetServiceStopHandle(), 0) !=
           WAIT_OBJECT_0) {
      if (WaitForSingleObject(srv->GetResetPowerTelemetryHandle(), 0) ==
          WAIT_OBJECT_0) {
        try {
          ptc->QueryPowerTelemetrySupport();
        } catch (...) {
        }          
      }
      for (auto& adapter : ptc->GetPowerTelemetryAdapters()) {
        adapter->Sample();
      }
      PmSleep(pm->GetGpuTelemetryPeriod());
      // Get the number of currently active streams
      auto num_active_streams = pm->GetActiveStreams();
      if (num_active_streams == 0) {
        break;
      }
    }
  }
}

void CpuTelemetry(Service* srv, PresentMon* pm,
                  std::shared_ptr<pwr::cpu::CpuTelemetry>* cpu) {

  if (srv == nullptr || pm == nullptr) {
    // TODO: log error on this condition
    return;
  }

  HANDLE events[2];
  events[0] = pm->GetStreamingStartHandle();
  events[1] = srv->GetServiceStopHandle();

  while (1) {
    auto waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);
    auto i = waitResult - WAIT_OBJECT_0;
    if (i == 1) {
      return;
    }
    while (WaitForSingleObject(srv->GetServiceStopHandle(), 0) !=
            WAIT_OBJECT_0) {
      cpu->get()->Sample();
      PmSleep(pm->GetGpuTelemetryPeriod());
      // Get the number of currently active streams
      auto num_active_streams = pm->GetActiveStreams();
      if (num_active_streams == 0) {
        break;
      }
    }
  }
}

DWORD WINAPI PresentMonMainThread(LPVOID lpParam)
{
    if (lpParam == nullptr) {
        return ERROR_INVALID_DATA;
    }

    // Extract out the PresentMon Service pointer
    const auto srv = static_cast<Service*>(lpParam);
    // Grab the stop service event handle
    const auto serviceStopHandle = srv->GetServiceStopHandle();

    // Simple checking of start parameters.
    auto start_parameters = srv->GetArguments();

    // Also check if the service is to be debugged while starting up
    auto debug_service =
        CheckStartParameters(start_parameters, L"--debug_service");

    while (debug_service) {
        if (WaitForSingleObject(serviceStopHandle, 0) != WAIT_OBJECT_0) {
            PmSleep(500);
        } else {
            return ERROR_SUCCESS;
        }
    }

    if (CheckStartParameters(start_parameters, L"--enable_file_logging")) {
        SetupFileLogging(start_parameters);
    }

    PresentMon pm;
    PowerTelemetryContainer ptc;

    // Set the created power telemetry container 
    pm.SetPowerTelemetryContainer(&ptc);

    // Start IPC communication thread
    std::jthread ipc_thread(IPCCommunication, srv, &pm);

    // Launch telemetry thread
    std::jthread telemetry_thread;

    try {
      telemetry_thread = std::jthread{ PowerTelemetry, srv, &pm, &ptc };
    } catch (...) {}
    
    // Create CPU telemetry
    std::shared_ptr<pwr::cpu::CpuTelemetry> cpu;
    std::jthread cpu_telemetry_thread;
    try {
      // Try to use WMI for metrics sampling
      cpu = std::make_shared<pwr::cpu::wmi::WmiCpu>();
    } catch (const std::runtime_error& e) {
      LOG(INFO) << "WMI Failure Status: " << e.what() << std::endl;
    } catch (...) {}

    if (cpu) {
      cpu_telemetry_thread = std::jthread{ CpuTelemetry, srv, &pm, &cpu };
      pm.SetCpu(cpu);
    }

    while (WaitForSingleObject(serviceStopHandle, 0) != WAIT_OBJECT_0) {
        pm.CheckTraceSessions();
        PmSleep(500);
    }

    // Stop the PresentMon session
    pm.StopTraceSession();

    return ERROR_SUCCESS;
}
