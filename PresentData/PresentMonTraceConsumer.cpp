// Copyright (C) 2017-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMonTraceConsumer.hpp"

#include "ETW/Microsoft_Windows_D3D9.h"
#include "ETW/Microsoft_Windows_Dwm_Core.h"
#include "ETW/Microsoft_Windows_DXGI.h"
#include "ETW/Microsoft_Windows_DxgKrnl.h"
#include "ETW/Microsoft_Windows_EventMetadata.h"
#include "ETW/Microsoft_Windows_Kernel_Process.h"
#include "ETW/Microsoft_Windows_Win32k.h"

#include <algorithm>
#include <assert.h>
#include <d3d9.h>
#include <dxgi.h>
#include <stdlib.h>
#include <unordered_set>

#ifdef DEBUG
static constexpr int PRESENTEVENT_CIRCULAR_BUFFER_SIZE = 32768;
#else
static constexpr int PRESENTEVENT_CIRCULAR_BUFFER_SIZE = 8192;
#endif

namespace {

// Detect if there are any missing expected events, and returns the number of
// PresentStop events that we should wait for them.
uint32_t GetDeferredCompletionWaitCount(PresentEvent const& p)
{
    // If the present was displayed or discarded before Present_Stop, defer
    // completion for for one Present_Stop.
    if (p.Runtime != Runtime::Other && p.PresentStopTime == 0) {
        return 1;
    }

    // All expected events already observed
    return 0;
}

}

// These macros, when enabled, record what PresentMon analysis below was done
// for each present.  The primary use case is to compute usage statistics and
// ensure test coverage.
//
// Add a TRACK_PRESENT_PATH() calls to every location that represents a unique
// analysis path.  e.g., as a starting point this might be one for every ETW
// event used below, with further instrumentation when there is different
// handling based on event property values.
//
// If the location is in a function that can be called by multiple parents, use
// TRACK_PRESENT_SAVE_PATH_ID() instead and call
// TRACK_PRESENT_GENERATE_PATH_ID() in each parent.
#ifdef TRACK_PRESENT_PATHS
#define TRACK_PRESENT_PATH_SAVE_ID(present, id) present->AnalysisPath |= 1ull << (id % 64)
#define TRACK_PRESENT_PATH(present) do { \
    enum { TRACK_PRESENT_PATH_ID = __COUNTER__ }; \
    TRACK_PRESENT_PATH_SAVE_ID(present, TRACK_PRESENT_PATH_ID); \
} while (0)
#define TRACK_PRESENT_PATH_GENERATE_ID()              mAnalysisPathID = __COUNTER__
#define TRACK_PRESENT_PATH_SAVE_GENERATED_ID(present) TRACK_PRESENT_PATH_SAVE_ID(present, mAnalysisPathID)
#else
#define TRACK_PRESENT_PATH(present)                   (void) present
#define TRACK_PRESENT_PATH_GENERATE_ID()
#define TRACK_PRESENT_PATH_SAVE_GENERATED_ID(present) (void) present
#endif

#if PRESENTMON_ENABLE_DEBUG_TRACE
static uint64_t gNextPresentId = 1;
#endif

PresentEvent::PresentEvent()
    : PresentStartTime(0)
    , ProcessId(0)
    , ThreadId(0)
    , PresentStopTime(0)
    , GPUStartTime(0)
    , ReadyTime(0)
    , GPUDuration(0)
    , GPUVideoDuration(0)
    , ScreenTime(0)
    , InputTime(0)

    , SwapChainAddress(0)
    , SyncInterval(-1)
    , PresentFlags(0)

    , CompositionSurfaceLuid(0)
    , Win32KPresentCount(0)
    , Win32KBindId(0)
    , DxgkPresentHistoryToken(0)
    , DxgkPresentHistoryTokenData(0)
    , DxgkContext(0)
    , Hwnd(0)
    , QueueSubmitSequence(0)
    , mAllPresentsTrackingIndex(UINT32_MAX)

    , DeferredCompletionWaitCount(0)

    , DestWidth(0)
    , DestHeight(0)
    , DriverThreadId(0)

    , Runtime(Runtime::Other)
    , PresentMode(PresentMode::Unknown)
    , FinalState(PresentResult::Unknown)
    , InputType(InputDeviceType::None)

    , SupportsTearing(false)
    , WaitForFlipEvent(false)
    , WaitForMPOFlipEvent(false)
    , SeenDxgkPresent(false)
    , SeenWin32KEvents(false)
    , DwmNotified(false)
    , SeenInFrameEvent(false)
    , GpuFrameCompleted(false)
    , IsCompleted(false)
    , IsLost(false)
    , PresentInDwmWaitingStruct(false)
    #ifdef TRACK_PRESENT_PATHS
    , AnalysisPath(0ull)
    #endif
    #if PRESENTMON_ENABLE_DEBUG_TRACE
    , Id(gNextPresentId++)
    #endif
{
}

PMTraceConsumer::PMTraceConsumer()
    : mAllPresents(PRESENTEVENT_CIRCULAR_BUFFER_SIZE)
    , mGpuTrace(this)
    , mLastInputDeviceReadTime(0)
    , mLastInputDeviceType(InputDeviceType::None)
{
}

