// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#include "PresentMonTraceConsumer.hpp"

#include "ETW/Intel_PresentMon.h"
#include "ETW/Microsoft_Windows_D3D9.h"
#include "ETW/Microsoft_Windows_Dwm_Core.h"
#include "ETW/Microsoft_Windows_Dwm_Core_Win7.h"
#include "ETW/Microsoft_Windows_DXGI.h"
#include "ETW/Microsoft_Windows_DxgKrnl.h"
#include "ETW/Microsoft_Windows_DxgKrnl_Win7.h"
#include "ETW/Microsoft_Windows_EventMetadata.h"
#include "ETW/Microsoft_Windows_Kernel_Process.h"
#include "ETW/Microsoft_Windows_Win32k.h"
#include "ETW/Nvidia_PCL.h"

#include <algorithm>
#include <assert.h>
#include <d3d9.h>
#include <dxgi.h>
#include <stdlib.h>
#include <unordered_set>

static uint32_t gNextFrameId = 1;

static inline uint64_t GenerateVidPnLayerId(uint32_t vidPnSourceId, uint32_t layerIndex)
{
    return (((uint64_t) vidPnSourceId) << 32) | (uint64_t) layerIndex;
}

static inline FrameType ConvertPMPFrameTypeToFrameType(Intel_PresentMon::FrameType frameType)
{
    switch (frameType) {
    case Intel_PresentMon::FrameType::Unspecified: return FrameType::Unspecified;
    case Intel_PresentMon::FrameType::Original:    return FrameType::Application;
    case Intel_PresentMon::FrameType::Repeated:    return FrameType::Repeated;
    case Intel_PresentMon::FrameType::Intel_XEFG:  return FrameType::Intel_XEFG;
    case Intel_PresentMon::FrameType::AMD_AFMF:    return FrameType::AMD_AFMF;
    }

    DebugAssert(false);
    return FrameType::Unspecified;
}

// Returns true if a ScreenTime has been set for this present.
static inline bool HasScreenTime(std::shared_ptr<PresentEvent> const& p)
{
    for (auto const& pr : p->Displayed) {
        if (pr.second != 0) {
            return true;
        }
    }
    return false;
}

// Set a ScreenTime for this present.
//
// If a ScreenTime has not yet been set, add it to Displayed and set Presented.
//
// Otherwise, if the set ScreenTime has no FrameType, overwrite both ScreenTime and FrameType.
//
// Otherwise, if the set ScreenTime is zero, overwrite ScreenTime.
//
// Otherwise, if the call has a FrameType, add it to the list.
static inline void SetScreenTime(std::shared_ptr<PresentEvent> const& p, uint64_t screenTime, FrameType frameType = FrameType::NotSet)
{
    DebugAssert(screenTime != 0);

    auto displayedCount = p->Displayed.size();
    if (displayedCount == 0) {
        p->Displayed.emplace_back(frameType, screenTime);
        p->FinalState = PresentResult::Presented;
    } else if (p->Displayed.back().first == FrameType::NotSet) {
        p->Displayed.back().first = frameType;
        p->Displayed.back().second = screenTime;
    } else if (p->Displayed.back().second == 0) {
        DebugAssert(frameType == FrameType::NotSet);
        for (size_t i = 0; i < displayedCount; ++i) {
            if (p->Displayed[i].second == 0) {
                p->Displayed.back().second = screenTime;
                p->FinalState = PresentResult::Presented;
                break;
            }
        }
    } else if (frameType != FrameType::NotSet) {
        p->Displayed.emplace_back(frameType, screenTime);
    }
}

PresentEvent::PresentEvent()
    : PresentStartTime(0)
    , ProcessId(0)
    , ThreadId(0)
    , TimeInPresent(0)
    , GPUStartTime(0)
    , ReadyTime(0)
    , GPUDuration(0)
    , GPUVideoDuration(0)
    , InputTime(0)
    , MouseClickTime(0)

    , AppPropagatedPresentStartTime(0)
    , AppPropagatedTimeInPresent(0)
    , AppPropagatedGPUStartTime(0)
    , AppPropagatedReadyTime(0)
    , AppPropagatedGPUDuration(0)
    , AppPropagatedGPUVideoDuration(0)

    , AppFrameId(0)
    , AppSleepStartTime(0)
    , AppSleepEndTime(0)
    , AppSimStartTime(0)
    , AppSimEndTime(0)
    , AppRenderSubmitStartTime(0)
    , AppRenderSubmitEndTime(0)
    , AppPresentStartTime(0)
    , AppPresentEndTime(0)
    , AppInputSample{ 0, InputDeviceType::None }

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
    , RingIndex(UINT32_MAX)

    , DestWidth(0)
    , DestHeight(0)
    , DriverThreadId(0)

    , FrameId(gNextFrameId++)

    , Runtime(Runtime::Other)
    , PresentMode(PresentMode::Unknown)
    , FinalState(PresentResult::Unknown)
    , InputType(InputDeviceType::None)

    , SupportsTearing(false)
    , WaitForFlipEvent(false)
    , WaitForMPOFlipEvent(false)
    , SeenDxgkPresent(false)
    , SeenWin32KEvents(false)
    , SeenInFrameEvent(false)
    , GpuFrameCompleted(false)
    , IsCompleted(false)
    , IsLost(false)
    , PresentFailed(false)
    , IsHybridPresent(false)
    , PresentInDwmWaitingStruct(false)

    , WaitingForPresentStop(false)
    , WaitingForFlipFrameType(false)
    , DoneWaitingForFlipFrameType(false)
    , WaitingForFrameId(false)
{
}

PresentEvent::PresentEvent(uint32_t fid)
    : PresentStartTime(0)
    , ProcessId(0)
    , ThreadId(0)
    , TimeInPresent(0)
    , GPUStartTime(0)
    , ReadyTime(0)
    , GPUDuration(0)
    , GPUVideoDuration(0)
    , InputTime(0)
    , MouseClickTime(0)

    , AppPropagatedPresentStartTime(0)
    , AppPropagatedTimeInPresent(0)
    , AppPropagatedGPUStartTime(0)
    , AppPropagatedReadyTime(0)
    , AppPropagatedGPUDuration(0)
    , AppPropagatedGPUVideoDuration(0)

    , AppFrameId(0)
    , AppSleepStartTime(0)
    , AppSleepEndTime(0)
    , AppSimStartTime(0)
    , AppSimEndTime(0)
    , AppRenderSubmitStartTime(0)
    , AppRenderSubmitEndTime(0)
    , AppPresentStartTime(0)
    , AppPresentEndTime(0)
    , AppInputSample{ 0, InputDeviceType::None }

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
    , RingIndex(UINT32_MAX)

    , DestWidth(0)
    , DestHeight(0)
    , DriverThreadId(0)

    , FrameId(fid)

    , Runtime(Runtime::Other)
    , PresentMode(PresentMode::Unknown)
    , FinalState(PresentResult::Unknown)
    , InputType(InputDeviceType::None)

    , SupportsTearing(false)
    , WaitForFlipEvent(false)
    , WaitForMPOFlipEvent(false)
    , SeenDxgkPresent(false)
    , SeenWin32KEvents(false)
    , SeenInFrameEvent(false)
    , GpuFrameCompleted(false)
    , IsCompleted(false)
    , IsLost(false)
    , PresentFailed(false)
    , IsHybridPresent(false)
    , PresentInDwmWaitingStruct(false)

    , WaitingForPresentStop(false)
    , WaitingForFlipFrameType(false)
    , DoneWaitingForFlipFrameType(false)
    , WaitingForFrameId(false)
{
}

PMTraceConsumer::PMTraceConsumer()
    : mTrackedPresents(PRESENTEVENT_CIRCULAR_BUFFER_SIZE)
    , mCompletedPresents(PRESENTEVENT_CIRCULAR_BUFFER_SIZE)
    , mCircularBufferSize(PRESENTEVENT_CIRCULAR_BUFFER_SIZE)
    , mGpuTrace(this)
{
    hEventsReadyEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
}

PMTraceConsumer::PMTraceConsumer(uint32_t circularBufferSize)
    : mTrackedPresents(circularBufferSize)
    , mCompletedPresents(circularBufferSize)
    , mCircularBufferSize(circularBufferSize)
    , mGpuTrace(this)
{
    hEventsReadyEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
}

