/*
Copyright 2017-2019 Intel Corporation

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

#pragma once

#define NOMINMAX

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <tuple>
#include <vector>
#include <windows.h>
#include <evntcons.h> // must include after windows.h

#include "Debug.hpp"
#include "TraceConsumer.hpp"

template <typename mutex_t> std::unique_lock<mutex_t> scoped_lock(mutex_t &m)
{
    return std::unique_lock<mutex_t>(m);
}

enum class PresentMode
{
    Unknown,
    Hardware_Legacy_Flip,
    Hardware_Legacy_Copy_To_Front_Buffer,
    /* Not detected:
    Hardware_Direct_Flip,
    */
    Hardware_Independent_Flip,
    Composed_Flip,
    Composed_Copy_GPU_GDI,
    Composed_Copy_CPU_GDI,
    Composed_Composition_Atlas,
    Hardware_Composed_Independent_Flip,
};

enum class PresentResult
{
    Unknown, Presented, Discarded, Error
};

enum class Runtime
{
    DXGI, D3D9, Other
};

struct NTProcessEvent {
    std::string ImageFileName;  // If ImageFileName.empty(), then event is that process ending
    uint64_t QpcTime;
    uint32_t ProcessId;
};

struct PresentEvent {
    // Initial event information (might be a kernel event if not presented
    // through DXGI or D3D9)
    uint64_t QpcTime;
    uint32_t ProcessId;
    uint32_t ThreadId;

    // Timestamps observed during present pipeline
    uint64_t TimeTaken;     // QPC duration between runtime present start and end
    uint64_t ReadyTime;     // QPC value when the last GPU commands completed prior to presentation
    uint64_t ScreenTime;    // QPC value when the present was displayed on screen

    // Extra present parameters obtained through DXGI or D3D9 present
    uint64_t SwapChainAddress;
    int32_t SyncInterval;
    uint32_t PresentFlags;

    // Properties deduced by watching events through present pipeline
    uint64_t Hwnd;
    uint64_t TokenPtr;
    uint32_t QueueSubmitSequence;
    Runtime Runtime;
    PresentMode PresentMode;
    PresentResult FinalState;
    uint32_t DestWidth;
    uint32_t DestHeight;
    uint64_t CompositionSurfaceLuid;
    bool SupportsTearing;
    bool MMIO;
    bool SeenDxgkPresent;
    bool SeenWin32KEvents;
    bool WasBatched;
    bool DwmNotified;
    bool Completed;

    // Additional transient state
    std::deque<std::shared_ptr<PresentEvent>> DependentPresents;

#if DEBUG_VERBOSE
    uint64_t Id;
#endif

    PresentEvent(EVENT_HEADER const& hdr, ::Runtime runtime);
    ~PresentEvent();

private:
    PresentEvent(PresentEvent const& copy); // dne
};

// A high-level description of the sequence of events for each present type,
// ignoring runtime end:
//
// Hardware Legacy Flip:
//   Runtime PresentStart -> Flip (by thread/process, for classification) -> QueueSubmit (by thread, for submit sequence) ->
//   MMIOFlip (by submit sequence, for ready time and immediate flags) [-> VSyncDPC (by submit sequence, for screen time)]
//
// Composed Flip (FLIP_SEQUENTIAL, FLIP_DISCARD, FlipEx):
//   Runtime PresentStart -> TokenCompositionSurfaceObject (by thread/process, for classification and token key) ->
//   PresentHistoryDetailed (by thread, for token ptr) -> QueueSubmit (by thread, for submit sequence) ->
//   DxgKrnl_PresentHistory (by token ptr, for ready time) and TokenStateChanged (by token key, for discard status and screen time)
//
// Hardware Direct Flip:
//   N/A, not currently uniquely detectable (follows the same path as composed flip)
//
// Hardware Independent Flip:
//   Follows composed flip, TokenStateChanged indicates IndependentFlip -> MMIOFlip (by submit sequence, for immediate flags)
//   [-> VSyncDPC or HSyncDPC (by submit sequence, for screen time)]
//
// Hardware Composed Independent Flip:
//   Identical to hardware independent flip, but MMIOFlipMPO is received instead of MMIOFlip
//
// Composed Copy with GPU GDI (a.k.a. Win7 Blit):
//   Runtime PresentStart -> DxgKrnl_Blit (by thread/process, for classification) ->
//   DxgKrnl_PresentHistoryDetailed (by thread, for token ptr and classification) -> DxgKrnl_Present (by thread, for hWnd) ->
//   DxgKrnl_PresentHistory (by token ptr, for ready time) -> DWM UpdateWindow (by hWnd, marks hWnd active for composition) ->
//   DWM Present (consumes most recent present per hWnd, marks DWM thread ID) ->
//   A fullscreen present is issued by DWM, and when it completes, this present is on screen
//
// Hardware Copy to front buffer:
//   Runtime PresentStart -> DxgKrnl_Blit (by thread/process, for classification) -> QueueSubmit (by thread, for submit sequence) ->
//   QueueComplete (by submit sequence, indicates ready and screen time)
//   Distinction between FS and windowed blt is done by LACK of other events
//
// Composed Copy with CPU GDI (a.k.a. Vista Blit):
//   Runtime PresentStart -> DxgKrnl_Blit (by thread/process, for classification) ->
//   SubmitPresentHistory (by thread, for token ptr, legacy blit token, and classification) ->
//   DxgKrnl_PresentHistory (by token ptr, for ready time) ->
//   DWM FlipChain (by legacy blit token, for hWnd and marks hWnd active for composition) ->
//   Follows the Windowed_Blit path for tracking to screen
//
// Composed Composition Atlas (DirectComposition):
//   SubmitPresentHistory (use model field for classification, get token ptr) -> DxgKrnl_PresentHistory (by token ptr) ->
//   Assume DWM will compose this buffer on next present (missing InFrame event), follow windowed blit paths to screen time

