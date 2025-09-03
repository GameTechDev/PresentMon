#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include "BlobContainer.h"
#include "ProcessTracker.h"
#include <span>
#include <functional>

namespace pmapi
{
    // FrameQuery specifies what data (metrics) should be gathered and processed for each piece of frame data consumed
    // it must be first registered with the API, and then can be queried when given a ProcessTracker to use
    // NOTE: do not create directly; Session is the factory for FrameQuery
    class FrameQuery
    {
        friend class Session;
    public:
        // create empty frame query
        FrameQuery() = default;
        // free frame query
        ~FrameQuery();
        // move ctor
        FrameQuery(FrameQuery&& other) noexcept;
        // move assign
        FrameQuery& operator=(FrameQuery&& rhs) noexcept;
        // get the size of blob memory required per frame of data consumed
        size_t GetBlobSize() const;
        // consume frames of present data from a process using this query object and a managed blob container
        // number of frames consumed is minimum between capacity of container and # of frames pending in queue)
        void Consume(const ProcessTracker& tracker, BlobContainer& blobs);
        // consume frames of present data from a process using this query object and manually-managed blob memory
        // numBlobsInOut: specifies max number of frames to consume, after function call contains number of blobs actually written
        // pBlobs: pointer to memory where frame query data is to be stored for consumed frames
        // NOTE: it is preferred to use above version of this function that takes in BlobContainer&
        void Consume(const ProcessTracker& tracker, uint8_t* pBlobs, uint32_t& numBlobsInOut);
        // consume frame events and invoke frameHandler for each frame consumed, setting active blob each time
        // will continue to call consume until all frames have been consumed from the queue
        size_t ForEachConsume(ProcessTracker& tracker, BlobContainer& blobs, std::function<void(const uint8_t*)> frameHandler);
        // create a blob container whose size is suited to fit this query
        // nBlobs: number of frames worth of data that the container can contain
        BlobContainer MakeBlobContainer(uint32_t nBlobs) const;
        // empty this query object (frees resources)
        void Reset() noexcept;
        // check if this query object is empty
        bool Empty() const;
        // alises Empty()
        operator bool() const;
    private:
        // functions
        FrameQuery(PM_SESSION_HANDLE hSession, std::span<PM_QUERY_ELEMENT> elements);
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        // data
        PM_FRAME_QUERY_HANDLE hQuery_ = nullptr;
        uint32_t blobSize_ = 0ull;
    };
}
