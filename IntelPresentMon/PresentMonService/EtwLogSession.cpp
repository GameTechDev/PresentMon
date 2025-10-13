#include "EtwLogSession.h"
#include <evntprov.h>
#include <evntcons.h>
#include <cstddef>
#include <format>
#include <vector>
#include <span>
#include <ranges>
#include <CommonUtilities/win/Utilities.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/log/Log.h>

namespace rn = std::ranges;

namespace pmon::svc
{
    void EtwLogProviderListener::EventAdded(uint16_t id)
    {
        eventsOnDeck_.push_back(id);
    }
    void EtwLogProviderListener::ProviderEnabled(const GUID& providerGuid, uint64_t anyKey, uint64_t allKey,
        uint8_t maxLevel, uint32_t controlCode)
    {
        providerDescriptions_.push_back({
            .events = std::move(eventsOnDeck_),
            .anyKeyMask = anyKey,
            .allKeyMask = allKey,
            .maxLevel = maxLevel,
            .providerGuid = providerGuid,
            .controlCode = controlCode,
        });
    }
    void EtwLogProviderListener::ClearEvents()
    {
        eventsOnDeck_.clear();
    }
    std::span<const EtwLogProviderListener::ProviderDescription> EtwLogProviderListener::GetProviderDescriptions() const
    {
        return providerDescriptions_;
    }

	EtwLogSession::EtwLogSession(const std::wstring& loggerName, const std::wstring& logFilePath,
        std::span<const EtwLogProviderListener::ProviderDescription> providers)
	{
        // create / start the trace session that outputs to .etl file
		traceProps_.Wnode.BufferSize = sizeof(traceProps_);
		traceProps_.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
		traceProps_.Wnode.ClientContext = TIMESTAMP_TYPE_QPC;
		traceProps_.LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
		traceProps_.LoggerNameOffset = offsetof(TraceProperties_, LoggerName);
		traceProps_.LogFileNameOffset = offsetof(TraceProperties_, LogFileName);
		// consider zeroing this to match PresentData
		traceProps_.BufferSize = 64;
		wcscpy_s(traceProps_.LoggerName, std::size(traceProps_.LoggerName), loggerName.c_str());
		wcscpy_s(traceProps_.LogFileName, std::size(traceProps_.LogFileName), logFilePath.c_str());
		// create the trace logger session
        if (auto sta = StartTraceW(&hTraceSession_, traceProps_.LoggerName, &traceProps_);
            sta != ERROR_SUCCESS) {
            pmlog_error("Failed to start ETL trace").hr(sta).raise<util::Exception>();
        }
        // enable providers with various filter mechanisms base on the injected provider descriptions
        for (auto& p : providers) {
            // filter by event id if there are any ids captured by the listener (otherwise assume unfiltered)
            if (!p.events.empty()) {
                // event filter that filters by event ID whitelist, payload size is dynamic so allocate blob
                // EVENT_FILTER_EVENT_ID contains a ushort placeholder representing start of array we subtract
                const size_t eventIdFilterSize = sizeof(EVENT_FILTER_EVENT_ID) +
                    sizeof(USHORT) * (p.events.size() - ANYSIZE_ARRAY);
                auto pEventIdFilter = static_cast<EVENT_FILTER_EVENT_ID*>(alloca(eventIdFilterSize));
                pEventIdFilter->FilterIn = TRUE;
                pEventIdFilter->Reserved = 0;
                pEventIdFilter->Count = (USHORT)p.events.size();
                rn::copy(p.events, pEventIdFilter->Events);
                // descriptor for the event filter
                EVENT_FILTER_DESCRIPTOR filterDesc{
                    .Ptr = reinterpret_cast<ULONGLONG>(pEventIdFilter),
                    .Size = (ULONG)eventIdFilterSize,
                    .Type = EVENT_FILTER_TYPE_EVENT_ID,
                };
                // parameter struct to feed our filter into the enable call
                ENABLE_TRACE_PARAMETERS enableParams{
                    .Version = ENABLE_TRACE_PARAMETERS_VERSION_2,
                    .EnableProperty = EVENT_ENABLE_PROPERTY_IGNORE_KEYWORD_0,
                    .SourceId = traceProps_.Wnode.Guid,
                    .EnableFilterDesc = &filterDesc,
                    .FilterDescCount = 1,
                };
                // enable the provider with event id filter
                if (auto sta = EnableTraceEx2(hTraceSession_, &p.providerGuid, p.controlCode, p.maxLevel,
                    p.anyKeyMask, p.allKeyMask, 0, &enableParams); sta != ERROR_SUCCESS) {
                    auto providerGuid = util::str::ToNarrow(util::win::GuidToString(p.providerGuid));
                    pmlog_warn("Failed to enable ETW provider").hr(sta).pmwatch(providerGuid);
                }
            }
            else {
                // enable the provider without event filter
                if (auto sta = EnableTraceEx2(hTraceSession_, &p.providerGuid, p.controlCode, p.maxLevel,
                    p.anyKeyMask, p.allKeyMask, 0, nullptr); sta != ERROR_SUCCESS) {
                    auto providerGuid = util::str::ToNarrow(util::win::GuidToString(p.providerGuid));
                    pmlog_warn("Failed to enable ETW provider").hr(sta).pmwatch(providerGuid);
                }
            }
        }
	}
    EtwLogSession::~EtwLogSession()
    {
        if (auto sta = ControlTraceW(hTraceSession_, traceProps_.LoggerName, &traceProps_, EVENT_TRACE_CONTROL_STOP);
            sta != ERROR_SUCCESS) {
            pmlog_error("Failed to stop ETL log session").hr(sta);
        }
    }
}