PMTraceConsumer::~PMTraceConsumer()
{
    if (hEventsReadyEvent && hEventsReadyEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(hEventsReadyEvent);
    }
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

            RuntimePresentStart(Runtime::DXGI, hdr, pSwapChain, Flags, SyncInterval);
        }
        break;
    case Microsoft_Windows_DXGI::Present_Stop::Id:
    case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id:
        if (IsProcessTrackedForFiltering(hdr.ProcessId)) {
            RuntimePresentStop(Runtime::DXGI, hdr, mMetadata.GetEventData<uint32_t>(pEventRecord, L"Result"));
        }
        break;
    case Microsoft_Windows_DXGI::SwapChain_Start::Id:
    case Microsoft_Windows_DXGI::ResizeBuffers_Start::Id:
        if (mTrackHybridPresent) {
            if (IsProcessTrackedForFiltering(hdr.ProcessId)) {
                EventDataDesc desc[] = {
                    { L"pIDXGISwapChain" },
                    { L"HybridPresentMode" },
                };
                // Check to see if the event has both the pIDXGISwapChain and HybridPresentMode
                // fields. If not do not process.
                uint32_t descCount = _countof(desc);
                mMetadata.GetEventData(pEventRecord, desc, &descCount);
                if (descCount == _countof(desc)) {
                    auto pSwapChain = desc[0].GetData<uint64_t>();
                    auto hybridPresentMode = desc[1].GetData<uint32_t>();
                    auto key = std::make_pair(hdr.ProcessId, pSwapChain);
                    mHybridPresentModeBySwapChainPid[key] = hybridPresentMode;
                }
            }
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

    // This could be one of several types of presents. Further events will clarify.
    // For now, assume that this is a blt straight into a surface which is already on-screen.
    VerboseTraceBeforeModifyingPresent(presentEvent.get());
    presentEvent->Hwnd = hwnd;
    if (redirectedPresent) {
        presentEvent->PresentMode = PresentMode::Composed_Copy_CPU_GDI;
        presentEvent->SupportsTearing = false;
    } else {
        presentEvent->PresentMode = PresentMode::Hardware_Legacy_Copy_To_Front_Buffer;
        presentEvent->SupportsTearing = true;
    }
}

