#pragma once
#include "BlobContainer.h"
#include <cassert>

namespace pmapi
{
    BlobContainer::BlobContainer(const void* handle, size_t blobSize, uint32_t nBlobs)
        :
        handle_{ handle },
        blobSize_{ blobSize },
        nBlobs_{ nBlobs }
    {
        pBlobArrayBytes_ = std::make_unique<uint8_t[]>(GetTotalSize());
        PopulateBlobPointers_();
    }

    BlobContainer::BlobContainer(BlobContainer&& other) noexcept
    {
        *this = std::move(other);
    }

    BlobContainer::BlobContainer(const BlobContainer& other)
    {
        *this = other;
    }

    BlobContainer& BlobContainer::operator=(BlobContainer&& rhs) noexcept
    {
        if (&rhs != this)
        {
            handle_ = rhs.handle_;
            blobSize_ = rhs.blobSize_;
            nBlobs_ = rhs.nBlobs_;
            nBlobsFilledInOut_ = rhs.nBlobsFilledInOut_;
            pBlobArrayBytes_ = std::move(rhs.pBlobArrayBytes_);
            blobPointers_ = std::move(rhs.blobPointers_);
            rhs.Reset();
        }
        return *this;
    }

    BlobContainer& BlobContainer::operator=(const BlobContainer& rhs)
    {
        if (&rhs != this)
        {
            handle_ = rhs.handle_;
            blobSize_ = rhs.blobSize_;
            nBlobs_ = rhs.nBlobs_;
            nBlobsFilledInOut_ = rhs.nBlobsFilledInOut_;
            pBlobArrayBytes_ = std::make_unique<uint8_t[]>(rhs.GetTotalSize());
            PopulateBlobPointers_();
            std::copy(rhs.pBlobArrayBytes_.get(),
                rhs.pBlobArrayBytes_.get() + rhs.GetTotalSize(),
                pBlobArrayBytes_.get());
        }
        return *this;
    }

    size_t BlobContainer::GetTotalSize() const { return blobSize_ * nBlobs_; }

    size_t BlobContainer::GetBlobSize() const { return blobSize_; }

    uint32_t BlobContainer::GetBlobCount() const { return nBlobs_; }

    uint32_t BlobContainer::GetNumBlobsPopulated() const { return nBlobsFilledInOut_; }

    uint8_t* BlobContainer::GetFirst() { return pBlobArrayBytes_.get(); }

    const uint8_t* BlobContainer::GetFirst() const { return pBlobArrayBytes_.get(); }

    uint8_t* BlobContainer::operator[](size_t index)
    {
        assert(index < nBlobs_);
        return pBlobArrayBytes_.get() + blobSize_ * index;
    }

    const uint8_t* BlobContainer::operator[](size_t index) const
    {
        return const_cast<BlobContainer&>(*this)[index];
    }

    std::vector<uint8_t*>::const_iterator BlobContainer::begin() const
    {
        return blobPointers_.begin();
    }

    std::vector<uint8_t*>::const_iterator BlobContainer::end() const
    {
        return blobPointers_.begin() +
            std::min(GetNumBlobsPopulated(), GetBlobCount());
    }

    bool BlobContainer::AllBlobsPopulated() const { return nBlobsFilledInOut_ == nBlobs_; }

    void BlobContainer::Reset()
    {
        handle_ = nullptr;
        blobSize_ = 0;
        nBlobs_ = 0;
        nBlobsFilledInOut_ = 0;
        pBlobArrayBytes_.reset();
        blobPointers_.clear();
    }

    bool BlobContainer::Empty() const
    {
        return (bool)pBlobArrayBytes_;
    }

    BlobContainer:: operator bool() const { return !Empty(); }

    void BlobContainer::PopulateBlobPointers_()
    {
        for (uint32_t i = 0; i < nBlobs_; i++) {
            blobPointers_.push_back((*this)[i]);
        }
    }

    uint32_t& BlobContainer::AcquireNumBlobsInRef_()
    {
        return nBlobsFilledInOut_ = nBlobs_;
    }
}