// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <assert.h>
#include <evntrace.h>
#include <evntprov.h>
#include <stdint.h>
#include <stdlib.h>

#include "PresentMonProvider.h"

namespace {

static GUID const ProviderGUID = { 0xecaa4712, 0x4644, 0x442f, { 0xb9, 0x4c, 0xa3, 0x2f, 0x6c, 0xf8, 0xa4, 0x99 }};

enum {
    ID_PresentFrameType         = 1,
    ID_FlipFrameType            = 2,
    ID_MeasuredInput            = 10,
    ID_MeasuredScreenChange     = 11,
    ID_AppSleepStart            = 50,
    ID_AppSleepEnd              = 51,
    ID_AppSimulationStart       = 52,
    ID_AppSimulationEnd         = 53,
    ID_AppRenderSubmitStart     = 54,
    ID_AppRenderSubmitEnd       = 55,
    ID_AppPresentStart          = 56,
    ID_AppPresentEnd            = 57,
    ID_AppInputSample           = 58,
};

enum {
    Keyword_FrameTypes      = 1 << 0,
    Keyword_Measurements    = 1 << 1,
    Keyword_Application     = 1 << 5,
};

enum Event {
    Event_PresentFrameType,
    Event_PresentFrameTypeV2,
    Event_FlipFrameType,
    Event_MeasuredInput,
    Event_MeasuredScreenChange,
    Event_AppSleepStart,
    Event_AppSleepEnd,
    Event_AppSimulationStart,
    Event_AppSimulationEnd,
    Event_AppRenderSubmitStart,
    Event_AppRenderSubmitEnd,
    Event_AppPresentStart,
    Event_AppPresentEnd,
    Event_AppInputSample,
    Event_Count
};

// Note: code below (e.g., EnableCallback()) assumes that all of these use TRACE_LEVEL_INFORMATION,
// and would need to be updated if you extend this to use other levels.
static EVENT_DESCRIPTOR const EventDescriptor[] = {
    // ID, Version, Channel, Level, Opcode, Task, Keyword
    { ID_PresentFrameType,       0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_PresentFrameType,       Keyword_FrameTypes },
    { ID_PresentFrameType,       1, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_PresentFrameType,       Keyword_FrameTypes },
    { ID_FlipFrameType,          0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_FlipFrameType,          Keyword_FrameTypes },
    { ID_MeasuredInput,          0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_MeasuredInput,          Keyword_Measurements },
    { ID_MeasuredScreenChange,   0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_MeasuredScreenChange,   Keyword_Measurements },
    { ID_AppSleepStart,          0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppSleepStart,          Keyword_Application },
    { ID_AppSleepEnd,            0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppSleepEnd,            Keyword_Application },
    { ID_AppSimulationStart,     0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppSimulationStart,     Keyword_Application },
    { ID_AppSimulationEnd,       0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppSimulationEnd,       Keyword_Application },
    { ID_AppRenderSubmitStart,   0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppRenderSubmitStart,   Keyword_Application },
    { ID_AppRenderSubmitEnd,     0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppRenderSubmitEnd,     Keyword_Application },
    { ID_AppPresentStart,        0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppPresentStart,        Keyword_Application },
    { ID_AppPresentEnd,          0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppPresentEnd,          Keyword_Application },
    { ID_AppInputSample,         0, 0, TRACE_LEVEL_INFORMATION, EVENT_TRACE_TYPE_INFO, ID_AppInputSample,         Keyword_Application },
};

static_assert(Event_Count == _countof(EventDescriptor), "Event enum and EventDescriptor size mismatch");
static_assert(Event_Count <= 32,                        "Too many events for current PresentMonProvider::EnableBits");

}

struct PresentMonProvider {
    HMODULE   Advapi32Module;
    ULONG     (__stdcall* pEventRegister)(LPCGUID, PENABLECALLBACK, PVOID, PREGHANDLE);
    ULONG     (__stdcall* pEventUnregister)(REGHANDLE);
    ULONG     (__stdcall* pEventWrite)(REGHANDLE, PCEVENT_DESCRIPTOR, ULONG, PEVENT_DATA_DESCRIPTOR);

    REGHANDLE ProviderHandle;
    ULONG     EnableBits;
};

