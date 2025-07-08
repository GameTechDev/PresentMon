// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdint.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <set>
#include <unordered_set>
#include <windows.h>
#include <evntcons.h> // must include after windows.h
#include <functional>

#include "Debug.hpp"
#include "GpuTrace.hpp"
#include "TraceConsumer.hpp"
#include "NvidiaTraceConsumer.hpp"
#include "../IntelPresentMon/CommonUtilities/Hash.h"

// PresentMode represents the different paths a present can take on windows.
//
// Hardware_Legacy_Flip:
//     Runtime PresentStart
//     -> Flip (by thread/process, for classification)
//     -> QueueSubmit (by thread, for submit sequence)
//     -> MMIOFlip (by submit sequence, for ready time and immediate flags)
//     -> VSyncDPC (by submit sequence, for screen time)
//
// Hardware_Legacy_Copy_To_Front_Buffer:
//     Runtime PresentStart
//     -> DxgKrnl_Blit (by thread/process, for classification)
//     -> QueueSubmit (by thread, for submit sequence)
//     -> QueueComplete (by submit sequence, indicates ready and screen time)
// Distinction between FS and windowed blt is done by the lack of other events.
//
// Hardware_Independent_Flip:
//     Follows same path as Composed_Flip, but TokenStateChanged indicates IndependentFlip
//     -> MMIOFlip (by submit sequence, for immediate flags)
//     -> VSyncDPC or HSyncDPC (by submit sequence, for screen time)
//
// Composed_Flip (FLIP_SEQUENTIAL, FLIP_DISCARD, FlipEx):
//     Runtime PresentStart
//     -> TokenCompositionSurfaceObject (by thread/process, for classification and token key)
//     -> DxgKrnl_PresentHistoryDetailed (by thread, for token ptr)
//     -> QueueSubmit (by thread, for submit sequence)
//     -> DxgKrnl_PresentHistory_Info (by token ptr, for ready time) and TokenStateChanged (by token key, for discard status and intent to present)
//     -> DWM Present (consumes most recent present per hWnd, marks DWM thread ID)
//     -> A fullscreen present is issued by DWM, and when it completes, this present is on screen
//
// Composed_Copy_with_GPU_GDI (a.k.a. Win7 Blit):
//     Runtime PresentStart
//     -> DxgKrnl_Blit (by thread/process, for classification)
//     -> DxgKrnl_PresentHistoryDetailed (by thread, for token ptr and classification)
//     -> DxgKrnl_Present (by thread, for hWnd)
//     -> DxgKrnl_PresentHistory_Info (by token ptr, for ready time)
//     -> DWM UpdateWindow (by hWnd, marks hWnd active for composition)
//     -> DWM Present (consumes most recent present per hWnd, marks DWM thread ID)
//     -> A fullscreen present is issued by DWM, and when it completes, this present is on screen
//
// Composed_Copy_with_CPU_GDI (a.k.a. Vista Blit):
//     Runtime PresentStart
//     -> DxgKrnl_Blit (by thread/process, for classification)
//     -> DxgKrnl_PresentHistory_Start (by thread, for token ptr, legacy blit token, and classification)
//     -> DxgKrnl_PresentHistory_Info (by token ptr, for ready time)
//     -> DWM FlipChain (by legacy blit token, for hWnd and marks hWnd active for composition)
//     -> Follows the Windowed_Blit path for tracking to screen
//
// Hardware_Composed_Independent_Flip:
//     Identical to hardware independent flip, but VSyncDPCMPO and HSyncDPCMPO contains more than one valid plane and SubmitSequence.
//
// The following present modes are not currently detected by PresentMon:
//
// Hardware_Direct_Flip:
//     Not uniquely detectable through ETW (follows the same path as Composed_Flip)
//
// Composed Composition Atlas (DirectComposition):
//     Unable to track composition dependencies, leading to incorrect/misleading metrics.
//     Runtime PresentStart
//     -> DxgKrnl_PresentHistory_Start (use model field for classification, get token ptr)
//     -> DxgKrnl_PresentHistory_Info (by token ptr)
//     -> Assume DWM will compose this buffer on next present (missing InFrame event), follow windowed blit paths to screen time

