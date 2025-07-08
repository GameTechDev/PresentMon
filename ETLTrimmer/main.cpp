#include "../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "../IntelPresentMon/CommonUtilities/win/Utilities.h"
#include "../IntelPresentMon/CommonUtilities/Hash.h"
#include <initguid.h>
#include <evntcons.h>
#include <cguid.h>
#include <atlbase.h>
#include <comdef.h>
#include <objbase.h>
#include <relogger.h>
#include <iostream>
#include <optional>
#include <format>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <ranges>
#include "CliOptions.h"
#include "../PresentData/PresentMonTraceSession.hpp"
#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../PresentData/ETW/Microsoft_Windows_EventMetadata.h"
#include "../PresentData/ETW/NT_Process.h"
#include "../PresentData/ETW/Microsoft_Windows_Kernel_Process.h"
#include "../PresentData/ETW/Microsoft_Windows_DxgKrnl.h"

namespace rn = std::ranges;
namespace vi = rn::views;

DEFINE_GUID(IID_ITraceRelogger, 0xF754AD43, 0x3BCC, 0x4286, 0x80, 0x09, 0x9C, 0x5D, 0xA2, 0x14, 0xE8, 0x4E); // {F754AD43-3BCC-4286-8009-9C5DA214E84E}
DEFINE_GUID(IID_ITraceEventCallback, 0x3ED25501, 0x593F, 0x43E9, 0x8F, 0x38, 0x3A, 0xB4, 0x6F, 0x5A, 0x4A, 0x52); // {3ED25501-593F-43E9-8F38-3AB46F5A4A52}

constexpr uint8_t EnableAllLevels = 255u;

enum class Mode
{
    Analysis,
    Trim,
};

struct ProviderFilter
{
    std::unordered_set<uint16_t> eventSet;
    uint64_t anyKeyMask;
    uint64_t allKeyMask;
    uint8_t maxLevel;
    GUID providerGuid;
    bool MatchesId(uint16_t eventId) const
    {
        return eventSet.empty() || eventSet.contains(eventId);
    }
};

namespace std
{
    template <>
    struct hash<GUID>
    {
        size_t operator()(const GUID& guid) const noexcept
        {
            return pmon::util::hash::HashGuid(guid);
        }
    };
}

class Filter : public IFilterBuildListener
{
public:
    Filter()
    {
        // there are events from providers that are not explicitly enabled / filtered by presentmon
        // but that are nonetheless required for PresentMon tracking operations
        // they are likely enabled by default when doing ETW tracing
        // we must add them manually here instead of adding them by listening to the FilteredProvider

        // NT_Process
        //
        ProviderEnabled(NT_Process::GUID, 0, 0, EnableAllLevels);

        // Microsoft_Windows_EventMetadata::GUID
        //
        ProviderEnabled(Microsoft_Windows_EventMetadata::GUID, 0, 0, EnableAllLevels);
    }
    // Inherited via IFilterBuildListener
    void EventAdded(uint16_t id) override
    {
        eventsOnDeck_.push_back(id);
    }
    void ProviderEnabled(const GUID& providerGuid, uint64_t anyKey, uint64_t allKey, uint8_t maxLevel) override
    {
        ProviderFilter filter{
            .anyKeyMask = anyKey ? anyKey : 0xFFFF'FFFF,
            .allKeyMask = allKey,
            .maxLevel = maxLevel,
            .providerGuid = providerGuid,
        };
        filter.eventSet.insert_range(eventsOnDeck_);
        ClearEvents();
        providers_.emplace(filter.providerGuid, filter);
    }
    void ClearEvents() override
    {
        eventsOnDeck_.clear();
    }
    const ProviderFilter* LookupProvider(const GUID& guid)
    {
        if (auto i = providers_.find({ guid }); i != providers_.end()) {
            return &i->second;
        }
        return nullptr;
    }
private:
    std::vector<uint16_t> eventsOnDeck_;
    std::unordered_map<GUID, ProviderFilter> providers_;
};

