// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TelemetryAvailabilityMerge.h"
#include "TelemetryMetricDiscovery.h"
#include <algorithm>

namespace pmon::tel
{
    ipc::MetricCapabilityEntry MergeProviderAvailabilityEntries(
        PM_DEVICE_TYPE deviceType,
        const std::vector<ipc::MetricCapabilityEntry>& providerEntries)
    {
        if (providerEntries.empty()) {
            return ipc::MetricCapabilityEntry{
                .arraySize = 0,
                .availability = DefaultMissingTelemetryMetricAvailability(deviceType),
            };
        }

        // Any AVAILABLE provider wins; otherwise pick the highest-priority non-available reason.
        size_t maxAvailableArraySize = 0;
        bool anyAvailable = false;
        bool anyNotSupportedByDevice = false;
        bool anyNotExported = false;
        bool anyNotImplemented = false;

        for (const auto& entry : providerEntries) {
            if (entry.availability == PM_METRIC_AVAILABILITY_AVAILABLE) {
                anyAvailable = true;
                maxAvailableArraySize = std::max(maxAvailableArraySize, entry.arraySize);
            }
            else if (entry.availability == PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE) {
                anyNotSupportedByDevice = true;
            }
            else if (entry.availability == PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE) {
                anyNotExported = true;
            }
            else if (entry.availability == PM_METRIC_AVAILABILITY_NOT_IMPLEMENTED_BY_PRESENTMON) {
                anyNotImplemented = true;
            }
        }

        if (anyAvailable) {
            return ipc::MetricCapabilityEntry{
                .arraySize = maxAvailableArraySize,
                .availability = PM_METRIC_AVAILABILITY_AVAILABLE,
            };
        }
        if (anyNotSupportedByDevice) {
            return ipc::MetricCapabilityEntry{
                .arraySize = 0,
                .availability = PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE,
            };
        }
        if (anyNotExported) {
            return ipc::MetricCapabilityEntry{
                .arraySize = 0,
                .availability = PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE,
            };
        }
        if (anyNotImplemented) {
            return ipc::MetricCapabilityEntry{
                .arraySize = 0,
                .availability = PM_METRIC_AVAILABILITY_NOT_IMPLEMENTED_BY_PRESENTMON,
            };
        }
        return ipc::MetricCapabilityEntry{
            .arraySize = 0,
            .availability = PM_METRIC_AVAILABILITY_UNAVAILABLE,
        };
    }

    ipc::MetricCapabilities MergeLogicalDeviceAvailability(
        PM_DEVICE_TYPE deviceType,
        const std::function<std::vector<ipc::MetricCapabilities>()>& getProviderCapsList)
    {
        ipc::MetricCapabilities merged;
        const auto providerCapsList = getProviderCapsList();

        ForEachTelemetryMetricForDeviceType(deviceType, [&](PM_METRIC metricId) {
            std::vector<ipc::MetricCapabilityEntry> providerEntries;
            providerEntries.reserve(providerCapsList.size());

            for (const auto& caps : providerCapsList) {
                if (const auto entry = caps.Lookup(metricId)) {
                    providerEntries.push_back(*entry);
                }
                else {
                    providerEntries.push_back(ipc::MetricCapabilityEntry{
                        .arraySize = 0,
                        .availability = DefaultMissingTelemetryMetricAvailability(deviceType),
                    });
                }
            }

            const auto mergedEntry = MergeProviderAvailabilityEntries(deviceType, providerEntries);
            merged.Set(metricId, mergedEntry.arraySize, mergedEntry.availability);
        });

        return merged;
    }
}