enum class PresentMode {
    Unknown = 0,
    Hardware_Legacy_Flip = 1,
    Hardware_Legacy_Copy_To_Front_Buffer = 2,
    Hardware_Independent_Flip = 3,
    Composed_Flip = 4,
    Composed_Copy_GPU_GDI = 5,
    Composed_Copy_CPU_GDI = 6,
    Hardware_Composed_Independent_Flip = 8,
};

enum class PresentResult {
    Unknown = 0,
    Presented = 1,
    Discarded = 2,
};

enum class Runtime {
    Other = 0,
    DXGI = 1,
    D3D9 = 2,
};

enum class InputDeviceType {
    None = 0,
    Unknown = 1,
    Mouse = 2,
    Keyboard = 3,
};

enum class FrameType {
    NotSet = 0,
    Unspecified = 1,
    Application = 2,
    Repeated = 3,
    Intel_XEFG = 50,
    AMD_AFMF = 100,
};

struct InputData {
    uint64_t Time;
    uint64_t MouseClickTime;
    uint64_t XFormTime;
    InputDeviceType Type;
    uint64_t hWnd;
};

struct MouseClickData {
    uint64_t CurrentMouseClickTime;
    uint64_t CurrentXFormTime;
    uint64_t LastMouseClickTime;
    uint64_t LastXFormTime;
    uint64_t hWnd;
};

struct PresentFrameTypeEvent {
    uint32_t FrameId = 0;
    FrameType FrameType = FrameType::NotSet;
    uint32_t AppFrameId = 0;
};

struct FlipFrameTypeEvent {
    uint64_t PresentId;
    uint64_t Timestamp;
    FrameType FrameType;
};

// Structure used to track application provided timing information 
// using ETW events provided by either the PresentMon provider or 
// PC Latency stats.
struct AppTimingData {
    uint32_t ProcessId = 0;
    uint32_t FrameId = 0;
    uint64_t AppSleepStartTime = 0;
    uint64_t AppSleepEndTime = 0;
    uint64_t AppSimStartTime = 0;
    uint64_t AppSimEndTime = 0;
    uint64_t AppRenderSubmitStartTime = 0;
    uint64_t AppRenderSubmitEndTime = 0;
    uint64_t AppPresentStartTime = 0;
    uint64_t AppPresentEndTime = 0;
    std::pair<uint64_t, InputDeviceType> AppInputSample = { 0, InputDeviceType::None };
    uint64_t PclSimStartTime = 0;
    uint64_t PclSimEndTime = 0;
    uint64_t PclRenderSubmitStartTime = 0;
    uint64_t PclRenderSubmitEndTime = 0;
    uint64_t PclPresentStartTime = 0;
    uint64_t PclPresentEndTime = 0;
    uint64_t PclInputPingTime = 0;
    uint64_t PclInputReceivedTime = 0;
    uint64_t PclOutOfBandPresentStartTime = 0;
    bool AssignedToPresent = false;
    bool PresentCompleted = false;
};

// A ProcessEvent occurs whenever a Process starts or stops.
struct ProcessEvent {
    std::wstring ImageFileName = {};    // The name of the process exe file.  This is only available on process start events.
    uint64_t QpcTime = 0;               // The time of the start/stop event.
    uint32_t ProcessId = 0;             // The id of the process.
    bool IsStartEvent = false;          // Whether this is a start event (true) or a stop event (false).
};

struct PresentEvent {
    uint64_t PresentStartTime;  // QPC value of the first event related to the Present (D3D9, DXGI, or DXGK Present_Start)
    uint32_t ProcessId;         // ID of the process that presented
    uint32_t ThreadId;          // ID of the thread that presented
    uint64_t TimeInPresent;     // The QPC duration of the Present call (only applicable for D3D9/DXGI)
    uint64_t GPUStartTime;      // QPC value when the frame's first DMA packet started
    uint64_t ReadyTime;         // QPC value when the frame's last DMA packet completed
    uint64_t GPUDuration;       // QPC duration during which a frame's DMA packet was running on
                                // ... any node (if mTrackGPUVideo==false) or non-video nodes (if mTrackGPUVideo==true)
    uint64_t GPUVideoDuration;  // QPC duration during which a frame's DMA packet was running on a video node (if mTrackGPUVideo==true)
    uint64_t InputTime;         // Earliest QPC value when the keyboard/mouse were tapped/moved and used by this frame
    uint64_t MouseClickTime;    // Earliest QPC value when the mouse was clicked and used by this frame

