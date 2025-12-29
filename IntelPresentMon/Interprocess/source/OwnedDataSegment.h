#pragma once
#include "SharedMemoryTypes.h"
#include "DataStores.h"
#include "../../CommonUtilities/log/Log.h"

namespace pmon::ipc
{
    // manages shared memory segment and hosts data store T
    template<class T>
    class OwnedDataSegment
    {
    public:
        // Uses DataStoreSizingInfo for sizing across all stores; permissions are optional.
        OwnedDataSegment(const std::string& segmentName,
            const DataStoreSizingInfo& sizing,
            const bip::permissions& perms = {})
            :
            shm_{ bip::create_only, segmentName.c_str(),
                ResolveSegmentBytes_(segmentName, sizing),
                nullptr, perms },
            pData_{ MakeStore_(sizing) }
        {}

        T& GetStore() { return *pData_; }
        const T& GetStore() const { return *pData_; }

    private:
        static constexpr const char* name_ = "seg-dat";

        static size_t ResolveSegmentBytes_(const std::string& segmentName,
            const DataStoreSizingInfo& sizing)
        {
            const auto calculatedSize = sizing.overrideBytes ? *sizing.overrideBytes :
                T::CalculateSegmentBytes(sizing);
            pmlog_dbg("Creating shm segment")
                .pmwatch(segmentName)
                .pmwatch(calculatedSize)
                .pmwatch(sizing.ringSamples)
                .pmwatch(sizing.overrideBytes.has_value())
                .pmwatch(sizing.backpressured);
            return calculatedSize;
        }

        ShmUniquePtr<T> MakeStore_(const DataStoreSizingInfo& sizing)
        {
            return ShmMakeNamedUnique<T>(
                name_,
                shm_.get_segment_manager(),
                *shm_.get_segment_manager(),
                sizing
            );
        }

        ShmSegment shm_;
        ShmUniquePtr<T> pData_;
    };
}