// A flip event is emitted during fullscreen present submission.
//
// We expect the following event sequence emitted on the same thread:
//     PresentStart
//     FlipMultiPlaneOverlay_Info
//     QueuePacket_Start SubmitSequence MMIOFLIP bPresent=1
//     QueuePacket_Stop SubmitSequence
//     PresentStop
std::shared_ptr<PresentEvent> PMTraceConsumer::HandleDxgkFlip(EVENT_HEADER const& hdr)
{
    // First, lookup the in-progress present on the same thread.
    //
    // The present should not have a known QueueSubmitSequence nor seen a DxgkPresent yet, so if it
    // does then we've looked up the wrong present.  Typically, this means we've lost tracking for
    // the looked-up present, and it should be discarded.
    //
    // However, DWM on recent windows may omit the PresentStart/PresentStop events.  In this case,
    // we'll end up creating the present here and, because there is no PresentStop, it will be left
    // in mPresentByThreadId and looked up again in the next HandleDxgkFlip().
    std::shared_ptr<PresentEvent> presentEvent;
    for (;;) {
        // Lookup the in-progress present on this thread.
        auto ii = mPresentByThreadId.find(hdr.ThreadId);
        if (ii != mPresentByThreadId.end()) {
            presentEvent = ii->second;

            // If the in-progress present has seen DxgkPresent, it has lost tracking.
            if (presentEvent->SeenDxgkPresent) {
                RemoveLostPresent(presentEvent);
                continue;
            }

            // If the in-progress present has seen PresentStop, it has lost tracking.
            if (presentEvent->QueueSubmitSequence != 0 &&
                presentEvent->TimeInPresent != 0 &&
                presentEvent->Runtime != Runtime::Other) {
                RemoveLostPresent(presentEvent);
                continue;
            }

            // If we did see a PresentStart, then use this present
            if (presentEvent->Runtime != Runtime::Other) {
                // There may be duplicate flip events for MPO situations, so only handle the first.
                if (presentEvent->PresentMode != PresentMode::Unknown) {
                    return nullptr;
                }
                break;
            }

            // The looked-up present was created by the last Flip, and didn't see a PresentStop; remove
            // it from the thread tracking and create a new present for this flip.
            mPresentByThreadId.erase(ii);
        }

        // Create a new present for this flip
        if (!IsProcessTrackedForFiltering(hdr.ProcessId)) {
            return nullptr;
        }

        presentEvent = std::make_shared<PresentEvent>();

        VerboseTraceBeforeModifyingPresent(presentEvent.get());
        presentEvent->PresentStartTime = *(uint64_t*) &hdr.TimeStamp;
        presentEvent->ProcessId = hdr.ProcessId;
        presentEvent->ThreadId = hdr.ThreadId;

        TrackPresent(presentEvent, &mOrderedPresentsByProcessId[hdr.ProcessId]);
        break;
    }

    // Update the present based on the flip event...
    VerboseTraceBeforeModifyingPresent(presentEvent.get());

    presentEvent->PresentMode = PresentMode::Hardware_Legacy_Flip;

    // If this is the DWM thread, make any presents waiting for DWM dependent on it (i.e., they will
    // be displayed when it is).
    if (hdr.ThreadId == DwmPresentThreadId) {
        DebugAssert(presentEvent->DependentPresents.empty());

        for (auto& p : mPresentsWaitingForDWM) {
            p->PresentInDwmWaitingStruct = false;
        }
        std::swap(presentEvent->DependentPresents, mPresentsWaitingForDWM);
    }

    return presentEvent;
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

            if (present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
                VerboseTraceBeforeModifyingPresent(present.get());
                present->SeenDxgkPresent = true;

                // If the work is already done, complete it now.
                if (HasScreenTime(present)) {
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
        auto present = FindPresentByThreadId(hdr.ThreadId);
        if (present != nullptr && present->QueueSubmitSequence == 0) {

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

                SetScreenTime(pEvent, timestamp);

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
        VerboseTraceBeforeModifyingPresent(pEvent.get());
        pEvent->ReadyTime = timestamp;

        if (pEvent->PresentMode == PresentMode::Composed_Flip) {
            pEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
        }

        if (flags & (uint32_t) Microsoft_Windows_DxgKrnl::SetVidPnSourceAddressFlags::FlipImmediate) {
            SetScreenTime(pEvent, timestamp);
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

        VerboseTraceBeforeModifyingPresent(pEvent.get());
        SetScreenTime(pEvent, timestamp);

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

    VerboseTraceBeforeModifyingPresent(presentEvent.get());
    presentEvent->ReadyTime = 0;
    presentEvent->SupportsTearing = false;
    presentEvent->DxgkPresentHistoryToken = token;

    if (presentEvent->Displayed.size() == 1 &&
        presentEvent->Displayed[0].first == FrameType::NotSet) {
        presentEvent->Displayed.clear();
        presentEvent->FinalState = PresentResult::Unknown;
    }

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

    if (hdr.EventDescriptor.Id == Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id ||
       (hdr.EventDescriptor.Id == Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id && mTrackDisplay)) {
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
            HandleDxgkPresentHistory(hdr, Token, TokenData, Model);
        }
        return;
    }

    if (mTrackDisplay) {
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

            auto p = HandleDxgkFlip(hdr);
            if (p != nullptr) {
                p->SyncInterval = FlipInterval;

                if (MMIOFlip) {
                    p->WaitForFlipEvent = true;
                } else if (FlipInterval == 0) {
                    p->SupportsTearing = true;
                }
            }
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
        {
            EventDataDesc desc[] = {
                { L"VidPnSourceId" },
                { L"LayerIndex" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto VidPnSourceId = desc[0].GetData<uint32_t>();
            auto LayerIndex    = desc[1].GetData<uint32_t>();

            auto p = HandleDxgkFlip(hdr);
            if (p != nullptr) {
                p->WaitForFlipEvent    = true;
                p->WaitForMPOFlipEvent = true;

                if (p->SwapChainAddress == 0) {
                    p->SwapChainAddress = GenerateVidPnLayerId(VidPnSourceId, LayerIndex);
                }
            }
            return;
        }
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
                { L"hContext" },
                { L"SubmitSequence" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto hContext       = desc[0].GetData<uint64_t>();
            auto SubmitSequence = desc[1].GetData<uint32_t>();

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
            auto FlipSubmitSequence = desc[0].GetData<uint64_t>();

            auto submitSequence = (uint32_t) (FlipSubmitSequence >> 32u);
            auto present = FindPresentBySubmitSequence(submitSequence);
            if (present != nullptr) {

                // Apply Nvidia FlipDelay, if any, to the presentEvent
                mNvTraceConsumer.ApplyFlipDelay(present.get(), hdr.ThreadId);

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

                        // Any of the non-vsync status present modes can tear.
                        VerboseTraceBeforeModifyingPresent(present.get());
                        present->SupportsTearing = true;

                        // For FlipWaitVSync and FlipWaitHSync, we'll wait for the
                        // corresponding ?SyncDPC event.  Otherwise we consider
                        // this the present screen time.
                        if (FlipEntryStatusAfterFlip != (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitHSync) {

                            SetScreenTime(present, hdr.TimeStamp.QuadPart + present->FlipDelay);

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
            auto FlipFenceId = mMetadata.GetEventData<uint64_t>(pEventRecord, L"FlipFenceId");
            if (FlipFenceId != 0) {
                HandleDxgkSyncDPC(hdr.TimeStamp.QuadPart, (uint32_t)(FlipFenceId >> 32u));
            }
            return;
        }
        case Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id:
        case Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id:
        {
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
                            if (isMultiPlane &&
                                (pEvent->PresentMode == PresentMode::Hardware_Independent_Flip || pEvent->PresentMode == PresentMode::Composed_Flip)) {
                                VerboseTraceBeforeModifyingPresent(pEvent.get());
                                pEvent->PresentMode = PresentMode::Hardware_Composed_Independent_Flip;
                            }

                            // ScreenTime may have already been written by a preceding
                            // VSyncDPC_Info event, which is more accurate, so don't
                            // overwrite it in that case.
                            if (pEvent->FinalState != PresentResult::Presented) {
                                VerboseTraceBeforeModifyingPresent(pEvent.get());
                                SetScreenTime(pEvent, hdr.TimeStamp.QuadPart);
                            }

                            // Complete the present.
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
                if (present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer && HasScreenTime(present)) {
                    CompletePresent(present);
                }
            }
            return;
        }
        case Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id:
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

            HandleDxgkBlt(hdr, hwnd, bRedirectedPresent);
            return;
        }

        // BlitCancel_Info indicates that DxgKrnl optimized a present blt away, and
        // no further work was needed.
        case Microsoft_Windows_DxgKrnl::BlitCancel_Info::Id:
        {
            auto present = FindPresentByThreadId(hdr.ThreadId);
            if (present != nullptr) {
                VerboseTraceBeforeModifyingPresent(present.get());
                present->FinalState = PresentResult::Discarded;

                CompletePresent(present);
            }
            return;
        }
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

    if (mTrackFrameType) {
        // MMIOFlipMultiPlaneOverlay3_Info is emitted immediately before MMIOFlipMultiPlaneOverlay_Info,
        // on the same thread, with the same SubmitSequence, and includes the PresentId(s) that the
        // driver uses to report flip completion.
        //
        // Version 8 is required for LayerIndex and FlipSubmitSequence.
        if (hdr.EventDescriptor.Id == Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay3_Info::Id) {
            if (hdr.EventDescriptor.Version >= 8) {
                // Enable FlipFrameType events as we now know this system supports it.
                mEnableFlipFrameTypeEvents = true;

                EventDataDesc desc[] = {
                    { L"VidPnSourceId" },
                    { L"PlaneCount" },
                    { L"PresentId" },
                    { L"LayerIndex" },
                    { L"FlipSubmitSequence" },
                };
                mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
                auto VidPnSourceId      = desc[0].GetData<uint32_t>();
                auto PlaneCount         = desc[1].GetData<uint32_t>();
                auto PresentId          = desc[2].GetArray<uint64_t>(PlaneCount);
                auto LayerIndex         = desc[3].GetArray<uint32_t>(PlaneCount);
                auto FlipSubmitSequence = desc[4].GetData<uint32_t>();

                // Lookup the present associated with this submit sequence
                if (PlaneCount > 0) {
                    auto present = FindPresentBySubmitSequence(FlipSubmitSequence);
                    DebugAssert(present == nullptr || present->PresentIds.empty());

                    for (uint32_t i = 0; i < PlaneCount; ++i) {

                        // Mark any present already assigned to this VidPnLayer as they will not be
                        // getting any more FlipFrameType events.
                        auto vidPnLayerId = GenerateVidPnLayerId(VidPnSourceId, LayerIndex[i]);
                        {
                            auto ii = mPresentByVidPnLayerId.find(vidPnLayerId);
                            if (ii != mPresentByVidPnLayerId.end()) {
                                auto p2 = ii->second;
                                p2->PresentIds.clear();
                                mPresentByVidPnLayerId.erase(ii);

                                VerboseTraceBeforeModifyingPresent(p2.get());
                                p2->DoneWaitingForFlipFrameType = true;

                                if (p2->WaitingForFlipFrameType) {
                                    p2->WaitingForFlipFrameType = false;
                                    StopTrackingPresent(p2);

                                    for (auto p3 : p2->DependentPresents) {
                                        VerboseTraceBeforeModifyingPresent(p3.get());
                                        p3->WaitingForFlipFrameType = false;
                                        StopTrackingPresent(p2);
                                    }

                                    UpdateReadyCount(true);
                                }
                                VerboseTraceBeforeModifyingPresent(nullptr);
                            }
                        }

                        // If this submit sequence represents a present we are tracking, add
                        // the present ids to it.
                        if (present != nullptr) {
                            VerboseTraceBeforeModifyingPresent(present.get());
                            present->PresentIds.emplace(vidPnLayerId, PresentId[i]);
                            mPresentByVidPnLayerId.emplace(vidPnLayerId, present);
                        }

                        // Apply any pending FlipFrameType events
                        auto ii = mPendingFlipFrameTypeEvents.find(vidPnLayerId);
                        if (ii != mPendingFlipFrameTypeEvents.end()) {
                            if (present != nullptr && ii->second.PresentId == PresentId[i]) {
                                ApplyFlipFrameType(present, ii->second.Timestamp, ii->second.FrameType);
                            }
                            mPendingFlipFrameTypeEvents.erase(ii);
                        }
                    }
                }
            }
            return;
        }
    }

    assert(!mFilteredEvents); // Assert that filtering is working if expected
}


void PMTraceConsumer::HandleNvidiaDisplayDriverEvent(EVENT_RECORD* pEventRecord)
{
    mNvTraceConsumer.HandleNvidiaDisplayDriverEvent(pEventRecord, this);
}

void PMTraceConsumer::HandleWin7DxgkBlt(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    auto pBltEvent = reinterpret_cast<DXGKETW_BLTEVENT*>(pEventRecord->UserData);
    HandleDxgkBlt(
        pEventRecord->EventHeader,
        pBltEvent->hwnd,
        pBltEvent->bRedirectedPresent != 0);
}

void PMTraceConsumer::HandleWin7DxgkFlip(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    auto pFlipEvent = reinterpret_cast<DXGKETW_FLIPEVENT*>(pEventRecord->UserData);
    auto p = HandleDxgkFlip(pEventRecord->EventHeader);
    if (p != nullptr) {
        p->SyncInterval = pFlipEvent->FlipInterval;

        if (pFlipEvent->MMIOFlip != 0) {
            p->WaitForFlipEvent = true;
        } else if (pFlipEvent->FlipInterval == 0) {
            p->SupportsTearing = true;
        }
    }
}

void PMTraceConsumer::HandleWin7DxgkPresentHistory(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    auto pPresentHistoryEvent = reinterpret_cast<DXGKETW_PRESENTHISTORYEVENT*>(pEventRecord->UserData);
    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START) {
        HandleDxgkPresentHistory(
            pEventRecord->EventHeader,
            pPresentHistoryEvent->Token,
            0,
            Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_UNINITIALIZED);
    } else if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO) {
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
        HandleDxgkQueueComplete(
            pEventRecord->EventHeader.TimeStamp.QuadPart,
            pCompleteEvent->hContext,
            pCompleteEvent->SubmitSequence);
    }
}

void PMTraceConsumer::HandleWin7DxgkVSyncDPC(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

    auto pVSyncDPCEvent = reinterpret_cast<DXGKETW_SCHEDULER_VSYNC_DPC*>(pEventRecord->UserData);

    // Windows 7 does not support MultiPlaneOverlay.
    HandleDxgkSyncDPC(pEventRecord->EventHeader.TimeStamp.QuadPart, (uint32_t)(pVSyncDPCEvent->FlipFenceId.QuadPart >> 32u));
}

void PMTraceConsumer::HandleWin7DxgkMMIOFlip(EVENT_RECORD* pEventRecord)
{
    using namespace Microsoft_Windows_DxgKrnl::Win7;

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

    if (mTrackDisplay) {
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
            return;
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
						
                        // In certain cases like when there are short bursts of presents, 
                        // we get multiple back to back flips and token tracking thread 
                        // ends up marking the first frame in the burst as dropped. 
                        // To fix this issue, we mark the frame as discarded only if 
                        // the frame already doesn’t have valid ScreenTime. 
                        if (!HasScreenTime(prevPresent)) {
                            prevPresent->FinalState = PresentResult::Discarded;
                        }

                        RemovePresentFromSubmitSequenceIdTracking(prevPresent);
                    }
                }
                break;
            }

            case (uint32_t) Microsoft_Windows_Win32k::TokenState::Confirmed: // Present has been submitted
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
                // Nullptr as we don't want to report clearing the key in the verbose trace
                VerboseTraceBeforeModifyingPresent(nullptr);
                presentEvent->CompositionSurfaceLuid = 0;
                presentEvent->Win32KPresentCount = 0;
                presentEvent->Win32KBindId = 0;
                mPresentByWin32KPresentHistoryToken.erase(eventIter);

                if (!presentEvent->SeenInFrameEvent && presentEvent->FinalState == PresentResult::Unknown) {
                    VerboseTraceBeforeModifyingPresent(presentEvent.get());
                    presentEvent->FinalState = PresentResult::Discarded;
                    CompletePresent(presentEvent);
                } else if (presentEvent->PresentMode != PresentMode::Composed_Flip) {
                    CompletePresent(presentEvent);
                }
                break;
            }
            }
            return;
        }
        }
    }

    if (mTrackInput) {
        switch (hdr.EventDescriptor.Id) {
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
            return;
        }

        case Microsoft_Windows_Win32k::RetrieveInputMessage_Info::Id:
        {
            EventDataDesc desc[] = {
                { L"hwnd" }
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto hWnd = desc[0].GetData<uint64_t>();

            auto ii = mRetrievedInput.find(hdr.ProcessId);
            if (ii == mRetrievedInput.end()) {
                InputData data = { mLastInputDeviceReadTime, 0, 0, mLastInputDeviceType, hWnd };
                auto it = mReceivedMouseClickByHwnd.find(hWnd);
                if (it != mReceivedMouseClickByHwnd.end()) {
                    data.MouseClickTime = it->second.CurrentMouseClickTime;
                    data.XFormTime = it->second.CurrentXFormTime;
                    it->second.LastMouseClickTime = it->second.CurrentMouseClickTime;
                    it->second.LastXFormTime = it->second.CurrentXFormTime;
                }
                mRetrievedInput.emplace(hdr.ProcessId, data);
            } else {
                if (ii->second.Time < mLastInputDeviceReadTime) {
                    ii->second.Time = mLastInputDeviceReadTime;
                    ii->second.Type = mLastInputDeviceType;
                }
                // We can recieve multiple RetrieveInputMessage_Info events
                // before we receive an OnInputXformUpdate_Info event. Because
                // of this if the last device input type was a mouse
                // check to see if it was a mouse click and update if
                // necessary
                if (mLastInputDeviceType == InputDeviceType::Mouse) {
                    auto it = mReceivedMouseClickByHwnd.find(hWnd);
                    if (it != mReceivedMouseClickByHwnd.end()) {
                        if (it->second.LastMouseClickTime < it->second.CurrentMouseClickTime) {
                            ii->second.MouseClickTime = it->second.CurrentMouseClickTime;
                            ii->second.XFormTime = it->second.CurrentXFormTime;
                            it->second.LastMouseClickTime = it->second.CurrentMouseClickTime;
                            it->second.LastXFormTime = it->second.CurrentXFormTime;
                        }
                    }
                }
            }
            return;
        }

        case Microsoft_Windows_Win32k::OnInputXformUpdate_Info::Id:
        {
            EventDataDesc desc[] = {
                { L"Hwnd" },
                { L"XformQPCTime"}
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto hWnd = desc[0].GetData<uint64_t>();
            auto xFormQPCTime = desc[1].GetData<uint64_t>();

            auto it = mReceivedMouseClickByHwnd.find(hWnd);
            if (it != mReceivedMouseClickByHwnd.end()) {
                if (it->second.LastMouseClickTime < mLastInputDeviceReadTime) {
                    it->second.CurrentMouseClickTime = mLastInputDeviceReadTime;
                    it->second.CurrentXFormTime = xFormQPCTime;
                }
            }
            else {
                MouseClickData data = { mLastInputDeviceReadTime, xFormQPCTime , 0, 0 };
                mReceivedMouseClickByHwnd.emplace(hWnd, data);
            }

            return;
        }

        }
    }

    assert(!mFilteredEvents); // Assert that filtering is working if expected
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
                VerboseTraceBeforeModifyingPresent(present.get());
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

            VerboseTraceBeforeModifyingPresent(present.get());
            present->DxgkPresentHistoryTokenData = 0;

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
            VerboseTraceBeforeModifyingPresent(eventIter->second.get());
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
    if (present->QueueSubmitSequence != 0) {
        auto ii = mPresentBySubmitSequence.find(present->QueueSubmitSequence);
        if (ii != mPresentBySubmitSequence.end()) {
            auto presentsBySubmitSequence = &ii->second;

            // Do a linear search for present here.  We could do a find() but that
            // would require storing the queue context in PresentEvent and since
            // presentsBySubmitSequence is expected to be small (typically one element)
            // this should be faster.
            if (presentsBySubmitSequence->size() == 1) {
                if (presentsBySubmitSequence->begin()->second == present) {
                    mPresentBySubmitSequence.erase(ii);
                }
            } else {
                for (auto jj = presentsBySubmitSequence->begin(), je = presentsBySubmitSequence->end(); jj != je; ++jj) {
                    if (jj->second == present) {
                        presentsBySubmitSequence->erase(jj);
                        break;
                    }
                }
            }
        }

        // Don't report clearing of key in verbose trace
        VerboseTraceBeforeModifyingPresent(nullptr);
        present->QueueSubmitSequence = 0;
    }
}