    // Used to track the application work when Intel XeSS-FG is enabled
    uint64_t AppPropagatedPresentStartTime;  // Propogated QPC value of the first event related to the Present (D3D9, DXGI, or DXGK Present_Start)
    uint64_t AppPropagatedTimeInPresent;     // Propogated  QPC duration of the Present call (only applicable for D3D9/DXGI)
    uint64_t AppPropagatedGPUStartTime;      // Propogated QPC value when the frame's first DMA packet started
    uint64_t AppPropagatedReadyTime;         // Propogated QPC value when the frame's last DMA packet completed
    uint64_t AppPropagatedGPUDuration;       // Propogated QPC duration during which a frame's DMA packet was running on
    // ... any node (if mTrackGPUVideo==false) or non-video nodes (if mTrackGPUVideo==true)
    uint64_t AppPropagatedGPUVideoDuration;  // Propogated QPC duration during which a frame's DMA packet was running on a video node (if mTrackGPUVideo==true)

    // Application provided events
    uint32_t AppFrameId;
    uint64_t AppSleepStartTime;         // QPC value of app sleep start time provided by Intel App Provider
    uint64_t AppSleepEndTime;           // QPC value of app sleep end time provided by Intel App Provider
    uint64_t AppSimStartTime;           // QPC value of app sim start time provided by Intel App Provider
    uint64_t AppSimEndTime;             // QPC value of app sim end time provided by Intel App Provider
    uint64_t AppRenderSubmitStartTime;  // QPC value of app render submit start time provided by Intel App Provider
    uint64_t AppRenderSubmitEndTime;    // QPC value of app render submit end time provided by Intel App Provider
    uint64_t AppPresentStartTime;       // QPC value of app present start time provided by Intel App Provider
    uint64_t AppPresentEndTime;         // QPC value of app present end time provided by Intel App Provider
    std::pair<uint64_t, InputDeviceType> AppInputSample;    // QPC value of app input data provided by Intel App Provider
    uint32_t PclFrameId = 0;
    uint64_t PclSimStartTime = 0;
    uint64_t PclSimEndTime = 0;
    uint64_t PclRenderSubmitStartTime = 0;
    uint64_t PclRenderSubmitEndTime = 0;
    uint64_t PclPresentStartTime = 0;
    uint64_t PclPresentEndTime = 0;
    uint64_t PclInputPingTime = 0;
    uint64_t PclInputReceivedTime = 0;

    // Extra present parameters obtained through DXGI or D3D9 present
    uint64_t SwapChainAddress;
    int32_t SyncInterval;
    uint32_t PresentFlags;

    // (FrameType, DisplayedQPC) for each time the frame was displayed
    std::vector<std::pair<FrameType, uint64_t>> Displayed;

    // Keys used to index into PMTraceConsumer's tracking data structures:
    uint64_t CompositionSurfaceLuid;      // mPresentByWin32KPresentHistoryToken
    uint64_t Win32KPresentCount;          // mPresentByWin32KPresentHistoryToken
    uint64_t Win32KBindId;                // mPresentByWin32KPresentHistoryToken
    uint64_t DxgkPresentHistoryToken;     // mPresentByDxgkPresentHistoryToken
    uint64_t DxgkPresentHistoryTokenData; // mPresentByDxgkPresentHistoryTokenData
    uint64_t DxgkContext;                 // mPresentByDxgkContext
    uint64_t Hwnd;                        // mLastPresentByWindow
    uint32_t QueueSubmitSequence;         // mPresentBySubmitSequence
    uint32_t RingIndex;                   // mTrackedPresents and mCompletedPresents
    std::unordered_map<uint64_t, uint64_t> PresentIds; // mPresentByVidPnLayerId
    // Note: the following index tracking structures as well but are defined elsewhere:
    //       ProcessId                 -> mOrderedPresentsByProcessId
    //       ThreadId, DriverThreadId  -> mPresentByThreadId
    //       PresentInDwmWaitingStruct -> mPresentsWaitingForDWM

