#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include "ProcessTracker.h"
#include "FrameQuery.h"
#include "DynamicQuery.h"
#include <string>
#include <memory>
#include <span>

namespace pmapi
{
    namespace intro
    {
        class Root;
    }

    // Session encapsulates the connection to the PresentMon service
    // use this class to track processes, register queries, get introspection data, and configure the service
    class Session
    {
    public:
        // types
        //
        // used to select the constructor that creates an empty session object
        struct EmptyTag {};

        // functions
        // 
        // create a session object managing connection to the service
        Session();
        // create an empty session object not connected to service
        Session(EmptyTag) noexcept;
        // connect to service with custom connection endpoints
        // NOTE: interface of this constructor will change in a future release, requiring a small code change
        Session(std::string controlPipe);
        // move ctor to accept connection resource from another Session
        Session(Session&& rhs) noexcept;
        // assignment operator to accept connection resource from another Session
        Session& operator=(Session&& rhs) noexcept;
        // closes the connection and frees other resources
        ~Session();
        // empty the Session object (frees resouces)
        void Reset() noexcept;
        // check if Session object owns a connection
        bool Empty() const;
        // alias for Empty()
        operator bool() const;
        // get the root interface for all introspection data
        // returns a cached pointer if available (use forceRefresh to force introspection data recreation)
        // see PresentMonAPIWrapperCommon/Introspection.h for details about introspection data available
        std::shared_ptr<intro::Root> GetIntrospectionRoot(bool forceRefresh = false) const;
        // begin tracking a process, necessary to consume frame data or query metrics involving that process
        ProcessTracker TrackProcess(uint32_t pid);
        // register (build/compile) a dynamic query used to poll metrics
        DynamicQuery RegisterDyanamicQuery(std::span<PM_QUERY_ELEMENT> elements, double winSizeMs = 1000, double metricOffsetMs = 1020);
        // register (build/compile) a frame query used to consume frame events
        FrameQuery RegisterFrameQuery(std::span<PM_QUERY_ELEMENT> elements);
        // set the rate at which the service polls device telemetry data
        // NOTE: this is independent/distinct from the rate at which a client app polls dynamic queries
        void SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t milliseconds);
        // set the rate at which ETW events are flushed from their buffers and processed at the service side
        // lower values will give lower latency on frame data and enable lower offset values for dynamic metric queries
        // a value of zero indicates no manual flushing (typically resulting in 1000ms latency on frame data)
        void SetEtwFlushPeriod(uint32_t milliseconds);
        // get the underlying C API handle to the PresentMon service session
        // NOTE: it is recommended to use the Session member functions instead of using this handle directly
        PM_SESSION_HANDLE GetHandle() const;
    private:
        // functions
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        void Initialize_();
        // data
        PM_SESSION_HANDLE handle_ = nullptr;
        mutable std::shared_ptr<intro::Root> pIntrospectionRootCache_;
    };
}