// Remove the present from all temporary tracking structures.
void PMTraceConsumer::StopTrackingPresent(std::shared_ptr<PresentEvent> const& p)
{
    // Don't report changes to the tracking members.
    VerboseTraceBeforeModifyingPresent(nullptr);

    // mTrackedPresents
    if (p->RingIndex != UINT32_MAX) {
        mTrackedPresents[p->RingIndex] = nullptr;
        p->RingIndex = UINT32_MAX;
    }

    // mPresentByThreadId
    //
    // If the present was batched, it will by referenced in mPresentByThreadId
    // by both ThreadId and DriverThreadId.
    //
    // We don't reset ThreadId nor DriverThreadId as both are useful outside of
    // tracking.
    if (!p->WaitingForPresentStop) {
        auto ii = mPresentByThreadId.find(p->ThreadId);
        if (ii != mPresentByThreadId.end() && ii->second == p) {
            mPresentByThreadId.erase(ii);
        }
    }
    if (p->DriverThreadId != 0) {
        auto ii = mPresentByThreadId.find(p->DriverThreadId);
        if (ii != mPresentByThreadId.end() && ii->second == p) {
            mPresentByThreadId.erase(ii);
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

    // mPresentByVidPnLayerId (unless it will be needed by deferred FlipFrameType handling)
    if (!p->WaitingForFlipFrameType) {
        for (auto const& pr : p->PresentIds) {
            auto ii = mPresentByVidPnLayerId.find(pr.first);
            if (ii != mPresentByVidPnLayerId.end() && ii->second == p) {
                mPresentByVidPnLayerId.erase(ii);
            }
        }
        p->PresentIds.clear();
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

    if (p->AppFrameId != 0) {
        SetAppTimingDataAsComplete(p->ProcessId, p->AppFrameId);
    }
}

void PMTraceConsumer::RemoveLostPresent(std::shared_ptr<PresentEvent> p)
{
    VerboseTraceBeforeModifyingPresent(p.get());
    p->IsLost = true;
    CompletePresent(p);
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
    // is already running.  When that happens, PresentStartTime/TimeInPresentAPI and
    // ReadyTime/ScreenTime times can become mis-matched, and that offset can
    // persist for the full capture.
    //
    // We handle this by throwing away all queued presents up to this point.
    if (!mHasCompletedAPresent && !p->IsLost) {
        for (auto const& pr : mOrderedPresentsByProcessId) {
            for (auto orderedPresents = &pr.second; !orderedPresents->empty(); ) {
                RemoveLostPresent(orderedPresents->begin()->second);
            }
        }

        mHasCompletedAPresent = true;
        return;
    }

    // Protect against CompletePresent() being called twice for the same present.  That isn't
    // intended,  but there have been cases observed where it happens (in some cases leading to
    // infinite recursion with a DWM present and a dependent present completing each other).
    //
    // The exact pattern causing this has not yet been identified, so is the best fix for now.
    DebugAssert(p->IsCompleted == false);
    if (p->IsCompleted) {
        return;
    }

    // Mark the present as completed.
    VerboseTraceBeforeModifyingPresent(p.get());
    p->IsCompleted = true;

    // It is possible for a present to be displayed or discarded before Present_Stop.  If this
    // happens, we stop tracking the present (except for mPresentByThreadId which is required by
    // Present_Stop to lookup the present) but do not add the present to the dequeue list yet.
    // Present_Stop will check if the present is completed and add it to the dequeue list if so.
    if (!p->IsLost && p->Runtime != Runtime::Other && p->TimeInPresent == 0) {
        VerboseTraceBeforeModifyingPresent(p.get());
        p->WaitingForPresentStop = true;
    }

    // If flip frame type tracking is enabled, we defer the completion because we may see subsequent
    // FlipFrameType events that need to refer back to this PresentEvent.  The deferral will be
    // completed when we see another MMIOFlipMultiPlaneOverlay3_Info event for the same
    // (VidPnSourceId, LayerIndex) pair.
    if (!p->IsLost && !p->DoneWaitingForFlipFrameType && mEnableFlipFrameTypeEvents && p->FinalState == PresentResult::Presented) {
        VerboseTraceBeforeModifyingPresent(p.get());
        p->WaitingForFlipFrameType = true;
    }

    // If this is a DWM present, update the dependent presents' final state and complete them as
    // well.
    //
    // Note: we need to check if the dependent presents are already completed because if they were
    // completed or lost elsewhere during analysis, they wouldn't have been removed from the
    // DependentPresents list.
    if (!p->DependentPresents.empty()) {
        if (p->IsLost) {
            for (auto p2 : p->DependentPresents) {
                VerboseTraceBeforeModifyingPresent(p2.get());
                p2->IsLost = true;
            }
        } else {
            // If there are multiple dependent presents from the same HWND, then discard all but the
            // last one.
            std::unordered_set<uint64_t> completedComposedFlipHwnds;
            for (auto ii = p->DependentPresents.rbegin(), ie = p->DependentPresents.rend(); ii != ie; ++ii) {
                auto p2 = *ii;
                if (!p2->IsCompleted) {
                    if ((p2->PresentMode == PresentMode::Composed_Flip ||
                         p2->PresentMode == PresentMode::Composed_Copy_GPU_GDI ||
                         p2->PresentMode == PresentMode::Composed_Copy_CPU_GDI) &&
                        !completedComposedFlipHwnds.emplace(p2->Hwnd).second) {
                        VerboseTraceBeforeModifyingPresent(p2.get());
                        p2->FinalState = PresentResult::Discarded;
                        p2->Displayed.clear();
                    } else if (p2->FinalState != PresentResult::Discarded) {
                        VerboseTraceBeforeModifyingPresent(p2.get());
                        p2->FinalState = p->FinalState;
                        p2->Displayed = p->Displayed;
                        p2->WaitingForFlipFrameType = p->WaitingForFlipFrameType;
                        p2->DoneWaitingForFlipFrameType = p->DoneWaitingForFlipFrameType;
                    }
                }
            }
        }

        for (auto ii = p->DependentPresents.rbegin(), ie = p->DependentPresents.rend(); ii != ie; ++ii) {
            auto p2 = *ii;
            if (!p2->IsCompleted) {
                CompletePresent(p2);
            }
        }
        if (!p->WaitingForFlipFrameType) {
            p->DependentPresents.clear();
            p->DependentPresents.shrink_to_fit();
        }
    }

    // If presented, remove any earlier presents made on the same swap chain.
    if (p->FinalState == PresentResult::Presented) {
        auto presentsByThisProcess = &mOrderedPresentsByProcessId[p->ProcessId];
        for (auto ii = presentsByThisProcess->begin(), ie = presentsByThisProcess->end(); ii != ie; ) {
            auto p2 = ii->second;
            if (p2->PresentStartTime >= p->PresentStartTime) break;
            ++ii; // Increment the iterator first since below calls may remove it from
                  // presentsByThisProcess.

            if (p2->SwapChainAddress == p->SwapChainAddress) {
                if (p->IsLost) {
                    VerboseTraceBeforeModifyingPresent(p2.get());
                    p2->IsLost = true;
                }

                CompletePresent(p2);
            }
        }
    }

    if (mTrackPcLatency) {
        auto* pclTimingData = ExtractAppTimingData(
            mPclTimingDataByPclFrameId,
            p->ProcessId,
            0,
            p->PresentStartTime,
            [this](const AppTimingData& d) {
                if (mUsingOutOfBoundPresentStart) {
                    return d.PclOutOfBandPresentStartTime;
                }
                else {
                    return d.PclPresentStartTime;
                }
            });
        if (pclTimingData) {
            if (p->ProcessId == pclTimingData->ProcessId) {
                p->PclFrameId = pclTimingData->FrameId;
                p->PclSimStartTime = pclTimingData->PclSimStartTime;
                p->PclSimEndTime = pclTimingData->PclSimEndTime;
                p->PclRenderSubmitStartTime = pclTimingData->PclRenderSubmitStartTime;
                p->PclRenderSubmitEndTime = pclTimingData->PclRenderSubmitEndTime;
                p->PclPresentStartTime = pclTimingData->PclPresentStartTime;
                p->PclPresentEndTime = pclTimingData->PclPresentEndTime;
                p->PclInputPingTime = pclTimingData->PclInputPingTime;
                p->PclInputReceivedTime = pclTimingData->PclInputReceivedTime;
            }
        }
    }

    // Remove the present from tracking structures.
    StopTrackingPresent(p);

    // lock mutex across this sequence:
    // 0) Merge present 1) add present to completed 2) update ready count 3) clear out stale deferred frames
    std::unique_lock<std::mutex> lock(mPresentEventMutex);

    // If the present has a PresentFrameType, propagate the QPC data.
    // TODO -> Ascii art
    auto present = p;
    auto presentStartTime = p->PresentStartTime;
    auto appFrameId = p->AppFrameId;
    auto processId = p->ProcessId;
    if (present->WaitingForFrameId) {
        for (uint32_t i = mReadyCount; i < mCompletedCount; ++i) {
            auto const& p2 = mCompletedPresents[GetRingIndex(mCompletedIndex + i)];
            if (p2->WaitingForFrameId && p2->ProcessId == present->ProcessId) {
                VerboseTraceBeforeModifyingPresent(p2.get());
                if (p2->FrameId == present->FrameId) {
                    auto it = std::find_if(p2->Displayed.begin(), p2->Displayed.end(),
                        [](const std::pair<FrameType, uint64_t>& p) {
                            return p.first == FrameType::Intel_XEFG;
                        });
                    if (it != p2->Displayed.end()) {
                        // Propagate the Present information attached to the generated frame
                        // TODO -> explain why we have two paths for copying
                        if (p2->AppPropagatedPresentStartTime != 0) {
                            present->AppPropagatedPresentStartTime = p2->AppPropagatedPresentStartTime;
                            present->AppPropagatedTimeInPresent = p2->AppPropagatedTimeInPresent;
                            present->AppPropagatedGPUStartTime = p2->AppPropagatedGPUStartTime;
                            present->AppPropagatedReadyTime = p2->AppPropagatedReadyTime;
                            present->AppPropagatedGPUDuration = p2->AppPropagatedGPUDuration;
                            present->AppPropagatedGPUVideoDuration = p2->AppPropagatedGPUVideoDuration;
                            p2->AppPropagatedPresentStartTime = 0;
                            p2->AppPropagatedTimeInPresent = 0;
                            p2->AppPropagatedGPUStartTime = 0;
                            p2->AppPropagatedReadyTime = 0;
                            p2->AppPropagatedGPUDuration = 0;
                            p2->AppPropagatedGPUVideoDuration = 0;
                        } else {
                            present->AppPropagatedPresentStartTime = p2->PresentStartTime;
                            present->AppPropagatedTimeInPresent = p2->TimeInPresent;
                            present->AppPropagatedGPUStartTime = p2->GPUStartTime;
                            present->AppPropagatedReadyTime = p2->ReadyTime;
                            present->AppPropagatedGPUDuration = p2->GPUDuration;
                            present->AppPropagatedGPUVideoDuration = p2->GPUVideoDuration;
                        }

                        // Propagate the Input information attached from the generated frame
                        if ((p2->InputTime != 0 && present->InputTime == 0) ||
                            (p2->MouseClickTime != 0 && present->MouseClickTime == 0)) {
                            present->InputTime = p2->InputTime;
                            present->MouseClickTime = p2->MouseClickTime;
                            present->InputType = p2->InputType;
                        }
                    }
                    break;
                }
                p2->WaitingForFrameId = false;
            }
        }
    }

    // Add the present to the completed list
    if (present != nullptr) {
        uint32_t index;
        // if completed buffer is full
        if (mCompletedCount == mCircularBufferSize) {
            // if we are in offline ETL processing mode, block instead of overwriting events
            // unless either A) the buffer is full of non-ready events or B) backpressure disabled via CLI option
            if (!mIsRealtimeSession && mReadyCount != 0 && !mDisableOfflineBackpressure) {
                mCompletedRingCondition.wait(lock, [this] { return mCompletedCount < mCircularBufferSize; });
                index = GetRingIndex(mCompletedIndex + mCompletedCount);
                mCompletedCount++;
            }
            // Completed present overflow routine (when not blocking):
            // If the completed list is full, throw away the oldest completed present, if it IsLost; or this
            // present, if it IsLost; or the oldest completed present.
            else {
                if (!mCompletedPresents[mCompletedIndex]->IsLost && present->IsLost) {
                    return;
                }
                index = mCompletedIndex;
                mCompletedIndex = GetRingIndex(mCompletedIndex + 1);
                if (mReadyCount > 0) {
                    mReadyCount--;
                }
                mNumOverflowedPresents++;
            }
            // otherwise, completed buffer still has available space
        }
        else {
            index = GetRingIndex(mCompletedIndex + mCompletedCount);
            mCompletedCount++;
        }
        // place the present in the completed presents ring buffer
        mCompletedPresents[index] = present;
    }

    // Update the ready count (do not lock mutex in this function, already locked)
    UpdateReadyCount(false);

    // clear out stale deferred frames
    // It's possible for a deferred condition to never be cleared.  e.g., a process' last present
    // doesn't get a Present_Stop event.  When this happens the deferred present will prevent all
    // subsequent presents from other processes from being dequeued until the ring buffer wraps and
    // forces it out, which is likely longer than we want to wait.  So we check here if there is 
    // a stuck deferred present and clear the deferral if it gets too old.
    if (mReadyCount == 0 && mCompletedCount > 0) {
        auto const& deferredPresent = mCompletedPresents[mCompletedIndex];
        if (presentStartTime >= deferredPresent->PresentStartTime &&
            presentStartTime - deferredPresent->PresentStartTime > mDeferralTimeLimit) {
            VerboseTraceBeforeModifyingPresent(deferredPresent.get());
            if (deferredPresent->WaitingForPresentStop) {
                deferredPresent->WaitingForPresentStop = false;
                deferredPresent->IsLost = true;
            }
            deferredPresent->WaitingForFlipFrameType = false;
            StopTrackingPresent(deferredPresent);
            UpdateReadyCount(false);
        }
    }

    // Prune out old app frame data
    for (auto it = mAppTimingDataByAppFrameId.begin(); it != mAppTimingDataByAppFrameId.end();) {
        if (appFrameId != 0 && it->second.ProcessId == processId &&
            static_cast<int32_t>(appFrameId - it->second.FrameId) >= 10) {
            // Remove the app frame data if the app frame id is too old
            it = mAppTimingDataByAppFrameId.erase(it); // Erase and move to the next element
        } else if (appFrameId == 0 && it->second.ProcessId == processId && 
            it->second.AssignedToPresent == false && it->second.AppPresentStartTime != 0 && 
            presentStartTime >= it->second.AppPresentStartTime &&
            presentStartTime - it->second.AppPresentStartTime >= mDeferralTimeLimit) {
            // Remove app frame data if the app present start time is too old
            it = mAppTimingDataByAppFrameId.erase(it); // Erase and move to the next element
        }
        else {
            ++it; // Move to the next element
        }
    }
}

void PMTraceConsumer::UpdateReadyCount(bool useLock)
{
    bool newPresentsReady = false;
    {
        std::unique_lock<std::mutex> lck{ mPresentEventMutex, std::defer_lock }; 
        if (useLock) {
            lck.lock();
        }
        else {
            assert(!lck.try_lock());
        }    
        for (; mReadyCount < mCompletedCount; ++mReadyCount) {
            newPresentsReady = true;
            auto const& p = mCompletedPresents[GetRingIndex(mCompletedIndex + mReadyCount)];
            if (p->WaitingForPresentStop ||
                p->WaitingForFlipFrameType ||
                p->WaitingForFrameId) {
                break;
            }
        }
    }
    if (newPresentsReady) {
        SignalEventsReady();
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

std::shared_ptr<PresentEvent> PMTraceConsumer::FindPresentByThreadId(uint32_t threadId)
{
    auto ii = mPresentByThreadId.find(threadId);
    return ii == mPresentByThreadId.end() ? std::shared_ptr<PresentEvent>() : ii->second;
}

std::shared_ptr<PresentEvent> PMTraceConsumer::FindOrCreatePresent(EVENT_HEADER const& hdr)
{
    // First, we check if there is an in-progress present that was last
    // operated on from this same thread.
    auto present = FindPresentByThreadId(hdr.ThreadId);
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

        ApplyPresentFrameType(present);

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
    if (mTrackedPresents[mNextFreeRingIndex] != nullptr) {
        RemoveLostPresent(mTrackedPresents[mNextFreeRingIndex]);
    }

    // Add the present into the initial tracking data structures
    VerboseTraceBeforeModifyingPresent(present.get());
    present->RingIndex = mNextFreeRingIndex;
    mTrackedPresents[mNextFreeRingIndex] = present;
    mNextFreeRingIndex = GetRingIndex(mNextFreeRingIndex + 1);

    presentsByThisProcess->emplace(present->PresentStartTime, present);

    SetThreadPresent(present->ThreadId, present);

    // Assign any pending retrieved input to this frame
    if (mTrackInput) {
        auto ii = mRetrievedInput.find(present->ProcessId);
        if (ii != mRetrievedInput.end() && ii->second.Type != InputDeviceType::None) {
            present->InputTime = ii->second.Time;
            present->InputType = ii->second.Type;
            if (ii->second.MouseClickTime != 0 && ii->second.XFormTime != 0) {
                present->MouseClickTime = ii->second.MouseClickTime;
            } else {
                present->MouseClickTime = 0;
            }
            ii->second.Type = InputDeviceType::None;
            ii->second.XFormTime = 0;
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
    
    present->AppFrameId = 0;
    present->AppSleepStartTime = 0;
    present->AppSleepEndTime = 0;
    present->AppSimStartTime = 0;
    present->AppSimEndTime = 0;
    present->AppRenderSubmitStartTime = 0;
    present->AppRenderSubmitEndTime = 0;
    present->AppPresentStartTime = 0;
    present->AppPresentEndTime = 0;

    ApplyPresentFrameType(present);

    TrackPresent(present, &mOrderedPresentsByProcessId[present->ProcessId]);
}

// No TRACK_PRESENT instrumentation here because each runtime Present::Start
// event is instrumented and we assume we'll see the corresponding Stop event
// for any completed present.
void PMTraceConsumer::RuntimePresentStop(Runtime runtime, EVENT_HEADER const& hdr, uint32_t result)
{
    // Present_Start and Present_Stop happen on the same thread, so Lookup the PresentEvent
    // most-recently operated on by the same thread.  If there is none, ignore this event.
    auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (eventIter == mPresentByThreadId.end()) {
        return;
    }
    auto present = eventIter->second;

    if (mTrackAppTiming) {
        if (IsApplicationPresent(present)) {
            auto* appTimingData = ExtractAppTimingData(
                mAppTimingDataByAppFrameId,
                present->ProcessId,
                present->AppFrameId,
                present->PresentStartTime,
                [](const AppTimingData& d) { return d.AppPresentStartTime; });
            if (appTimingData) {
                if (present->ProcessId == appTimingData->ProcessId) {
                    if (present->AppFrameId == 0) {
                        present->AppFrameId = appTimingData->FrameId;
                    }
                    present->AppSimStartTime = appTimingData->AppSimStartTime;
                    present->AppSimEndTime = appTimingData->AppSimEndTime;
                    present->AppSleepStartTime = appTimingData->AppSleepStartTime;
                    present->AppSleepEndTime = appTimingData->AppSleepEndTime;
                    present->AppRenderSubmitStartTime = appTimingData->AppRenderSubmitStartTime;
                    present->AppRenderSubmitEndTime = appTimingData->AppRenderSubmitEndTime;
                    present->AppPresentStartTime = appTimingData->AppPresentStartTime;
                    present->AppPresentEndTime = appTimingData->AppPresentEndTime;
                    present->AppInputSample.first = appTimingData->AppInputSample.first;
                    present->AppInputSample.second = appTimingData->AppInputSample.second;
                    // Start tracking using the app frame id to this present
                    mPresentByAppFrameId.emplace(std::make_pair(present->AppFrameId, present->ProcessId), present);
                }
            }
        }
    }

    if (mTrackHybridPresent) {
        // Determine if this is a hybrid present
        auto key = std::make_pair(present->ProcessId, present->SwapChainAddress);
        auto swapIter = mHybridPresentModeBySwapChainPid.find(key);
        if (swapIter != mHybridPresentModeBySwapChainPid.end()) {
            Microsoft_Windows_DXGI::HybridPresentMode hybridPresentMode = (Microsoft_Windows_DXGI::HybridPresentMode)swapIter->second;
            if (hybridPresentMode != Microsoft_Windows_DXGI::HybridPresentMode::NOT_HYBRID) {
                present->IsHybridPresent = true;
            }
        }
    }

    // Set the runtime and Present_Stop time.
    VerboseTraceBeforeModifyingPresent(present.get());
    present->Runtime = runtime;
    present->TimeInPresent = *(uint64_t*) &hdr.TimeStamp - present->PresentStartTime;

    // If this present completed early and was deferred until the Present_Stop, then no more
    // analysis is needed; we just clear the deferral.
    if (present->WaitingForPresentStop) {
        VerboseTraceBeforeModifyingPresent(present.get());
        present->WaitingForPresentStop = false;

        mPresentByThreadId.erase(eventIter);
        UpdateReadyCount(true);
        return;
    }

    // If the Present() call failed, no more analysis is needed.
    if (FAILED(result)) {
        // Check expected state (a new Present() that has only been started).
        DebugAssert(present->TimeInPresent == 0);
        DebugAssert(present->IsCompleted   == false);
        DebugAssert(present->IsLost        == false);
        DebugAssert(present->DependentPresents.empty());

        present->PresentFailed = true;
        CompletePresent(present);
        return;
    }

    // If the present wasn't visibility, no more analysis is needed.
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

    // If we are not tracking presents to display, then no more analysis is needed.
    if (!mTrackDisplay) {
        present->FinalState = PresentResult::Presented;
        CompletePresent(present);
        return;
    }

    // Continue with analysis, but we won't need to lookup the present by thread any more.  Future
    // events (e.g., from DXGK/Win32K/etc.) can happen on other threads and are looked up by other
    // mechanisms (e.g., by process id, swapchain, etc.).
    mPresentByThreadId.erase(eventIter);
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
            size_t start = ImageName.find_last_of(L'\\') + 1;
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

    {
        std::lock_guard<std::mutex> lock(mProcessEventMutex);
        mProcessEvents.emplace_back(event);
    }
    SignalEventsReady();
}

InputDeviceType ConvertIntelProviderInputTypes(Intel_PresentMon::InputType ipmInputType) {
    switch (ipmInputType) {
    case Intel_PresentMon::InputType::Unspecified:   return InputDeviceType::Unknown;
    case Intel_PresentMon::InputType::MouseClick:    return InputDeviceType::Mouse;
    case Intel_PresentMon::InputType::KeyboardClick: return InputDeviceType::Keyboard;
    }
    DebugAssert(false);
    return InputDeviceType::Unknown;
}

void PMTraceConsumer::SetAppTimingData(const EVENT_RECORD* pEventRecord) {
    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
    auto processId = pEventRecord->EventHeader.ProcessId;

    if (processId == DwmProcessId) {
        return;
    }

    switch (pEventRecord->EventHeader.EventDescriptor.Id) {
    case Intel_PresentMon::AppSleepStart_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppSleepStart_Info_Props));
        auto props = (Intel_PresentMon::AppSleepStart_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppSleepStartTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppSleepEndTime = timestamp;
            } else {
                AppTimingData data;
                data.AppSleepStartTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppSleepEnd_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppSleepEnd_Info_Props));
        auto props = (Intel_PresentMon::AppSleepEnd_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppSleepEndTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppSleepEndTime = timestamp;
            } else {
                AppTimingData data;
                data.AppSleepEndTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppSimulationStart_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppSimulationStart_Info_Props));
        auto props = (Intel_PresentMon::AppSimulationStart_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppSimStartTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppSimStartTime = timestamp;
            } else {
                AppTimingData data;
                data.AppSimStartTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppSimulationEnd_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppSimulationEnd_Info_Props));
        auto props = (Intel_PresentMon::AppSimulationEnd_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppSimEndTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppSimEndTime = timestamp;
            } else {
                AppTimingData data;
                data.AppSimEndTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppRenderSubmitStart_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppRenderSubmitStart_Info_Props));
        auto props = (Intel_PresentMon::AppRenderSubmitStart_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppRenderSubmitStartTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppRenderSubmitStartTime = timestamp;
            } else {
                AppTimingData data;
                data.AppRenderSubmitStartTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppRenderSubmitEnd_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppRenderSubmitEnd_Info_Props));
        auto props = (Intel_PresentMon::AppRenderSubmitEnd_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppRenderSubmitEndTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppRenderSubmitEndTime = timestamp;
            } else {
                AppTimingData data;
                data.AppRenderSubmitEndTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppPresentStart_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppPresentStart_Info_Props));
        auto props = (Intel_PresentMon::AppPresentStart_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppPresentStartTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppPresentStartTime = timestamp;
            } else {
                AppTimingData data;
                data.AppPresentStartTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppPresentEnd_Info::Id: {
        DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppPresentEnd_Info_Props));
        auto props = (Intel_PresentMon::AppPresentEnd_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppPresentEndTime = timestamp;
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppPresentEndTime = timestamp;
            } else {
                AppTimingData data;
                data.AppPresentEndTime = timestamp;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    case Intel_PresentMon::AppInputSample_Info::Id: {
        //DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::AppInputSample_Info_Props));
        auto props = (Intel_PresentMon::AppInputSample_Info_Props*)pEventRecord->UserData;
        auto key = std::make_pair(props->FrameId, processId);
        auto ii = mPresentByAppFrameId.find(key);
        if (ii != mPresentByAppFrameId.end()) {
            DebugAssert(ii->second->ProcessId == processId);
            ii->second->AppInputSample.first = timestamp;
            ii->second->AppInputSample.second = ConvertIntelProviderInputTypes(props->InputType);
        } else {
            auto ij = mAppTimingDataByAppFrameId.find(key);
            if (ij != mAppTimingDataByAppFrameId.end()) {
                ij->second.AppInputSample.first = timestamp;
                ij->second.AppInputSample.second = ConvertIntelProviderInputTypes(props->InputType);
            } else {
                AppTimingData data;
                data.ProcessId = processId;
                data.FrameId = props->FrameId;
                data.AppInputSample.first = timestamp;
                data.AppInputSample.second = ConvertIntelProviderInputTypes(props->InputType);
                mAppTimingDataByAppFrameId.emplace(key, data);
            }
        }
    }
    break;
    }
}