    // Additional transient tracking state
    std::deque<std::shared_ptr<PresentEvent>> DependentPresents;

    // Properties deduced by watching events through present pipeline
    uint32_t DestWidth;
    uint32_t DestHeight;
    uint32_t DriverThreadId;    // If the present is deferred by the driver, this will hold the
                                // threaad id that the driver finally presented on.

    uint32_t FrameId;           // ID for the logical frame that this Present is associated with.

    Runtime Runtime;            // Whether PresentStart originated from D3D9, DXGI, or DXGK.
    PresentMode PresentMode;
    PresentResult FinalState;
    InputDeviceType InputType;
    bool SupportsTearing;
    bool WaitForFlipEvent;
    bool WaitForMPOFlipEvent;
    bool SeenDxgkPresent;
    bool SeenWin32KEvents;
    bool SeenInFrameEvent;      // This present has gotten a Win32k TokenStateChanged event into InFrame state
    bool GpuFrameCompleted;     // This present has already seen an event that caused GpuTrace::CompleteFrame() to be called.
    bool IsCompleted;           // All expected events have been observed.
    bool IsLost;                // This PresentEvent was found in an unexpected state and analysis could not continue (potentially
                                // due to missing a critical ETW event'.
    bool PresentFailed;         // The Present() call failed.
    bool IsHybridPresent;       // This present is a hybrid present and is performing a cross adapter copy.

    bool PresentInDwmWaitingStruct; // Whether this PresentEvent is currently stored in
                                    // PMTraceConsumer::mPresentsWaitingForDWM

    // If WaitingForPresentStop, then the present has been completed but it has not seen a
    // PresentStop event yet. The present is marked not-ready for dequeue until the PresentStop is
    // seen.
    bool WaitingForPresentStop;

    // If WaitingForFlipFrameType, the present is waiting for a FlipFrameType event (or, until an
    // MMIOFlipMultiPlaneOverlay3_Info event signals a higher present id). If
    // DoneWaitingForFlipFrameType, then the present did WaitingForFlipFrameType but is no longer
    // waiting.
    bool WaitingForFlipFrameType;
    bool DoneWaitingForFlipFrameType;

    // If WaitingForFrameId, the present is waiting for another present with the same FrameId (or,
    // until a PresentFrameType_Info event with a different FrameId).
    bool WaitingForFrameId;

    // Data from NV DisplayDriver event
    uint64_t FlipDelay = 0;
    uint32_t FlipToken = 0;

    PresentEvent();
    PresentEvent(uint32_t fid);
private:
    PresentEvent(PresentEvent const& copy); // dne
};

struct PMTraceConsumer
{
    static constexpr int PRESENTEVENT_CIRCULAR_BUFFER_SIZE = 2048;

    // -------------------------------------------------------------------------------------------
    // The following are parameters to the analysis, which can be configured prior to starting the
    // trace session.

    bool mFilteredEvents = false;       // Whether the trace session was configured to filter non-PresentMon events
    bool mFilteredProcessIds = false;   // Whether to filter presents to specific processes

    // Whether the analysis should track...
    bool mTrackDisplay = true;         // ... presents to the display.
    bool mTrackGPU = false;            // ... GPU work.
    bool mTrackGPUVideo = false;       // ... GPU video work (separately from non-video GPU work).
    bool mTrackInput = false;          // ... keyboard/mouse latency.
    bool mTrackFrameType = false;      // ... the frame type communicated through the Intel-PresentMon provider.
    bool mTrackPMMeasurements = false; // ... external measurements provided through the Intel-PresentMon provider
    bool mTrackAppTiming = false;      // ... app timing data communicated through the Intel-PresentMon provider.
    bool mTrackHybridPresent = false;  // ... hybrid presents.
    bool mTrackPcLatency = false;      // ... Nvidia PC Latency stats.

    // When PresentEvents are missing data that may still arrive later, they get put into a deferred
    // state until the data arrives.  This time limit specifies how long a PresentEvent can be
    // deferred before being considered lost.  The default of 0 means that no PresentEvents will be
    // deferred, potentially leading to dequeued events that are missing data.
    uint64_t mDeferralTimeLimit = 0; // QPC duration

