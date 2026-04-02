// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NvidiaPowerTelemetryProvider.h"
#include "NvidiaPowerTelemetryAdapter.h"
#include "Logging.h"
#include "SignatureComparison.h"

namespace pwr::nv
{
    NvidiaPowerTelemetryProvider::NvidiaPowerTelemetryProvider(DeviceIdAllocator& allocator)
    {
        // enumerate nvapi gpu device handles
        std::vector<std::pair<NvPhysicalGpuHandle, NvapiAdapterSignature>> nvapiHandlePairs;
        {
            std::vector<NvPhysicalGpuHandle> nvapiHandles(32);

            // enumerate gpu handles
            auto count = (NvU32)nvapiHandles.size();
            if (!nvapi.Ok(nvapi.EnumPhysicalGPUs(nvapiHandles.data(), &count)))
            {
                throw std::runtime_error{ "Failed to enumerate nvapi gpus" };
            }
            // resize container to actual count of handles that were set
            nvapiHandles.resize(size_t(count));
            // package handles together with their adapter signatures
            for (auto& handle : nvapiHandles)
            {
                nvapiHandlePairs.emplace_back(handle, nvapi.GetAdapterSignature(handle));
            }
        }

        // enumerate nvml gpu device handles
        using NvmlHandlePair = std::pair<nvmlDevice_t, NvmlAdapterSignature>;
        std::vector<NvmlHandlePair> nvmlHandlePairs;
        {
            std::vector<nvmlDevice_t> nvmlHandles;

            // get number of devices
            unsigned int count = 0;
            if (!nvml.Ok(nvml.DeviceGetCount_v2(&count)))
            {
                throw std::runtime_error{ "Failed to get nvml device count" };
            }

            // get device handles by index from 0 to n_devices-1
            for (unsigned int i = 0; i < count; i++)
            {
                nvmlDevice_t handle;
                if (nvml.Ok(nvml.DeviceGetHandleByIndex_v2(i, &handle)))
                {
                    nvmlHandles.push_back(handle);
                }
                // TODO: else consider logging low severity error on fail
            }

            // package handles together with their adapter signatures
            for (auto& handle : nvmlHandles)
            {
                nvmlHandlePairs.emplace_back(handle, nvml.GetAdapterSignature(handle));
            }
        }

        // create adaptor object for each api adapter handle pair
        // lambda to factor out repeated code pattern adapter creation and insertion into container, with exception quelling
        const auto TryCreateAddAdapter = [this, &allocator](NvPhysicalGpuHandle nvapiHandle, std::optional<nvmlDevice_t> nvmlHandle) {
            try {
                adapterPtrs.push_back(std::make_shared<NvidiaPowerTelemetryAdapter>(
                    allocator.Next(), &nvapi, &nvml, nvapiHandle, nvmlHandle
                ));
            }
            catch (const std::exception& e) { TELE_ERR(e.what()); }
            catch (...) { TELE_ERR("unknown error"); }
        };
        // in the special case where there is only one handle for each api, we assume they match
        if (nvapiHandlePairs.size() == 1 && nvmlHandlePairs.size() == 1)
        {
            TryCreateAddAdapter(nvapiHandlePairs.front().first, nvmlHandlePairs.front().first);
            return;
        }
        // otherwise use signatures to greedy find nvml matches for each nvapi adapter handle
        for (const auto nvapiHandlePair : nvapiHandlePairs)
        {
            // find first matching nvml handle
            const auto i = std::ranges::find_if(nvmlHandlePairs, [&nvapiHandlePair](const NvmlHandlePair& nvmlPair) {
                return nvapiHandlePair.second == nvmlPair.second;
            });
            // create adapter object using both handles if we find a match
            if (i != nvmlHandlePairs.end())
            {
                TryCreateAddAdapter(nvapiHandlePair.first, i->first);
                // remove nvml handle from list of candidates after creating
                nvmlHandlePairs.erase(i);
            }
            else
            {
                // create adapter object using nvapi only
                TryCreateAddAdapter(nvapiHandlePair.first, std::nullopt);
            }
        }
    }

    const std::vector<std::shared_ptr<PowerTelemetryAdapter>>& NvidiaPowerTelemetryProvider::GetAdapters() noexcept
    {
        return adapterPtrs;
    }

    uint32_t NvidiaPowerTelemetryProvider::GetAdapterCount() const noexcept
    {
        return (uint32_t)adapterPtrs.size();
    }
}