namespace {

bool EventIsDisabled(
    PresentMonProvider const& ctxt,
    Event event)
{
    return (ctxt.EnableBits & (1u << event)) == 0;
}

bool KeywordIsEnabled(
    ULONGLONG Keyword,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeyword)
{
    return Keyword == 0ull || ((Keyword & MatchAnyKeyword) && ((Keyword & MatchAllKeyword) == MatchAllKeyword));
}

void __stdcall EnableCallback(
    LPCGUID, // SourceId
    ULONG ControlCode,
    UCHAR Level,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeyword,
    PEVENT_FILTER_DESCRIPTOR, // FilterData
    PVOID CallbackContext)
{
    auto ctxt = (PresentMonProvider*) CallbackContext;
    if (ctxt != nullptr) {
        switch (ControlCode) {
        case EVENT_CONTROL_CODE_ENABLE_PROVIDER:
            ctxt->EnableBits = 0;
            if (Level == 0 || Level >= TRACE_LEVEL_INFORMATION) {
                for (uint32_t i = 0; i < Event_Count; ++i) {
                    ULONG bit = KeywordIsEnabled(EventDescriptor[i].Keyword, MatchAnyKeyword, MatchAllKeyword) ? 1u : 0u;
                    ctxt->EnableBits |= (bit << i);
                }
            }
            break;

        case EVENT_CONTROL_CODE_DISABLE_PROVIDER:
            ctxt->EnableBits = 0;
            break;
        }
    }
}

void FillDesc(EVENT_DATA_DESCRIPTOR*) {}

template<typename T, typename... Ts>
void FillDesc(
    EVENT_DATA_DESCRIPTOR* data,
    T* param,
    Ts... params)
{
    data->Ptr = (ULONGLONG) param;
    data->Size = sizeof(*param);

    FillDesc(data + 1, params...);
}

ULONG WriteEvent(
    PresentMonProvider* ctxt,
    Event event)
{
    if (EventIsDisabled(*ctxt, event)) return ERROR_SUCCESS;

    return (*ctxt->pEventWrite)(ctxt->ProviderHandle, &EventDescriptor[event], 0, nullptr);
}

template<typename... Ts>
ULONG WriteEvent(
    PresentMonProvider* ctxt,
    Event event,
    Ts... params)
{
    if (EventIsDisabled(*ctxt, event)) return ERROR_SUCCESS;

    EVENT_DATA_DESCRIPTOR data[sizeof...(Ts)];
    FillDesc(data, &params...);

    return (*ctxt->pEventWrite)(ctxt->ProviderHandle, &EventDescriptor[event], _countof(data), data);
}

bool IsValid(
    PresentMonProvider_FrameType frameType)
{
    return frameType == PresentMonProvider_FrameType_Unspecified ||
           frameType == PresentMonProvider_FrameType_Original ||
           frameType == PresentMonProvider_FrameType_Repeated ||
           frameType == PresentMonProvider_FrameType_Intel_XEFG ||
           frameType == PresentMonProvider_FrameType_AMD_AFMF;
}

bool IsValid(
    PresentMonProvider_InputType inputType)
{
    return (inputType & ~(PresentMonProvider_Input_MouseClick |
        PresentMonProvider_Input_KeyboardClick)) == 0;
}
}

PresentMonProvider* PresentMonProvider_Initialize()
{
    auto ctxt = new PresentMonProvider;
    if (ctxt == nullptr) {
        return nullptr;
    }

    ctxt->Advapi32Module = LoadLibraryExA("advapi32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (ctxt->Advapi32Module == nullptr) {
        delete ctxt;
        return nullptr;
    }

    ctxt->pEventRegister   = reinterpret_cast<decltype(ctxt->pEventRegister)>(GetProcAddress(ctxt->Advapi32Module, "EventRegister"));
    ctxt->pEventUnregister = reinterpret_cast<decltype(ctxt->pEventUnregister)>(GetProcAddress(ctxt->Advapi32Module, "EventUnregister"));
    ctxt->pEventWrite      = reinterpret_cast<decltype(ctxt->pEventWrite)>(GetProcAddress(ctxt->Advapi32Module, "EventWrite"));
    if (ctxt->pEventRegister == nullptr ||
        ctxt->pEventUnregister == nullptr ||
        ctxt->pEventWrite == nullptr) {
        FreeLibrary(ctxt->Advapi32Module);
        delete ctxt;
        return nullptr;
    }

    ctxt->EnableBits = 0;
    ULONG status = (*ctxt->pEventRegister)(&ProviderGUID, (PENABLECALLBACK) &EnableCallback, ctxt, &ctxt->ProviderHandle);
    if (status != ERROR_SUCCESS) {
        FreeLibrary(ctxt->Advapi32Module);
        delete ctxt;
        return nullptr;
    }

    return ctxt;
}

