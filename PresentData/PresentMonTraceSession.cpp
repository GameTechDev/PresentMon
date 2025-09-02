// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#include "Debug.hpp"
#include "PresentMonTraceConsumer.hpp"
#include "PresentMonTraceSession.hpp"
#include "NvidiaTraceConsumer.hpp"

#include "ETW/Microsoft_Windows_D3D9.h"
#include "ETW/Microsoft_Windows_Dwm_Core.h"
#include "ETW/Microsoft_Windows_Dwm_Core_Win7.h"
#include "ETW/Microsoft_Windows_DXGI.h"
#include "ETW/Microsoft_Windows_DxgKrnl.h"
#include "ETW/Microsoft_Windows_DxgKrnl_Win7.h"
#include "ETW/Microsoft_Windows_EventMetadata.h"
#include "ETW/Microsoft_Windows_Kernel_Process.h"
#include "ETW/Microsoft_Windows_Win32k.h"
#include "ETW/NT_Process.h"
#include "ETW/Intel_PresentMon.h"
#include "ETW/NV_DD.h"
#include "ETW/Nvidia_PCL.h"

namespace {

struct TraceProperties : public EVENT_TRACE_PROPERTIES {
    wchar_t mSessionName[MAX_PATH];
};

template<typename T> void PatchKeyword(uint64_t*) {}
template<typename T> void PatchPreWin11Keyword(uint64_t*) {}

// Win11 changed some Microsoft-Windows-Dwm-Core event kewords from Composition to Scheduling:
template<> void PatchPreWin11Keyword<Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start>     (uint64_t* k) { *k = (*k & ~(uint64_t) Microsoft_Windows_Dwm_Core::Keyword::Scheduling) | (uint64_t) Microsoft_Windows_Dwm_Core::Keyword::Composition; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info>(uint64_t* k) { *k = (*k & ~(uint64_t) Microsoft_Windows_Dwm_Core::Keyword::Scheduling) | (uint64_t) Microsoft_Windows_Dwm_Core::Keyword::Composition; }

// Win11 added a Present keyword to some Microsoft-Windows-DxgKrnl events:
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::BlitCancel_Info>               (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::Blit_Info>                     (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info>    (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::Flip_Info>                     (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info>       (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info>(uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::MMIOFlip_Info>                 (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start>  (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::PresentHistory_Info>           (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::PresentHistory_Start>          (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info>       (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }
template<> void PatchPreWin11Keyword<Microsoft_Windows_DxgKrnl::VSyncDPC_Info>                 (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Present; }

// Never filter DxgKrnl events using the Performance keyword, as that can have side-effects with
// negative performance impact on some versions of Windows.
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::PresentHistory_Start>           (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Blit_Info>                      (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::BlitCancel_Info>                (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Flip_Info>                      (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::IndependentFlip_Info>           (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info>     (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info>        (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info>        (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::MMIOFlip_Info>                  (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info> (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Present_Info>                   (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::PresentHistory_Info>            (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start>   (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::QueuePacket_Start>              (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::QueuePacket_Start_2>            (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::QueuePacket_Stop>               (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::VSyncDPC_Info>                  (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Context_DCStart>                (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Context_Start>                  (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Context_Stop>                   (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Device_DCStart>                 (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Device_Start>                   (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::Device_Stop>                    (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::HwQueue_DCStart>                (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::HwQueue_Start>                  (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::DmaPacket_Info>                 (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::DmaPacket_Start>                (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::NodeMetadata_Info>              (uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }
template<> void PatchKeyword<Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay3_Info>(uint64_t* k) { *k &= ~(uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance; }


struct FilteredProvider {
    EVENT_FILTER_DESCRIPTOR filterDesc_;
    ENABLE_TRACE_PARAMETERS params_;
    uint64_t anyKeywordMask_;
    uint64_t allKeywordMask_;
    uint8_t maxLevel_;
    bool isWin11OrGreater_;
    bool isDryRun_;
    std::shared_ptr<IFilterBuildListener> pListener_;

    FilteredProvider(
        const GUID* pSessionGuid,
        bool filterEventIds,
        bool isWin11OrGreater,
        std::shared_ptr<IFilterBuildListener> pListener)
    {
        memset(&filterDesc_, 0, sizeof(filterDesc_));
        memset(&params_,     0, sizeof(params_));

        anyKeywordMask_ = 0;
        allKeywordMask_ = 0;
        maxLevel_ = 0;
        isWin11OrGreater_ = isWin11OrGreater;
        isDryRun_ = pSessionGuid == nullptr;
        pListener_ = std::move(pListener);

        if (filterEventIds && !isDryRun_) {
            static_assert(MAX_EVENT_FILTER_EVENT_ID_COUNT >= ANYSIZE_ARRAY, "Unexpected MAX_EVENT_FILTER_EVENT_ID_COUNT");
            auto memorySize = sizeof(EVENT_FILTER_EVENT_ID) + sizeof(USHORT) * (MAX_EVENT_FILTER_EVENT_ID_COUNT - ANYSIZE_ARRAY);
            void* memory = _aligned_malloc(memorySize, alignof(USHORT));
            if (memory != nullptr) {
                auto filteredEventIds = (EVENT_FILTER_EVENT_ID*) memory;
                filteredEventIds->FilterIn = TRUE;
                filteredEventIds->Reserved = 0;
                filteredEventIds->Count = 0;

                filterDesc_.Ptr = (ULONGLONG) filteredEventIds;
                filterDesc_.Size = (ULONG) memorySize;
                filterDesc_.Type = EVENT_FILTER_TYPE_EVENT_ID;

                params_.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
                params_.EnableProperty = EVENT_ENABLE_PROPERTY_IGNORE_KEYWORD_0;
                params_.SourceId = *pSessionGuid;
                params_.EnableFilterDesc = &filterDesc_;
                params_.FilterDescCount = 1;
            }
        }
    }

    ~FilteredProvider()
    {
        if (filterDesc_.Ptr != 0) {
            auto memory = (void*) filterDesc_.Ptr;
            _aligned_free(memory);
        }
    }

    void ClearFilter()
    {
        if (filterDesc_.Ptr != 0) {
            auto filteredEventIds = (EVENT_FILTER_EVENT_ID*) filterDesc_.Ptr;
            filteredEventIds->Count = 0;
        }

        anyKeywordMask_ = 0;
        allKeywordMask_ = 0;
        maxLevel_ = 0;
    }

    void AddKeyword(uint64_t keyword)
    {
        if (anyKeywordMask_ == 0) {
            anyKeywordMask_ = keyword;
            allKeywordMask_ = keyword;
        } else {
            anyKeywordMask_ |= keyword;
            allKeywordMask_ &= keyword;
        }
    }

    template<typename T>
    void AddEvent()
    {
        if (pListener_) {
            pListener_->EventAdded(T::Id);
        }

        uint64_t keyword = (uint64_t)T::Keyword;
        PatchKeyword<T>(&keyword);
        if (!isWin11OrGreater_) {
            PatchPreWin11Keyword<T>(&keyword);
        }
        AddKeyword(keyword);

        maxLevel_ = std::max(maxLevel_, T::Level);

        if (filterDesc_.Ptr != 0 && !isDryRun_) {
            auto filteredEventIds = (EVENT_FILTER_EVENT_ID*) filterDesc_.Ptr;
            assert(filteredEventIds->Count < MAX_EVENT_FILTER_EVENT_ID_COUNT);
            filteredEventIds->Events[filteredEventIds->Count++] = T::Id;
        }
    }

    ULONG Enable(
        TRACEHANDLE sessionHandle,
        GUID const& providerGuid,
        ULONG controlCode = EVENT_CONTROL_CODE_ENABLE_PROVIDER)
    {
        if (pListener_) {
            if (controlCode == EVENT_CONTROL_CODE_ENABLE_PROVIDER) {
                pListener_->ProviderEnabled(providerGuid, anyKeywordMask_, allKeywordMask_, maxLevel_);
            }
            else {
                pListener_->ClearEvents();
            }
        }

        if (!isDryRun_) {
            ENABLE_TRACE_PARAMETERS* pparams = nullptr;
            if (filterDesc_.Ptr != 0) {
                pparams = &params_;

                // EnableTraceEx2() fails unless Size agrees with Count.
                auto filterEventIds = (EVENT_FILTER_EVENT_ID*)filterDesc_.Ptr;
                filterDesc_.Size = sizeof(EVENT_FILTER_EVENT_ID) + sizeof(USHORT) * (filterEventIds->Count - ANYSIZE_ARRAY);
            }

            ULONG timeout = 0;
            return EnableTraceEx2(sessionHandle, &providerGuid, controlCode,
                maxLevel_, anyKeywordMask_, allKeywordMask_, timeout, pparams);
        }
        return ERROR_SUCCESS;
    }

    ULONG EnableWithoutFiltering(
        TRACEHANDLE sessionHandle,
        GUID const& providerGuid,
        UCHAR maxLevel)
    {
        if (pListener_) {
            pListener_->ProviderEnabled(providerGuid, 0, 0, maxLevel);
        }

        if (!isDryRun_) {
            return EnableTraceEx2(sessionHandle, &providerGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                maxLevel, 0, 0, 0, nullptr);
        }
        return ERROR_SUCCESS;
    }
};

// detects windows version, enables providers, sets event filters in realtime mode
ULONG EnableProviders(
    TRACEHANDLE sessionHandle,
    GUID const& sessionGuid,
    PMTraceConsumer* pmConsumer)
{
    ULONG status = 0;

    // Lookup what OS we're running on
    //
    // We can't use helpers like IsWindows8Point1OrGreater() since they FALSE
    // if the application isn't built with a manifest.
    bool isWin81OrGreater = false;
    bool isWin11OrGreater = false;
    {
        auto hmodule = LoadLibraryExA("ntdll.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (hmodule != NULL) {
            auto pRtlGetVersion = (LONG (WINAPI*)(RTL_OSVERSIONINFOW*)) GetProcAddress(hmodule, "RtlGetVersion");
            if (pRtlGetVersion != nullptr) {
                RTL_OSVERSIONINFOW info = {};
                info.dwOSVersionInfoSize = sizeof(info);
                status = (*pRtlGetVersion)(&info);
                if (status == 0 /* STATUS_SUCCESS */) {
                    // win8.1 = version 6.3
                    // win11  = version 10.0 build >= 22000
                    isWin81OrGreater = info.dwMajorVersion >  6 || (info.dwMajorVersion ==  6 && info.dwMinorVersion >= 3);
                    isWin11OrGreater = info.dwMajorVersion > 10 || (info.dwMajorVersion == 10 && info.dwBuildNumber  >= 22000);
                }
            }
            FreeLibrary(hmodule);
        }
    }

    // Scope filtering based on event ID only works on Win8.1 or greater.
    bool filterEventIds = isWin81OrGreater;
    pmConsumer->mFilteredEvents = filterEventIds;

    return EnableProvidersListing(sessionHandle, &sessionGuid, pmConsumer, filterEventIds, isWin11OrGreater);
}

void DisableProviders(TRACEHANDLE sessionHandle)
{
    ULONG status = 0;
    status = EnableTraceEx2(sessionHandle, &Intel_PresentMon::GUID,                 EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_D3D9::GUID,           EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DXGI::GUID,           EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::GUID,       EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::Win7::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::GUID,        EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::Win7::GUID,  EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Kernel_Process::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Win32k::GUID,         EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &NvidiaDisplayDriver_Events::GUID,       EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
}

template<
    bool IS_REALTIME_SESSION,
    bool TRACK_DISPLAY,
    bool TRACK_INPUT,
    bool TRACK_PRESENTMON,
    bool TRACK_PCL>
void CALLBACK EventRecordCallback(EVENT_RECORD* pEventRecord)
{
    auto session = (PMTraceSession*) pEventRecord->UserContext;
    const auto& hdr = pEventRecord->EventHeader;

    if constexpr (!IS_REALTIME_SESSION) {
        if (session->mStartTimestamp.QuadPart == 0) {
            session->mStartTimestamp = hdr.TimeStamp;
            // one-time capture of timing info needed to calibrate ETL replay event pacing
            if (session->mPMConsumer->mPaceEvents || session->mPMConsumer->mRetimeEvents) {
                session->mPacingActualLogStartTimestamp = hdr.TimeStamp.QuadPart;
                session->mPacingRealtimeStartTimestamp = pmon::util::GetCurrentTimestamp();
                session->mPacingQpcPeriod = pmon::util::GetTimestampPeriodSeconds();
                session->mPacingQpcOffset = session->mPacingRealtimeStartTimestamp - hdr.TimeStamp.QuadPart;
                // override the processing start timestamp with the adjusted value
                session->mStartTimestamp.QuadPart = session->mPacingRealtimeStartTimestamp;
                if (session->mPMConsumer->mRetimeEvents) {
                    // perform the first adjustment of the event header (no wait or calculation necessary)
                    pEventRecord->EventHeader.TimeStamp.QuadPart = session->mPacingRealtimeStartTimestamp;
                }
            }
        }
        else if (session->mPMConsumer->mPaceEvents || session->mPMConsumer->mRetimeEvents) {
            const auto currentQpc = pmon::util::GetCurrentTimestamp();
            const auto adjustedTimestamp = hdr.TimeStamp.QuadPart + session->mPacingQpcOffset;
            const auto delta = pmon::util::TimestampDeltaToSeconds(currentQpc, adjustedTimestamp, session->mPacingQpcPeriod);
            if (session->mPMConsumer->mPaceEvents && delta > 0.001) {
                session->mPacingWaiter.Wait(delta);
            }
            if (session->mPMConsumer->mRetimeEvents) {
                pEventRecord->EventHeader.TimeStamp.QuadPart = adjustedTimestamp;
            }
        }
    }

    VerboseTraceEvent(session->mPMConsumer, pEventRecord, &session->mPMConsumer->mMetadata);

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::GUID) {
        session->mPMConsumer->HandleDXGKEvent(pEventRecord);
        return;
    }
    if (hdr.ProviderId == Microsoft_Windows_DXGI::GUID) {
        session->mPMConsumer->HandleDXGIEvent(pEventRecord);
        return;
    }
    if constexpr (TRACK_DISPLAY || TRACK_INPUT) {
        if (hdr.ProviderId == Microsoft_Windows_Win32k::GUID) {
            session->mPMConsumer->HandleWin32kEvent(pEventRecord);
            return;
        }
    }
    if constexpr (TRACK_DISPLAY) {
        if (hdr.ProviderId == Microsoft_Windows_Dwm_Core::GUID) {
            session->mPMConsumer->HandleDWMEvent(pEventRecord);
            return;
        }
    }
    if (hdr.ProviderId == Microsoft_Windows_D3D9::GUID) {
        session->mPMConsumer->HandleD3D9Event(pEventRecord);
        return;
    }
    if (hdr.ProviderId == Microsoft_Windows_Kernel_Process::GUID ||
        hdr.ProviderId == NT_Process::GUID) {
        session->mPMConsumer->HandleProcessEvent(pEventRecord);
        return;
    }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::PRESENTHISTORY_GUID) {
        session->mPMConsumer->HandleWin7DxgkPresentHistory(pEventRecord);
        return;
    }
    if (hdr.ProviderId == Microsoft_Windows_EventMetadata::GUID) {
        session->mPMConsumer->HandleMetadataEvent(pEventRecord);
        return;
    }

    if constexpr (TRACK_DISPLAY) {
        if (hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID) {
            session->mPMConsumer->HandleDWMEvent(pEventRecord);
            return;
        }
        if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::BLT_GUID) {
            session->mPMConsumer->HandleWin7DxgkBlt(pEventRecord);
            return;
        }
        if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::FLIP_GUID) {
            session->mPMConsumer->HandleWin7DxgkFlip(pEventRecord);
            return;
        }
        if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::QUEUEPACKET_GUID) {
            session->mPMConsumer->HandleWin7DxgkQueuePacket(pEventRecord);
            return;
        }
        if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::VSYNCDPC_GUID) {
            session->mPMConsumer->HandleWin7DxgkVSyncDPC(pEventRecord);
            return;
        }
        if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::MMIOFLIP_GUID) {
            session->mPMConsumer->HandleWin7DxgkMMIOFlip(pEventRecord);
            return;
        }
        if (hdr.ProviderId == NvidiaDisplayDriver_Events::GUID) {
            session->mPMConsumer->HandleNvidiaDisplayDriverEvent(pEventRecord);
            return;
        }
    }

    if constexpr (TRACK_PRESENTMON) {
        if (hdr.ProviderId == Intel_PresentMon::GUID) {
            session->mPMConsumer->HandleIntelPresentMonEvent(pEventRecord);
            return;
        }
    }

    if constexpr (TRACK_PCL) {
        if (hdr.ProviderId == Nvidia_PCL::GUID) {
            session->mPMConsumer->HandlePclEvent(pEventRecord);
            return;
        }
    }
}

template<bool... Ts>
PEVENT_RECORD_CALLBACK GetEventRecordCallback(bool t1)
{
    return t1 ? &EventRecordCallback<Ts..., true>
              : &EventRecordCallback<Ts..., false>;
}

template<bool... Ts>
PEVENT_RECORD_CALLBACK GetEventRecordCallback(bool t1, bool t2)
{
    return t1 ? GetEventRecordCallback<Ts..., true>(t2)
              : GetEventRecordCallback<Ts..., false>(t2);
}

template<bool... Ts>
PEVENT_RECORD_CALLBACK GetEventRecordCallback(bool t1, bool t2, bool t3)
{
    return t1 ? GetEventRecordCallback<Ts..., true>(t2, t3)
              : GetEventRecordCallback<Ts..., false>(t2, t3);
}

template<bool... Ts>
PEVENT_RECORD_CALLBACK GetEventRecordCallback(bool t1, bool t2, bool t3, bool t4)
{
    return t1 ? GetEventRecordCallback<Ts..., true>(t2, t3, t4)
              : GetEventRecordCallback<Ts..., false>(t2, t3, t4);
}

template<bool... Ts>
PEVENT_RECORD_CALLBACK GetEventRecordCallback(bool t1, bool t2, bool t3, bool t4, bool t5)
{
    return t1 ? GetEventRecordCallback<Ts..., true>(t2, t3, t4, t5)
        : GetEventRecordCallback<Ts..., false>(t2, t3, t4, t5);
}

ULONG CALLBACK BufferCallback(EVENT_TRACE_LOGFILE* pLogFile)
{
    auto session = (PMTraceSession*) pLogFile->Context;
    return session->mContinueProcessingBuffers; // TRUE = continue processing events, FALSE = return out of ProcessTrace()
}

}

ULONG PMTraceSession::Start(
    wchar_t const* etlPath,
    wchar_t const* sessionName)
{
    assert(mPMConsumer != nullptr);
    assert(mSessionHandle == 0);
    assert(mTraceHandle == INVALID_PROCESSTRACE_HANDLE);
    mStartTimestamp.QuadPart = 0;
    mContinueProcessingBuffers = TRUE;
    mIsRealtimeSession = etlPath == nullptr;
    mPMConsumer->mIsRealtimeSession = mIsRealtimeSession;

    // If we're not reading an ETL, start a realtime trace session with the
    // required providers enabled.
    if (mIsRealtimeSession) {
        TraceProperties sessionProps = {};
        sessionProps.Wnode.BufferSize = (ULONG) sizeof(TraceProperties);
        sessionProps.Wnode.ClientContext = mTimestampType;          // Clock resolution to use when logging the timestamp for each event
        sessionProps.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        sessionProps.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;      // We have a realtime consumer, not writing to a log file
        sessionProps.LoggerNameOffset = offsetof(TraceProperties, mSessionName);  // Location of session name; will be written by StartTrace()

        auto status = StartTraceW(&mSessionHandle, sessionName, &sessionProps);
        if (status != ERROR_SUCCESS) {
            mSessionHandle = 0;
            return status;
        }

        status = EnableProviders(mSessionHandle, sessionProps.Wnode.Guid, mPMConsumer);
        if (status != ERROR_SUCCESS) {
            Stop();
            return status;
        }
    }

    // Open a trace to collect the session events
    EVENT_TRACE_LOGFILEW traceProps = {};
    traceProps.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    traceProps.Context = this;

    if (mIsRealtimeSession) {
        traceProps.LoggerName = (wchar_t*) sessionName;
        traceProps.ProcessTraceMode |= PROCESS_TRACE_MODE_REAL_TIME;
    } else {
        traceProps.LogFileName = (wchar_t*) etlPath;

        // When processing log files, we need to use the buffer callback in
        // case the user wants to stop processing before the entire log has
        // been parsed.
        traceProps.BufferCallback = &BufferCallback;
    }

    traceProps.EventRecordCallback = GetEventRecordCallback(
        mIsRealtimeSession,            // IS_REALTIME_SESSION
        mPMConsumer->mTrackDisplay,    // TRACK_DISPLAY
        mPMConsumer->mTrackInput,      // TRACK_INPUT
        mPMConsumer->mTrackFrameType || mPMConsumer->mTrackPMMeasurements || mPMConsumer->mTrackAppTiming, // TRACK_PRESENTMON
        mPMConsumer->mTrackPcLatency); // TRACK_PC_LATENCY

    mTraceHandle = OpenTraceW(&traceProps);
    if (mTraceHandle == INVALID_PROCESSTRACE_HANDLE) {
        auto openTraceError = GetLastError();
        Stop();
        return openTraceError;
    }

    // Save the initial time to base capture off of.  ETL captures use the
    // time of the first event, which matches GPUVIEW usage, and realtime
    // captures are based off the timestamp here.
    mTimestampType = (TimestampType) traceProps.LogfileHeader.ReservedFlags;
    switch (mTimestampType) {
    case TIMESTAMP_TYPE_SYSTEM_TIME:
        mTimestampFrequency.QuadPart = 10000000ull;
        break;
    case TIMESTAMP_TYPE_CPU_CYCLE_COUNTER:
        mTimestampFrequency.QuadPart = 1000000ull * traceProps.LogfileHeader.CpuSpeedInMHz;
        break;
    case TIMESTAMP_TYPE_QPC:
    default:
        mTimestampFrequency = traceProps.LogfileHeader.PerfFreq;
        break;
    }

    // Default to systemtime frequency if the frequency didn't load correctly.
    if (mTimestampFrequency.QuadPart == 0) {
        mTimestampFrequency.QuadPart = 10000000ull;
    }

    if (mIsRealtimeSession) {
        LARGE_INTEGER qpc1 = {};
        LARGE_INTEGER qpc2 = {};
        FILETIME ft = {};
        QueryPerformanceCounter(&qpc1);
        GetSystemTimeAsFileTime(&ft);
        QueryPerformanceCounter(&qpc2);
        FileTimeToLocalFileTime(&ft, (FILETIME*) &mStartFileTime);
        mStartTimestamp.QuadPart = (qpc1.QuadPart + qpc2.QuadPart) / 2;
    } else {
        // Convert start FILETIME to local start FILETIME
        SYSTEMTIME ust{};
        SYSTEMTIME lst{};
        FileTimeToSystemTime((FILETIME const*) &traceProps.LogfileHeader.StartTime, &ust);
        SystemTimeToTzSpecificLocalTime(&traceProps.LogfileHeader.TimeZone, &ust, &lst);
        SystemTimeToFileTime(&lst, (FILETIME*) &mStartFileTime);
        // The above conversion stops at milliseconds, so copy the rest over too
        mStartFileTime += traceProps.LogfileHeader.StartTime.QuadPart % 10000;
    }

    InitializeTimestampInfo(&mStartTimestamp, mTimestampFrequency);

    return ERROR_SUCCESS;
}

void PMTraceSession::Stop()
{
    ULONG status = 0;

    // Stop the session
    if (mSessionHandle != 0) {
        DisableProviders(mSessionHandle);

        TraceProperties sessionProps = {};
        sessionProps.Wnode.BufferSize = (ULONG) sizeof(TraceProperties);
        sessionProps.LoggerNameOffset = offsetof(TraceProperties, mSessionName);

        status = ControlTraceW(mSessionHandle, nullptr, &sessionProps, EVENT_TRACE_CONTROL_QUERY);
        mNumEventsLost = sessionProps.EventsLost;
        mNumBuffersLost = sessionProps.LogBuffersLost + sessionProps.RealTimeBuffersLost;

        status = ControlTraceW(mSessionHandle, nullptr, &sessionProps, EVENT_TRACE_CONTROL_STOP);

        mSessionHandle = 0; // mSessionHandle is no longer valid after EVENT_TRACE_CONTROL_STOP
    }

    // Stop the trace (remains open until ProcessTrace() finishes)
    if (mTraceHandle != INVALID_PROCESSTRACE_HANDLE) {
        status = CloseTrace(mTraceHandle);
        mTraceHandle = INVALID_PROCESSTRACE_HANDLE;
    }

    // If collecting realtime events, CloseTrace() will cause ProcessTrace() to
    // stop filling buffers and it will return after it finishes processing
    // events already in it's buffers.
    //
    // If collecting from a log file, ProcessTrace() normally continues to
    // process the entire file so we cancel processing from the BufferCallback
    // in this case.
    if (!mIsRealtimeSession) {
        mContinueProcessingBuffers = FALSE;
    }
}

ULONG StopNamedTraceSession(wchar_t const* sessionName)
{
    TraceProperties sessionProps = {};
    sessionProps.Wnode.BufferSize = (ULONG) sizeof(TraceProperties);
    sessionProps.LoggerNameOffset = offsetof(TraceProperties, mSessionName);
    return ControlTraceW((TRACEHANDLE) 0, sessionName, &sessionProps, EVENT_TRACE_CONTROL_STOP);
}

double PMTraceSession::TimestampDeltaToMilliSeconds(uint64_t timestampDelta) const
{
    return 1000.0 * timestampDelta / mTimestampFrequency.QuadPart;
}

double PMTraceSession::TimestampDeltaToUnsignedMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo) const
{
    return timestampFrom == 0 || timestampTo <= timestampFrom ? 0.0 : TimestampDeltaToMilliSeconds(timestampTo - timestampFrom);
}

double PMTraceSession::TimestampDeltaToMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo) const
{
    return timestampFrom == 0 || timestampTo == 0 || timestampFrom == timestampTo ? 0.0 :
           timestampTo > timestampFrom                                            ? TimestampDeltaToMilliSeconds(timestampTo - timestampFrom)
                                                                                  : -TimestampDeltaToMilliSeconds(timestampFrom - timestampTo);
}

uint64_t PMTraceSession::MilliSecondsDeltaToTimestamp(double millisecondsDelta) const
{
    return (uint64_t) (0.001 * millisecondsDelta * mTimestampFrequency.QuadPart);
}

double PMTraceSession::TimestampToMilliSeconds(uint64_t timestamp) const
{
    return TimestampDeltaToMilliSeconds(timestamp - mStartTimestamp.QuadPart);
}

void PMTraceSession::TimestampToLocalSystemTime(uint64_t timestamp, SYSTEMTIME* st, uint64_t* ns) const
{
    if (mTimestampType != PMTraceSession::TIMESTAMP_TYPE_SYSTEM_TIME) {
        auto delta100ns = (timestamp - mStartTimestamp.QuadPart) * 10000000ull / mTimestampFrequency.QuadPart;
        timestamp = mStartFileTime + delta100ns;
    }

    FILETIME lft{};
    FileTimeToLocalFileTime((FILETIME*) &timestamp, &lft);
    FileTimeToSystemTime(&lft, st);
    *ns = (timestamp % 10000000) * 100;
}

ULONG EnableProvidersListing(
    TRACEHANDLE sessionHandle,
    const GUID* pSessionGuid,
    const PMTraceConsumer* pmConsumer,
    bool filterEventIds,
    bool isWin11OrGreater,
    std::shared_ptr<IFilterBuildListener> pListener)
{
    // Start backend providers first to reduce Presents being queued up before
    // we can track them.
    FilteredProvider provider(pSessionGuid, filterEventIds, isWin11OrGreater, std::move(pListener));


    // Microsoft_Windows_DxgKrnl
    //
    // WARNING: When adding a DxgKrnl event, make sure to patch it's Performance keyword (see
    // above).
    provider.ClearFilter();
    provider.AddEvent<Microsoft_Windows_DxgKrnl::PresentHistory_Start>();
    if (pmConsumer->mTrackDisplay) {
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Blit_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::BlitCancel_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Flip_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::IndependentFlip_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::MMIOFlip_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Present_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::PresentHistory_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::QueuePacket_Start>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::QueuePacket_Start_2>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::QueuePacket_Stop>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::VSyncDPC_Info>();
    }
    if (pmConsumer->mTrackGPU) {
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Context_DCStart>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Context_Start>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Context_Stop>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Device_DCStart>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Device_Start>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Device_Stop>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::HwQueue_DCStart>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::HwQueue_Start>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::DmaPacket_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::DmaPacket_Start>();
    }
    if (pmConsumer->mTrackGPUVideo) {
        provider.AddEvent<Microsoft_Windows_DxgKrnl::NodeMetadata_Info>();
    }
    if (pmConsumer->mTrackFrameType) {
        provider.AddEvent<Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay3_Info>();
    }
    auto status = provider.Enable(sessionHandle, Microsoft_Windows_DxgKrnl::GUID);
    if (status != ERROR_SUCCESS) return status;

    // we call Enable here once more to capture initial state of the device contexts
    if (pmConsumer->mTrackGPU) {
        provider.ClearFilter();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Context_DCStart>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Device_DCStart>();
        status = provider.Enable(sessionHandle, Microsoft_Windows_DxgKrnl::GUID, EVENT_CONTROL_CODE_CAPTURE_STATE);
        if (status != ERROR_SUCCESS) return status;
    }

    // enable dxgkrnl win7 provider directly (no filtering because win7 doesn't support it anyways)
    status = provider.EnableWithoutFiltering(sessionHandle, Microsoft_Windows_DxgKrnl::Win7::GUID, TRACE_LEVEL_INFORMATION);
    if (status != ERROR_SUCCESS) return status;


    // Microsoft_Windows_Win32k
    //
    if (pmConsumer->mTrackDisplay || pmConsumer->mTrackInput) {
        provider.ClearFilter();
        if (pmConsumer->mTrackDisplay) {
            provider.AddEvent<Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info>();
            provider.AddEvent<Microsoft_Windows_Win32k::TokenStateChanged_Info>();
        }
        if (pmConsumer->mTrackInput) {
            provider.AddEvent<Microsoft_Windows_Win32k::InputDeviceRead_Stop>();
            provider.AddEvent<Microsoft_Windows_Win32k::RetrieveInputMessage_Info>();
            provider.AddEvent<Microsoft_Windows_Win32k::OnInputXformUpdate_Info>();
        }
        status = provider.Enable(sessionHandle, Microsoft_Windows_Win32k::GUID);
        if (status != ERROR_SUCCESS) return status;
    }


    // Microsoft_Windows_Dwm_Core
    //
    if (pmConsumer->mTrackDisplay) {
        provider.ClearFilter();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::FlipChain_Pending>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::FlipChain_Complete>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::FlipChain_Dirty>();
        status = provider.Enable(sessionHandle, Microsoft_Windows_Dwm_Core::GUID);
        if (status != ERROR_SUCCESS) return status;

        // enable dwm_core win7 provider directly (no filtering because win7 doesn't support it anyways)
        status = provider.EnableWithoutFiltering(sessionHandle, Microsoft_Windows_Dwm_Core::Win7::GUID, TRACE_LEVEL_VERBOSE);
        if (status != ERROR_SUCCESS) return status;
    }


    // Microsoft_Windows_DXGI
    //
    provider.ClearFilter();
    provider.AddEvent<Microsoft_Windows_DXGI::Present_Start>();
    provider.AddEvent<Microsoft_Windows_DXGI::Present_Stop>();
    provider.AddEvent<Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start>();
    provider.AddEvent<Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop>();
    if (pmConsumer->mTrackHybridPresent) {
        provider.AddEvent<Microsoft_Windows_DXGI::SwapChain_Start>();
        provider.AddEvent<Microsoft_Windows_DXGI::ResizeBuffers_Start>();
    }
    status = provider.Enable(sessionHandle, Microsoft_Windows_DXGI::GUID);
    if (status != ERROR_SUCCESS) return status;


    // Microsoft_Windows_Kernel_Process
    //
    provider.ClearFilter();
    provider.AddEvent<Microsoft_Windows_Kernel_Process::ProcessStart_Start>();
    provider.AddEvent<Microsoft_Windows_Kernel_Process::ProcessStop_Stop>();
    status = provider.Enable(sessionHandle, Microsoft_Windows_Kernel_Process::GUID);
    if (status != ERROR_SUCCESS && status != ERROR_ACCESS_DENIED) return status;


    // Microsoft_Windows_D3D9
    //
    provider.ClearFilter();
    provider.AddEvent<Microsoft_Windows_D3D9::Present_Start>();
    provider.AddEvent<Microsoft_Windows_D3D9::Present_Stop>();
    status = provider.Enable(sessionHandle, Microsoft_Windows_D3D9::GUID);
    if (status != ERROR_SUCCESS) return status;


    // Intel_PresentMon
    //
    if (pmConsumer->mTrackFrameType || pmConsumer->mTrackPMMeasurements || pmConsumer->mTrackAppTiming) {
        provider.ClearFilter();
        if (pmConsumer->mTrackFrameType) {
            provider.AddEvent<Intel_PresentMon::PresentFrameType_Info>();
            provider.AddEvent<Intel_PresentMon::FlipFrameType_Info>();
        }
        if (pmConsumer->mTrackPMMeasurements) {
            provider.AddEvent<Intel_PresentMon::MeasuredInput_Info>();
            provider.AddEvent<Intel_PresentMon::MeasuredScreenChange_Info>();
        }
        if (pmConsumer->mTrackAppTiming) {
            provider.AddEvent<Intel_PresentMon::AppInputSample_Info>();
            provider.AddEvent<Intel_PresentMon::AppPresentStart_Info>();
            provider.AddEvent<Intel_PresentMon::AppPresentEnd_Info>();
            provider.AddEvent<Intel_PresentMon::AppSimulationStart_Info>();
            provider.AddEvent<Intel_PresentMon::AppSimulationEnd_Info>();
            provider.AddEvent<Intel_PresentMon::AppRenderSubmitStart_Info>();
            provider.AddEvent<Intel_PresentMon::AppRenderSubmitEnd_Info>();
            provider.AddEvent<Intel_PresentMon::AppSleepStart_Info>();
            provider.AddEvent<Intel_PresentMon::AppSleepEnd_Info>();
        }
        status = provider.Enable(sessionHandle, Intel_PresentMon::GUID);
        if (status != ERROR_SUCCESS) return status;
    }

    // Nvidia_DisplayDriver
    //
    provider.ClearFilter();
    provider.AddEvent<NvidiaDisplayDriver_Events::FlipRequest>();
    status = provider.Enable(sessionHandle, NvidiaDisplayDriver_Events::GUID);
    if (status != ERROR_SUCCESS) return status;

    if (pmConsumer->mTrackPcLatency) {
        provider.ClearFilter();
        status = provider.EnableWithoutFiltering(sessionHandle, Nvidia_PCL::GUID, TRACE_LEVEL_VERBOSE);
        if (status != ERROR_SUCCESS) return status;
    }

    return ERROR_SUCCESS;
}