struct PMTraceConsumer
{
    PMTraceConsumer(bool filteredEvents, bool simple) : mFilteredEvents(filteredEvents), mSimpleMode(simple) { }
    ~PMTraceConsumer();

    EventMetadata mMetadata;

    bool mFilteredEvents;
    bool mSimpleMode;

    std::mutex mMutex;
    // A set of presents that are "completed":
    // They progressed as far as they can through the pipeline before being either discarded or hitting the screen.
    // These will be handed off to the consumer thread.
    std::vector<std::shared_ptr<PresentEvent>> mCompletedPresents;

    // For each process, stores each in-progress present in order. Used for present batching
    std::map<uint32_t, std::map<uint64_t, std::shared_ptr<PresentEvent>>> mPresentsByProcess;

    // For each (process, swapchain) pair, stores each present started. Used to ensure consumer sees presents targeting the same swapchain in the order they were submitted.
    typedef std::tuple<uint32_t, uint64_t> ProcessAndSwapChainKey;
    std::map<ProcessAndSwapChainKey, std::deque<std::shared_ptr<PresentEvent>>> mPresentsByProcessAndSwapChain;

    // Presents in the process of being submitted
    // The first map contains a single present that is currently in-between a set of expected events on the same thread:
    //   (e.g. DXGI_Present_Start/DXGI_Present_Stop, or Flip/QueueSubmit)
    // Used for mapping from runtime events to future events, and thread map used extensively for correlating kernel events
    std::map<uint32_t, std::shared_ptr<PresentEvent>> mPresentByThreadId;

    // Maps from queue packet submit sequence
    // Used for Flip -> MMIOFlip -> VSyncDPC for FS, for PresentHistoryToken -> MMIOFlip -> VSyncDPC for iFlip,
    // and for Blit Submission -> Blit completion for FS Blit
    std::map<uint32_t, std::shared_ptr<PresentEvent>> mPresentsBySubmitSequence;

    // Win32K present history tokens are uniquely identified by (composition surface pointer, present count, bind id)
    // Using a tuple instead of named struct simply to have auto-generated comparison operators
    // These tokens are used for "flip model" presents (windowed flip, dFlip, iFlip) only
    typedef std::tuple<uint64_t, uint64_t, uint64_t> Win32KPresentHistoryTokenKey;
    std::map<Win32KPresentHistoryTokenKey, std::shared_ptr<PresentEvent>> mWin32KPresentHistoryTokens;

    // DxgKrnl present history tokens are uniquely identified and used for all
    // types of windowed presents to track a "ready" time.
    //
    // The token is assigned to the last present on the same thread, on
    // non-REDIRECTED_GDI model DxgKrnl_Event_PresentHistoryDetailed or
    // DxgKrnl_Event_SubmitPresentHistory events.
    //
    // We stop tracking the token on a DxgKrnl_Event_PropagatePresentHistory
    // (which signals handing-off to DWM) -- or in CompletePresent() if the
    // hand-off wasn't detected.
    //
    // The following events lookup presents based on this token:
    // Dwm_Event_FlipChain_Pending, Dwm_Event_FlipChain_Complete,
    // Dwm_Event_FlipChain_Dirty,
    std::map<uint64_t, std::shared_ptr<PresentEvent>> mDxgKrnlPresentHistoryTokens;

    // For blt presents on Win7, it's not possible to distinguish between DWM-off or fullscreen blts, and the DWM-on blt to redirection bitmaps.
    // The best we can do is make the distinction based on the next packet submitted to the context. If it's not a PHT, it's not going to DWM.
    std::map<uint64_t, std::shared_ptr<PresentEvent>> mBltsByDxgContext;

