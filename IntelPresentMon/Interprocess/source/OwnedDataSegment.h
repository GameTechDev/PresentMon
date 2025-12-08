#pragma once
#include "SharedMemoryTypes.h"
#include "MetricCapabilities.h"
#include "DataStores.h"
#include <type_traits>

namespace pmon::ipc
{
    // manages shared memory segment and hosts data store T
    template<class T>
    class OwnedDataSegment
    {
    public:
        // No ACL version
        // For FrameDataStore: pass (ringCapacity)
        // For GpuDataStore/SystemDataStore: pass no extra args
        template<class... StoreArgs>
        OwnedDataSegment(const std::string& segmentName,
            StoreArgs&&... storeArgs)
            :
            shm_{ bip::create_only, segmentName.c_str(), T::virtualSegmentSize },
            pData_{ MakeStore_(std::forward<StoreArgs>(storeArgs)...) }
        {}

        // ACL version
        template<class... StoreArgs>
        OwnedDataSegment(const std::string& segmentName,
            const bip::permissions& perms,
            StoreArgs&&... storeArgs)
            :
            shm_{ bip::create_only, segmentName.c_str(), T::virtualSegmentSize,
                  nullptr, perms },
            pData_{ MakeStore_(std::forward<StoreArgs>(storeArgs)...) }
        {}

        T& GetStore() { return *pData_; }
        const T& GetStore() const { return *pData_; }

    private:
        static constexpr const char* name_ = "seg-dat";

        // Helper to enforce a single size_t-like argument for FrameDataStore
        template<typename Arg>
        static size_t GetFrameCapacity_(Arg&& arg)
        {
            static_assert(std::is_convertible_v<Arg, size_t>,
                "FrameDataStore capacity must be convertible to size_t");
            return static_cast<size_t>(std::forward<Arg>(arg));
        }

        // Factory that constructs the store in shared memory with the right allocator
        template<class... StoreArgs>
        ShmUniquePtr<T> MakeStore_(StoreArgs&&... storeArgs)
        {
            // FrameDataStore: expects (ShmAllocator<PmFrameData>&, size_t cap)
            if constexpr (std::is_same_v<T, FrameDataStore>) {
                static_assert(sizeof...(StoreArgs) == 1,
                    "OwnedStreamedSegment<FrameDataStore> requires a single ring capacity argument");
                return ShmMakeNamedUnique<FrameDataStore>(
                    name_,
                    shm_.get_segment_manager(),
                    *shm_.get_segment_manager(),
                    GetFrameCapacity_(std::forward<StoreArgs>(storeArgs)...)
                );
            }
            // Telemetry stores: expect TelemetryMap::AllocatorType& only
            else if constexpr (std::is_same_v<T, GpuDataStore> ||
                std::is_same_v<T, SystemDataStore>) {
                static_assert(sizeof...(StoreArgs) == 0,
                    "OwnedStreamedSegment<GpuDataStore/SystemDataStore> "
                    "does not take extra ctor args");
                return ShmMakeNamedUnique<T>(
                    name_,
                    shm_.get_segment_manager(),
                    *shm_.get_segment_manager()
                );
            }
        }

        ShmSegment shm_;
        ShmUniquePtr<T> pData_;
    };
}
