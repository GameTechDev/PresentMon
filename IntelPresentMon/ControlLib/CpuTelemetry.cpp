// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include <format>
#include "CpuTelemetry.h"

#define GOOGLE_GLOG_DLL_DECL
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

namespace pwr::cpu {

std::string CpuTelemetry::GetCpuName() {
  if (cpu_name_.size() == 0) {
    std::wstring local_cpu_name{};
    if (ExecuteWQLProcessorNameQuery(local_cpu_name)) {
      cpu_name_ = ConvertFromWideString(std::move(local_cpu_name));
    } else {
      cpu_name_ = "UNKNOWN_CPU";
    }
  }

  return cpu_name_;
}

bool CpuTelemetry::ExecuteWQLProcessorNameQuery(
    std::wstring& processor_name) {
  struct ComHelper {
    ComHelper() {
      HRESULT result{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};

      if (FAILED(result)) {
        throw std::runtime_error(
            std::format("Failed CoInitializeEx. Result: {}", result).c_str());
      }

      result = CoInitializeSecurity(
          nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

      if (FAILED(result)) {
        CoUninitialize();
        throw std::runtime_error(
            std::format("Failed CoInitializeSecurity. Result: {}", result)
                .c_str());
      }
    }
    ComHelper(const ComHelper& t) = delete;
    ComHelper& operator=(const ComHelper& t) = delete;
    ~ComHelper() { CoUninitialize(); }
  };

  // Initialize to empty state
  processor_name.clear();

  try {
    ComHelper com_helper;

    std::wstring query(L"SELECT Name FROM Win32_Processor");
    std::wstring server_name(L"ROOT\\CIMV2");
    std::wstring locale(L"MS_409");

    Microsoft::WRL::ComPtr<IWbemLocator> locator;
    HRESULT result =
        CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                         IID_IWbemLocator, (&locator));

    if (FAILED(result)) {
      LOG(INFO) << "Failed CoCreateInstance: " << result << std::endl;
      return false;
    }

    Microsoft::WRL::ComPtr<IWbemServices> services;
    result = locator->ConnectServer(_bstr_t(server_name.c_str()), nullptr,
                                    nullptr, _bstr_t(locale.c_str()), NULL,
                                    nullptr, nullptr, &services);

    if (FAILED(result)) {
      LOG(INFO) << "Failed ConnectServer: " << result << std::endl;
      return false;
    }

    result =
        CoSetProxyBlanket(services.Get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                          nullptr, RPC_C_AUTHN_LEVEL_CALL,
                          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    if (FAILED(result)) {
      LOG(INFO) << "Failed CoSetProxyBlanket: " << result << std::endl;
      return false;
    }

    Microsoft::WRL::ComPtr<IEnumWbemClassObject> enumerator;
    result = services->ExecQuery(
        bstr_t(L"WQL"), bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
        &enumerator);

    if (FAILED(result)) {
      LOG(INFO) << "Failed ExecQuery: " << result << std::endl;
      return false;
    }

    IWbemClassObject* ClassObject{};
    DWORD returned{};
    result = enumerator->Next(WBEM_INFINITE, 1, &ClassObject, &returned);

    if (FAILED(result) || returned == 0) {
      LOG(INFO) << "Failed Next: " << result << std::endl;
      return false;
    }

    VARIANT variant{};
    result = ClassObject->Get(L"Name", 0, &variant, nullptr, nullptr);
    if (FAILED(result)) {
      LOG(INFO) << "Failed Name Get: " << result << std::endl;
      return false;
    }

    if (variant.vt == VT_BSTR) {
      processor_name = variant.bstrVal;
    }

    VariantClear(&variant);
    ClassObject->Release();

    return true;

  } catch (const std::runtime_error& e) {
    LOG(INFO) << e.what() << std::endl;
    return false;
  } catch (...) {
    LOG(INFO) << "Unknown COM Initialize Error" << std::endl;
    return false;
  }
}

}