    // mLastWindowPresent is used as storage for presents handed off to DWM.
    //
    // For blit (Composed_Copy_GPU_GDI) presents:
    // DxgKrnl_Event_PropagatePresentHistory causes the present to be moved
    // from mDxgKrnlPresentHistoryTokens to mLastWindowPresent.
    //
    // For flip presents: Dwm_Event_FlipChain_Pending,
    // Dwm_Event_FlipChain_Complete, or Dwm_Event_FlipChain_Dirty sets
    // mLastWindowPresent to the present that matches the token from
    // mDxgKrnlPresentHistoryTokens (but doesn't clear mDxgKrnlPresentHistory).
    //
    // Dwm_Event_GetPresentHistory will move all the Composed_Copy_GPU_GDI and
    // Composed_Copy_CPU_GDI mLastWindowPresents to mPresentsWaitingForDWM
    // before clearing mLastWindowPresent.
    //
    // For Win32K-tracked events, Win32K_Event_TokenStateChanged InFrame will
    // set mLastWindowPresent (and set any current present as discarded), and
    // Win32K_Event_TokenStateChanged Confirmed will clear mLastWindowPresent.
    std::map<uint64_t, std::shared_ptr<PresentEvent>> mLastWindowPresent;

    // Presents that will be completed by DWM's next present
    std::deque<std::shared_ptr<PresentEvent>> mPresentsWaitingForDWM;
    // Used to understand that a flip event is coming from the DWM
    uint32_t DwmPresentThreadId = 0;

    // Yet another unique way of tracking present history tokens, this time from DxgKrnl -> DWM, only for legacy blit
    std::map<uint64_t, std::shared_ptr<PresentEvent>> mPresentsByLegacyBlitToken;

    // Process events
    std::mutex mNTProcessEventMutex;
    std::vector<NTProcessEvent> mNTProcessEvents;

    bool DequeueProcessEvents(std::vector<NTProcessEvent>& outProcessEvents)
    {
        if (mNTProcessEvents.empty()) {
            return false;
        }

        auto lock = scoped_lock(mNTProcessEventMutex);
        outProcessEvents.swap(mNTProcessEvents);
        return true;
    }

    bool DequeuePresents(std::vector<std::shared_ptr<PresentEvent>>& outPresents)
    {
        if (mCompletedPresents.empty()) {
            return false;
        }

        auto lock = scoped_lock(mMutex);
        outPresents.swap(mCompletedPresents);
        return true;
    }

    void HandleDxgkBlt(EVENT_HEADER const& hdr, uint64_t hwnd, bool redirectedPresent);
    void HandleDxgkFlip(EVENT_HEADER const& hdr, int32_t flipInterval, bool mmio);
    void HandleDxgkQueueSubmit(EVENT_HEADER const& hdr, uint32_t packetType, uint32_t submitSequence, uint64_t context, bool present, bool supportsDxgkPresentEvent);
    void HandleDxgkQueueComplete(EVENT_HEADER const& hdr, uint32_t submitSequence);
    void HandleDxgkMMIOFlip(EVENT_HEADER const& hdr, uint32_t flipSubmitSequence, uint32_t flags);
    void HandleDxgkMMIOFlipMPO(EVENT_HEADER const& hdr, uint32_t flipSubmitSequence, uint32_t flipEntryStatusAfterFlip, bool flipEntryStatusAfterFlipValid);
    void HandleDxgkSyncDPC(EVENT_HEADER const& hdr, uint32_t flipSubmitSequence);
    void HandleDxgkSubmitPresentHistoryEventArgs(EVENT_HEADER const& hdr, uint64_t token, uint64_t tokenData, PresentMode knownPresentMode);
    void HandleDxgkPropagatePresentHistoryEventArgs(EVENT_HEADER const& hdr, uint64_t token);

    void CompletePresent(std::shared_ptr<PresentEvent> p, uint32_t recurseDepth=0);
    std::shared_ptr<PresentEvent> FindBySubmitSequence(uint32_t submitSequence);
    decltype(mPresentByThreadId.begin()) FindOrCreatePresent(EVENT_HEADER const& hdr);
    decltype(mPresentByThreadId.begin()) CreatePresent(std::shared_ptr<PresentEvent> present, decltype(mPresentsByProcess.begin()->second)& processMap);
    void CreatePresent(std::shared_ptr<PresentEvent> present);
    void RuntimePresentStop(EVENT_HEADER const& hdr, bool AllowPresentBatching);

    void HandleNTProcessEvent(EVENT_RECORD* pEventRecord);
    void HandleDXGIEvent(EVENT_RECORD* pEventRecord);
    void HandleD3D9Event(EVENT_RECORD* pEventRecord);
    void HandleDXGKEvent(EVENT_RECORD* pEventRecord);
    void HandleWin32kEvent(EVENT_RECORD* pEventRecord);
    void HandleDWMEvent(EVENT_RECORD* pEventRecord);
    void HandleMetadataEvent(EVENT_RECORD* pEventRecord);

    void HandleWin7DxgkBlt(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkFlip(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkPresentHistory(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkQueuePacket(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkVSyncDPC(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkMMIOFlip(EVENT_RECORD* pEventRecord);
};