    bool mIsRealtimeSession = true; // allow consumer to have different behavior for realtime vs. offline analysis
    bool mDisableOfflineBackpressure = false;
    bool mPaceEvents = false;
    bool mRetimeEvents = false;

    // -------------------------------------------------------------------------------------------
    // These functions can be used to filter PresentEvents by process from within the consumer.

    void AddTrackedProcessForFiltering(uint32_t processID);
    void RemoveTrackedProcessForFiltering(uint32_t processID);
    bool IsProcessTrackedForFiltering(uint32_t processID);


    // -------------------------------------------------------------------------------------------
    // Once the session is started the consumer will consume and analyze ETW data to produce
    // completed process and present events.  Call Dequeue*Events() to periodically to remove these
    // completed events.  If the functions are not called quick enough, the consumer's internal ring
    // buffer may wrap and cause completed events to be lost.
    //
    // PresentEvents from each swapchain are ordered by their PresentStart time, but presents from
    // separate swapchains may appear out of order.

    void DequeueProcessEvents(std::vector<ProcessEvent>& outProcessEvents);
    void DequeuePresentEvents(std::vector<std::shared_ptr<PresentEvent>>& outPresentEvents);


    // -------------------------------------------------------------------------------------------
    // The rest of this structure are internal data and functions for analysing the collected ETW
    // data.

    // Storage for process and present events:
    std::vector<ProcessEvent> mProcessEvents;
    std::vector<std::shared_ptr<PresentEvent>> mTrackedPresents;
    std::vector<std::shared_ptr<PresentEvent>> mCompletedPresents;
    uint32_t mNextFreeRingIndex = 0;    // The index of mTrackedPresents to use when creating the next present.
    uint32_t mCompletedIndex = 0;       // The index of mCompletedPresents of the oldest completed present.
    uint32_t mCompletedCount = 0;       // The total number of presents in mCompletedPresents.
    uint32_t mReadyCount = 0;           // The number of presents in mCompletedPresents, starting at mCompletedIndex, that are ready to be dequeued.
    uint32_t mNumOverflowedPresents = 0; // The number of presents that have been lost due to the ring buffer wrapping.
    uint32_t mCircularBufferSize = 0;   // The size of the ring buffers for presents.

    // Mutexs to protect consumer/dequeue access from different threads:
    std::mutex mProcessEventMutex;
    std::mutex mPresentEventMutex;
    // condition variable to signal when output ring space becomes available, used for backpressure in offline mode
    std::condition_variable mCompletedRingCondition;
    // event used to signal when new events are available for dequeing
    HANDLE hEventsReadyEvent;

    // EventMetadata stores the structure of ETW events to optimize subsequent property retrieval.
    EventMetadata mMetadata;

    // TraceLoggingContext decodes trace logging events and allows for easy property retrieval.
    TraceLoggingContext mTraceLoggingDecoder;

    // Limit tracking to specified processes
    std::set<uint32_t> mTrackedProcessFilter;
    std::shared_mutex mTrackedProcessFilterMutex;

    // Whether we've completed any presents yet.  This is used to indicate that all the necessary
    // providers have started and it's safe to start tracking presents.
    bool mHasCompletedAPresent = false;

    // Store the DWM process id, and the last DWM thread id to have started a present.  This is
    // needed to determine if a flip event is coming from DWM, but can also be useful for targetting
    // non-DWM processes.
    //
    // mPresentsWaitingForDWM stores all in-progress presents that have been handed off to DWM.
    // Once the next DWM present is detected, they are added as its' DependentPresents.
    std::deque<std::shared_ptr<PresentEvent>> mPresentsWaitingForDWM;
    uint32_t DwmProcessId = 0;
    uint32_t DwmPresentThreadId = 0;

    // Storage for passing present path tracking id to Handle...() functions.
    #ifdef TRACK_PRESENT_PATHS
    uint32_t mAnalysisPathID;
    #endif

