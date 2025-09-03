#pragma once
#include "DynamicQuery.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Introspection.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>
#include <format>
#include <cassert>

namespace pmapi
{
    DynamicQuery::~DynamicQuery() { Reset(); }

    DynamicQuery::DynamicQuery(DynamicQuery&& other) noexcept
    {
        *this = std::move(other);
    }

    DynamicQuery& DynamicQuery::operator=(DynamicQuery&& rhs) noexcept
    {
        if (&rhs != this)
        {
            hQuery_ = rhs.hQuery_;
            blobSize_ = rhs.blobSize_;
            rhs.Clear_();
        }
        return *this;
    }

    size_t DynamicQuery::GetBlobSize() const
    {
        return blobSize_;
    }

    void DynamicQuery::Poll(const ProcessTracker& tracker, uint8_t* pBlob, uint32_t& numSwapChains) const
    {
        if (auto sta = pmPollDynamicQuery(hQuery_, tracker.GetPid(), pBlob, &numSwapChains);
            sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "dynamic poll call failed" };
        }
    }

    void DynamicQuery::Poll(const ProcessTracker& tracker, BlobContainer& blobs) const
    {
        assert(!Empty());
        assert(blobs.CheckHandle(hQuery_));
        Poll(tracker, blobs.GetFirst(), blobs.AcquireNumBlobsInRef_());
    }

    BlobContainer DynamicQuery::MakeBlobContainer(uint32_t nBlobs) const
    {
        assert(!Empty());
        return { hQuery_, blobSize_, nBlobs };
    }

    void DynamicQuery::Reset()
    {
        if (!Empty()) {
            // TODO: report error noexcept
            pmFreeDynamicQuery(hQuery_);
        }
        Clear_();
    }

    bool DynamicQuery::Empty() const
    {
        return hQuery_ == nullptr;
    }

    DynamicQuery::operator bool() const { return !Empty(); }

    DynamicQuery::DynamicQuery(PM_SESSION_HANDLE hSession, std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs)
    {
        if (auto sta = pmRegisterDynamicQuery(hSession, &hQuery_, elements.data(),
            elements.size(), winSizeMs, metricOffsetMs); sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "dynamic query register call failed" };
        }
        if (elements.size() > 0) {
            blobSize_ = elements.back().dataOffset + elements.back().dataSize;
        }
    }

    void DynamicQuery::Clear_() noexcept
    {
        hQuery_ = nullptr;
        blobSize_ = 0ull;
    }
}