class EventCallback : public ITraceEventCallback
{
private:
    // COM support
    DWORD ref_count_ = 0;
    // operation mode
    Mode mode_;
    bool listProcesses_ = false;
    // trim region
    std::optional<std::pair<uint64_t, uint64_t>> trimRangeQpc_;
    std::optional<std::pair<double, double>> trimRangeMs_;
    // pruning options
    std::shared_ptr<Filter> pFilter_;
    Filter stateFilter_; // don't discard these events outside trim range
    bool byId_;
    bool trimState_;
    // analysis stats
    int eventCount_ = 0;
    int keepCount_ = 0;
    std::optional<uint64_t> firstTimestamp_;
    uint64_t lastTimestamp_ = 0;
    std::unordered_map<uint32_t, uint64_t> eventCountByProcess_;

public:
    EventCallback(bool infoOnly, std::shared_ptr<Filter> pFilter, bool trimState, bool byId, bool listProcesses)
        :
        mode_{ infoOnly ? Mode::Analysis : Mode::Trim },
        pFilter_{ std::move(pFilter) },
        trimState_{ trimState },
        byId_{ byId },
        listProcesses_{ listProcesses }
    {
        // when trimming by timestamp, we must take care not to remove the state data psuedo-events generated
        // at the beginning of the trace (also true state events coming before the trim region)
        // nt process
        stateFilter_.ProviderEnabled(NT_Process::GUID, 0, 0, EnableAllLevels);
        // dxgkrnl --> DCs
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::Context_DCStart::Id);
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::Device_DCStart::Id);
        //         --> Contexts
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::Context_Start::Id);
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::Context_Stop::Id);
        //         --> Devices
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::Device_Start::Id);
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::Device_Stop::Id);
        //         --> hwqueue starts
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::HwQueue_DCStart::Id);
        stateFilter_.EventAdded(Microsoft_Windows_DxgKrnl::HwQueue_Start::Id);
        // <-- finish
        stateFilter_.ProviderEnabled(Microsoft_Windows_DxgKrnl::GUID, 0, 0, EnableAllLevels);
        // kernel proc start/stop
        stateFilter_.EventAdded(Microsoft_Windows_Kernel_Process::ProcessStart_Start::Id);
        stateFilter_.EventAdded(Microsoft_Windows_Kernel_Process::ProcessStop_Stop::Id);
        stateFilter_.ProviderEnabled(Microsoft_Windows_Kernel_Process::GUID, 0, 0, EnableAllLevels);
    }
    STDMETHODIMP QueryInterface(const IID& iid, void** pObj)
    {
        if (iid == IID_IUnknown) {
            *pObj = dynamic_cast<IUnknown*>(this);
        }
        else if (iid == IID_ITraceEventCallback) {
            *pObj = dynamic_cast<ITraceEventCallback*>(this);
        }
        else {
            *pObj = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&ref_count_);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG ucount = InterlockedDecrement(&ref_count_);
        if (ucount == 0) {
            delete this;
        }
        return ucount;
    }
    HRESULT STDMETHODCALLTYPE OnBeginProcessTrace(ITraceEvent* pHeaderEvent, ITraceRelogger* pRelogger)
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnEvent(ITraceEvent* pEvent, ITraceRelogger* pRelogger)
    {
        EVENT_RECORD* pEvt = nullptr;
        pEvent->GetEventRecord(&pEvt);
        const auto& hdr = pEvt->EventHeader;
        const auto& desc = hdr.EventDescriptor;
        const auto ts = (uint64_t)hdr.TimeStamp.QuadPart;
        if (!firstTimestamp_) {
            firstTimestamp_ = ts;
            if (trimRangeMs_) {
                trimRangeQpc_ = {
                    ts + uint64_t(trimRangeMs_->first * 10'000),
                    ts + uint64_t(trimRangeMs_->second * 10'000),
                };
            }
        }
        lastTimestamp_ = ts;
        eventCount_++;
        bool canDiscard = true;
        if (trimRangeQpc_) {
            // tail events always discardable
            if (ts > trimRangeQpc_->second) {
                return S_OK;
            }
            // if we are trimming by time range, we probably want to preserve state events
            if (!trimState_) {
                if (auto pProducerFilter = stateFilter_.LookupProvider(hdr.ProviderId)) {
                    if (pProducerFilter->MatchesId(desc.Id)) {
                        canDiscard = false;
                    }
                }
            }
            // trim non-state events in head preceding the trim range
            if (canDiscard && ts < trimRangeQpc_->first) {
                return S_OK;
            }
        }
        // filter only if we have a filter object and not skipping filter step
        if (canDiscard && pFilter_) {
            if (auto pProducerFilter = pFilter_->LookupProvider(hdr.ProviderId)) {
                // if filtering by event id, discard if id not allowed by producer filter
                if (byId_ && !pProducerFilter->MatchesId(desc.Id)) {
                    return S_OK;
                }
            }
            else {
                return S_OK;
            }
        }
        keepCount_++;
        if (mode_ == Mode::Trim) {
            pRelogger->Inject(pEvent);
        }
        if (listProcesses_) {
            eventCountByProcess_[hdr.ProcessId]++;
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnFinalizeProcessTrace(ITraceRelogger* pRelogger)
    {
        return S_OK;
    }
    int GetEventCount() const
    {
        return eventCount_;
    }
    std::pair<uint64_t, uint64_t> GetTimestampRange() const
    {
        return { firstTimestamp_.value_or(0), lastTimestamp_};
    }
    void SetTrimRangeQpc(std::pair<uint64_t, uint64_t> range)
    {
        trimRangeQpc_ = range;
    }
    void SetTrimRangeMs(std::pair<double, double> range)
    {
        trimRangeMs_ = range;
    }
    int GetKeepCount() const
    {
        return keepCount_;
    }
    auto GetProcessList() const
    {
        return eventCountByProcess_
            | vi::transform([](auto const& kv) {return std::pair{ kv.first, kv.second };})
            | rn::to<std::vector>();
    }
};

class TempFile
{
public:
    operator const CComBSTR& () const
    {
        return name_;
    }
    ~TempFile()
    {
        std::filesystem::remove((const wchar_t*)name_);
    }
private:
    CComBSTR name_ = "null-log.etl.tmp";
};


int main(int argc, const char** argv)
{
    using namespace pmon;

    // parse command line, return with error code from CLI11 if running as app
    if (auto e = clio::Options::Init(argc, argv)) {
        return *e;
    }
    auto& opt = clio::Options::Get();

    const auto ValidateRange = [](const auto& range) {
        if (range) {
            auto& r = *range;
            if (r.first > r.second) {
                std::cout << "Lower bound of trim range [" << r.first <<
                    "] must not exceed upper [" << r.second << "]" << std::endl;
                return 1;
            }
        }
        return 0;
    };
    if (ValidateRange(opt.trimRangeQpc)) return -1;
    if (ValidateRange(opt.trimRangeMs)) return -1;
    if (ValidateRange(opt.trimRangeNs)) return -1;

    std::locale::global(std::locale("en_US.UTF-8"));

    if (auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); FAILED(hr)) {
        std::cout << "Failed to init COM" << std::endl;
        return -1;
    }

    ITraceRelogger* pRelogger = nullptr;
    if (auto hr = CoCreateInstance(CLSID_TraceRelogger, 0, CLSCTX_INPROC_SERVER, IID_ITraceRelogger, reinterpret_cast<void**>(&pRelogger));
        FAILED(hr)) {
        std::cout << "Failed to create trace relogger instance" << std::endl;
        return -1;
    }

    TRACEHANDLE relogTraceHandle;
    const CComBSTR inputEtlName = (*opt.inputFile).c_str();
    if (auto hr = pRelogger->AddLogfileTraceStream(inputEtlName, nullptr, &relogTraceHandle); FAILED(hr)) {
        std::cout << "Failed to add logfile: " << *opt.inputFile << std::endl;
    }

    // we must output to etl file no matter what
    // create a dummy temp etl if user did not specify output
    CComBSTR outputEtlName;
    std::optional<TempFile> temp;
    if (opt.outputFile) {
        outputEtlName = (*opt.outputFile).c_str();
    }
    else {
        temp.emplace();
        outputEtlName = *temp;
    }
    if (auto hr = pRelogger->SetOutputFilename(outputEtlName); FAILED(hr)) {
        std::cout << "Failed to set output file: " << *opt.outputFile << std::endl;
    }

    // do a dry run of PresentMon provider/filter processing to enumerate the filter parameters
    std::shared_ptr<Filter> pFilter;
    if (opt.provider) {
        pFilter = std::make_shared<Filter>();
        PMTraceConsumer traceConsumer;
        traceConsumer.mTrackDisplay = true;   // ... presents to the display.
        traceConsumer.mTrackGPU = true;       // ... GPU work.
        traceConsumer.mTrackGPUVideo = true;  // ... GPU video work (separately from non-video GPU work).
        traceConsumer.mTrackInput = true;     // ... keyboard/mouse latency.
        traceConsumer.mTrackFrameType = true; // ... the frame type communicated through the Intel-PresentMon provider.
        traceConsumer.mTrackAppTiming = true; // ... app timing data communicated through the Intel-PresentMon provider.
        traceConsumer.mTrackPcLatency = true; // ... Nvidia PCL stats.
        EnableProvidersListing(0, nullptr, &traceConsumer, true, true, pFilter);
    }

    auto pCallbackProcessor = std::make_unique<EventCallback>(!opt.outputFile,
        pFilter, (bool)opt.trimState, (bool)opt.event, (bool)opt.listProcesses);
    if (auto hr = pRelogger->RegisterCallback(pCallbackProcessor.get()); FAILED(hr)) {
        std::cout << "Failed to register callback" << std::endl;
    }

    if (opt.trimRangeQpc) {
        pCallbackProcessor->SetTrimRangeQpc(*opt.trimRangeQpc);
    }
    else if (opt.trimRangeMs) {
        pCallbackProcessor->SetTrimRangeMs(*opt.trimRangeMs);
    }
    else if (opt.trimRangeNs) {
        auto& r = *opt.trimRangeNs;
        pCallbackProcessor->SetTrimRangeMs({
            r.first * 1'000'000.,
            r.second * 1'000'000.
        });
    }

    if (auto hr = pRelogger->ProcessTrace(); FAILED(hr)) {
        std::cout << "Failed to process trace: " << util::win::GetErrorDescription(hr) << std::endl;
    }

    const auto tsr = pCallbackProcessor->GetTimestampRange();
    const auto dur = tsr.second - tsr.first;
    std::cout << std::format(" ======== Report for [ {} ] ========\n", *opt.inputFile);
    std::cout << std::format("Total event count: {:L}\n", pCallbackProcessor->GetEventCount());
    std::cout << std::format("Timestamp range {:L} - {:L} (duration: {:L})\n", tsr.first, tsr.second, dur);
    std::cout << std::format("Duration of trace in milliseconds: {:L}\n\n", double(dur) / 10'000.);

    std::cout << std::format("Events trimmed and/or filtered: {:L}\n",
        pCallbackProcessor->GetEventCount() - pCallbackProcessor->GetKeepCount());
    std::cout << std::format("Count of persisted events: {:L}\n", pCallbackProcessor->GetKeepCount());

    if (opt.listProcesses) {
        std::cout << "\n ======== Processes ========\n\n";
        auto procs = pCallbackProcessor->GetProcessList();
        std::erase_if(procs, [](auto& p) { return p.first == 0xFFFF'FFFF; });
        rn::sort(procs, std::greater{}, &decltype(procs)::value_type::second);

        // figure out how wide each column must be (based on the values)
        size_t pidW = 0, cntW = 0;
        for (auto& [pid, cnt] : procs) {
            pidW = std::max(pidW, std::to_string(pid).size());
            cntW = std::max(cntW, std::to_string(cnt).size());
        }

        // print header
        std::cout
            << std::format("{:>{}}  {:>{}}\n", "PID", pidW, "EVTs", cntW)
            << std::string(pidW + 2 + cntW, '-') << "\n";

        // print each row, right-aligned into those widths
        for (auto& [pid, cnt] : procs) {
            std::cout << std::format("{:>{}}  {:>{}}\n",
                pid, pidW,
                cnt, cntW);
        }

        std::cout << std::endl;
    }


    if (!opt.outputFile) {
        std::cout << "No output specified; ran in analysis mode\n";
    }
    else {
        std::cout << "Output written to: " << *opt.outputFile << std::endl;
    }

    return 0;
}

namespace pmon::util::log
{
    std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
    {
        return {};
    }
}