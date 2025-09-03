#pragma once
#include "FrameQuery.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Introspection.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>
#include <format>
#include <string>
#include <cassert>

namespace pmapi
{
    FrameQuery::~FrameQuery() { Reset(); }

    FrameQuery::FrameQuery(FrameQuery&& other) noexcept
    {
        *this = std::move(other);
    }

    FrameQuery& FrameQuery::operator=(FrameQuery&& rhs) noexcept
    {
        if (&rhs != this)
        {
            hQuery_ = rhs.hQuery_;
            blobSize_ = rhs.blobSize_;
            rhs.Clear_();;
        }
        return *this;
    }

    size_t FrameQuery::GetBlobSize() const
    {
        return blobSize_;
    }

    void FrameQuery::Consume(const ProcessTracker& tracker, uint8_t* pBlobs, uint32_t& numBlobsInOut)
    {
        if (auto sta = pmConsumeFrames(hQuery_, tracker.GetPid(), pBlobs, &numBlobsInOut);
            sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "consume frame call failed" };
        }
    }

    void FrameQuery::Consume(const ProcessTracker& tracker, BlobContainer& blobs)
    {
        assert(!Empty());
        assert(blobs.CheckHandle(hQuery_));
        Consume(tracker, blobs.GetFirst(), blobs.AcquireNumBlobsInRef_());
    }

    size_t FrameQuery::ForEachConsume(ProcessTracker& tracker, BlobContainer& blobs, std::function<void(const uint8_t*)> frameHandler)
    {
        size_t nFramesProcessed = 0;
        do {
            Consume(tracker, blobs);
            const auto nPopulated = blobs.GetNumBlobsPopulated();
            for (uint32_t i = 0; i < nPopulated; i++) {
                frameHandler(blobs[i]);
            }
            nFramesProcessed += nPopulated;
        } while (blobs.AllBlobsPopulated());
        return nFramesProcessed;
    }

    BlobContainer FrameQuery::MakeBlobContainer(uint32_t nBlobs) const
    {
        assert(!Empty());
        return { hQuery_, blobSize_, nBlobs };
    }

    void FrameQuery::Reset() noexcept
    {
        if (!Empty()) {
            // TODO: check and report error here (nothrow)
            pmFreeFrameQuery(hQuery_);
        }
        Clear_();
    }

    bool FrameQuery::Empty() const
    {
        return hQuery_ == nullptr;
    }

    FrameQuery::operator bool() const { return !Empty(); }

    FrameQuery::FrameQuery(PM_SESSION_HANDLE hSession, std::span<PM_QUERY_ELEMENT> elements)
    {
        if (auto sta = pmRegisterFrameQuery(hSession, &hQuery_, elements.data(), elements.size(), &blobSize_);
            sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "register frame query call failed" };
        }
    }

    void FrameQuery::Clear_() noexcept
    {
        hQuery_ = nullptr;
        blobSize_ = 0ull;
    }
}