    // These data structures store in-progress presents that are being processed by PMTraceConsumer.
    //
    // mTrackedPresents is a circular buffer storage for all in-progress presents.  Presents that
    // are still in-progress when the buffer wraps are considered lost due to age.  mNextFreeIndex
    // is the index of the element to use when creating the next present.
    //
    // Once presents are completed, they are moved into the mCompletedPresents ring buffer.
    // mCompletedIndex and mCompletedCount specify a list of presents are ready to be dequeued by
    // the user.
    //
    // mPresentByThreadId stores the in-progress present that was last operated on by each thread.
    // This is used to look up the right present for event sequences that are known to execute on
    // the same thread.  The present should be removed once those sequences are complete.
    //
    // mOrderedPresentsByProcessId stores each process' in-progress presents in the order that they
    // were created.  This is used to look up presents for event sequences across different threads
    // of the process (e.g., DXGI, DXGK, driver threads).  It's also used to detect discarded
    // presents when newer presents are displayed from the same swapchain.
    //
    // mPresentBySubmitSequence stores presents who have had a present packet submitted on to a
    // queue until they are completed or discarded.  It's used to associate those presents to
    // various DXGK events (such as MMIOFlip, IndependentFlip and *SyncDPC) which reference the
    // submit sequence id.
    //
    // mPresentByWin32KPresentHistoryToken stores the in-progress present associated with each
    // Win32KPresentHistoryToken, which is a unique key used to identify all flip model presents,
    // during composition.  Presents should be removed once they have been confirmed.
    //
    // mPresentByDxgkPresentHistoryToken stores the in-progress present associated with each DxgKrnl
    // present history token, which is a unique key used to identify all windowed presents. Presents
    // should be removed on DxgKrnl_Event_PropagatePresentHistory, which signals hand-off to DWM.
    //
    // mPresentByDxgkPresentHistoryTokenData stores the in-progress present associated with a
    // DxgKrnl->DWM token used only for Composed_Copy_CPU_GDI presents.
    //
    // mPresentByDxgkContext stores the in-progress present associated with each DxgContext.  It's
    // only used for Hardware_Legacy_Copy_To_Front_Buffer presents on Win7, and is needed to
    // distinguish between DWM-off fullscreen blts and the DWM-on blt to redirection bitmaps.  The
    // present is removed on the next queue submisison.
    //
    // mLastPresentByWindow stores the latest in-progress present handed off to DWM from each
    // window.  It's needed to discard some legacy blts, which don't always get a Win32K token
    // Discarded transition.  The present is either overwritten, or removed when DWM confirms the
    // present.

    using OrderedPresents = std::map<uint64_t, std::shared_ptr<PresentEvent>>;

    using Win32KPresentHistoryToken = std::tuple<uint64_t, uint64_t, uint64_t>; // (composition surface pointer, present count, bind id)
    struct Win32KPresentHistoryTokenHash : private std::hash<uint64_t> {
        std::size_t operator()(Win32KPresentHistoryToken const& v) const noexcept;
    };
    
    template <typename T, typename U>
    struct PairHash {
        size_t operator()(const std::pair<T, U>& p) const noexcept
        {
            using namespace pmon::util::hash;
            return DualHash(p.first, p.second);
        }
    };

    std::unordered_map<uint32_t, std::shared_ptr<PresentEvent>> mPresentByThreadId;                     // ThreadId -> PresentEvent
    std::unordered_map<uint32_t, OrderedPresents>               mOrderedPresentsByProcessId;            // ProcessId -> ordered PresentStartTime -> PresentEvent
    std::unordered_map<uint32_t, std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>>>
                                                                mPresentBySubmitSequence;               // SubmitSequenceId -> hContext -> PresentEvent
    std::unordered_map<Win32KPresentHistoryToken, std::shared_ptr<PresentEvent>,
                       Win32KPresentHistoryTokenHash>           mPresentByWin32KPresentHistoryToken;    // Win32KPresentHistoryToken -> PresentEvent
    std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mPresentByDxgkPresentHistoryToken;      // DxgkPresentHistoryToken -> PresentEvent
    std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mPresentByDxgkPresentHistoryTokenData;  // DxgkPresentHistoryTokenData -> PresentEvent
    std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mPresentByDxgkContext;                  // DxgkContex -> PresentEvent
    std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mPresentByVidPnLayerId;                 // VidPnLayerId -> PresentEvent
    std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mLastPresentByWindow;                   // HWND -> PresentEvent
    std::unordered_map<uint64_t, MouseClickData>                mReceivedMouseClickByHwnd;              // HWND -> MouseClickData
    std::unordered_map<std::pair<uint32_t, uint32_t>, 
                       std::shared_ptr<PresentEvent>,
                       PairHash<uint32_t, uint32_t>>            mPresentByAppFrameId;                   // Intel provider app frame id -> PresentEvent
    std::unordered_map<std::pair<uint32_t, uint32_t>,
                       AppTimingData,
                       PairHash<uint32_t, uint32_t>>            mAppTimingDataByAppFrameId;             // Intel provider app frame id -> AppTimingData
    std::unordered_map<std::pair<uint32_t, uint64_t>,
                       uint32_t,
                       PairHash<uint32_t, uint64_t>>            mHybridPresentModeBySwapChainPid;       // SwapChain and process id -> HybridPresentMode