void PresentMonProvider_ShutDown(
    PresentMonProvider* ctxt)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    if (ctxt->ProviderHandle != 0) {
        ULONG e = (*ctxt->pEventUnregister)(ctxt->ProviderHandle);
        (void) e;
    }

    if (ctxt->Advapi32Module != nullptr) {
        FreeLibrary(ctxt->Advapi32Module);
    }

    RtlZeroMemory(ctxt, sizeof(PresentMonProvider));
    delete ctxt;
}

ULONG PresentMonProvider_PresentFrameType(
    PresentMonProvider* ctxt,
    uint32_t frameId,
    PresentMonProvider_FrameType frameType)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);
    PRESENTMONPROVIDER_ASSERT(IsValid(frameType));

    return WriteEvent(ctxt, Event_PresentFrameType, frameId,
                                                    (uint8_t) frameType);
}

ULONG PresentMonProvider_PresentFrameType(
    PresentMonProvider* ctxt,
    uint32_t frameId,
    PresentMonProvider_FrameType frameType,
    uint32_t appFrameId)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);
    PRESENTMONPROVIDER_ASSERT(IsValid(frameType));

    return WriteEvent(ctxt, Event_PresentFrameTypeV2, frameId,
                                                      (uint8_t)frameType,
                                                      appFrameId);
}

ULONG PresentMonProvider_FlipFrameType(
    PresentMonProvider* ctxt,
    uint32_t vidPnSourceId,
    uint32_t layerIndex,
    uint64_t presentId,
    PresentMonProvider_FrameType frameType)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);
    PRESENTMONPROVIDER_ASSERT(IsValid(frameType));

    return WriteEvent(ctxt, Event_FlipFrameType, vidPnSourceId,
                                                 layerIndex,
                                                 presentId,
                                                 (uint8_t) frameType);
}

ULONG PresentMonProvider_MeasuredInput(
    PresentMonProvider* ctxt,
    PresentMonProvider_InputType inputType,
    uint64_t inputQPCTime)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);
    PRESENTMONPROVIDER_ASSERT(IsValid(inputType));

    return WriteEvent(ctxt, Event_MeasuredInput, inputQPCTime,
                                                 (uint8_t) inputType);
}

ULONG PresentMonProvider_MeasuredScreenChange(
    PresentMonProvider* ctxt,
    uint64_t screenQPCTime)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_MeasuredScreenChange, screenQPCTime);
}

ULONG PresentMonProvider_Application_SleepStart(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppSleepStart, frame_id);
}

ULONG PresentMonProvider_Application_SleepEnd(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppSleepEnd, frame_id);
}

ULONG PresentMonProvider_Application_SimulationStart(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppSimulationStart, frame_id);
}

ULONG PresentMonProvider_Application_SimulationEnd(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppSimulationEnd, frame_id);
}

ULONG PresentMonProvider_Application_RenderSubmitStart(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppRenderSubmitStart, frame_id);
}

ULONG PresentMonProvider_Application_RenderSubmitEnd(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppRenderSubmitEnd, frame_id);
}

ULONG PresentMonProvider_Application_PresentStart(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppPresentStart, frame_id);
}

ULONG PresentMonProvider_Application_PresentEnd(
    PresentMonProvider* ctxt,
    uint32_t frame_id)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);

    return WriteEvent(ctxt, Event_AppPresentEnd, frame_id);
}

ULONG PresentMonProvider_Application_InputSample(
    PresentMonProvider* ctxt,
    uint32_t frame_id,
    PresentMonProvider_InputType inputType)
{
    PRESENTMONPROVIDER_ASSERT(ctxt != nullptr);
    PRESENTMONPROVIDER_ASSERT(IsValid(inputType));

    return WriteEvent(ctxt, Event_AppInputSample, frame_id, inputType);
}