void PMTraceConsumer::HandleD3D9Event(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_D3D9::Present_Start::Id:
        if (IsProcessTrackedForFiltering(hdr.ProcessId)) {
            EventDataDesc desc[] = {
                { L"pSwapchain" },
                { L"Flags" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto pSwapchain = desc[0].GetData<uint64_t>();
            auto Flags      = desc[1].GetData<uint32_t>();

            uint32_t dxgiPresentFlags = 0;
            if (Flags & D3DPRESENT_DONOTFLIP)   dxgiPresentFlags |= DXGI_PRESENT_DO_NOT_SEQUENCE;
            if (Flags & D3DPRESENT_DONOTWAIT)   dxgiPresentFlags |= DXGI_PRESENT_DO_NOT_WAIT;
            if (Flags & D3DPRESENT_FLIPRESTART) dxgiPresentFlags |= DXGI_PRESENT_RESTART;

            int32_t syncInterval = -1;
            if (Flags & D3DPRESENT_FORCEIMMEDIATE) {
                syncInterval = 0;
            }

            TRACK_PRESENT_PATH_GENERATE_ID();

            RuntimePresentStart(Runtime::D3D9, hdr, pSwapchain, dxgiPresentFlags, syncInterval);
        }
        break;
    case Microsoft_Windows_D3D9::Present_Stop::Id:
        if (IsProcessTrackedForFiltering(hdr.ProcessId)) {
            RuntimePresentStop(Runtime::D3D9, hdr, mMetadata.GetEventData<uint32_t>(pEventRecord, L"Result"));
        }
        break;
    default:
        assert(!mFilteredEvents); // Assert that filtering is working if expected
        break;
    }
}

void PMTraceConsumer::HandleDXGIEvent(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_DXGI::Present_Start::Id:
    case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start::Id:
        if (IsProcessTrackedForFiltering(hdr.ProcessId)) {
            EventDataDesc desc[] = {
                { L"pIDXGISwapChain" },
                { L"Flags" },
                { L"SyncInterval" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto pSwapChain   = desc[0].GetData<uint64_t>();
            auto Flags        = desc[1].GetData<uint32_t>();
            auto SyncInterval = desc[2].GetData<int32_t>();

            TRACK_PRESENT_PATH_GENERATE_ID();

            RuntimePresentStart(Runtime::DXGI, hdr, pSwapChain, Flags, SyncInterval);
        }
        break;
    case Microsoft_Windows_DXGI::Present_Stop::Id:
    case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id:
        if (IsProcessTrackedForFiltering(hdr.ProcessId)) {
            RuntimePresentStop(Runtime::DXGI, hdr, mMetadata.GetEventData<uint32_t>(pEventRecord, L"Result"));
        }
        break;
    default:
        assert(!mFilteredEvents); // Assert that filtering is working if expected
        break;
    }
}

void PMTraceConsumer::HandleDxgkBlt(EVENT_HEADER const& hdr, uint64_t hwnd, bool redirectedPresent)
{
    // Lookup the in-progress present.  It should not have a known present mode
    // yet, so if it does we assume we looked up a present whose tracking was
    // lost.
    std::shared_ptr<PresentEvent> presentEvent;
    for (;;) {
        presentEvent = FindOrCreatePresent(hdr);
        if (presentEvent == nullptr) {
            return;
        }

        if (presentEvent->PresentMode == PresentMode::Unknown) {
            break;
        }

        RemoveLostPresent(presentEvent);
    }

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(presentEvent);

    // This could be one of several types of presents. Further events will clarify.
    // For now, assume that this is a blt straight into a surface which is already on-screen.
    VerboseTraceBeforeModifyingPresent(presentEvent.get());
    presentEvent->Hwnd = hwnd;
    if (redirectedPresent) {
        TRACK_PRESENT_PATH(presentEvent);
        presentEvent->PresentMode = PresentMode::Composed_Copy_CPU_GDI;
        presentEvent->SupportsTearing = false;
    } else {
        presentEvent->PresentMode = PresentMode::Hardware_Legacy_Copy_To_Front_Buffer;
        presentEvent->SupportsTearing = true;
    }
}

// A flip event is emitted during fullscreen present submission.  The only
// events that we expect before a Flip/FlipMPO are a runtime present start, or
// a previous FlipMPO.  Afterwards, we expect an MMIOFlip packet on the same
// thread used to trace the flip to screen.
void PMTraceConsumer::HandleDxgkFlip(EVENT_HEADER const& hdr, int32_t flipInterval, bool isMMIOFlip, bool isMPOFlip)
{
    // Lookup the in-progress present. It should not have a known
    // QueueSubmitSequence nor seen a DxgkPresent yet, so if it has we assume
    // we looked up a present whose tracking was lost.
    std::shared_ptr<PresentEvent> presentEvent;
    for (;;) {
        presentEvent = FindOrCreatePresent(hdr);
        if (presentEvent == nullptr) {
            return;
        }

        if (presentEvent->QueueSubmitSequence == 0 && !presentEvent->SeenDxgkPresent) {
            break;
        }

        RemoveLostPresent(presentEvent);
    }

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(presentEvent);

    // There may be duplicate flip events for MPO situations, so only handle
    // the first.
    if (presentEvent->PresentMode == PresentMode::Unknown) {
        VerboseTraceBeforeModifyingPresent(presentEvent.get());

        presentEvent->PresentMode = PresentMode::Hardware_Legacy_Flip;

        if (flipInterval != -1) {
            presentEvent->SyncInterval = flipInterval;
        }
        if (isMMIOFlip) {
            presentEvent->WaitForFlipEvent = true;
        }
        if (isMPOFlip) {
            presentEvent->WaitForMPOFlipEvent = true;
        }
        if (!isMMIOFlip && flipInterval == 0) {
            presentEvent->SupportsTearing = true;
        }

        // If this is the DWM thread, piggyback these pending presents on our fullscreen present
        if (hdr.ThreadId == DwmPresentThreadId) {
            DebugAssert(presentEvent->DependentPresents.empty());

            for (auto& p : mPresentsWaitingForDWM) {
                p->PresentInDwmWaitingStruct = false;
            }
            std::swap(presentEvent->DependentPresents, mPresentsWaitingForDWM);
        }
    }
}

void PMTraceConsumer::HandleDxgkQueueSubmit(
    EVENT_HEADER const& hdr,
    uint64_t hContext,
    uint32_t submitSequence,
    uint32_t packetType,
    bool isPresentPacket,
    bool isWin7)
{
    // Track GPU execution
    if (mTrackGPU) {
        bool isWaitPacket = packetType == (uint32_t) Microsoft_Windows_DxgKrnl::QueuePacketType::DXGKETW_WAIT_COMMAND_BUFFER;
        mGpuTrace.EnqueueQueuePacket(hContext, submitSequence, hdr.ProcessId, hdr.TimeStamp.QuadPart, isWaitPacket);
    }

    // For blt presents on Win7, the only way to distinguish between DWM-off
    // fullscreen blts and the DWM-on blt to redirection bitmaps is to track
    // whether a PHT was submitted before submitting anything else to the same
    // context, which indicates it's a redirected blt.  If we get to here
    // instead, the present is a fullscreen blt and considered completed once
    // its work is done.
    if (isWin7) {
        auto eventIter = mPresentByDxgkContext.find(hContext);
        if (eventIter != mPresentByDxgkContext.end()) {
            auto present = eventIter->second;

            TRACK_PRESENT_PATH(present);

            if (present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
                VerboseTraceBeforeModifyingPresent(present.get());
                present->SeenDxgkPresent = true;

                // If the work is already done, complete it now.
                if (present->ScreenTime != 0) {
                    CompletePresent(present);
                }
            }

            // We're done with DxgkContext tracking, if the present hasn't
            // completed remove it from the tracking now.
            if (present->DxgkContext != 0) {
                mPresentByDxgkContext.erase(eventIter);
                present->DxgkContext = 0;
            }
        }
    }

    // This event is emitted after a flip/blt/PHT event, and may be the only
    // way to trace completion of the present.
    if (packetType == (uint32_t) Microsoft_Windows_DxgKrnl::QueuePacketType::DXGKETW_MMIOFLIP_COMMAND_BUFFER ||
        packetType == (uint32_t) Microsoft_Windows_DxgKrnl::QueuePacketType::DXGKETW_SOFTWARE_COMMAND_BUFFER ||
        isPresentPacket) {
        auto present = FindThreadPresent(hdr.ThreadId);
        if (present != nullptr && present->QueueSubmitSequence == 0) {

            TRACK_PRESENT_PATH(present);

            VerboseTraceBeforeModifyingPresent(present.get());
            present->QueueSubmitSequence = submitSequence;

            auto presentsBySubmitSequence = &mPresentBySubmitSequence[submitSequence];
            DebugAssert(presentsBySubmitSequence->find(hContext) == presentsBySubmitSequence->end());
            (*presentsBySubmitSequence)[hContext] = present;

            if (isWin7 && present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
                mPresentByDxgkContext[hContext] = present;
                present->DxgkContext = hContext;
            }
        }
    }
}

void PMTraceConsumer::HandleDxgkQueueComplete(uint64_t timestamp, uint64_t hContext, uint32_t submitSequence)
{
    // Track GPU execution of the packet
    if (mTrackGPU) {
        mGpuTrace.CompleteQueuePacket(hContext, submitSequence, timestamp);
    }

    // If this packet was a present packet being tracked...
    auto ii = mPresentBySubmitSequence.find(submitSequence);
    if (ii != mPresentBySubmitSequence.end()) {
        auto presentsBySubmitSequence = &ii->second;
        auto jj = presentsBySubmitSequence->find(hContext);
        if (jj != presentsBySubmitSequence->end()) {
            auto pEvent = jj->second;

            TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

            // Stop tracking GPU work for this present.
            //
            // Note: there is a potential race here because QueuePacket_Stop
            // occurs sometime after DmaPacket_Info it's possible that some
            // small portion of the next frame's GPU work has started before
            // QueuePacket_Stop and will be attributed to this frame.  However,
            // this is necessarily a small amount of work, and we can't use DMA
            // packets as not all present types create them.
            if (mTrackGPU) {
                mGpuTrace.CompleteFrame(pEvent.get(), timestamp);
            }

            // We use present packet completion as the screen time for
            // Hardware_Legacy_Copy_To_Front_Buffer and Hardware_Legacy_Flip
            // present modes, unless we are expecting a subsequent flip/*sync
            // event from DXGK.
            if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer ||
                (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip && !pEvent->WaitForFlipEvent)) {
                VerboseTraceBeforeModifyingPresent(pEvent.get());

                if (pEvent->ReadyTime == 0) {
                    pEvent->ReadyTime = timestamp;
                }

                pEvent->ScreenTime = timestamp;
                pEvent->FinalState = PresentResult::Presented;

                // Sometimes, the queue packets associated with a present will complete
                // before the DxgKrnl PresentInfo event is fired.  For blit presents in
                // this case, we have no way to differentiate between fullscreen and
                // windowed blits, so we defer the completion of this present until
                // we've also seen the Dxgk Present_Info event.
                if (pEvent->SeenDxgkPresent || pEvent->PresentMode != PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
                    CompletePresent(pEvent);
                }
            }
        }
    }
}

// Lookup the present associated with this submit sequence.  Because some DXGK
// events that reference submit sequence id don't include the queue context,
// it's possible (though rare) that there are multiple presents in flight with
// the same submit sequence id.  If that is the case, we pick the oldest one.
std::shared_ptr<PresentEvent> PMTraceConsumer::FindPresentBySubmitSequence(uint32_t submitSequence)
{
    auto ii = mPresentBySubmitSequence.find(submitSequence);
    if (ii != mPresentBySubmitSequence.end()) {
        auto presentsBySubmitSequence = &ii->second;

        auto jj = presentsBySubmitSequence->begin();
        auto je = presentsBySubmitSequence->end();
        DebugAssert(jj != je);
        if (jj != je) {
            auto present = jj->second;
            for (++jj; jj != je; ++jj) {
                if (present->PresentStartTime > jj->second->PresentStartTime) {
                    present = jj->second;
                }
            }
            return present;
        }
    }

    return std::shared_ptr<PresentEvent>();
}

// An MMIOFlip event is emitted when an MMIOFlip packet is dequeued.  All GPU
// work submitted prior to the flip has been completed.
//
// It also is emitted when an independent flip PHT is dequed, and will tell us
// whether the present is immediate or vsync.
void PMTraceConsumer::HandleDxgkMMIOFlip(uint64_t timestamp, uint32_t submitSequence, uint32_t flags)
{
    auto pEvent = FindPresentBySubmitSequence(submitSequence);
    if (pEvent != nullptr) {
        TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

        VerboseTraceBeforeModifyingPresent(pEvent.get());
        pEvent->ReadyTime = timestamp;

        if (pEvent->PresentMode == PresentMode::Composed_Flip) {
            pEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
        }

        if (flags & (uint32_t) Microsoft_Windows_DxgKrnl::SetVidPnSourceAddressFlags::FlipImmediate) {
            pEvent->FinalState = PresentResult::Presented;
            pEvent->ScreenTime = timestamp;
            pEvent->SupportsTearing = true;
            if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip) {
                CompletePresent(pEvent);
            }
        }
    }
}

// The VSyncDPC/HSyncDPC contains a field telling us what flipped to screen.
// This is the way to track completion of a fullscreen present.
void PMTraceConsumer::HandleDxgkSyncDPC(uint64_t timestamp, uint32_t submitSequence)
{
    auto pEvent = FindPresentBySubmitSequence(submitSequence);
    if (pEvent != nullptr) {

        TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

        VerboseTraceBeforeModifyingPresent(pEvent.get());
        pEvent->ScreenTime = timestamp;
        pEvent->FinalState = PresentResult::Presented;

        // For Hardware_Legacy_Flip, we are done tracking the present.  If we
        // aren't expecting a subsequent *SyncMultiPlaneDPC_Info event, then we
        // complete the present now.
        //
        // If we are expecting a subsequent *SyncMultiPlaneDPC_Info event, then
        // we wait for it to complete the present.  This is because there are
        // rare cases where there may be multiple in-flight presents with the
        // same submit sequence and waiting ensures that both the VSyncDPC and
        // *SyncMultiPlaneDPC events match to the same present.
        if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip && !pEvent->WaitForMPOFlipEvent) {
            CompletePresent(pEvent);
        }
    }
}

