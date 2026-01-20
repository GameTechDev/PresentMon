#pragma once
#include "ProcessTracker.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>
#include <format>
#include <string>
#include <cassert>

namespace pmapi
{
    ProcessTracker::~ProcessTracker() { Reset(); }

    ProcessTracker::ProcessTracker(ProcessTracker&& other) noexcept
    {
        *this = std::move(other);
    }

    ProcessTracker& ProcessTracker::operator=(ProcessTracker&& rhs) noexcept
    {
        if (&rhs != this) {
            pid_ = rhs.pid_;
            hSession_ = rhs.hSession_;
            rhs.Clear_();;
        }
        return *this;
    }

    uint32_t ProcessTracker::GetPid() const
    {
        assert(!Empty());
        return pid_;
    }

    void ProcessTracker::Reset() noexcept
    {
        if (!Empty()) {
            // TODO: report error here noexcept
            pmStopTrackingProcess(hSession_, pid_);
        }
        Clear_();
    }

    bool ProcessTracker::Empty() const
    {
        return hSession_ == nullptr;
    }

    ProcessTracker::operator bool() const { return !Empty(); }

    ProcessTracker::ProcessTracker(PM_SESSION_HANDLE hSession, uint32_t pid, bool isPlayback, bool isBackpressured)
        :
        pid_{ pid },
        hSession_{ hSession }
    {
        PM_STATUS sta = PM_STATUS_SUCCESS;
        if (isPlayback) {
            sta = pmStartPlaybackTracking(hSession_, pid_, isBackpressured ? 1u : 0u);
        }
        else {
            sta = pmStartTrackingProcess(hSession_, pid_);
        }
        if (sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "start process tracking call failed" };
        }
    }

    void ProcessTracker::Clear_() noexcept
    {
        pid_ = 0;
        hSession_ = nullptr;
    }
}