void PMTraceConsumer::HandleIntelPresentMonEvent(EVENT_RECORD* pEventRecord)
{
    if (mTrackFrameType) {
        switch (pEventRecord->EventHeader.EventDescriptor.Id) {
        case Intel_PresentMon::PresentFrameType_Info::Id: {
            if (pEventRecord->EventHeader.EventDescriptor.Version == 0) {
                DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::PresentFrameType_Info_Props));

                auto props = (Intel_PresentMon::PresentFrameType_Info_Props*)pEventRecord->UserData;
                auto event = &mPendingPresentFrameTypeEvents[pEventRecord->EventHeader.ThreadId];
                event->FrameId = props->FrameId;
                event->FrameType = ConvertPMPFrameTypeToFrameType(props->FrameType);
                event->AppFrameId = 0;
            } else if (pEventRecord->EventHeader.EventDescriptor.Version == 1) {
                DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::PresentFrameType_Info_2_Props));

                auto props = (Intel_PresentMon::PresentFrameType_Info_2_Props*)pEventRecord->UserData;
                auto event = &mPendingPresentFrameTypeEvents[pEventRecord->EventHeader.ThreadId];
                event->FrameId = props->FrameId;
                event->FrameType = ConvertPMPFrameTypeToFrameType(props->FrameType);
                event->AppFrameId = props->AppFrameId;
            } else {
                // Unknown version
            }
        }
        return;
        case Intel_PresentMon::FlipFrameType_Info::Id:
            if (mEnableFlipFrameTypeEvents) {
                DebugAssert(pEventRecord->UserDataLength == sizeof(Intel_PresentMon::FlipFrameType_Info_Props));

                auto props = (Intel_PresentMon::FlipFrameType_Info_Props*) pEventRecord->UserData;
                auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                auto frameType = ConvertPMPFrameTypeToFrameType(props->FrameType);

                // Look up the present associated with this (VidPnSourceId, LayerIndex, PresentId).
                //
                // It is possible to see the FlipFrameType event before the MMIOFlipMultiPlaneOverlay3_Info
                // event, in which case the lookup will fail.  In this case we deferr application of the
                // FlipFrameType until we see the MMIOFlipMultiPlaneOverlay3_Info event.
                auto vidPnLayerId = GenerateVidPnLayerId(props->VidPnSourceId, props->LayerIndex);
                auto ii = mPresentByVidPnLayerId.find(vidPnLayerId);
                if (ii == mPresentByVidPnLayerId.end()) {
                    DeferFlipFrameType(vidPnLayerId, props->PresentId, timestamp, frameType);
                    return;
                }

                auto present = ii->second;
                auto jj = present->PresentIds.find(vidPnLayerId);
                if (jj == present->PresentIds.end() || jj->second != props->PresentId) {
                    DeferFlipFrameType(vidPnLayerId, props->PresentId, timestamp, frameType);
                } else {
                    ApplyFlipFrameType(present, timestamp, frameType);
                }
        }
        return;
        }
    }

    if (mTrackPMMeasurements) {
        switch (pEventRecord->EventHeader.EventDescriptor.Id) {
        case Intel_PresentMon::MeasuredInput_Info::Id: {
            EventDataDesc desc[] = {
                { L"InputType" },
                { L"Time" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto InputType = desc[0].GetData<uint8_t>();
            auto Time      = desc[1].GetData<uint64_t>();

            (void) InputType, Time;
            // TODO

        }   return;

        case Intel_PresentMon::MeasuredScreenChange_Info::Id: {
            EventDataDesc desc[] = {
                { L"Time" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto Time = desc[0].GetData<uint64_t>();

            (void) Time;
            // TODO

        }   return;
        }
    }


    if (mTrackAppTiming) {
        SetAppTimingData(pEventRecord);
        return;
    }

    assert(!mFilteredEvents); // Assert that filtering is working if expected
}

void PMTraceConsumer::HandleTraceLoggingEvent(EVENT_RECORD* pEventRecord)
{
    if (mTrackPcLatency) {
        HandlePclEvent(pEventRecord);
    }
}

void PMTraceConsumer::HandlePclEvent(EVENT_RECORD* pEventRecord)
{
    try {
        if (!mTraceLoggingDecoder.DecodeTraceLoggingEventRecord(pEventRecord)) {
            pmlog_dbg("Failed to decode trace logging event"); // too spammy?
            return;
        }

        auto eventName = mTraceLoggingDecoder.GetEventName();
        if (!eventName.has_value()) {
            pmlog_warn("Could not get trace logging event name");
            return;
        }

        if (eventName.has_value()) {
            if (eventName.value() == L"PCLStatsInit" ||
                eventName.value() == L"ReflexStatsInit") {
                pmlog_info("Received init event: " + pmon::util::str::ToNarrow(*eventName));
            }
            else if (eventName.value() == L"PCLStatsEvent" || 
                     eventName.value() == L"ReflexStatsEvent") {
                auto marker = (Nvidia_PCL::PCLMarker)mTraceLoggingDecoder.GetNumericPropertyValue<uint32_t>(L"Marker");
                auto frameId = (uint32_t)mTraceLoggingDecoder.GetNumericPropertyValue<uint64_t>(L"FrameID");
                switch (marker) {
                case Nvidia_PCL::PCLMarker::SimulationStart: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    auto key = std::make_pair(frameId, processId);
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        ij->second.PclSimStartTime = timestamp;
                    } else {
                        AppTimingData data;
                        data.PclSimStartTime = timestamp;
                        data.ProcessId = processId;
                        data.FrameId = frameId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }
                }
                break;
                case Nvidia_PCL::PCLMarker::SimulationEnd: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    auto key = std::make_pair(frameId, processId);
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        ij->second.PclSimEndTime = timestamp;
                    } else {
                        AppTimingData data;
                        data.PclSimEndTime = timestamp;
                        data.ProcessId = processId;
                        data.FrameId = frameId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }
                }
                break;
                case Nvidia_PCL::PCLMarker::RenderSubmitStart: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    auto key = std::make_pair(frameId, processId);
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        ij->second.PclRenderSubmitStartTime = timestamp;
                    } else {
                        AppTimingData data;
                        data.PclRenderSubmitStartTime = timestamp;
                        data.ProcessId = processId;
                        data.FrameId = frameId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }
                }
                break;
                case Nvidia_PCL::PCLMarker::RenderSubmitEnd: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    auto key = std::make_pair(frameId, processId);
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        ij->second.PclRenderSubmitEndTime = timestamp;
                    } else {
                        AppTimingData data;
                        data.PclRenderSubmitEndTime = timestamp;
                        data.ProcessId = processId;
                        data.FrameId = frameId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }
                }
                break;
                case Nvidia_PCL::PCLMarker::PresentStart: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    auto key = std::make_pair(frameId, processId);
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        ij->second.PclPresentStartTime = timestamp;
                    } else {
                        AppTimingData data;
                        data.PclPresentStartTime = timestamp;
                        data.ProcessId = processId;
                        data.FrameId = frameId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }
                }
                break;
                case Nvidia_PCL::PCLMarker::PresentEnd: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    auto key = std::make_pair(frameId, processId);
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        ij->second.PclPresentEndTime = timestamp;
                    } else {
                        AppTimingData data;
                        data.PclPresentEndTime = timestamp;
                        data.ProcessId = processId;
                        data.FrameId = frameId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }
                }
                break;
                case Nvidia_PCL::PCLMarker::PCLLatencyPing: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    auto key = std::make_pair(frameId, processId);
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        ij->second.PclInputReceivedTime = timestamp;
                        ij->second.PclInputPingTime = mLatestPingTimestampByProcessId[processId];
                        if (mLatestPingTimestampByProcessId[processId] != 0) {
                            ij->second.PclInputPingTime = mLatestPingTimestampByProcessId[processId];
                            mLatestPingTimestampByProcessId[processId] = 0;
                        }
                    } else {
                        AppTimingData data;
                        data.PclInputReceivedTime = timestamp;
                        if (mLatestPingTimestampByProcessId[processId] != 0) {
                            data.PclInputPingTime = mLatestPingTimestampByProcessId[processId];
                            mLatestPingTimestampByProcessId[processId] = 0;
                        }
                        data.ProcessId = processId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }
                }
                break;
                case Nvidia_PCL::PCLMarker::OutOfBandPresentStart: {
                    auto processId = pEventRecord->EventHeader.ProcessId;
                    auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                    // If we receive an out of band present start, we will use it
                    // to attach the pcl timing data to the present
                    mUsingOutOfBoundPresentStart = true;
                    auto key = std::make_pair(frameId, processId);
                    // We do not track the out of band present in the same way as the other markers.
                    // We only use it in an attempt to attach the pcl timing data to the present
                    auto ij = mPclTimingDataByPclFrameId.find(key);
                    if (ij != mPclTimingDataByPclFrameId.end()) {
                        if (ij->second.PclOutOfBandPresentStartTime == 0) {
                            ij->second.PclOutOfBandPresentStartTime = timestamp;
                        }
                    }
                    else {
                        AppTimingData data;
                        data.PclOutOfBandPresentStartTime = timestamp;
                        data.ProcessId = processId;
                        data.FrameId = frameId;
                        mPclTimingDataByPclFrameId.emplace(key, data);
                    }

                }
                break;
                }
            } else if (eventName.value() == L"PCLStatsInput" ||
                       eventName.value() == L"ReflexStatsInput") {
                auto processId = pEventRecord->EventHeader.ProcessId;
                auto timestamp = pEventRecord->EventHeader.TimeStamp.QuadPart;
                mLatestPingTimestampByProcessId[processId] = timestamp;

            } else if (eventName.value() == L"PCLStatsShutdown" ||
                       eventName.value() == L"ReflexStatsShutdown") {
                // PCL stats is shutting down for this process. Remove
                // all PCL tracking structure for this process id. 
                pmlog_info("Shutting down PCL stats tracking");
                auto processId = pEventRecord->EventHeader.ProcessId;
                std::erase_if(mPclTimingDataByPclFrameId, [processId](const auto& p) {
                    return p.first.second == processId; });
                std::erase_if(mLatestPingTimestampByProcessId, [processId](const auto& p) {
                    return p.first == processId; });

            }
            else {
                // unhandled trace event => ignore
            }
        }
    }
    catch (...) {
        pmlog_error(pmon::util::ReportException());
    }
}

