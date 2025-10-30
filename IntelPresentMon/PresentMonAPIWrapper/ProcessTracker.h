#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>

namespace pmapi
{
    // manages a tracking of a pid by the PresentMon service
    // PresentMon service will only allow queries on processes that are being explicitly tracked
    // NOTE: do not create this explicitly; Session is the factory for trackers
    class ProcessTracker
    {
        friend class Session;
    public:
        // create an empty tracker
        ProcessTracker() = default;
        // stops tracking the associated process
        ~ProcessTracker();
        // move ctor
        ProcessTracker(ProcessTracker&& other) noexcept;
        // move assignment
        ProcessTracker& operator=(ProcessTracker&& rhs) noexcept;
        // get the id of process being tracked
        uint32_t GetPid() const;
        // empty this tracker (stop tracking process if any)
        void Reset() noexcept;
        // check if tracker is empty
        bool Empty() const;
        // alias for Empty();
        operator bool() const;
    private:
        // functions
        ProcessTracker(PM_SESSION_HANDLE hSession, uint32_t pid);
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        // data
        uint32_t pid_ = 0u;
        PM_SESSION_HANDLE hSession_ = nullptr;
    };
}
