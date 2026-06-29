// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../kernel/OverlaySpec.h"
#include <PresentMonAPIWrapper/PresentMonAPIWrapper.h>
#include "../../../Interprocess/source/SystemDeviceId.h"
#include <optional>

namespace p2c::pmon
{
    inline bool IsQualifiedMetricAvailable(
        const pmapi::intro::Root& introRoot,
        const kern::QualifiedMetric& qmet)
    {
        const auto metricView = introRoot.FindMetric((PM_METRIC)qmet.metricId);
        const auto metricType = metricView.GetType();
        const bool isStaticMetric = metricType == PM_METRIC_TYPE_STATIC;

        if (qmet.deviceId != ::pmon::ipc::kUniversalDeviceId) {
            try {
                introRoot.FindDevice(qmet.deviceId);
            }
            catch (const pmapi::LookupException&) {
                return false;
            }
        }

        std::optional<pmapi::intro::DeviceMetricInfoView> deviceMetricInfo;
        for (auto info : metricView.GetDeviceMetricInfo()) {
            if (info.GetDevice().GetId() == qmet.deviceId) {
                deviceMetricInfo = info;
                break;
            }
        }

        if (!deviceMetricInfo.has_value()) {
            if (isStaticMetric && qmet.deviceId == ::pmon::ipc::kSystemDeviceId) {
                return qmet.arrayIndex == 0;
            }
            return false;
        }

        if (!deviceMetricInfo->IsAvailable()) {
            return false;
        }

        const auto arraySize = deviceMetricInfo->GetArraySize();
        if (qmet.arrayIndex >= arraySize) {
            return false;
        }

        return true;
    }
}