// These events are emitted during submission of all types of windowed presents
// while DWM is on and gives us a token we can use to match with the
// Microsoft_Windows_DxgKrnl::PresentHistory_Info event.
void PMTraceConsumer::HandleDxgkPresentHistory(
    EVENT_HEADER const& hdr,
    uint64_t token,
    uint64_t tokenData,
    Microsoft_Windows_DxgKrnl::PresentModel presentModel)
{
    // Lookup the in-progress present.  It should not have a known
    // DxgkPresentHistoryToken yet, so if it does we assume we looked up a
    // present whose tracking was lost.
    std::shared_ptr<PresentEvent> presentEvent;
    for (;;) {
        presentEvent = FindOrCreatePresent(hdr);
        if (presentEvent == nullptr) {
            return;
        }

        if (presentEvent->DxgkPresentHistoryToken == 0) {
            break;
        }

        RemoveLostPresent(presentEvent);
    }

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(presentEvent);

    VerboseTraceBeforeModifyingPresent(presentEvent.get());
    presentEvent->ReadyTime = 0;
    presentEvent->ScreenTime = 0;
    presentEvent->SupportsTearing = false;
    presentEvent->FinalState = PresentResult::Unknown;
    presentEvent->DxgkPresentHistoryToken = token;

    auto iter = mPresentByDxgkPresentHistoryToken.find(token);
    if (iter != mPresentByDxgkPresentHistoryToken.end()) {
        RemoveLostPresent(iter->second);
    }
    DebugAssert(mPresentByDxgkPresentHistoryToken.find(token) == mPresentByDxgkPresentHistoryToken.end());
    mPresentByDxgkPresentHistoryToken[token] = presentEvent;

    switch (presentEvent->PresentMode) {
    case PresentMode::Hardware_Legacy_Copy_To_Front_Buffer:
        DebugAssert(presentModel == Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_UNINITIALIZED ||
                    presentModel == Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_REDIRECTED_BLT);
        presentEvent->PresentMode = PresentMode::Composed_Copy_GPU_GDI;
        break;

    case PresentMode::Unknown:
        if (presentModel == Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_REDIRECTED_COMPOSITION) {
            /* BEGIN WORKAROUND: DirectComposition presents are currently ignored. There
             * were rare situations observed where they would lead to issues during present
             * completion (including stackoverflow caused by recursive completion).  This
             * could not be diagnosed, so we removed support for this present mode for now...
            presentEvent->PresentMode = PresentMode::Composed_Composition_Atlas;
            */
            RemoveLostPresent(presentEvent);
            return;
            /* END WORKAROUND */
        } else {
            // When there's no Win32K events, we'll assume PHTs that aren't after a blt, and aren't composition tokens
            // are flip tokens and that they're displayed. There are no Win32K events on Win7, and they might not be
            // present in some traces - don't let presents get stuck/dropped just because we can't track them perfectly.
            DebugAssert(!presentEvent->SeenWin32KEvents);
            presentEvent->PresentMode = PresentMode::Composed_Flip;
        }
        break;

    case PresentMode::Composed_Copy_CPU_GDI:
        if (tokenData == 0) {
            // This is the best we can do, we won't be able to tell how many frames are actually displayed.
            mPresentsWaitingForDWM.emplace_back(presentEvent);
            presentEvent->PresentInDwmWaitingStruct = true;
        } else {
            DebugAssert(mPresentByDxgkPresentHistoryTokenData.find(tokenData) == mPresentByDxgkPresentHistoryTokenData.end());
            mPresentByDxgkPresentHistoryTokenData[tokenData] = presentEvent;
            presentEvent->DxgkPresentHistoryTokenData = tokenData;
        }
        break;
    }

    // If we are not tracking further GPU/display-related events, complete the
    // present here.
    if (!mTrackDisplay) {
        CompletePresent(presentEvent);
    }
}

// This event is emitted when a token is being handed off to DWM, and is a good
// way to indicate a ready state.
void PMTraceConsumer::HandleDxgkPresentHistoryInfo(EVENT_HEADER const& hdr, uint64_t token)
{
    auto eventIter = mPresentByDxgkPresentHistoryToken.find(token);
    if (eventIter == mPresentByDxgkPresentHistoryToken.end()) {
        return;
    }

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(eventIter->second);

    VerboseTraceBeforeModifyingPresent(eventIter->second.get());
    eventIter->second->ReadyTime = eventIter->second->ReadyTime == 0
        ? hdr.TimeStamp.QuadPart
        : std::min(eventIter->second->ReadyTime, (uint64_t) hdr.TimeStamp.QuadPart);

    // Neither Composed Composition Atlas or Win7 Flip has DWM events indicating intent
    // to present this frame.
    //
    // Composed presents are currently ignored.
    if (//eventIter->second->PresentMode == PresentMode::Composed_Composition_Atlas ||
        (eventIter->second->PresentMode == PresentMode::Composed_Flip && !eventIter->second->SeenWin32KEvents)) {
        mPresentsWaitingForDWM.emplace_back(eventIter->second);
        eventIter->second->PresentInDwmWaitingStruct = true;
        eventIter->second->DwmNotified = true;
    }

    if (eventIter->second->PresentMode == PresentMode::Composed_Copy_GPU_GDI) {
        // This present will be handed off to DWM.  When DWM is ready to
        // present, we'll query for the most recent blt targeting this window
        // and take it out of the map.
        mLastPresentByWindow[eventIter->second->Hwnd] = eventIter->second;
    }

    mPresentByDxgkPresentHistoryToken.erase(eventIter);
}