void PMTraceConsumer::DeferFlipFrameType(
    uint64_t vidPnLayerId,
    uint64_t presentId,
    uint64_t timestamp,
    FrameType frameType)
{
    DebugAssert(mPendingFlipFrameTypeEvents.find(vidPnLayerId) == mPendingFlipFrameTypeEvents.end());

    FlipFrameTypeEvent e;
    e.PresentId = presentId;
    e.Timestamp = timestamp;
    e.FrameType = frameType;
    mPendingFlipFrameTypeEvents.emplace(vidPnLayerId, e);
}

void PMTraceConsumer::ApplyFlipFrameType(
    std::shared_ptr<PresentEvent> const& present,
    uint64_t timestamp,
    FrameType frameType)
{
    DebugAssert(frameType != FrameType::NotSet);

    for (auto p2 : present->DependentPresents) {
        if (p2->FinalState != PresentResult::Discarded) {
            VerboseTraceBeforeModifyingPresent(p2.get());
            SetScreenTime(p2, timestamp, frameType);
        }
    }

    VerboseTraceBeforeModifyingPresent(present.get());
    SetScreenTime(present, timestamp, frameType);
}

void PMTraceConsumer::ApplyPresentFrameType(
    std::shared_ptr<PresentEvent> const& present)
{
    DebugAssert(present->Displayed.empty());

    auto ii = mPendingPresentFrameTypeEvents.find(present->ThreadId);
    if (ii != mPendingPresentFrameTypeEvents.end()) {
        present->FrameId = ii->second.FrameId;
        present->AppFrameId = ii->second.AppFrameId;
        present->Displayed.emplace_back(ii->second.FrameType, 0);
        present->WaitingForFrameId = true;
        mPendingPresentFrameTypeEvents.erase(ii);
    }
}

