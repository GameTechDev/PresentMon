#include "EtwLogger.h"
#include "../PresentData/PresentMonTraceSession.hpp"
#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "CliOptions.h"

using namespace std::literals;

namespace pmon::svc
{
    uint32_t EtwLogger::nextSessionId_ = 0;

    uint32_t EtwLogger::StartLogSession(std::shared_ptr<EtwLogProviderListener> pListener)
    {
        std::lock_guard lk{ mtx_ };
        if (!pListener) {
            pListener = GetDefaultProviderDescriptions_();
        }
        const auto id = GetNextSessionId_();
        const auto name = MakeSessionName_(id);
        EnsureSessionNameAvailability_(name);
        sessions_.emplace(std::piecewise_construct, std::forward_as_tuple(id),
            std::forward_as_tuple(
                util::str::ToWide(name),
                L"C:\\EtlTesting\\newlog\\oct13-newlog.etl"s,
                pListener->GetProviderDescriptions()
            )
        );
        return id;
    }
    void EtwLogger::StopLogSession(uint32_t id)
    {
        sessions_.erase(id);
    }
    void EtwLogger::CancelLogSession(uint32_t id)
    {
        sessions_.erase(id);
    }
    std::shared_ptr<EtwLogProviderListener> EtwLogger::CaptureProviderDescriptions_()
	{
        auto pTraceFilter = std::make_shared<EtwLogProviderListener>();
        // trace consumer that configures what events are processed
        PMTraceConsumer traceConsumer;
        traceConsumer.mTrackDisplay = true;   // ... presents to the display.
        traceConsumer.mTrackGPU = true;       // ... GPU work.
        traceConsumer.mTrackGPUVideo = true;  // ... GPU video work (separately from non-video GPU work).
        traceConsumer.mTrackInput = true;     // ... keyboard/mouse latency.
        traceConsumer.mTrackFrameType = true; // ... the frame type communicated through the Intel-PresentMon provider.
        traceConsumer.mTrackAppTiming = true; // ... app timing data communicated through the Intel-PresentMon provider.
        traceConsumer.mTrackPcLatency = true; // ... Nvidia PCL stats.
        // dry run of the provider enablement routine to extract the provider.event list
        EnableProvidersListing(0, nullptr, &traceConsumer, true, true, pTraceFilter);
        return pTraceFilter;
	}
    std::shared_ptr<EtwLogProviderListener> EtwLogger::GetDefaultProviderDescriptions_()
    {
        if (!defaultProviderDescriptionCache_) {
            defaultProviderDescriptionCache_ = CaptureProviderDescriptions_();
        }
        return defaultProviderDescriptionCache_;
    }
    std::string EtwLogger::MakeSessionBaseName_()
    {
        auto& opt = clio::Options::Get();
        return std::format("{}_ETL", *opt.etwSessionName);
    }
    std::string EtwLogger::MakeSessionName_(uint32_t id)
    {
        return std::format("{}_{}", MakeSessionBaseName_(), id);
    }
    void EtwLogger::EnsureSessionNameAvailability_(const std::string& name)
    {
        EVENT_TRACE_PROPERTIES props{};
        props.Wnode.BufferSize = sizeof(props);
        // attempt to close a session with the target name
        auto sta = ControlTraceA(0, name.c_str(), &props, EVENT_TRACE_CONTROL_STOP);
        if (sta == ERROR_SUCCESS || sta == ERROR_MORE_DATA) {
            pmlog_info("Removed stale log session").pmwatch(name);
        } else if (sta != ERROR_WMI_INSTANCE_NOT_FOUND) {
            pmlog_error("Failed to clear stale log session name").pmwatch(name).hr(sta);
        }
    }
    uint32_t EtwLogger::GetNextSessionId_()
    {
        return nextSessionId_++;
    }
}