    // State used to track pcl events and their associated presents.
    std::unordered_map<std::pair<uint32_t, uint32_t>,
        AppTimingData,
        PairHash<uint32_t, uint32_t>>                           mPclTimingDataByPclFrameId;             // PCL frame id -> PCLTimingData
    std::unordered_map<uint32_t, uint64_t>                      mLatestPingTimestampByProcessId;        // ProcessId -> Latest Ping Timestamp
    bool mUsingOutOfBoundPresentStart = false;

    // mGpuTrace tracks work executed on the GPU.
    GpuTrace mGpuTrace;

    // PresentFrameTypeEvents are stored into mPendingPresentFrameTypeEvents and then looked-up and
    // attached to the PresentEvent in Present_Start.
    //
    // FlipFrameType events are typically stored into the present looked up by PresentId.  If the
    // PresentId has not been observed yet, it is stored into mPendingFlipFrameTypeEvents and then
    // attached to the present when the PresentId is observed.  mEnableFlipFrameTypeEvents is only
    // set to true once a compatible MMIOFlipMultiPlaneOverlay3_Info event is observed.
    std::unordered_map<uint32_t, PresentFrameTypeEvent> mPendingPresentFrameTypeEvents; // ThreadId -> PresentFrameTypeEvent
    std::unordered_map<uint64_t, FlipFrameTypeEvent>    mPendingFlipFrameTypeEvents;    // VidPnLayerId -> FlipFrameTypeEvent
    bool mEnableFlipFrameTypeEvents = false;

    // State for tracking keyboard/mouse click times
    //
    // mLastInputDeviceReadTime and mLastInputDeviceType are the time/type of the most-recent input
    // event.  This is global and if multiple input events are observed, but not retrieved by any
    // window, then older ones are lost.
    //
    // mRetrievedInput stores the time/type of the most-recent input event that has been retrieved
    // by each process's window(s).  Once that data is applied to a present, the InputDeviceType is
    // set to None, but the time is not changed so that we know whether mLastInputDeviceReadTime has
    // already been retrieved or not.
    uint64_t mLastInputDeviceReadTime = 0;
    InputDeviceType mLastInputDeviceType = InputDeviceType::None;

    std::unordered_map<uint32_t, InputData> mRetrievedInput; // ProcessID -> InputData<InputTime, InputType, isMouseClick>

    // Trace consumer that handles events coming from Nvidia DisplayDriver
    NVTraceConsumer mNvTraceConsumer;

    // -------------------------------------------------------------------------------------------
    // Functions for decoding ETW and analysing process and present events.

    PMTraceConsumer();
    PMTraceConsumer(uint32_t circularBufferSize);
    ~PMTraceConsumer();

    PMTraceConsumer(const PMTraceConsumer&) = delete;
    PMTraceConsumer& operator=(const PMTraceConsumer&) = delete;
    PMTraceConsumer(PMTraceConsumer&&) = delete;
    PMTraceConsumer& operator=(PMTraceConsumer&&) = delete;