// TODO: consider separating process and present events, would reduce unneccessary mutex locking
void PMTraceConsumer::SignalEventsReady()
{
    SetEvent(hEventsReadyEvent);
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

void PMTraceConsumer::DequeueProcessEvents(std::vector<ProcessEvent>& outProcessEvents)
{
    std::lock_guard<std::mutex> lock(mProcessEventMutex);
    outProcessEvents.swap(mProcessEvents);
}

void PMTraceConsumer::DequeuePresentEvents(std::vector<std::shared_ptr<PresentEvent>>& outPresentEvents)
{
    outPresentEvents.clear();
    {
        std::lock_guard<std::mutex> lock(mPresentEventMutex);
        if (mReadyCount > 0) {
            outPresentEvents.resize(mReadyCount, nullptr);
            for (uint32_t i = 0; i < mReadyCount; ++i) {
                std::swap(outPresentEvents[i], mCompletedPresents[mCompletedIndex]);
                mCompletedIndex = GetRingIndex(mCompletedIndex + 1);
            }

            mCompletedCount -= mReadyCount;
            mReadyCount = 0;
        }
    }
    if (!mIsRealtimeSession && !mDisableOfflineBackpressure) {
        mCompletedRingCondition.notify_one();
    }
}
AppTimingData* PMTraceConsumer::ExtractAppTimingData(
    std::unordered_map<std::pair<uint32_t, uint32_t>, AppTimingData, PairHash<uint32_t, uint32_t>>& timingDataByFrameId,
    uint32_t processId, uint32_t appFrameId, uint64_t presentStartTime, std::function<uint64_t(const AppTimingData&)> timingSelector) {
    if (appFrameId != 0) {
        // If the incoming app frame id is already assigned then we receiving
        // the information from version 2 of the PresentFrameType event.
        auto key = std::make_pair(appFrameId, processId);
        auto ii = timingDataByFrameId.find(key);
        if (ii != timingDataByFrameId.end()) {
            ii->second.AssignedToPresent = true;
            ii->second.PresentCompleted = false;
            return &ii->second;
        }
    } else {
        auto itToReturn = timingDataByFrameId.end();
        uint32_t earlierFrameId = std::numeric_limits<uint32_t>::max();
        uint64_t smallestPresentStartDelta = std::numeric_limits<uint64_t>::max();
        // Search for the timing data with the closest PresentStartTime to the passed
        // in PresentStartTime that has not been assigned. This is a hack and will not work for x-platform.
        for (auto it = timingDataByFrameId.begin(); it != timingDataByFrameId.end(); ++it) {
            if (it->first.second == processId) {
                if (it->second.AssignedToPresent == false) {
                    uint64_t timingValue = timingSelector(it->second);
                    if (timingValue != 0 && timingValue < presentStartTime) {
                        auto tempPresentStartDelta = presentStartTime - timingValue;
                        if (tempPresentStartDelta < smallestPresentStartDelta) {
                            smallestPresentStartDelta = tempPresentStartDelta;
                            earlierFrameId = it->first.first;
                            itToReturn = it;
                        }
                    }
                }
            }
        }

        // If we found the app timing data and if the Present it is attached is not completed return
        // the data
        if (itToReturn != timingDataByFrameId.end() && itToReturn->second.PresentCompleted != true) {
            itToReturn->second.AssignedToPresent = true;
            return &itToReturn->second;
        }
    }

    return nullptr; // No frame found
}

bool PMTraceConsumer::IsApplicationPresent(std::shared_ptr<PresentEvent> const& present) {
    DebugAssert(present->Displayed.size() <= 1);
    if (present->Displayed.empty()) {
        return true;
    }

    auto frameType = present->Displayed[0];
    return ((frameType.first != FrameType::Intel_XEFG) && (frameType.first != FrameType::AMD_AFMF));
}

void PMTraceConsumer::SetAppTimingDataAsComplete(uint32_t processId, uint32_t appFrameId) {
    auto key = std::make_pair(appFrameId, processId);
    auto ii = mAppTimingDataByAppFrameId.find(key);
    if (ii != mAppTimingDataByAppFrameId.end()) {
        DebugAssert(ii->second.ProcessId == processId);
        ii->second.PresentCompleted = true;
    }
}