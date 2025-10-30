// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "IntelPowerTelemetryProvider.h"
#include "IntelPowerTelemetryAdapter.h"
#include "Logging.h"
#include "Exceptions.h"
#include "../CommonUtilities/ref/GeneratedReflection.h"

using namespace pmon;
using namespace util;
using v = log::V;

namespace pwr::intel
{
    IntelPowerTelemetryProvider::IntelPowerTelemetryProvider()
    {
        // TODO(megalvan): Currently using the default Id of all zeros. Do we need
        // to obtain a legit application Id or is default fine?
        ctl_init_args_t ctl_init_args{
            .Size = sizeof(ctl_init_args),
            .AppVersion = CTL_MAKE_VERSION(CTL_IMPL_MAJOR_VERSION, CTL_IMPL_MINOR_VERSION),
            .flags = CTL_INIT_FLAG_USE_LEVEL_ZERO,
        };

        // initialize the igcl api
        if (const auto result = ctlInit(&ctl_init_args, &apiHandle); result != CTL_RESULT_SUCCESS) {
            if (result != CTL_RESULT_ERROR_NOT_INITIALIZED) {
                IGCL_ERR(result);
            }
            throw Except<TelemetrySubsystemAbsent>("Unable to initialize Intel Graphics Control Library");
        }
        pmlog_verb(v::tele_gpu)("Initializing IGCL").pmwatch(ref::DumpGenerated(ctl_init_args));

        pmlog_info(std::format("Initialized IGCL with version={}.{}",
            CTL_MAJOR_VERSION(ctl_init_args.SupportedVersion),
            CTL_MINOR_VERSION(ctl_init_args.SupportedVersion)
        ));

        // enumerate devices available via igcl (get a list of device handles)
        std::vector<ctl_device_adapter_handle_t> handles;
        {
            uint32_t count = 0;
            if (const auto result = ctlEnumerateDevices(apiHandle, &count, nullptr); result != CTL_RESULT_SUCCESS) {
                IGCL_ERR(result);
                throw std::runtime_error{ "failed igcl device enumeration (get count)" };
            }
            pmlog_verb(v::tele_gpu)("Getting device count").pmwatch(count);

            handles.resize(count);
            if (const auto result = ctlEnumerateDevices(apiHandle, &count, handles.data()); result != CTL_RESULT_SUCCESS) {
                IGCL_ERR(result);
                throw std::runtime_error{ "failed igcl device enumeration (get list)" };
            }
        }

        // create adaptor object for each device handle
        for (auto& handle : handles)
        {
            try {
                adapterPtrs.push_back(std::make_shared<IntelPowerTelemetryAdapter>(handle));
            }
            catch (const IntelPowerTelemetryAdapter::NonGraphicsDeviceException&) {}
            catch (const std::exception& e) { TELE_ERR(e.what()); }
            catch (...) { TELE_ERR("unknown error"); }
        }
    }

    IntelPowerTelemetryProvider::~IntelPowerTelemetryProvider()
    {
        // want to make sure we clear adapters *before* we close the API
        adapterPtrs.clear();

        if (apiHandle)
        {
            ctlClose(apiHandle);
        }
    }

    const std::vector<std::shared_ptr<PowerTelemetryAdapter>>& IntelPowerTelemetryProvider::GetAdapters() noexcept
    {
        return adapterPtrs;
    }

    uint32_t IntelPowerTelemetryProvider::GetAdapterCount() const noexcept
    {
        return (uint32_t)adapterPtrs.size();
    }
}