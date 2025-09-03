#include "Session.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/EnumMap.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Introspection.h>
#include <cassert>
#include <format>

namespace pmapi
{
    Session::Session()
    {
        if (auto sta = pmOpenSession(&handle_); sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "error opening session" };
        }
        Initialize_();
    }

    Session::Session(EmptyTag) noexcept {}

    Session::Session(std::string controlPipe)
    {
        if (auto sta = pmOpenSessionWithPipe(&handle_, controlPipe.c_str());
            sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, std::format("error opening session ctrl={}", controlPipe) };
        }
        Initialize_();
    }

    Session::Session(Session&& rhs) noexcept
    {
        *this = std::move(rhs);
    }

    Session& Session::operator=(Session&& rhs) noexcept
    {
        if (&rhs != this)
        {
            handle_ = rhs.handle_;
            pIntrospectionRootCache_ = std::move(rhs.pIntrospectionRootCache_);
            rhs.Clear_();
        }
        return *this;
    }

    Session::~Session() { Reset(); }

    void Session::Reset() noexcept
    {
        pIntrospectionRootCache_.reset();
        if (!Empty()) {
            // TODO: report error here noexcept
            pmCloseSession(handle_);
        }
        Clear_();
    }

    bool Session::Empty() const
    {
        return handle_ == nullptr;
    }

    Session::operator bool() const
    {
        return !Empty();
    }

    std::shared_ptr<intro::Root> Session::GetIntrospectionRoot(bool forceRefresh) const
    {
        assert(handle_);
        if (!forceRefresh && pIntrospectionRootCache_) {
            return pIntrospectionRootCache_;
        }
        const PM_INTROSPECTION_ROOT* pRoot{};
        if (auto sta = pmGetIntrospectionRoot(handle_, &pRoot); sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "introspection call failed" };
        }
        return pIntrospectionRootCache_ = std::make_shared<intro::Root>(pRoot, [](const PM_INTROSPECTION_ROOT* ptr) { pmFreeIntrospectionRoot(ptr); });
    }

    ProcessTracker Session::TrackProcess(uint32_t pid)
    {
        assert(handle_);
        return { handle_, pid };
    }

    DynamicQuery Session::RegisterDyanamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs, double metricOffsetMs)
    {
        assert(handle_);
        return { handle_, elements, winSizeMs, metricOffsetMs };
    }

    FrameQuery Session::RegisterFrameQuery(std::span<PM_QUERY_ELEMENT> elements)
    {
        assert(handle_);
        return { handle_, elements };
    }

    void Session::SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t milliseconds)
    {
        assert(handle_);
        if (auto sta = pmSetTelemetryPollingPeriod(handle_, deviceId, milliseconds); sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "set telemetry period call failed" };
        }
    }

    void Session::SetEtwFlushPeriod(uint32_t milliseconds)
    {
        assert(handle_);
        if (auto sta = pmSetEtwFlushPeriod(handle_, milliseconds); sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "set ETW flush period call failed" };
        }
    }

    PM_SESSION_HANDLE Session::GetHandle() const
    {
        return handle_;
    }

    void Session::Clear_() noexcept
    {
        handle_ = nullptr;
    }

    void Session::Initialize_()
    {
        // initialize the enum map so that it doesn't need to be initialized explicitly
        EnumMap::Refresh(*GetIntrospectionRoot());
    }
}