    void HandleDxgkBlt(EVENT_HEADER const& hdr, uint64_t hwnd, bool redirectedPresent);
    std::shared_ptr<PresentEvent> HandleDxgkFlip(EVENT_HEADER const& hdr);
    void HandleDxgkQueueSubmit(EVENT_HEADER const& hdr, uint64_t hContext, uint32_t submitSequence, uint32_t packetType, bool isPresentPacket, bool isWin7);
    void HandleDxgkQueueComplete(uint64_t timestamp, uint64_t hContext, uint32_t submitSequence);
    void HandleDxgkMMIOFlip(uint64_t timestamp, uint32_t submitSequence, uint32_t flags);
    void HandleDxgkSyncDPC(uint64_t timestamp, uint32_t submitSequence);
    void HandleDxgkPresentHistory(EVENT_HEADER const& hdr, uint64_t token, uint64_t tokenData, Microsoft_Windows_DxgKrnl::PresentModel presentModel);
    void HandleDxgkPresentHistoryInfo(EVENT_HEADER const& hdr, uint64_t token);

    void HandleProcessEvent(EVENT_RECORD* pEventRecord);
    void HandleDXGIEvent(EVENT_RECORD* pEventRecord);
    void HandleD3D9Event(EVENT_RECORD* pEventRecord);
    void HandleDXGKEvent(EVENT_RECORD* pEventRecord);
    void HandleWin32kEvent(EVENT_RECORD* pEventRecord);
    void HandleDWMEvent(EVENT_RECORD* pEventRecord);
    void HandleMetadataEvent(EVENT_RECORD* pEventRecord);
    void HandleIntelPresentMonEvent(EVENT_RECORD* pEventRecord);
    void HandleNvidiaDisplayDriverEvent(EVENT_RECORD* pEventRecord);
    void HandleTraceLoggingEvent(EVENT_RECORD* pEventRecord);
    void HandlePclEvent(EVENT_RECORD* pEventRecord);

    void HandleWin7DxgkBlt(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkFlip(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkPresentHistory(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkQueuePacket(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkVSyncDPC(EVENT_RECORD* pEventRecord);
    void HandleWin7DxgkMMIOFlip(EVENT_RECORD* pEventRecord);


    void SetThreadPresent(uint32_t threadId, std::shared_ptr<PresentEvent> const& present);
    std::shared_ptr<PresentEvent> FindPresentByThreadId(uint32_t threadId);
    std::shared_ptr<PresentEvent> FindPresentBySubmitSequence(uint32_t submitSequence);
    std::shared_ptr<PresentEvent> FindOrCreatePresent(EVENT_HEADER const& hdr);

    void TrackPresent(std::shared_ptr<PresentEvent> present, OrderedPresents* presentsByThisProcess);
    void StopTrackingPresent(std::shared_ptr<PresentEvent> const& present);
    void RemovePresentFromSubmitSequenceIdTracking(std::shared_ptr<PresentEvent> const& present);

    void RuntimePresentStart(Runtime runtime, EVENT_HEADER const& hdr, uint64_t swapchainAddr, uint32_t dxgiPresentFlags, int32_t syncInterval);
    void RuntimePresentStop(Runtime runtime, EVENT_HEADER const& hdr, uint32_t result);
    void CompletePresent(std::shared_ptr<PresentEvent> const& present);
    void RemoveLostPresent(std::shared_ptr<PresentEvent> present);

    void UpdateReadyCount(bool useLock);

    void DeferFlipFrameType(uint64_t vidPnLayerId, uint64_t presentId, uint64_t timestamp, FrameType frameType);
    void ApplyFlipFrameType(std::shared_ptr<PresentEvent> const& present, uint64_t timestamp, FrameType frameType);
    void ApplyPresentFrameType(std::shared_ptr<PresentEvent> const& present);
    void SetAppTimingData(const EVENT_RECORD* pEventRecord);

    void SignalEventsReady();
    
    // -------------------------------------------------------------------------------------------
    // Function for managing app provided events
    AppTimingData* ExtractAppTimingData(std::unordered_map<std::pair<uint32_t, uint32_t>, AppTimingData, PairHash<uint32_t, uint32_t>>& timingDataByFrameId, uint32_t processId, uint32_t appFrameId, uint64_t presentStartTime, std::function<uint64_t(const AppTimingData&)> timingSelector);
    bool IsApplicationPresent(std::shared_ptr<PresentEvent> const& present);
    void SetAppTimingDataAsComplete(uint32_t processId, uint32_t appFrameId);

    inline uint32_t GetRingIndex(uint32_t index)
    {
        return index % mCircularBufferSize;
    }

};
