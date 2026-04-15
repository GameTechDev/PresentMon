#pragma once
#include "SharedMemoryTypes.h"

namespace pmon::ipc
{
    namespace bip = boost::interprocess;

    // Non-owning view over a shared memory segment that hosts a data store T.
    // Opens an existing managed segment and finds the named T instance inside.
    // ReadOnly=true opens the segment with read-only access (FILE_MAP_READ); the
    // segment's DACL must have been created with at most GR for this to succeed.
    template<class T, bool ReadOnly = false>
    class ViewedDataSegment
    {
    public:
        explicit ViewedDataSegment(const std::string& segmentName)
            :
            shm_{ MakeShm_(segmentName) }
        {
            auto result = shm_.find<T>(name_);
            if (!result.first) {
                throw std::runtime_error("Failed to find data store in shared memory");
            }
            pData_ = result.first; // non-owning
        }

        const T& GetStore() const { return *pData_; }

    private:
        static constexpr const char* name_ = "seg-dat";

        static ShmSegment MakeShm_(const std::string& segmentName)
        {
            if constexpr (ReadOnly) {
                return ShmSegment{ bip::open_read_only, segmentName.c_str() };
            }
            else {
                return ShmSegment{ bip::open_only, segmentName.c_str() };
            }
        }

        ShmSegment shm_;
        T* pData_ = nullptr; // non-owning
    };
}
