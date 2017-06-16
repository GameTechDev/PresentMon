/*
Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <algorithm>
#include <d3d9.h>
#include <dxgi.h>

#include "PresentMonTraceConsumer.hpp"
#include "TraceConsumer.hpp"

PresentEvent::PresentEvent(EVENT_HEADER const& hdr, ::Runtime runtime)
    : QpcTime(*(uint64_t*) &hdr.TimeStamp)
    , SwapChainAddress(0)
    , SyncInterval(-1)
    , PresentFlags(0)
    , ProcessId(hdr.ProcessId)
    , PresentMode(PresentMode::Unknown)
    , SupportsTearing(false)
    , MMIO(false)
    , SeenDxgkPresent(false)
    , WasBatched(false)
    , Runtime(runtime)
    , TimeTaken(0)
    , ReadyTime(0)
    , ScreenTime(0)
    , FinalState(PresentResult::Unknown)
    , PlaneIndex(0)
    , QueueSubmitSequence(0)
    , RuntimeThread(hdr.ThreadId)
    , Hwnd(0)
    , Completed(false)
{
}

#ifndef NDEBUG
static bool gPresentMonTraceConsumer_Exiting = false;
PresentEvent::~PresentEvent()
{
    assert(Completed || gPresentMonTraceConsumer_Exiting);
}

PMTraceConsumer::~PMTraceConsumer()
{
    gPresentMonTraceConsumer_Exiting = true;
}
#endif

void HandleDXGIEvent(EVENT_RECORD* pEventRecord, PMTraceConsumer* pmConsumer)
{
    enum {
        DXGIPresent_Start = 42,
        DXGIPresent_Stop,
        DXGIPresentMPO_Start = 55,
        DXGIPresentMPO_Stop = 56,
    };

    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id)
    {
    case DXGIPresent_Start:
    case DXGIPresentMPO_Start:
    {
        PresentEvent event(hdr, Runtime::DXGI);
        GetEventData(pEventRecord, L"pIDXGISwapChain", &event.SwapChainAddress);
        GetEventData(pEventRecord, L"Flags",           &event.PresentFlags);
        GetEventData(pEventRecord, L"SyncInterval",    &event.SyncInterval);

        pmConsumer->RuntimePresentStart(event);
        break;
    }
    case DXGIPresent_Stop:
    case DXGIPresentMPO_Stop:
    {
        auto result = GetEventData<uint32_t>(pEventRecord, L"Result");
        bool AllowBatching = SUCCEEDED(result) && result != DXGI_STATUS_OCCLUDED && result != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS && result != DXGI_STATUS_NO_DESKTOP_ACCESS;
        pmConsumer->RuntimePresentStop(hdr, AllowBatching);
        break;
    }
    }
}

void HandleDXGKEvent(EVENT_RECORD* pEventRecord, PMTraceConsumer* pmConsumer)
{
    enum {
        DxgKrnl_Flip = 168,
        DxgKrnl_FlipMPO = 252,
        DxgKrnl_QueueSubmit = 178,
        DxgKrnl_QueueComplete = 180,
        DxgKrnl_MMIOFlip = 116,
        DxgKrnl_MMIOFlipMPO = 259,
        DxgKrnl_VSyncDPC = 17,
        DxgKrnl_Present = 184,
        DxgKrnl_PresentHistoryDetailed = 215,
        DxgKrnl_SubmitPresentHistory = 171,
        DxgKrnl_PropagatePresentHistory = 172,
        DxgKrnl_Blit = 166,
    };

    auto const& hdr = pEventRecord->EventHeader;

    uint64_t EventTime = *(uint64_t*)&hdr.TimeStamp;

    switch (hdr.EventDescriptor.Id)
    {
    case DxgKrnl_Flip:
    case DxgKrnl_FlipMPO:
    {
        // A flip event is emitted during fullscreen present submission.
        // Afterwards, expect an MMIOFlip packet on the same thread, used
        // to trace the flip to screen.
        auto eventIter = pmConsumer->FindOrCreatePresent(hdr);

        // Check if we might have retrieved a 'stuck' present from a previous frame.
        // The only events that we can expect before a Flip/FlipMPO are a runtime present start, or a previous FlipMPO.
        if (eventIter->second->QueueSubmitSequence != 0 || eventIter->second->SeenDxgkPresent) {
            // It's already progressed further but didn't complete, ignore it and create a new one.
            pmConsumer->mPresentByThreadId.erase(eventIter);
            eventIter = pmConsumer->FindOrCreatePresent(hdr);
        }

        if (eventIter->second->PresentMode != PresentMode::Unknown) {
            // For MPO, N events may be issued, but we only care about the first
            return;
        }

        eventIter->second->PresentMode = PresentMode::Hardware_Legacy_Flip;
        if (hdr.EventDescriptor.Id == DxgKrnl_Flip) {
            if (eventIter->second->Runtime != Runtime::DXGI) {
                // Only DXGI gives us the sync interval in the runtime present start event
                GetEventData(pEventRecord, L"FlipInterval", &eventIter->second->SyncInterval);
            }

            eventIter->second->MMIO = GetEventData<BOOL>(pEventRecord, L"MMIOFlip") != 0;
        }
        else {
            eventIter->second->MMIO = true; // All MPO flips are MMIO
        }

        // If this is the DWM thread, piggyback these pending presents on our fullscreen present
        if (hdr.ThreadId == pmConsumer->DwmPresentThreadId) {
            std::swap(eventIter->second->DependentPresents, pmConsumer->mPresentsWaitingForDWM);
            pmConsumer->DwmPresentThreadId = 0;
        }
        break;
    }
    case DxgKrnl_QueueSubmit:
    {
        // A QueueSubmit can be many types, but these are interesting for present.
        // This event is emitted after a flip/blt/PHT event, and may be the only way
        // to trace completion of the present.
        enum class DxgKrnl_QueueSubmit_Type {
            MMIOFlip = 3,
            Software = 7,
        };
        auto Type = GetEventData<DxgKrnl_QueueSubmit_Type>(pEventRecord, L"PacketType");
        auto SubmitSequence = GetEventData<uint32_t>(pEventRecord, L"SubmitSequence");
        bool Present = GetEventData<BOOL>(pEventRecord, L"bPresent") != 0;

        if (Type == DxgKrnl_QueueSubmit_Type::MMIOFlip ||
            Type == DxgKrnl_QueueSubmit_Type::Software ||
            Present) {
            auto eventIter = pmConsumer->mPresentByThreadId.find(hdr.ThreadId);
            if (eventIter == pmConsumer->mPresentByThreadId.end()) {
                return;
            }

            if (eventIter->second->QueueSubmitSequence == 0) {
                eventIter->second->QueueSubmitSequence = SubmitSequence;
                pmConsumer->mPresentsBySubmitSequence.emplace(SubmitSequence, eventIter->second);
            }
        }

        break;
    }
    case DxgKrnl_QueueComplete:
    {
        auto SubmitSequence = GetEventData<uint32_t>(pEventRecord, L"SubmitSequence");
        auto eventIter = pmConsumer->mPresentsBySubmitSequence.find(SubmitSequence);
        if (eventIter == pmConsumer->mPresentsBySubmitSequence.end()) {
            return;
        }

        auto pEvent = eventIter->second;

        if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer ||
            (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip && !pEvent->MMIO)) {
            pEvent->ScreenTime = pEvent->ReadyTime = EventTime;
            pEvent->FinalState = PresentResult::Presented;

            // Sometimes, the queue packets associated with a present will complete before the DxgKrnl present event is fired
            // In this case, for blit presents, we have no way to differentiate between fullscreen and windowed blits
            // So, defer the completion of this present until we know all events have been fired
            if (pEvent->SeenDxgkPresent) {
                pmConsumer->CompletePresent(pEvent);
            }
        }
        break;
    }
    case DxgKrnl_MMIOFlip:
    {
        // An MMIOFlip event is emitted when an MMIOFlip packet is dequeued.
        // This corresponds to all GPU work prior to the flip being completed
        // (i.e. present "ready")
        // It also is emitted when an independent flip PHT is dequed,
        // and will tell us whether the present is immediate or vsync.
        enum DxgKrnl_MMIOFlip_Flags {
            FlipImmediate = 0x2,
            FlipOnNextVSync = 0x4
        };

        auto FlipSubmitSequence = GetEventData<uint32_t>(pEventRecord, L"FlipSubmitSequence");
        auto Flags = GetEventData<DxgKrnl_MMIOFlip_Flags>(pEventRecord, L"Flags");

        auto eventIter = pmConsumer->mPresentsBySubmitSequence.find(FlipSubmitSequence);
        if (eventIter == pmConsumer->mPresentsBySubmitSequence.end()) {
            return;
        }

        eventIter->second->ReadyTime = EventTime;
        if (eventIter->second->PresentMode == PresentMode::Composed_Flip) {
            eventIter->second->PresentMode = PresentMode::Hardware_Independent_Flip;
        }

        if (Flags & DxgKrnl_MMIOFlip_Flags::FlipImmediate) {
            eventIter->second->FinalState = PresentResult::Presented;
            eventIter->second->ScreenTime = *(uint64_t*)&pEventRecord->EventHeader.TimeStamp;
            eventIter->second->SupportsTearing = true;
            if (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Flip) {
                pmConsumer->CompletePresent(eventIter->second);
            }
        }

        break;
    }
    case DxgKrnl_MMIOFlipMPO:
    {
        // See above for more info about this packet.
        // Note: this packet currently does not support immediate flips
        auto FlipFenceId = GetEventData<uint64_t>(pEventRecord, L"FlipSubmitSequence");
        uint32_t FlipSubmitSequence = (uint32_t)(FlipFenceId >> 32u);

        auto eventIter = pmConsumer->mPresentsBySubmitSequence.find(FlipSubmitSequence);
        if (eventIter == pmConsumer->mPresentsBySubmitSequence.end()) {
            return;
        }

        // Avoid double-marking a single present packet coming from the MPO API
        if (eventIter->second->ReadyTime == 0) {
            eventIter->second->ReadyTime = EventTime;
            eventIter->second->PlaneIndex = GetEventData<uint32_t>(pEventRecord, L"LayerIndex");
        }

        if (eventIter->second->PresentMode == PresentMode::Hardware_Independent_Flip ||
            eventIter->second->PresentMode == PresentMode::Composed_Flip) {
            eventIter->second->PresentMode = PresentMode::Hardware_Composed_Independent_Flip;
        }

        if (hdr.EventDescriptor.Version >= 2)
        {
            enum class DxgKrnl_MMIOFlipMPO_FlipEntryStatus {
                FlipWaitVSync = 5,
                FlipWaitComplete = 11,
                // There are others, but they're more complicated to deal with.
            };

            auto FlipEntryStatusAfterFlip = GetEventData<DxgKrnl_MMIOFlipMPO_FlipEntryStatus>(pEventRecord, L"FlipEntryStatusAfterFlip");
            if (FlipEntryStatusAfterFlip != DxgKrnl_MMIOFlipMPO_FlipEntryStatus::FlipWaitVSync) {
                eventIter->second->FinalState = PresentResult::Presented;
                eventIter->second->SupportsTearing = true;
                if (FlipEntryStatusAfterFlip == DxgKrnl_MMIOFlipMPO_FlipEntryStatus::FlipWaitComplete) {
                    eventIter->second->ScreenTime = *(uint64_t*)&pEventRecord->EventHeader.TimeStamp;
                }
                if (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Flip) {
                    pmConsumer->CompletePresent(eventIter->second);
                }
            }
        }

        break;
    }
    case DxgKrnl_VSyncDPC:
    {
        // The VSyncDPC contains a field telling us what flipped to screen.
        // This is the way to track completion of a fullscreen present.
        auto FlipFenceId = GetEventData<uint64_t>(pEventRecord, L"FlipFenceId");

        uint32_t FlipSubmitSequence = (uint32_t)(FlipFenceId >> 32u);
        auto eventIter = pmConsumer->mPresentsBySubmitSequence.find(FlipSubmitSequence);
        if (eventIter == pmConsumer->mPresentsBySubmitSequence.end()) {
            return;
        }

        eventIter->second->ScreenTime = EventTime;
        eventIter->second->FinalState = PresentResult::Presented;
        if (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Flip) {
            pmConsumer->CompletePresent(eventIter->second);
        }
        break;
    }
    case DxgKrnl_Present:
    {
        enum class DxgKrnl_Present_Flags {
            RedirectedBlt = 0x00010000,
        };

        // This event is emitted at the end of the kernel present, before returning.
        // All other events have already been logged, but this one contains one
        // extra piece of useful information: the hWnd that a present targeted,
        // used to determine when presents are discarded instead of composed.
        auto eventIter = pmConsumer->mPresentByThreadId.find(hdr.ThreadId);
        if (eventIter == pmConsumer->mPresentByThreadId.end()) {
            return;
        }

        auto hWnd = GetEventData<uint64_t>(pEventRecord, L"hWindow");
        auto Flags = GetEventData<uint32_t>(pEventRecord, L"Flags");
        if ((Flags & (uint32_t)DxgKrnl_Present_Flags::RedirectedBlt) != 0 &&
            eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
            // The present history token got dropped for some reason. Discard this present.
            eventIter->second->PresentMode = PresentMode::Unknown;
            eventIter->second->FinalState = PresentResult::Discarded;
            pmConsumer->CompletePresent(eventIter->second);
        }

        // For all other events, just remember the hWnd, we might need it later
        eventIter->second->Hwnd = hWnd;
        eventIter->second->SeenDxgkPresent = true;

        if ((eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer ||
            (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Flip && !eventIter->second->MMIO)) &&
            eventIter->second->ScreenTime != 0) {
            // This is a fullscreen blit where all work associated was already done, so it's on-screen
            // It was deferred to here because there was no way to be sure it was really fullscreen until now
            pmConsumer->CompletePresent(eventIter->second);
        }

        if (eventIter->second->RuntimeThread != hdr.ThreadId) {
            if (eventIter->second->TimeTaken == 0) {
                eventIter->second->TimeTaken = EventTime - eventIter->second->QpcTime;
            }
            eventIter->second->WasBatched = true;
            pmConsumer->mPresentByThreadId.erase(eventIter);
        }
        break;
    }
    case DxgKrnl_PresentHistoryDetailed:
    {
        // This event is emitted during submission of most windowed presents.
        // In the case of flip and blit model, it is used to find a key to watch for the
        // event which triggers the "ready" state.
        // In the case of blit model, it is also used to distinguish between fs/windowed.
        auto eventIter = pmConsumer->mPresentByThreadId.find(hdr.ThreadId);
        if (eventIter == pmConsumer->mPresentByThreadId.end()) {
            return;
        }

        if (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
            eventIter->second->PresentMode = PresentMode::Composed_Copy_GPU_GDI;
            eventIter->second->SupportsTearing = false;
            // Overwrite some fields that may have been filled out while we thought it was fullscreen
            assert(!eventIter->second->Completed);
            eventIter->second->ReadyTime = eventIter->second->ScreenTime = 0;
            eventIter->second->FinalState = PresentResult::Unknown;
        }
        uint64_t TokenPtr = GetEventData<uint64_t>(pEventRecord, L"Token");
        pmConsumer->mDxgKrnlPresentHistoryTokens[TokenPtr] = eventIter->second;
        break;
    }
    case DxgKrnl_SubmitPresentHistory:
    {
        // This event is emitted during submission of other types of windowed presents.
        // It gives us up to two different types of keys to correlate further.
        auto eventIter = pmConsumer->FindOrCreatePresent(hdr);

        // Check if we might have retrieved a 'stuck' present from a previous frame.
        // This event always results in a classification, though for blts it's a clarifying classification.
        if (eventIter->second->PresentMode != PresentMode::Unknown &&
            eventIter->second->PresentMode != PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
            // It's already progressed further but didn't complete, ignore it and create a new one.
            pmConsumer->mPresentByThreadId.erase(eventIter);
            eventIter = pmConsumer->FindOrCreatePresent(hdr);
        }

        if (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
            auto TokenData = GetEventData<uint64_t>(pEventRecord, L"TokenData");
            pmConsumer->mPresentsByLegacyBlitToken[TokenData] = eventIter->second;

            eventIter->second->ReadyTime = EventTime;
            eventIter->second->PresentMode = PresentMode::Composed_Copy_CPU_GDI;
            eventIter->second->SupportsTearing = false;
        }
        else if (eventIter->second->PresentMode == PresentMode::Unknown) {
            enum class TokenModel {
                Composition = 7,
            };

            auto Model = GetEventData<TokenModel>(pEventRecord, L"Model");
            if (Model == TokenModel::Composition) {
                eventIter->second->PresentMode = PresentMode::Composed_Composition_Atlas;
                uint64_t TokenPtr = GetEventData<uint64_t>(pEventRecord, L"Token");
                pmConsumer->mDxgKrnlPresentHistoryTokens[TokenPtr] = eventIter->second;
            }
        }

        if (eventIter->second->Runtime == Runtime::Other ||
            eventIter->second->PresentMode == PresentMode::Composed_Composition_Atlas)
        {
            // We're not expecting any other events from this thread (no DxgKrnl Present or EndPresent runtime event)
            pmConsumer->mPresentByThreadId.erase(eventIter);
        }
        break;
    }
    case DxgKrnl_PropagatePresentHistory:
    {
        // This event is emitted when a token is being handed off to DWM, and is a good way to indicate a ready state
        uint64_t TokenPtr = GetEventData<uint64_t>(pEventRecord, L"Token");
        auto eventIter = pmConsumer->mDxgKrnlPresentHistoryTokens.find(TokenPtr);
        if (eventIter == pmConsumer->mDxgKrnlPresentHistoryTokens.end()) {
            return;
        }

        auto& ReadyTime = eventIter->second->ReadyTime;
        ReadyTime = (ReadyTime == 0 ?
            EventTime : min(ReadyTime, EventTime));

        if (eventIter->second->PresentMode == PresentMode::Composed_Composition_Atlas) {
            pmConsumer->mPresentsWaitingForDWM.emplace_back(eventIter->second);
        }

        if (eventIter->second->PresentMode == PresentMode::Composed_Copy_GPU_GDI) {
            // Manipulate the map here
            // When DWM is ready to present, we'll query for the most recent blt targeting this window and take it out of the map
            pmConsumer->mPresentByWindow[eventIter->second->Hwnd] = eventIter->second;
        }

        pmConsumer->mDxgKrnlPresentHistoryTokens.erase(eventIter);
        break;
    }
    case DxgKrnl_Blit:
    {
        auto eventIter = pmConsumer->FindOrCreatePresent(hdr);

        // Check if we might have retrieved a 'stuck' present from a previous frame.
        // If the present mode isn't unknown at this point, we've already seen this present progress further
        if (eventIter->second->PresentMode != PresentMode::Unknown) {
            pmConsumer->mPresentByThreadId.erase(eventIter);
            eventIter = pmConsumer->FindOrCreatePresent(hdr);
        }

        eventIter->second->PresentMode = PresentMode::Hardware_Legacy_Copy_To_Front_Buffer;
        eventIter->second->SupportsTearing = true;
        break;
    }
    }
}

void HandleWin32kEvent(EVENT_RECORD* pEventRecord, PMTraceConsumer* pmConsumer)
{
    enum {
        Win32K_TokenCompositionSurfaceObject = 201,
        Win32K_TokenStateChanged = 301,
    };

    auto const& hdr = pEventRecord->EventHeader;

    uint64_t EventTime = *(uint64_t*)&hdr.TimeStamp;

    switch (hdr.EventDescriptor.Id)
    {
    case Win32K_TokenCompositionSurfaceObject:
    {
        auto eventIter = pmConsumer->FindOrCreatePresent(hdr);

        // Check if we might have retrieved a 'stuck' present from a previous frame.
        // If the present mode isn't unknown at this point, we've already seen this present progress further
        if (eventIter->second->PresentMode != PresentMode::Unknown) {
            pmConsumer->mPresentByThreadId.erase(eventIter);
            eventIter = pmConsumer->FindOrCreatePresent(hdr);
        }

        eventIter->second->PresentMode = PresentMode::Composed_Flip;

        PMTraceConsumer::Win32KPresentHistoryTokenKey key(GetEventData<uint64_t>(pEventRecord, L"pCompositionSurfaceObject"),
            GetEventData<uint64_t>(pEventRecord, L"PresentCount"),
            GetEventData<uint32_t>(pEventRecord, L"SwapChainIndex"));
        pmConsumer->mWin32KPresentHistoryTokens[key] = eventIter->second;
        break;
    }
    case Win32K_TokenStateChanged:
    {
        PMTraceConsumer::Win32KPresentHistoryTokenKey key(GetEventData<uint64_t>(pEventRecord, L"pCompositionSurfaceObject"),
            GetEventData<uint32_t>(pEventRecord, L"PresentCount"),
            GetEventData<uint32_t>(pEventRecord, L"SwapChainIndex"));
        auto eventIter = pmConsumer->mWin32KPresentHistoryTokens.find(key);
        if (eventIter == pmConsumer->mWin32KPresentHistoryTokens.end()) {
            return;
        }

        enum class TokenState {
            InFrame = 3,
            Confirmed = 4,
            Retired = 5,
            Discarded = 6,
        };

        auto &event = *eventIter->second;
        auto state = GetEventData<TokenState>(pEventRecord, L"NewState");
        switch (state)
        {
        case TokenState::InFrame:
        {
            // InFrame = composition is starting
            if (event.Hwnd) {
                auto hWndIter = pmConsumer->mPresentByWindow.find(event.Hwnd);
                if (hWndIter == pmConsumer->mPresentByWindow.end()) {
                    pmConsumer->mPresentByWindow.emplace(event.Hwnd, eventIter->second);
                }
                else if (hWndIter->second != eventIter->second) {
                    hWndIter->second->FinalState = PresentResult::Discarded;
                    hWndIter->second = eventIter->second;
                }
            }

            bool iFlip = GetEventData<BOOL>(pEventRecord, L"IndependentFlip") != 0;
            if (iFlip && event.PresentMode == PresentMode::Composed_Flip) {
                event.PresentMode = PresentMode::Hardware_Independent_Flip;
            }

            break;
        }
        case TokenState::Confirmed:
        {
            // Confirmed = present has been submitted
            // If we haven't already decided we're going to discard a token, now's a good time to indicate it'll make it to screen
            if (event.FinalState == PresentResult::Unknown) {
                if (event.PresentFlags & DXGI_PRESENT_DO_NOT_SEQUENCE) {
                    // DO_NOT_SEQUENCE presents may get marked as confirmed,
                    // if a frame was composed when this token was completed
                    event.FinalState = PresentResult::Discarded;
                }
                else {
                    event.FinalState = PresentResult::Presented;
                }
            }
            if (event.Hwnd) {
                pmConsumer->mPresentByWindow.erase(event.Hwnd);
            }
            break;
        }
        case TokenState::Retired:
        {
            // Retired = present has been completed, token's buffer is now displayed
            event.ScreenTime = EventTime;
            break;
        }
        case TokenState::Discarded:
        {
            // Discarded = destroyed - discard if we never got any indication that it was going to screen
            auto sharedPtr = eventIter->second;
            pmConsumer->mWin32KPresentHistoryTokens.erase(eventIter);

            if (event.FinalState == PresentResult::Unknown || event.ScreenTime == 0) {
                event.FinalState = PresentResult::Discarded;
            }

            pmConsumer->CompletePresent(sharedPtr);
            break;
        }
        }
        break;
    }
    }
}

void HandleDWMEvent(EVENT_RECORD* pEventRecord, PMTraceConsumer* pmConsumer)
{
    enum {
        DWM_DwmUpdateWindow = 46,
        DWM_Schedule_Present_Start = 15,
        DWM_FlipChain_Pending = 69,
        DWM_FlipChain_Complete = 70,
        DWM_FlipChain_Dirty = 101,
    };

    auto& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id)
    {
    case DWM_DwmUpdateWindow:
    {
        auto hWnd = (uint32_t)GetEventData<uint64_t>(pEventRecord, L"hWnd");

        // Piggyback on the next DWM present
        pmConsumer->mWindowsBeingComposed.insert(hWnd);
        break;
    }
    case DWM_Schedule_Present_Start:
    {
        pmConsumer->DwmPresentThreadId = hdr.ThreadId;
        for (auto hWnd : pmConsumer->mWindowsBeingComposed)
        {
            // Pickup the most recent present from a given window
            auto hWndIter = pmConsumer->mPresentByWindow.find(hWnd);
            if (hWndIter != pmConsumer->mPresentByWindow.end()) {
                if (hWndIter->second->PresentMode != PresentMode::Composed_Copy_GPU_GDI &&
                    hWndIter->second->PresentMode != PresentMode::Composed_Copy_CPU_GDI) {
                    continue;
                }
                pmConsumer->mPresentsWaitingForDWM.emplace_back(hWndIter->second);
                pmConsumer->mPresentByWindow.erase(hWndIter);
            }
        }
        pmConsumer->mWindowsBeingComposed.clear();
        break;
    }
    case DWM_FlipChain_Pending:
    case DWM_FlipChain_Complete:
    case DWM_FlipChain_Dirty:
    {
        // As it turns out, the 64-bit token data from the PHT submission is actually two 32-bit data chunks,
        // corresponding to a "flip chain" id and present id
        uint32_t flipChainId = (uint32_t)GetEventData<uint64_t>(pEventRecord, L"ulFlipChain");
        uint32_t serialNumber = (uint32_t)GetEventData<uint64_t>(pEventRecord, L"ulSerialNumber");
        uint64_t token = ((uint64_t)flipChainId << 32ull) | serialNumber;
        auto flipIter = pmConsumer->mPresentsByLegacyBlitToken.find(token);
        if (flipIter == pmConsumer->mPresentsByLegacyBlitToken.end()) {
            return;
        }

        // Watch for multiple legacy blits completing against the same window
        auto hWnd = (uint32_t)GetEventData<uint64_t>(pEventRecord, L"hwnd");
        pmConsumer->mPresentByWindow[hWnd] = flipIter->second;

        pmConsumer->mPresentsByLegacyBlitToken.erase(flipIter);
        pmConsumer->mWindowsBeingComposed.insert(hWnd);
        break;
    }
    }
}

void HandleD3D9Event(EVENT_RECORD* pEventRecord, PMTraceConsumer* pmConsumer)
{
    enum {
        D3D9PresentStart = 1,
        D3D9PresentStop,
    };

    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id)
    {
    case D3D9PresentStart:
    {
        PresentEvent event(hdr, Runtime::D3D9);
        GetEventData(pEventRecord, L"pSwapchain", &event.SwapChainAddress);
        uint32_t D3D9Flags = GetEventData<uint32_t>(pEventRecord, L"Flags");
        event.PresentFlags =
            ((D3D9Flags & D3DPRESENT_DONOTFLIP) ? DXGI_PRESENT_DO_NOT_SEQUENCE : 0) |
            ((D3D9Flags & D3DPRESENT_DONOTWAIT) ? DXGI_PRESENT_DO_NOT_WAIT : 0) |
            ((D3D9Flags & D3DPRESENT_FLIPRESTART) ? DXGI_PRESENT_RESTART : 0);
        if ((D3D9Flags & D3DPRESENT_FORCEIMMEDIATE) != 0) {
            event.SyncInterval = 0;
        }

        pmConsumer->RuntimePresentStart(event);
        break;
    }
    case D3D9PresentStop:
    {
        auto result = GetEventData<uint32_t>(pEventRecord, L"Result");
        bool AllowBatching = SUCCEEDED(result) && result != S_PRESENT_OCCLUDED;
        pmConsumer->RuntimePresentStop(hdr, AllowBatching);
        break;
    }
    }
}

void PMTraceConsumer::CompletePresent(std::shared_ptr<PresentEvent> p)
{
    if (p->Completed)
    {
        p->FinalState = PresentResult::Error;
        return;
    }

    // Complete all other presents that were riding along with this one (i.e. this one came from DWM)
    for (auto& p2 : p->DependentPresents) {
        p2->ScreenTime = p->ScreenTime;
        p2->FinalState = PresentResult::Presented;
        CompletePresent(p2);
    }
    p->DependentPresents.clear();

    // Remove it from any tracking maps that it may have been inserted into
    if (p->QueueSubmitSequence != 0) {
        mPresentsBySubmitSequence.erase(p->QueueSubmitSequence);
    }
    if (p->Hwnd != 0) {
        auto hWndIter = mPresentByWindow.find(p->Hwnd);
        if (hWndIter != mPresentByWindow.end() && hWndIter->second == p) {
            mPresentByWindow.erase(hWndIter);
        }
    }
    auto& processMap = mPresentsByProcess[p->ProcessId];
    processMap.erase(p->QpcTime);

    auto& presentDeque = mPresentsByProcessAndSwapChain[std::make_tuple(p->ProcessId, p->SwapChainAddress)];
    auto presentIter = presentDeque.begin();
    assert(!presentIter->get()->Completed); // It wouldn't be here anymore if it was

    if (p->FinalState == PresentResult::Presented) {
        while (*presentIter != p) {
            CompletePresent(*presentIter);
            presentIter = presentDeque.begin();
        }
    }

    p->Completed = true;
    if (*presentIter == p) {
        auto lock = scoped_lock(mMutex);
        while (presentIter != presentDeque.end() && presentIter->get()->Completed) {
            mCompletedPresents.push_back(*presentIter);
            presentDeque.pop_front();
            presentIter = presentDeque.begin();
        }
    }

}

decltype(PMTraceConsumer::mPresentByThreadId.begin()) PMTraceConsumer::FindOrCreatePresent(EVENT_HEADER const& hdr)
{
    // Easy: we're on a thread that had some step in the present process
    auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (eventIter != mPresentByThreadId.end()) {
        return eventIter;
    }

    // No such luck, check for batched presents
    auto& processMap = mPresentsByProcess[hdr.ProcessId];
    auto processIter = std::find_if(processMap.begin(), processMap.end(),
        [](auto processIter) {return processIter.second->PresentMode == PresentMode::Unknown; });
    if (processIter == processMap.end()) {
        // This likely didn't originate from a runtime whose events we're tracking (DXGI/D3D9)
        // Could be composition buffers, or maybe another runtime (e.g. GL)
        auto newEvent = std::make_shared<PresentEvent>(hdr, Runtime::Other);
        processMap.emplace(newEvent->QpcTime, newEvent);

        auto& processSwapChainDeque = mPresentsByProcessAndSwapChain[std::make_tuple(hdr.ProcessId, 0ull)];
        processSwapChainDeque.emplace_back(newEvent);

        eventIter = mPresentByThreadId.emplace(hdr.ThreadId, newEvent).first;
    }
    else {
        // Assume batched presents are popped off the front of the driver queue by process in order, do the same here
        eventIter = mPresentByThreadId.emplace(hdr.ThreadId, processIter->second).first;
        processMap.erase(processIter);
    }

    return eventIter;
}

void PMTraceConsumer::RuntimePresentStart(PresentEvent &event)
{
    // Ignore PRESENT_TEST: it's just to check if you're still fullscreen, doesn't actually do anything
    if ((event.PresentFlags & DXGI_PRESENT_TEST) != 0) {
        event.Completed = true;
        return;
    }

    auto pEvent = std::make_shared<PresentEvent>(event);
    mPresentByThreadId[event.RuntimeThread] = pEvent;

    auto& processMap = mPresentsByProcess[event.ProcessId];
    processMap.emplace(event.QpcTime, pEvent);

    auto& processSwapChainDeque = mPresentsByProcessAndSwapChain[std::make_tuple(event.ProcessId, event.SwapChainAddress)];
    processSwapChainDeque.emplace_back(pEvent);

    // Set the caller's local event instance to completed so the assert
    // in ~PresentEvent() doesn't fire when it is destructed.
    event.Completed = true;
}

void PMTraceConsumer::RuntimePresentStop(EVENT_HEADER const& hdr, bool AllowPresentBatching)
{
    auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (eventIter == mPresentByThreadId.end()) {
        return;
    }
    auto &event = *eventIter->second;

    assert(event.QpcTime <= *(uint64_t*) &hdr.TimeStamp);
    event.TimeTaken = *(uint64_t*) &hdr.TimeStamp - event.QpcTime;

    if (!AllowPresentBatching || mSimpleMode) {
        event.FinalState = AllowPresentBatching ? PresentResult::Presented : PresentResult::Discarded;
        CompletePresent(eventIter->second);
    }

    mPresentByThreadId.erase(eventIter);
}

void HandleNTProcessEvent(PEVENT_RECORD pEventRecord, PMTraceConsumer* pmConsumer)
{
    NTProcessEvent event;

    switch (pEventRecord->EventHeader.EventDescriptor.Opcode) {
    case EVENT_TRACE_TYPE_START:
    case EVENT_TRACE_TYPE_DC_START:
        GetEventData(pEventRecord, L"ProcessId",     &event.ProcessId);
        GetEventData(pEventRecord, L"ImageFileName", &event.ImageFileName);
        break;

    case EVENT_TRACE_TYPE_END:
    case EVENT_TRACE_TYPE_DC_END:
        GetEventData(pEventRecord, L"ProcessId", &event.ProcessId);
        break;
    }

    {
        auto lock = scoped_lock(pmConsumer->mNTProcessEventMutex);
        pmConsumer->mNTProcessEvents.emplace_back(event);
    }
}

