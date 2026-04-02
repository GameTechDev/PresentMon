#include "EtwLogger.h"
#include "../PresentData/PresentMonTraceSession.hpp"
#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "CliOptions.h"

using namespace std::literals;

namespace pmon::svc
{
    using namespace util;

    namespace
    {
        std::vector<EtwProviderDescription> CaptureProviderDescriptions_()
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
            traceConsumer.mTrackProcessState = true; // initial process state dump (gets us names)
            // dry run of the provider enablement routine to extract the provider.event list
            EnableProvidersListing(0, nullptr, &traceConsumer, true, true, pTraceFilter);
            return std::vector{ std::from_range, pTraceFilter->GetProviderDescriptions() };
        }
    }

    uint32_t EtwLogger::nextSessionId_ = 0;

    EtwLogger::EtwLogger(bool isElevated)
    {
        try {
            workDirectory_ = file::SecureSubdirectory::CreateInSystemTemp(
                L"PresentMonServiceEtl", isElevated, true, true);
        }
        catch (...) {
            pmlog_error(ReportException("Failed establishing etw logger work directory"));
        }
    }

    uint32_t EtwLogger::StartLogSession(std::span<const EtwProviderDescription> providers)
    {
        if (!workDirectory_) {
            throw Except<Exception>("Failed ETL session start: no working dir");
        }
        std::lock_guard lk{ mtx_ };
        if (providers.empty()) {
            providers = GetDefaultProviderDescriptions_();
        }
        const auto id = GetNextSessionId_();
        const auto name = MakeSessionName_(id);
        EnsureSessionNameAvailability_(name);
        sessions_.emplace(std::piecewise_construct, std::forward_as_tuple(id),
            std::forward_as_tuple(
                util::str::ToWide(name),
                workDirectory_.Path(),
                providers
            )
        );
        return id;
    }
    util::file::TempFile EtwLogger::FinishLogSession(uint32_t id)
    {
        std::lock_guard lk{ mtx_ };
        auto file = sessions_.at(id).Finish();
        sessions_.erase(id);
        return file;
    }
    void EtwLogger::CancelLogSession(uint32_t id)
    {
        std::lock_guard lk{ mtx_ };
        sessions_.erase(id);
    }
    bool EtwLogger::HasActiveSession(uint32_t id) const
    {
        std::lock_guard lk{ mtx_ };
        return sessions_.contains(id);
    }
    std::span<const EtwProviderDescription> EtwLogger::GetDefaultProviderDescriptions_()
    {
        if (defaultProviderDescriptionCache_.empty()) {
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