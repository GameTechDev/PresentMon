/*
Copyright 2017-2020 Intel Corporation

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

// A ProcessEvent occurs whenever a Process starts or stops.
struct ProcessEvent {
    std::string ImageFileName;
    uint64_t QpcTime;
    uint32_t ProcessId;
    bool IsStartEvent;
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

    // Track the path the present took through the PresentMon analysis.
#ifdef TRACK_PRESENT_PATHS
    uint64_t AnalysisPath;
#endif

    // Give every present a unique id for debugging.
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
    PMTraceConsumer(bool filteredEvents, bool simple);
    ~PMTraceConsumer();

    EventMetadata mMetadata;

    bool mFilteredEvents;
    bool mSimpleMode;

    // Store completed presents until the consumer thread removes them using
    // DequeuePresents().  Completed presents are those that have progressed as
    // far as they can through the pipeline before being either discarded or
    // hitting the screen.
    std::mutex mPresentEventMutex;
    std::vector<std::shared_ptr<PresentEvent>> mPresentEvents;

    // Process events
    std::mutex mProcessEventMutex;
    std::vector<ProcessEvent> mProcessEvents;


    // These data structures store in-progress presents (i.e., ones that are
    // still being processed by the system and are not yet completed).
    //
    // mPresentByThreadId stores the in-progress present that was last operated
    // on by each thread for event sequences that are known to execute on the
    // same thread.
    //
    // mPresentsByProcess stores each process' in-progress presents in the
    // order that they were presented.  This is used to look up presents across
    // systems running on different threads (DXGI/D3D/DXGK/Win32) and for
    // batched present tracking, so we know to discard all older presents with
    // one is completed.
    //
    // mPresentsByProcessAndSwapChain stores each swapchain's in-progress
    // presents in the order that they were created by PresentMon.  This is
    // primarily used to ensure that the consumer sees per-swapchain presents
    // in the same order that they were submitted.
    //
    // TODO: shouldn't batching via mPresentsByProcess be per swapchain as
    // well?  Is the create order used by mPresentsByProcessAndSwapChain really
    // different than QpcTime order?  If no on these, should we combine
    // mPresentsByProcess and mPresentsByProcessAndSwapChain?
    //
    // All flip model presents (windowed flip, dFlip, iFlip) are uniquely
    // identifyed by a Win32K present history token (composition surface,
    // present count, and bind id).  mWin32KPresentHistoryTokens stores the
    // mapping from this token to in-progress present to optimize lookups
    // during Win32K events.

    // [thread id]
    std::map<uint32_t, std::shared_ptr<PresentEvent>> mPresentByThreadId;

    // [process id][qpc time]
    std::map<uint32_t, std::map<uint64_t, std::shared_ptr<PresentEvent>>> mPresentsByProcess;

    // [(process id, swapchain address)]
    typedef std::tuple<uint32_t, uint64_t> ProcessAndSwapChainKey;
    std::map<ProcessAndSwapChainKey, std::deque<std::shared_ptr<PresentEvent>>> mPresentsByProcessAndSwapChain;


    // Maps from queue packet submit sequence
    // Used for Flip -> MMIOFlip -> VSyncDPC for FS, for PresentHistoryToken -> MMIOFlip -> VSyncDPC for iFlip,
    // and for Blit Submission -> Blit completion for FS Blit
    std::map<uint32_t, std::shared_ptr<PresentEvent>> mPresentsBySubmitSequence;

    // [(composition surface pointer, present count, bind id)]
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
    uint32_t DwmProcessId = 0;
    uint32_t DwmPresentThreadId = 0;

    // Yet another unique way of tracking present history tokens, this time from DxgKrnl -> DWM, only for legacy blit
    std::map<uint64_t, std::shared_ptr<PresentEvent>> mPresentsByLegacyBlitToken;

    // Storage for passing present path tracking id to Handle...() functions.
#ifdef TRACK_PRESENT_PATHS
    uint32_t mAnalysisPathID;
#endif

    void DequeueProcessEvents(std::vector<ProcessEvent>& outProcessEvents)
    {
        std::lock_guard<std::mutex> lock(mProcessEventMutex);
        outProcessEvents.swap(mProcessEvents);
    }

    void DequeuePresentEvents(std::vector<std::shared_ptr<PresentEvent>>& outPresentEvents)
    {
        std::lock_guard<std::mutex> lock(mPresentEventMutex);
        outPresentEvents.swap(mPresentEvents);
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
    decltype(mPresentByThreadId.begin()) CreatePresent(std::shared_ptr<PresentEvent> present, decltype(mPresentsByProcess.begin()->second)& presentsByThisProcess);
    void CreatePresent(std::shared_ptr<PresentEvent> present);
    void HandleStuckPresent(EVENT_HEADER const& hdr, decltype(mPresentByThreadId.begin())* eventIter);
    void RuntimePresentStop(EVENT_HEADER const& hdr, bool AllowPresentBatching, ::Runtime runtime);

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

