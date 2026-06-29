// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <PresentMonAPI2/PresentMonAPI.h>
#include <PresentMonAPIWrapperCommon/EnumMap.h>
#include <Core/source/kernel/OverlaySpec.h>
#include "MetricFetcher.h"
#include "../DynamicQuery.h"
#include <CommonUtilities//str/String.h>
#include <concepts>
#include <limits>
#include <cmath>


namespace p2c::pmon::met
{
    class DynamicPollingFetcher : public MetricFetcher
    {
    protected:
        // functions
        DynamicPollingFetcher(const PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& introRoot,
            std::shared_ptr<DynamicQuery> pQuery);
        // data
        std::shared_ptr<DynamicQuery> pQuery_;
        uint32_t offset_ = std::numeric_limits<uint32_t>::max();
        float scale_ = 1.f;
    };

    template<typename T>
    class TypedDynamicPollingFetcher : public DynamicPollingFetcher
    {
    public:
        TypedDynamicPollingFetcher(const PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& introRoot,
            std::shared_ptr<DynamicQuery> pQuery)
            :
            DynamicPollingFetcher{ qel, introRoot, std::move(pQuery) }
        {}
        std::optional<float> ReadValue() override
        {
            if constexpr (std::integral<T> || std::floating_point<T>) {
                if (auto pBlobBytes = pQuery_->GetBlobData()) {
                    const T raw = *reinterpret_cast<const T*>(&pBlobBytes[offset_]);
                    const float val = scale_ * (float)raw;
                    if constexpr (std::floating_point<T>) {
                        if (std::isnan(val)) {
                            return {};
                        }
                    }
                    return val;
                }
                return {};
            }
            if constexpr (std::same_as<T, const char*>) {
                pmlog_warn("Reading float value from string-typed metric");
            }
            else {
                pmlog_warn("Unknown type");
            }
            return {};
        }
        std::wstring ReadStringValue() override
        {
            if constexpr (std::integral<T> || std::floating_point<T>) {
                return MetricFetcher::ReadStringValue();
            }
            else if constexpr (std::same_as<T, const char*>) {
                if (auto pBlobBytes = pQuery_->GetBlobData()) {
                    return ::pmon::util::str::ToWide(reinterpret_cast<const char*>(&pBlobBytes[offset_]));
                }
            }
            else {
                pmlog_warn("Unknown type");
            }
            return {};
        }
    };

    template<>
    class TypedDynamicPollingFetcher<PM_ENUM> : public DynamicPollingFetcher
    {
    public:
        TypedDynamicPollingFetcher(const PM_QUERY_ELEMENT& qel, const pmapi::intro::Root& introRoot,
            std::shared_ptr<DynamicQuery> pQuery, std::shared_ptr<const pmapi::EnumMap::KeyMap> pKeyMap);
        std::wstring ReadStringValue() override;
        std::optional<float> ReadValue() override;
    private:
        std::shared_ptr<const pmapi::EnumMap::KeyMap> pKeyMap_;
    };

    std::shared_ptr<DynamicPollingFetcher> MakeDynamicPollingFetcher(const PM_QUERY_ELEMENT& qel,
        const pmapi::intro::Root& introRoot, std::shared_ptr<DynamicQuery> pQuery);
}