void PMTraceConsumer::HandleDXGKEvent(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_DxgKrnl::Flip_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"FlipInterval" },
            { L"MMIOFlip" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto FlipInterval = desc[0].GetData<uint32_t>();
        auto MMIOFlip     = desc[1].GetData<BOOL>() != 0;

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkFlip(hdr, FlipInterval, MMIOFlip, false);
        return;
    }
    case Microsoft_Windows_DxgKrnl::IndependentFlip_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"SubmitSequence" },
            { L"FlipInterval" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto SubmitSequence = desc[0].GetData<uint32_t>();
        auto FlipInterval   = desc[1].GetData<uint32_t>();

        auto pEvent = FindPresentBySubmitSequence(SubmitSequence);
        if (pEvent != nullptr) {
            // We should not have already identified as hardware_composed - this
            // can only be detected around Vsync/HsyncDPC time.
            DebugAssert(pEvent->PresentMode != PresentMode::Hardware_Composed_Independent_Flip);

            VerboseTraceBeforeModifyingPresent(pEvent.get());
            pEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
            pEvent->SyncInterval = FlipInterval;
        }
        return;
    }
    case Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info::Id:
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkFlip(hdr, -1, true, true);
        return;
    // QueuPacket_Start are used for render queue packets
    // QueuPacket_Start_2 are used for monitor wait packets
    // QueuPacket_Start_3 are used for monitor signal packets
    case Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id:
    {
        EventDataDesc desc[] = {
            { L"PacketType" },
            { L"SubmitSequence" },
            { L"hContext" },
            { L"bPresent" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto PacketType     = desc[0].GetData<uint32_t>();
        auto SubmitSequence = desc[1].GetData<uint32_t>();
        auto hContext       = desc[2].GetData<uint64_t>();
        auto bPresent       = desc[3].GetData<BOOL>() != 0;

        HandleDxgkQueueSubmit(hdr, hContext, SubmitSequence, PacketType, bPresent, false);
        return;
    }
    case Microsoft_Windows_DxgKrnl::QueuePacket_Start_2::Id:
    {
        EventDataDesc desc[] = {
            { L"hContext" },
            { L"SubmitSequence" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto hContext       = desc[0].GetData<uint64_t>();
        auto SubmitSequence = desc[1].GetData<uint32_t>();

        uint32_t PacketType = (uint32_t) Microsoft_Windows_DxgKrnl::QueuePacketType::DXGKETW_WAIT_COMMAND_BUFFER;
        bool bPresent = false;
        HandleDxgkQueueSubmit(hdr, hContext, SubmitSequence, PacketType, bPresent, false);
        return;
    }
    case Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id:
    {
        EventDataDesc desc[] = {
            { L"SubmitSequence" },
            { L"hContext" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto SubmitSequence = desc[0].GetData<uint32_t>();
        auto hContext       = desc[1].GetData<uint64_t>();

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkQueueComplete(hdr.TimeStamp.QuadPart, hContext, SubmitSequence);
        return;
    }
    case Microsoft_Windows_DxgKrnl::MMIOFlip_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"FlipSubmitSequence" },
            { L"Flags" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto FlipSubmitSequence = desc[0].GetData<uint32_t>();
        auto Flags              = desc[1].GetData<uint32_t>();

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkMMIOFlip(hdr.TimeStamp.QuadPart, FlipSubmitSequence, Flags);
        return;
    }
    case Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info::Id:
    {
        auto flipEntryStatusAfterFlipValid = hdr.EventDescriptor.Version >= 2;
        EventDataDesc desc[] = {
            { L"FlipSubmitSequence" },
            { L"FlipEntryStatusAfterFlip" }, // optional
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc) - (flipEntryStatusAfterFlipValid ? 0 : 1));
        auto FlipFenceId = desc[0].GetData<uint64_t>();

        auto submitSequence = (uint32_t) (FlipFenceId >> 32u);
        auto present = FindPresentBySubmitSequence(submitSequence);
        if (present != nullptr) {

            TRACK_PRESENT_PATH(present);

            // Complete the GPU tracking for this frame.
            //
            // For some present modes (e.g., Hardware_Legacy_Flip) this may be
            // the first event telling us the present is ready.
            mGpuTrace.CompleteFrame(present.get(), hdr.TimeStamp.QuadPart);

            // Check and handle the post-flip status if available.
            if (flipEntryStatusAfterFlipValid) {
                auto FlipEntryStatusAfterFlip = desc[1].GetData<uint32_t>();

                // Nothing to do for FlipWaitVSync other than wait for the VSync events.
                if (FlipEntryStatusAfterFlip != (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitVSync) {

                    TRACK_PRESENT_PATH(present);

                    // Any of the non-vsync status present modes can tear.
                    VerboseTraceBeforeModifyingPresent(present.get());
                    present->SupportsTearing = true;

                    // For FlipWaitVSync and FlipWaitHSync, we'll wait for the
                    // corresponding ?SyncDPC event.  Otherwise we consider
                    // this the present screen time.
                    if (FlipEntryStatusAfterFlip != (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitHSync) {

                        TRACK_PRESENT_PATH(present);

                        present->FinalState = PresentResult::Presented;

                        if (FlipEntryStatusAfterFlip == (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitComplete) {
                            present->ScreenTime = hdr.TimeStamp.QuadPart;
                        }

                        if (present->PresentMode == PresentMode::Hardware_Legacy_Flip) {
                            CompletePresent(present);
                        }
                    }
                }
            }
        }
        return;
    }

    // VSyncDPC_Info only includes the FlipSubmitSequence for one layer.
    //
    // *SyncDPCMultiPlane_Info is sent afterward for non-legacy flip paths, and
    // contains info on whether this vsync/hsync contains an overlay.  So, we
    // avoid updating ScreenTime and FinalState with the second event, but
    // update isMultiPlane with the correct information when we have them.
    //
    // On Windows >= 10.17134 HSyncDPCMultiPlane_Info is used when the
    // associated display is connected to integrated graphics.
    case Microsoft_Windows_DxgKrnl::VSyncDPC_Info::Id:
    {
        TRACK_PRESENT_PATH_GENERATE_ID();

        auto FlipFenceId = mMetadata.GetEventData<uint64_t>(pEventRecord, L"FlipFenceId");
        if (FlipFenceId != 0) {
            HandleDxgkSyncDPC(hdr.TimeStamp.QuadPart, (uint32_t)(FlipFenceId >> 32u));
        }
        return;
    }
    case Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id:
    case Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id:
    {
        TRACK_PRESENT_PATH_GENERATE_ID();

        EventDataDesc desc[] = {
            { L"PlaneCount" },
            { L"ScannedPhysicalAddress" },
            { L"FlipEntryCount" },
            { L"FlipSubmitSequence" },
        };

        // Name changed from "ScannedPhysicalAddress" to "PresentIdOrPhysicalAddress"
        if (hdr.EventDescriptor.Id == Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id &&
            hdr.EventDescriptor.Version >= 1) {
            desc[1].name_ = L"PresentIdOrPhysicalAddress";
        }

        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto PlaneCount         = desc[0].GetData<uint32_t>();
        auto PlaneAddress       = desc[1].GetArray<uint64_t>(PlaneCount);
        auto FlipEntryCount     = desc[2].GetData<uint32_t>();
        auto FlipSubmitSequence = desc[3].GetArray<uint64_t>(FlipEntryCount);

        if (FlipEntryCount > 0) {
            // The number of active planes is determined by the number of
            // non-zero addresses.  All we care about is if there are more
            // than one or not.
            bool isMultiPlane = false;
            for (uint32_t i = 0, activePlaneCount = 0; i < PlaneCount; ++i) {
                if (PlaneAddress[i] != 0) {
                    if (activePlaneCount == 1) {
                        isMultiPlane = true;
                        break;
                    }
                    activePlaneCount += 1;
                }
            }

            for (uint32_t i = 0; i < FlipEntryCount; ++i) {
                if (FlipSubmitSequence[i] > 0) {
                    auto submitSequence = (uint32_t)(FlipSubmitSequence[i] >> 32u);
                    auto pEvent = FindPresentBySubmitSequence(submitSequence);
                    if (pEvent != nullptr) {

                        TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

                        if (isMultiPlane &&
                            (pEvent->PresentMode == PresentMode::Hardware_Independent_Flip || pEvent->PresentMode == PresentMode::Composed_Flip)) {
                            VerboseTraceBeforeModifyingPresent(pEvent.get());
                            pEvent->PresentMode = PresentMode::Hardware_Composed_Independent_Flip;
                        }

                        // ScreenTime may have already been written by a preceeding
                        // VSyncDPC_Info event, which is more accurate, so don't
                        // overwrite it in that case.
                        if (pEvent->FinalState != PresentResult::Presented) {
                            VerboseTraceBeforeModifyingPresent(pEvent.get());
                            pEvent->ScreenTime = hdr.TimeStamp.QuadPart;
                            pEvent->FinalState = PresentResult::Presented;
                        }

                        CompletePresent(pEvent);
                    }
                }
            }
        }
        return;
    }
    case Microsoft_Windows_DxgKrnl::Present_Info::Id:
    {
        // This event is emitted at the end of the kernel present.
        auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
        if (eventIter != mPresentByThreadId.end()) {
            auto present = eventIter->second;

            TRACK_PRESENT_PATH(present);

            // Store the fact we've seen this present.  This is used to improve
            // tracking and to defer blt present completion until both Present_Info
            // and present QueuePacket_Stop have been seen.
            VerboseTraceBeforeModifyingPresent(present.get());
            present->SeenDxgkPresent = true;

            if (present->Hwnd == 0) {
                present->Hwnd = mMetadata.GetEventData<uint64_t>(pEventRecord, L"hWindow");
            }

            // If we are not expecting an API present end event, then this
            // should be the last operation on this thread.  This can happen
            // due to batched presents or non-instrumented present APIs (i.e.,
            // not DXGI nor D3D9).
            if (present->Runtime == Runtime::Other ||
                present->ThreadId != hdr.ThreadId) {
                mPresentByThreadId.erase(eventIter);
            }

            // If this is a deferred blit that's already seen QueuePacket_Stop,
            // then complete it now.
            if (present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer &&
                present->ScreenTime != 0) {
                CompletePresent(present);
            }
        }
        return;
    }
    case Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id:
    case Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id:
    {
        EventDataDesc desc[] = {
            { L"Token" },
            { L"Model" },
            { L"TokenData" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto Token     = desc[0].GetData<uint64_t>();
        auto Model     = desc[1].GetData<Microsoft_Windows_DxgKrnl::PresentModel>();
        auto TokenData = desc[2].GetData<uint64_t>();

        if (Model != Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_REDIRECTED_GDI) {
            TRACK_PRESENT_PATH_GENERATE_ID();
            HandleDxgkPresentHistory(hdr, Token, TokenData, Model);
        }
        return;
    }
    case Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id:
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkPresentHistoryInfo(hdr, mMetadata.GetEventData<uint64_t>(pEventRecord, L"Token"));
        return;
    case Microsoft_Windows_DxgKrnl::Blit_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"hwnd" },
            { L"bRedirectedPresent" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto hwnd               = desc[0].GetData<uint64_t>();
        auto bRedirectedPresent = desc[1].GetData<uint32_t>() != 0;

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkBlt(hdr, hwnd, bRedirectedPresent);
        return;
    }

    // BlitCancel_Info indicates that DxgKrnl optimized a present blt away, and
    // no further work was needed.
    case Microsoft_Windows_DxgKrnl::BlitCancel_Info::Id:
    {
        auto present = FindThreadPresent(hdr.ThreadId);
        if (present != nullptr) {
            TRACK_PRESENT_PATH(present);

            VerboseTraceBeforeModifyingPresent(present.get());
            present->FinalState = PresentResult::Discarded;

            CompletePresent(present);
        }
        return;
    }
    }

    if (mTrackGPU) {
        switch (hdr.EventDescriptor.Id) {

        // We need a mapping from hContext to GPU node.
        //
        // There's two ways I've tried to get this. One is to use
        // Microsoft_Windows_DxgKrnl::SelectContext2_Info events which include
        // all the required info (hContext, pDxgAdapter, and NodeOrdinal) but
        // that event fires often leading to significant overhead.
        //
        // The current implementaiton requires a CAPTURE_STATE on start up to
        // get all existing context/device events but after that the event
        // overhead should be minimal.
        case Microsoft_Windows_DxgKrnl::Device_DCStart::Id:
        case Microsoft_Windows_DxgKrnl::Device_Start::Id:
        {
            EventDataDesc desc[] = {
                { L"pDxgAdapter" },
                { L"hDevice" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto pDxgAdapter = desc[0].GetData<uint64_t>();
            auto hDevice     = desc[1].GetData<uint64_t>();

            mGpuTrace.RegisterDevice(hDevice, pDxgAdapter);
            return;
        }
        // Sometimes a trace will miss a Device_Start, so we also check
        // AdapterAllocation events (which also provide the pDxgAdapter-hDevice
        // mapping).  These are not currently enabled for realtime collection.
        case Microsoft_Windows_DxgKrnl::AdapterAllocation_Start::Id:
        case Microsoft_Windows_DxgKrnl::AdapterAllocation_DCStart::Id:
        case Microsoft_Windows_DxgKrnl::AdapterAllocation_Stop::Id:
        {
            EventDataDesc desc[] = {
                { L"pDxgAdapter" },
                { L"hDevice" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto pDxgAdapter = desc[0].GetData<uint64_t>();
            auto hDevice     = desc[1].GetData<uint64_t>();

            if (hDevice != 0) {
                mGpuTrace.RegisterDevice(hDevice, pDxgAdapter);
            }
            return;
        }
        case Microsoft_Windows_DxgKrnl::Device_Stop::Id:
        {
            auto hDevice = mMetadata.GetEventData<uint64_t>(pEventRecord, L"hDevice");

            mGpuTrace.UnregisterDevice(hDevice);
            return;
        }
        case Microsoft_Windows_DxgKrnl::Context_DCStart::Id:
        case Microsoft_Windows_DxgKrnl::Context_Start::Id:
        {
            EventDataDesc desc[] = {
                { L"hContext" },
                { L"hDevice" },
                { L"NodeOrdinal" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto hContext    = desc[0].GetData<uint64_t>();
            auto hDevice     = desc[1].GetData<uint64_t>();
            auto NodeOrdinal = desc[2].GetData<uint32_t>();

            // If this is a DCStart, then it was generated by xperf instead of
            // the context's process.
            uint32_t processId = hdr.EventDescriptor.Id == Microsoft_Windows_DxgKrnl::Context_DCStart::Id
                ? 0
                : hdr.ProcessId;

            mGpuTrace.RegisterContext(hContext, hDevice, NodeOrdinal, processId);
            return;
        }
        case Microsoft_Windows_DxgKrnl::Context_Stop::Id:
            mGpuTrace.UnregisterContext(mMetadata.GetEventData<uint64_t>(pEventRecord, L"hContext"));
            return;

        case Microsoft_Windows_DxgKrnl::HwQueue_DCStart::Id:
        case Microsoft_Windows_DxgKrnl::HwQueue_Start::Id:
        {
            EventDataDesc desc[] = {
                { L"hContext" },
                { L"ParentDxgHwQueue" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto hContext        = desc[0].GetData<uint64_t>();
            auto hHwQueueContext = desc[1].GetData<uint64_t>();

            mGpuTrace.RegisterHwQueueContext(hContext, hHwQueueContext);
            return;
        }

        case Microsoft_Windows_DxgKrnl::NodeMetadata_Info::Id:
        {
            EventDataDesc desc[] = {
                { L"pDxgAdapter" },
                { L"NodeOrdinal" },
                { L"EngineType" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto pDxgAdapter = desc[0].GetData<uint64_t>();
            auto NodeOrdinal = desc[1].GetData<uint32_t>();
            auto EngineType  = desc[2].GetData<Microsoft_Windows_DxgKrnl::DXGK_ENGINE>();

            mGpuTrace.SetEngineType(pDxgAdapter, NodeOrdinal, EngineType);
            return;
        }

        // DmaPacket_Start occurs when a packet is enqueued onto a node.
        // 
        // There are certain DMA packets that don't result in GPU work.
        // Examples are preemption packets or notifications for
        // VIDSCH_QUANTUM_EXPIRED.  These will have a sequence id of zero (also
        // DmaBuffer will be null).
        case Microsoft_Windows_DxgKrnl::DmaPacket_Start::Id:
        {
            EventDataDesc desc[] = {
                { L"hContext" },
                { L"ulQueueSubmitSequence" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto hContext   = desc[0].GetData<uint64_t>();
            auto SequenceId = desc[1].GetData<uint32_t>();

            if (SequenceId != 0) {
                mGpuTrace.EnqueueDmaPacket(hContext, SequenceId, hdr.TimeStamp.QuadPart);
            }
            return;
        }

        // DmaPacket_Info occurs on packet-related interrupts.  We could use
        // DmaPacket_Stop here, but the DMA_COMPLETED interrupt is a tighter
        // bound.
        case Microsoft_Windows_DxgKrnl::DmaPacket_Info::Id:
        {
            EventDataDesc desc[] = {
                { L"hContext" },
                { L"ulQueueSubmitSequence" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto hContext   = desc[0].GetData<uint64_t>();
            auto SequenceId = desc[1].GetData<uint32_t>();

            if (SequenceId != 0) {
                mGpuTrace.CompleteDmaPacket(hContext, SequenceId, hdr.TimeStamp.QuadPart);
            }
            return;
        }
        }
    }

    assert(!mFilteredEvents); // Assert that filtering is working if expected
}

void PMTraceConsumer::HandleWin7DxgkBlt(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    TRACK_PRESENT_PATH_GENERATE_ID();

    auto pBltEvent = reinterpret_cast<DXGKETW_BLTEVENT*>(pEventRecord->UserData);
    HandleDxgkBlt(
        pEventRecord->EventHeader,
        pBltEvent->hwnd,
        pBltEvent->bRedirectedPresent != 0);
}

void PMTraceConsumer::HandleWin7DxgkFlip(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    TRACK_PRESENT_PATH_GENERATE_ID();

    auto pFlipEvent = reinterpret_cast<DXGKETW_FLIPEVENT*>(pEventRecord->UserData);
    HandleDxgkFlip(
        pEventRecord->EventHeader,
        pFlipEvent->FlipInterval,
        pFlipEvent->MMIOFlip != 0,
        false);
}

void PMTraceConsumer::HandleWin7DxgkPresentHistory(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    auto pPresentHistoryEvent = reinterpret_cast<DXGKETW_PRESENTHISTORYEVENT*>(pEventRecord->UserData);
    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START) {
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkPresentHistory(
            pEventRecord->EventHeader,
            pPresentHistoryEvent->Token,
            0,
            Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_UNINITIALIZED);
    } else if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO) {
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkPresentHistoryInfo(pEventRecord->EventHeader, pPresentHistoryEvent->Token);
    }
}

void PMTraceConsumer::HandleWin7DxgkQueuePacket(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START) {
        auto pSubmitEvent = reinterpret_cast<DXGKETW_QUEUESUBMITEVENT*>(pEventRecord->UserData);
        HandleDxgkQueueSubmit(
            pEventRecord->EventHeader,
            pSubmitEvent->hContext,
            pSubmitEvent->SubmitSequence,
            pSubmitEvent->PacketType,
            pSubmitEvent->bPresent != 0,
            true);
    } else if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_STOP) {
        auto pCompleteEvent = reinterpret_cast<DXGKETW_QUEUECOMPLETEEVENT*>(pEventRecord->UserData);
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkQueueComplete(
            pEventRecord->EventHeader.TimeStamp.QuadPart,
            pCompleteEvent->hContext,
            pCompleteEvent->SubmitSequence);
    }
}

void PMTraceConsumer::HandleWin7DxgkVSyncDPC(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    TRACK_PRESENT_PATH_GENERATE_ID();

    auto pVSyncDPCEvent = reinterpret_cast<DXGKETW_SCHEDULER_VSYNC_DPC*>(pEventRecord->UserData);

    // Windows 7 does not support MultiPlaneOverlay.
    HandleDxgkSyncDPC(pEventRecord->EventHeader.TimeStamp.QuadPart, (uint32_t)(pVSyncDPCEvent->FlipFenceId.QuadPart >> 32u));
}

void PMTraceConsumer::HandleWin7DxgkMMIOFlip(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    TRACK_PRESENT_PATH_GENERATE_ID();

    if (pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) {
        auto pMMIOFlipEvent = reinterpret_cast<DXGKETW_SCHEDULER_MMIO_FLIP_32*>(pEventRecord->UserData);
        HandleDxgkMMIOFlip(
            pEventRecord->EventHeader.TimeStamp.QuadPart,
            pMMIOFlipEvent->FlipSubmitSequence,
            pMMIOFlipEvent->Flags);
    } else {
        auto pMMIOFlipEvent = reinterpret_cast<DXGKETW_SCHEDULER_MMIO_FLIP_64*>(pEventRecord->UserData);
        HandleDxgkMMIOFlip(
            pEventRecord->EventHeader.TimeStamp.QuadPart,
            pMMIOFlipEvent->FlipSubmitSequence,
            pMMIOFlipEvent->Flags);
    }
}

std::size_t PMTraceConsumer::Win32KPresentHistoryTokenHash::operator()(PMTraceConsumer::Win32KPresentHistoryToken const& v) const noexcept
{
    auto CompositionSurfaceLuid = std::get<0>(v);
    auto PresentCount           = std::get<1>(v);
    auto BindId                 = std::get<2>(v);
    PresentCount = (PresentCount << 32) | (PresentCount >> (64-32));
    BindId       = (BindId       << 56) | (BindId       >> (64-56));
    auto h64 = CompositionSurfaceLuid ^ PresentCount ^ BindId;
    return std::hash<uint64_t>::operator()(h64);
}

void PMTraceConsumer::HandleWin32kEvent(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"CompositionSurfaceLuid" },
            { L"PresentCount" },
            { L"BindId" },
            { L"DestWidth" },  // version >= 1
            { L"DestHeight" }, // version >= 1
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc) - (hdr.EventDescriptor.Version == 0 ? 2 : 0));
        auto CompositionSurfaceLuid = desc[0].GetData<uint64_t>();
        auto PresentCount           = desc[1].GetData<uint64_t>();
        auto BindId                 = desc[2].GetData<uint64_t>();

        // Lookup the in-progress present.  It should not have seen any Win32K
        // events yet, so if it has we assume we looked up a present whose
        // tracking was lost.
        std::shared_ptr<PresentEvent> present;
        for (;;) {
            present = FindOrCreatePresent(hdr);
            if (present == nullptr) {
                return;
            }

            if (!present->SeenWin32KEvents) {
                break;
            }

            RemoveLostPresent(present);
        }

        TRACK_PRESENT_PATH(present);

        present->PresentMode = PresentMode::Composed_Flip;
        present->SeenWin32KEvents = true;

        if (hdr.EventDescriptor.Version >= 1) {
            present->DestWidth  = desc[3].GetData<uint32_t>();
            present->DestHeight = desc[4].GetData<uint32_t>();
        }

        Win32KPresentHistoryToken key(CompositionSurfaceLuid, PresentCount, BindId);
        DebugAssert(mPresentByWin32KPresentHistoryToken.find(key) == mPresentByWin32KPresentHistoryToken.end());
        mPresentByWin32KPresentHistoryToken[key] = present;
        present->CompositionSurfaceLuid = CompositionSurfaceLuid;
        present->Win32KPresentCount = PresentCount;
        present->Win32KBindId = BindId;
        break;
    }

    case Microsoft_Windows_Win32k::TokenStateChanged_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"CompositionSurfaceLuid" },
            { L"PresentCount" },
            { L"BindId" },
            { L"NewState" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto CompositionSurfaceLuid = desc[0].GetData<uint64_t>();
        auto PresentCount           = desc[1].GetData<uint32_t>();
        auto BindId                 = desc[2].GetData<uint64_t>();
        auto NewState               = desc[3].GetData<uint32_t>();

        Win32KPresentHistoryToken key(CompositionSurfaceLuid, PresentCount, BindId);
        auto eventIter = mPresentByWin32KPresentHistoryToken.find(key);
        if (eventIter == mPresentByWin32KPresentHistoryToken.end()) {
            return;
        }
        auto presentEvent = eventIter->second;

        switch (NewState) {
        case (uint32_t) Microsoft_Windows_Win32k::TokenState::InFrame: // Composition is starting
        {
            TRACK_PRESENT_PATH(presentEvent);

            VerboseTraceBeforeModifyingPresent(presentEvent.get());
            presentEvent->SeenInFrameEvent = true;

            bool iFlip = mMetadata.GetEventData<BOOL>(pEventRecord, L"IndependentFlip") != 0;
            if (iFlip && presentEvent->PresentMode == PresentMode::Composed_Flip) {
                presentEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
            }

            // We won't necessarily see a transition to Discarded for all
            // presents so we check here instead: if we're compositing a newer
            // present than the window's last known present, then the last
            // known one will be discarded.
            if (presentEvent->Hwnd) {
                auto hWndIter = mLastPresentByWindow.find(presentEvent->Hwnd);
                if (hWndIter == mLastPresentByWindow.end()) {
                    mLastPresentByWindow.emplace(presentEvent->Hwnd, presentEvent);
                } else if (hWndIter->second != presentEvent) {
                    auto prevPresent = hWndIter->second;
                    hWndIter->second = presentEvent;

                    // Even though we know it will be discarded at this point,
                    // we keep tracking it through the composition steps
                    // instead of completing it now, to ensure that the
                    // collector will get presents from each swap chain in
                    // order.
                    //
                    // That said, we do need to remove it from the submit
                    // sequence id tracking to reduce the likelyhood of id
                    // collisions during lookup for events that don't reference
                    // a context.
                    VerboseTraceBeforeModifyingPresent(prevPresent.get());
                    prevPresent->FinalState = PresentResult::Discarded;
                    RemovePresentFromSubmitSequenceIdTracking(prevPresent);
                }
            }
            break;
        }

        case (uint32_t) Microsoft_Windows_Win32k::TokenState::Confirmed: // Present has been submitted
            TRACK_PRESENT_PATH(presentEvent);

            // Handle DO_NOT_SEQUENCE presents, which may get marked as confirmed,
            // if a frame was composed when this token was completed
            if (presentEvent->FinalState == PresentResult::Unknown &&
                (presentEvent->PresentFlags & DXGI_PRESENT_DO_NOT_SEQUENCE) != 0) {
                VerboseTraceBeforeModifyingPresent(presentEvent.get());
                presentEvent->FinalState = PresentResult::Discarded;
                RemovePresentFromSubmitSequenceIdTracking(presentEvent);
            }
            if (presentEvent->Hwnd) {
                mLastPresentByWindow.erase(presentEvent->Hwnd);
            }
            break;

        // Note: Going forward, TokenState::Retired events are no longer
        // guaranteed to be sent at the end of a frame in multi-monitor
        // scenarios.  Instead, we use DWM's present stats to understand the
        // Composed Flip timeline.
        case (uint32_t) Microsoft_Windows_Win32k::TokenState::Discarded: // Present has been discarded
        {
            TRACK_PRESENT_PATH(presentEvent);

            // Nullptr as we don't want to report clearing the key in the verbose trace
            VerboseTraceBeforeModifyingPresent(nullptr);
            presentEvent->CompositionSurfaceLuid = 0;
            presentEvent->Win32KPresentCount = 0;
            presentEvent->Win32KBindId = 0;
            mPresentByWin32KPresentHistoryToken.erase(eventIter);

            if (!presentEvent->SeenInFrameEvent && (presentEvent->FinalState == PresentResult::Unknown || presentEvent->ScreenTime == 0)) {
                VerboseTraceBeforeModifyingPresent(presentEvent.get());
                presentEvent->FinalState = PresentResult::Discarded;
                CompletePresent(presentEvent);
            } else if (presentEvent->PresentMode != PresentMode::Composed_Flip) {
                CompletePresent(presentEvent);
            }
            break;
        }
        }
        break;
    }

    case Microsoft_Windows_Win32k::InputDeviceRead_Stop::Id:
    {
        EventDataDesc desc[] = {
            { L"DeviceType" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto DeviceType = desc[0].GetData<uint32_t>();

        switch (DeviceType) {
        case 0: mLastInputDeviceType = InputDeviceType::Mouse; break;
        case 1: mLastInputDeviceType = InputDeviceType::Keyboard; break;
        default: mLastInputDeviceType = InputDeviceType::Unknown; break;
        }

        mLastInputDeviceReadTime = hdr.TimeStamp.QuadPart;
        break;
    }

    case Microsoft_Windows_Win32k::RetrieveInputMessage_Info::Id:
    {
        auto ii = mRetrievedInput.find(hdr.ProcessId);
        if (ii == mRetrievedInput.end()) {
            mRetrievedInput.emplace(hdr.ProcessId, std::make_pair(
                mLastInputDeviceReadTime,
                mLastInputDeviceType));
        } else {
            if (ii->second.first < mLastInputDeviceReadTime) {
                ii->second.first = mLastInputDeviceReadTime;
                ii->second.second = mLastInputDeviceType;
            }
        }
        break;
    }

    default:
        assert(!mFilteredEvents); // Assert that filtering is working if expected
        break;
    }
}

void PMTraceConsumer::HandleDWMEvent(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id:
        // Move all the latest in-progress Composed_Copy from each window into
        // mPresentsWaitingForDWM, to be attached to the next DWM present's
        // DependentPresents.
        for (auto& hWndPair : mLastPresentByWindow) {
            auto& present = hWndPair.second;
            if (present->PresentMode == PresentMode::Composed_Copy_GPU_GDI ||
                present->PresentMode == PresentMode::Composed_Copy_CPU_GDI) {
                TRACK_PRESENT_PATH(present);
                VerboseTraceBeforeModifyingPresent(present.get());
                present->DwmNotified = true;
                mPresentsWaitingForDWM.emplace_back(present);
                present->PresentInDwmWaitingStruct = true;
            }
        }
        mLastPresentByWindow.clear();
        break;

    case Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start::Id:
        DwmProcessId = hdr.ProcessId;
        DwmPresentThreadId = hdr.ThreadId;
        break;

    // These events are only used for Composed_Copy_CPU_GDI presents.  They are
    // used to identify when such presents are handed off to DWM. 
    case Microsoft_Windows_Dwm_Core::FlipChain_Pending::Id:
    case Microsoft_Windows_Dwm_Core::FlipChain_Complete::Id:
    case Microsoft_Windows_Dwm_Core::FlipChain_Dirty::Id:
    {
        if (InlineIsEqualGUID(hdr.ProviderId, Microsoft_Windows_Dwm_Core::Win7::GUID)) {
            break;
        }

        // ulFlipChain and ulSerialNumber are expected to be uint32_t data, but
        // on Windows 8.1 the event properties are specified as uint64_t.
        auto GetU32FromU32OrU64 = [](EventDataDesc const& desc) {
            if (desc.size_ == 4) {
                return desc.GetData<uint32_t>();
            } else {
                auto u64 = desc.GetData<uint64_t>();
                DebugAssert(u64 <= UINT32_MAX);
                return (uint32_t) u64;
            }
        };

        EventDataDesc desc[] = {
            { L"ulFlipChain" },
            { L"ulSerialNumber" },
            { L"hwnd" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto ulFlipChain    = GetU32FromU32OrU64(desc[0]);
        auto ulSerialNumber = GetU32FromU32OrU64(desc[1]);
        auto hwnd           = desc[2].GetData<uint64_t>();

        // Lookup the present using the 64-bit token data from the PHT
        // submission, which is actually two 32-bit data chunks corresponding
        // to a flip chain id and present id.
        auto tokenData = ((uint64_t) ulFlipChain << 32ull) | ulSerialNumber;
        auto flipIter = mPresentByDxgkPresentHistoryTokenData.find(tokenData);
        if (flipIter != mPresentByDxgkPresentHistoryTokenData.end()) {
            auto present = flipIter->second;

            TRACK_PRESENT_PATH(present);

            VerboseTraceBeforeModifyingPresent(present.get());
            present->DxgkPresentHistoryTokenData = 0;
            present->DwmNotified = true;

            mLastPresentByWindow[hwnd] = present;

            mPresentByDxgkPresentHistoryTokenData.erase(flipIter);
        }
        break;
    }
    case Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info::Id:
    {
        // On Windows 8.1 PresentCount is named
        // OutOfFrameDirectFlipPresentCount, so we look up both allowing one to
        // be optional and then check which one we found.
        EventDataDesc desc[] = {
            { L"luidSurface" },
            { L"PresentCount" },
            { L"OutOfFrameDirectFlipPresentCount" },
            { L"bindId" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc), 1);
        auto luidSurface  = desc[0].GetData<uint64_t>();
        auto PresentCount = (desc[1].status_ & PROP_STATUS_FOUND) ? desc[1].GetData<uint64_t>()
                                                                  : desc[2].GetData<uint64_t>();
        auto bindId       = desc[3].GetData<uint64_t>();

        Win32KPresentHistoryToken key(luidSurface, PresentCount, bindId);
        auto eventIter = mPresentByWin32KPresentHistoryToken.find(key);
        if (eventIter != mPresentByWin32KPresentHistoryToken.end() && eventIter->second->SeenInFrameEvent) {
            TRACK_PRESENT_PATH(eventIter->second);
            VerboseTraceBeforeModifyingPresent(eventIter->second.get());
            eventIter->second->DwmNotified = true;
            mPresentsWaitingForDWM.emplace_back(eventIter->second);
            eventIter->second->PresentInDwmWaitingStruct = true;
        }
        break;
    }
    default:
        assert(!mFilteredEvents || // Assert that filtering is working if expected
               hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID);
        break;
    }
}

void PMTraceConsumer::RemovePresentFromSubmitSequenceIdTracking(std::shared_ptr<PresentEvent> const& present)
{
    if (present->QueueSubmitSequence == 0) {
        return;
    }

    auto ii = mPresentBySubmitSequence.find(present->QueueSubmitSequence);
    if (ii == mPresentBySubmitSequence.end()) {
        return;
    }
    auto presentsBySubmitSequence = &ii->second;

    // Do a linear search for present here.  We could do a find() but that
    // would require storing the queue context in PresentEvent and since
    // presentsBySubmitSequence is expected to be small (typically one element)
    // this should be faster.
    if (presentsBySubmitSequence->size() == 1) {
        DebugAssert(presentsBySubmitSequence->begin()->second == present);
        mPresentBySubmitSequence.erase(ii);
    } else {
        for (auto jj = presentsBySubmitSequence->begin(), je = presentsBySubmitSequence->end(); jj != je; ++jj) {
            if (jj->second == present) {
                presentsBySubmitSequence->erase(jj);
                break;
            }
        }
    }

    // Don't report clearing of key in verbose trace
    VerboseTraceBeforeModifyingPresent(nullptr);
    present->QueueSubmitSequence = 0;
}

// Remove the present from all temporary tracking structures.
void PMTraceConsumer::RemovePresentFromTemporaryTrackingCollections(std::shared_ptr<PresentEvent> const& p)
{
    // mAllPresents
    if (p->mAllPresentsTrackingIndex != UINT32_MAX) {
        mAllPresents[p->mAllPresentsTrackingIndex] = nullptr;
        p->mAllPresentsTrackingIndex = UINT32_MAX;
    }

    // mPresentByThreadId
    //
    // If the present was batched, it will by referenced in mPresentByThreadId
    // by both ThreadId and DriverThreadId.
    //
    // We don't reset ThreadId nor DriverThreadId as both are useful outside of
    // tracking.
    auto threadEventIter = mPresentByThreadId.find(p->ThreadId);
    if (threadEventIter != mPresentByThreadId.end() && threadEventIter->second == p) {
        mPresentByThreadId.erase(threadEventIter);
    }
    if (p->DriverThreadId != 0) {
        threadEventIter = mPresentByThreadId.find(p->DriverThreadId);
        if (threadEventIter != mPresentByThreadId.end() && threadEventIter->second == p) {
            mPresentByThreadId.erase(threadEventIter);
        }
    }

    // mOrderedPresentsByProcessId
    mOrderedPresentsByProcessId[p->ProcessId].erase(p->PresentStartTime);

    // mPresentBySubmitSequence
    RemovePresentFromSubmitSequenceIdTracking(p);

    // mPresentByWin32KPresentHistoryToken
    if (p->CompositionSurfaceLuid != 0) {
        Win32KPresentHistoryToken key(
            p->CompositionSurfaceLuid,
            p->Win32KPresentCount,
            p->Win32KBindId
        );

        auto eventIter = mPresentByWin32KPresentHistoryToken.find(key);
        if (eventIter != mPresentByWin32KPresentHistoryToken.end() && (eventIter->second == p)) {
            mPresentByWin32KPresentHistoryToken.erase(eventIter);
        }

        p->CompositionSurfaceLuid = 0;
        p->Win32KPresentCount = 0;
        p->Win32KBindId = 0;
    }

    // mPresentByDxgkPresentHistoryToken
    if (p->DxgkPresentHistoryToken != 0) {
        auto eventIter = mPresentByDxgkPresentHistoryToken.find(p->DxgkPresentHistoryToken);
        if (eventIter != mPresentByDxgkPresentHistoryToken.end() && eventIter->second == p) {
            mPresentByDxgkPresentHistoryToken.erase(eventIter);
        }
        p->DxgkPresentHistoryToken = 0;
    }

    // mPresentByDxgkPresentHistoryTokenData
    if (p->DxgkPresentHistoryTokenData != 0) {
        auto eventIter = mPresentByDxgkPresentHistoryTokenData.find(p->DxgkPresentHistoryTokenData);
        if (eventIter != mPresentByDxgkPresentHistoryTokenData.end() && eventIter->second == p) {
            mPresentByDxgkPresentHistoryTokenData.erase(eventIter);
        }
        p->DxgkPresentHistoryTokenData = 0;
    }

    // mPresentByDxgkContext
    if (p->DxgkContext != 0) {
        auto eventIter = mPresentByDxgkContext.find(p->DxgkContext);
        if (eventIter != mPresentByDxgkContext.end() && eventIter->second == p) {
            mPresentByDxgkContext.erase(eventIter);
        }
        p->DxgkContext = 0;
    }

    // mLastPresentByWindow
    if (p->Hwnd != 0) {
        auto eventIter = mLastPresentByWindow.find(p->Hwnd);
        if (eventIter != mLastPresentByWindow.end() && eventIter->second == p) {
            mLastPresentByWindow.erase(eventIter);
        }
        p->Hwnd = 0;
    }

    // mPresentsWaitingForDWM
    if (p->PresentInDwmWaitingStruct) {
        for (auto presentIter = mPresentsWaitingForDWM.begin(); presentIter != mPresentsWaitingForDWM.end(); presentIter++) {
            if (p == *presentIter) {
                mPresentsWaitingForDWM.erase(presentIter);
                break;
            }
        }
        p->PresentInDwmWaitingStruct = false;
    }
}

void PMTraceConsumer::RemoveLostPresent(std::shared_ptr<PresentEvent> p)
{
    VerboseTraceBeforeModifyingPresent(p.get());
    p->IsLost = true;
    CompletePresent(p);
}

void PMTraceConsumer::CompletePresentHelper(std::shared_ptr<PresentEvent> const& p)
{
    // First, protect against double-completion.  Double-completion is not
    // intended, but there have been cases observed where it happens (in some
    // cases leading to infinite recursion with a DWM present and a dependent
    // present completing eachother).
    //
    // The exact pattern causing this has not yet been identified, so is the
    // best fix for now.
    DebugAssert(p->IsCompleted == false);
    if (p->IsCompleted) return;

    // Complete the present.
    VerboseTraceBeforeModifyingPresent(p.get());
    p->IsCompleted = true;
    mDeferredCompletions[p->ProcessId][p->SwapChainAddress].mOrderedPresents.emplace(p->PresentStartTime, p);

    // If the present is still missing some expected events, defer it's
    // enqueuing for some number of presents for cases where the event may
    // arrive after present completion.
    //
    // These cases must have special code to patch complete-but-deferred
    // presents, which are no longer in the standard tracking structures.
    if (!p->IsLost) {
        p->DeferredCompletionWaitCount = GetDeferredCompletionWaitCount(*p);
    }

    // Stop any subsequent changes to p from appearing in the verbose trace
    VerboseTraceBeforeModifyingPresent(nullptr);

    // Remove the present from any tracking structures.
    RemovePresentFromTemporaryTrackingCollections(p);

    // If this is a DWM present, complete any other present that contributed to
    // it.  A DWM present only completes each HWND's most-recent Composed_Flip
    // PresentEvent, so we mark any others as discarded.
    //
    // PresentEvents that become lost are not removed from DependentPresents
    // tracking, so we need to protect against lost events (but they have
    // already been added to mLostPresentEvents etc.).
    if (!p->DependentPresents.empty()) {
        std::unordered_set<uint64_t> completedComposedFlipHwnds;
        for (auto ii = p->DependentPresents.rbegin(), ie = p->DependentPresents.rend(); ii != ie; ++ii) {
            auto p2 = *ii;
            if (!p2->IsCompleted) {
                if (p2->PresentMode == PresentMode::Composed_Flip && !completedComposedFlipHwnds.emplace(p2->Hwnd).second) {
                    VerboseTraceBeforeModifyingPresent(p2.get());
                    p2->FinalState = PresentResult::Discarded;
                } else if (p2->FinalState != PresentResult::Discarded) {
                    VerboseTraceBeforeModifyingPresent(p2.get());
                    p2->FinalState = p->FinalState;
                    p2->ScreenTime = p->ScreenTime;
                }

                if (p->IsLost) {
                    VerboseTraceBeforeModifyingPresent(p2.get());
                    p2->IsLost = true;
                }
            }
        }
        for (auto p2 : p->DependentPresents) {
            if (!p2->IsCompleted) {
                CompletePresentHelper(p2);
            }
        }
        p->DependentPresents.clear();
    }

    // If presented, remove any earlier presents made on the same swap chain.
    if (p->FinalState == PresentResult::Presented) {
        auto presentsByThisProcess = &mOrderedPresentsByProcessId[p->ProcessId];
        for (auto ii = presentsByThisProcess->begin(), ie = presentsByThisProcess->end(); ii != ie; ) {
            auto p2 = ii->second;
            ++ii; // increment iterator first as CompletePresentHelper() will remove it
            if (p2->SwapChainAddress == p->SwapChainAddress) {
                if (p2->PresentStartTime >= p->PresentStartTime) break;
                CompletePresentHelper(p2);
            }
        }
    }
}

void PMTraceConsumer::CompletePresent(std::shared_ptr<PresentEvent> const& p)
{
    // We use the first completed present to indicate that all necessary
    // providers are running and able to successfully track/complete presents.
    //
    // At the first completion, there may be numerous presents that have been
    // created but not properly tracked due to missed events.  This is
    // especially prevalent in ETLs that start runtime providers before backend
    // providers and/or start capturing while an intensive graphics application
    // is already running.  When that happens, PresentStartTime/PresentStopTime and
    // ReadyTime/ScreenTime times can become mis-matched, and that offset can
    // persist for the full capture.
    //
    // We handle this by throwing away all queued presents up to this point.
    if (!mHasCompletedAPresent && !p->IsLost) {
        mHasCompletedAPresent = true;

        for (auto const& pr : mOrderedPresentsByProcessId) {
            auto processPresents = &pr.second;
            for (auto ii = processPresents->begin(), ie = processPresents->end(); ii != ie; ) {
                auto p2 = ii->second;
                ++ii; // Increment before calling CompletePresentHelper(), which removes from processPresents

                // Clear DependentPresents as an optimization to avoid the extra
                // recursion in CompletePresentHelper(), since we know that we're
                // completing all the presents anyway.
                p2->DependentPresents.clear();

                VerboseTraceBeforeModifyingPresent(p2.get());
                p2->IsLost = true;
                CompletePresentHelper(p2);
            }
        }
    }

    // Complete the present and any of its dependencies.
    else {
        CompletePresentHelper(p);
    }

    // In order, move any completed presents into the consumer thread queue.
    for (auto& pr1 : mDeferredCompletions) {
        for (auto& pr2 : pr1.second) {
            EnqueueDeferredCompletions(&pr2.second);
        }
    }
}

void PMTraceConsumer::EnqueueDeferredPresent(std::shared_ptr<PresentEvent> const& p)
{
    auto i1 = mDeferredCompletions.find(p->ProcessId);
    if (i1 != mDeferredCompletions.end()) {
        auto i2 = i1->second.find(p->SwapChainAddress);
        if (i2 != i1->second.end()) {
            EnqueueDeferredCompletions(&i2->second);
        }
    }
}

void PMTraceConsumer::EnqueueDeferredCompletions(DeferredCompletions* deferredCompletions)
{
    size_t completedCount = 0;
    size_t lostCount = 0;

    auto iterBegin = deferredCompletions->mOrderedPresents.begin();
    auto iterEnqueueEnd = iterBegin;
    for (auto iterEnd = deferredCompletions->mOrderedPresents.end(); iterEnqueueEnd != iterEnd; ++iterEnqueueEnd) {
        auto present = iterEnqueueEnd->second;
        if (present->DeferredCompletionWaitCount > 0) {
            break;
        }

        // Assert the present is complete and has been removed from all
        // internal tracking structures.
        DebugAssert(present->IsCompleted);
        DebugAssert(present->CompositionSurfaceLuid == 0);
        DebugAssert(present->Win32KPresentCount == 0);
        DebugAssert(present->Win32KBindId == 0);
        DebugAssert(present->DxgkPresentHistoryToken == 0);
        DebugAssert(present->DxgkPresentHistoryTokenData == 0);
        DebugAssert(present->DxgkContext == 0);
        DebugAssert(present->Hwnd == 0);
        DebugAssert(present->mAllPresentsTrackingIndex == UINT32_MAX);
        DebugAssert(present->QueueSubmitSequence == 0);
        DebugAssert(present->PresentInDwmWaitingStruct == false);

        // If later presents have already be enqueued for the user, mark this
        // present as lost.
        if (deferredCompletions->mLastEnqueuedQpcTime > present->PresentStartTime) {
            VerboseTraceBeforeModifyingPresent(present.get());
            present->IsLost = true;
        }

        if (present->IsLost) {
            lostCount += 1;
        } else {
            deferredCompletions->mLastEnqueuedQpcTime = present->PresentStartTime;
            completedCount += 1;
        }
    }

    if (lostCount + completedCount > 0) {
        if (completedCount > 0) {
            std::lock_guard<std::mutex> lock(mPresentEventMutex);
            mCompletePresentEvents.reserve(mCompletePresentEvents.size() + completedCount);
            for (auto iter = iterBegin; iter != iterEnqueueEnd; ++iter) {
                if (!iter->second->IsLost) {
                    mCompletePresentEvents.emplace_back(iter->second);
                }
            }
        }

        if (lostCount > 0) {
            std::lock_guard<std::mutex> lock(mLostPresentEventMutex);
            mLostPresentEvents.reserve(mLostPresentEvents.size() + lostCount);
            for (auto iter = iterBegin; iter != iterEnqueueEnd; ++iter) {
                if (iter->second->IsLost) {
                    mLostPresentEvents.emplace_back(iter->second);
                }
            }
        }

        deferredCompletions->mOrderedPresents.erase(iterBegin, iterEnqueueEnd);
    }
}

void PMTraceConsumer::SetThreadPresent(uint32_t threadId, std::shared_ptr<PresentEvent> const& present)
{
    // If there is an in-flight present on this thread already, then something
    // has gone wrong with it's tracking so consider it lost.
    auto ii = mPresentByThreadId.find(threadId);
    if (ii != mPresentByThreadId.end()) {
        RemoveLostPresent(ii->second);
    }

    mPresentByThreadId.emplace(threadId, present);
}

std::shared_ptr<PresentEvent> PMTraceConsumer::FindThreadPresent(uint32_t threadId)
{
    auto ii = mPresentByThreadId.find(threadId);
    return ii == mPresentByThreadId.end() ? std::shared_ptr<PresentEvent>() : ii->second;
}

std::shared_ptr<PresentEvent> PMTraceConsumer::FindOrCreatePresent(EVENT_HEADER const& hdr)
{
    // First, we check if there is an in-progress present that was last
    // operated on from this same thread.
    auto present = FindThreadPresent(hdr.ThreadId);
    if (present != nullptr) {
        return present;
    }

    // Next, we look for the oldest present from the same process that may have
    // been deferred by the driver and submitted on a different thread.  Such
    // presents should have only seen present start/stop events so should not
    // have a known PresentMode, etc. yet.
    auto presentsByThisProcess = &mOrderedPresentsByProcessId[hdr.ProcessId];
    for (auto const& pr : *presentsByThisProcess) {
        present = pr.second;
        if (present->DriverThreadId == 0 &&
            present->SeenDxgkPresent == false &&
            present->SeenWin32KEvents == false &&
            present->PresentMode == PresentMode::Unknown) {
            VerboseTraceBeforeModifyingPresent(present.get());
            present->DriverThreadId = hdr.ThreadId;

            // Set this present as the one the driver thread is working on.  We
            // leave it assigned to the one the application thread is working
            // on as well, in case we haven't yet seen application events such
            // as Present::Stop.
            SetThreadPresent(hdr.ThreadId, present);

            return present;
        }
    }

    // If we couldn't find an in-progress present on the same thread/process,
    // then we create a new one and start tracking it from here (unless the
    // user is filtering this process out).
    //
    // This can happen if there was a lost event, or if the present didn't
    // originate from a runtime whose events we're tracking (i.e., DXGI or
    // D3D9) in which case a DxgKrnl event will be the first present-related
    // event we ever see.
    if (IsProcessTrackedForFiltering(hdr.ProcessId)) {
        present = std::make_shared<PresentEvent>();

        VerboseTraceBeforeModifyingPresent(present.get());
        present->PresentStartTime = *(uint64_t*) &hdr.TimeStamp;
        present->ProcessId = hdr.ProcessId;
        present->ThreadId = hdr.ThreadId;

        TrackPresent(present, presentsByThisProcess);

        return present;
    }

    return nullptr;
}

void PMTraceConsumer::TrackPresent(
    std::shared_ptr<PresentEvent> present,
    OrderedPresents* presentsByThisProcess)
{
    // If there is an existing present that hasn't completed by the time the
    // circular buffer has come around, consider it lost.
    if (mAllPresents[mAllPresentsNextIndex] != nullptr) {
        RemoveLostPresent(mAllPresents[mAllPresentsNextIndex]);
    }

    // Add the present into the initial tracking data structures
    VerboseTraceBeforeModifyingPresent(present.get());
    present->mAllPresentsTrackingIndex = mAllPresentsNextIndex;
    mAllPresents[mAllPresentsNextIndex] = present;
    mAllPresentsNextIndex = (mAllPresentsNextIndex + 1) % PRESENTEVENT_CIRCULAR_BUFFER_SIZE;

    presentsByThisProcess->emplace(present->PresentStartTime, present);

    SetThreadPresent(present->ThreadId, present);

    // Assign any pending retrieved input to this frame
    if (mTrackInput) {
        auto ii = mRetrievedInput.find(present->ProcessId);
        if (ii != mRetrievedInput.end() && ii->second.second != InputDeviceType::None) {
            present->InputTime = ii->second.first;
            present->InputType = ii->second.second;
            ii->second.second = InputDeviceType::None;
        }
    }
}

void PMTraceConsumer::RuntimePresentStart(Runtime runtime, EVENT_HEADER const& hdr, uint64_t swapchainAddr,
                                          uint32_t dxgiPresentFlags, int32_t syncInterval)
{
    // Ignore PRESENT_TEST as it doesn't present, it's used to check if you're
    // fullscreen.
    if (dxgiPresentFlags & DXGI_PRESENT_TEST) {
        return;
    }

    auto present = std::make_shared<PresentEvent>();

    VerboseTraceBeforeModifyingPresent(present.get());
    present->PresentStartTime = *(uint64_t*) &hdr.TimeStamp;
    present->ProcessId = hdr.ProcessId;
    present->ThreadId = hdr.ThreadId;
    present->Runtime = runtime;
    present->SwapChainAddress = swapchainAddr;
    present->PresentFlags = dxgiPresentFlags;
    present->SyncInterval = syncInterval;

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(present);

    TrackPresent(present, &mOrderedPresentsByProcessId[present->ProcessId]);
}

// No TRACK_PRESENT instrumentation here because each runtime Present::Start
// event is instrumented and we assume we'll see the corresponding Stop event
// for any completed present.
void PMTraceConsumer::RuntimePresentStop(Runtime runtime, EVENT_HEADER const& hdr, uint32_t result)
{
    // If the Present() call failed, we lookup the present most-recently
    // operated on by the same thread and, if found, throw it away (i.e., it
    // won't be treated as either a completed nor lost present).
    if (FAILED(result)) {
        auto present = FindThreadPresent(hdr.ThreadId);
        if (present != nullptr) {
            // Check expected state (a new Present() that has only been started).
            DebugAssert(present->PresentStopTime             == 0);
            DebugAssert(present->IsCompleted                 == false);
            DebugAssert(present->IsLost                      == false);
            DebugAssert(present->DeferredCompletionWaitCount == 0);
            DebugAssert(present->DependentPresents.empty());

            // Remove the present from any tracking structures.
            RemovePresentFromTemporaryTrackingCollections(present);
        }
        return;
    }

    // If there are any deferred presents for this process, decrement their
    // DeferredCompletionWaitCount and enqueue any that reach
    // DeferredCompletionWaitCount==0.
    //
    // One of the deferred cases is when a present is displayed/dropped before
    // Present() returns, so if any deferred presents have a missing
    // Present_Stop we use this Present_Stop for the oldest one.
    {
        bool presentStopUsed = false;

        auto iter = mDeferredCompletions.find(hdr.ProcessId);
        if (iter != mDeferredCompletions.end()) {
            for (auto& pr1 : iter->second) {
                auto deferredCompletions = &pr1.second;

                bool enqueuePresents = false;
                bool isOldest = true;
                for (auto const& pr2 : deferredCompletions->mOrderedPresents) {
                    auto present = pr2.second;
                    if (present->DeferredCompletionWaitCount > 0 && present->ThreadId == hdr.ThreadId && present->Runtime == runtime) {

                        VerboseTraceBeforeModifyingPresent(present.get());
                        present->DeferredCompletionWaitCount -= 1;

                        if (!presentStopUsed && present->PresentStopTime == 0) {
                            present->PresentStopTime = *(uint64_t*) &hdr.TimeStamp;
                            if (GetDeferredCompletionWaitCount(*present) == 0) {
                                present->DeferredCompletionWaitCount = 0;
                            }
                            presentStopUsed = true;
                        }

                        if (present->DeferredCompletionWaitCount == 0 && isOldest) {
                            enqueuePresents = true;
                        }
                    }
                    isOldest = false;
                }

                if (enqueuePresents) {
                    EnqueueDeferredCompletions(deferredCompletions); 
                }
            }
        }
        if (presentStopUsed) {
            return;
        }
    }

    // Next, lookup the PresentEvent most-recently operated on by the same
    // thread.  If there isn't one, ignore this event.
    auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (eventIter != mPresentByThreadId.end()) {
        auto present = eventIter->second;

        VerboseTraceBeforeModifyingPresent(present.get());
        present->Runtime = runtime;
        present->PresentStopTime = *(uint64_t*) &hdr.TimeStamp;

        bool visible = false;
        switch (runtime) {
        case Runtime::DXGI:
            if (result != DXGI_STATUS_OCCLUDED &&
                result != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS &&
                result != DXGI_STATUS_NO_DESKTOP_ACCESS) {
                visible = true;
            }
            break;
        case Runtime::D3D9:
            if (result != S_PRESENT_OCCLUDED) {
                visible = true;
            }
            break;
        }

        if (!visible) {
            present->FinalState = PresentResult::Discarded;
            CompletePresent(present);
            return;
        }

        if (!mTrackDisplay) {
            present->FinalState = PresentResult::Presented;
            CompletePresent(present);
            return;
        }

        // We now remove this present from mPresentByThreadId because any future
        // event related to it (e.g., from DXGK/Win32K/etc.) is not expected to
        // come from this thread.
        mPresentByThreadId.erase(eventIter);
    }
}

void PMTraceConsumer::HandleProcessEvent(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;

    ProcessEvent event;
    event.QpcTime = hdr.TimeStamp.QuadPart;

    if (hdr.ProviderId == Microsoft_Windows_Kernel_Process::GUID) {
        switch (hdr.EventDescriptor.Id) {
        case Microsoft_Windows_Kernel_Process::ProcessStart_Start::Id: {
            EventDataDesc desc[] = {
                { L"ProcessID" },
                { L"ImageName" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            event.ProcessId     = desc[0].GetData<uint32_t>();
            auto ImageName      = desc[1].GetData<std::wstring>();
            event.IsStartEvent  = true;

            // When run as-administrator, ImageName will be a fully-qualified path.
            // e.g.: \Device\HarddiskVolume...\...\Proces.exe.  We prune off everything other than
            // the filename here to be consistent.
            size_t start = ImageName.find_last_of('\\') + 1;
            event.ImageFileName = ImageName.c_str() + start;
            break;
        }
        case Microsoft_Windows_Kernel_Process::ProcessStop_Stop::Id: {
            EventDataDesc desc[] = {
                { L"ProcessID" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            event.ProcessId    = desc[0].GetData<uint32_t>();
            event.IsStartEvent = false;
            break;
        }
        default:
            assert(!mFilteredEvents); // Assert that filtering is working if expected
            return;
        }
    } else { // hdr.ProviderId == NT_Process::GUID
        if (hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START ||
            hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_DC_START) {
            EventDataDesc desc[] = {
                { L"ProcessId" },
                { L"ImageFileName" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            event.ProcessId     = desc[0].GetData<uint32_t>();
            std::string str     = desc[1].GetData<std::string>();
            event.ImageFileName = std::wstring(str.begin(), str.end());
            event.IsStartEvent  = true;
        } else if (hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_END||
                   hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_DC_END) {
            EventDataDesc desc[] = {
                { L"ProcessId" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            event.ProcessId    = desc[0].GetData<uint32_t>();
            event.IsStartEvent = false;
        } else {
            return;
        }
    }

    std::lock_guard<std::mutex> lock(mProcessEventMutex);
    mProcessEvents.emplace_back(event);
}

void PMTraceConsumer::HandleMetadataEvent(EVENT_RECORD* pEventRecord)
{
    mMetadata.AddMetadata(pEventRecord);
}

void PMTraceConsumer::AddTrackedProcessForFiltering(uint32_t processID)
{
    std::unique_lock<std::shared_mutex> lock(mTrackedProcessFilterMutex);
    mTrackedProcessFilter.insert(processID);
}

void PMTraceConsumer::RemoveTrackedProcessForFiltering(uint32_t processID)
{
    std::unique_lock<std::shared_mutex> lock(mTrackedProcessFilterMutex);
    auto iterator = mTrackedProcessFilter.find(processID);
    if (iterator != mTrackedProcessFilter.end()) {
        mTrackedProcessFilter.erase(iterator);
    }

    // Completion events will remove any currently tracked events for this process
    // from data structures, so we don't need to proactively remove them now.
}

bool PMTraceConsumer::IsProcessTrackedForFiltering(uint32_t processID)
{
    if (!mFilteredProcessIds || processID == DwmProcessId) {
        return true;
    }

    std::shared_lock<std::shared_mutex> lock(mTrackedProcessFilterMutex);
    auto iterator = mTrackedProcessFilter.find(processID);
    return (iterator != mTrackedProcessFilter.end());
}

#ifdef TRACK_PRESENT_PATHS
static_assert(__COUNTER__ <= 64, "Too many TRACK_PRESENT ids to store in PresentEvent::AnalysisPath");
#endif
