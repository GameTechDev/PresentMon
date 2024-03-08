// Copyright (C) 2019-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include "Debug.hpp"

#include "PresentMonTraceConsumer.hpp"

#include "ETW/Microsoft_Windows_D3D9.h"
#include "ETW/Microsoft_Windows_Dwm_Core.h"
#include "ETW/Microsoft_Windows_DXGI.h"
#include "ETW/Microsoft_Windows_DxgKrnl.h"
#include "ETW/Microsoft_Windows_Kernel_Process.h"
#include "ETW/Microsoft_Windows_Win32k.h"
#include "ETW/NT_Process.h"

#include <assert.h>
#include <dxgi.h>

namespace {

LARGE_INTEGER* gFirstTimestamp = nullptr;
LARGE_INTEGER gTimestampFrequency = {};

uint64_t ConvertTimestampDeltaToNs(uint64_t timestampDelta)
{
    return 1000000000ull * timestampDelta / gTimestampFrequency.QuadPart;
}

uint64_t ConvertTimestampToNs(uint64_t timestamp)
{
    return ConvertTimestampDeltaToNs(timestamp - gFirstTimestamp->QuadPart);
}

char* AddCommas(uint64_t t)
{
    static char buf[128];
    auto r = sprintf_s(buf, "%llu", t);

    auto commaCount = r == 0 ? 0 : ((r - 1) / 3);
    for (int i = 0; i < commaCount; ++i) {
        auto p = r + commaCount - 4 * i;
        auto q = r - 3 * i;
        buf[p - 1] = buf[q - 1];
        buf[p - 2] = buf[q - 2];
        buf[p - 3] = buf[q - 3];
        buf[p - 4] = ',';
    }

    r += commaCount;
    buf[r] = '\0';
    return buf;
}

}

void InitializeTimestampInfo(LARGE_INTEGER* firstTimestamp, LARGE_INTEGER const& timestampFrequency)
{
    gFirstTimestamp = firstTimestamp;
    gTimestampFrequency = timestampFrequency;
}

int PrintTime(uint64_t value)
{
    return value == 0
        ? wprintf(L"0")
        : value < (uint64_t) gFirstTimestamp->QuadPart
        ? wprintf(L"-%hs", AddCommas(ConvertTimestampDeltaToNs((uint64_t) gFirstTimestamp->QuadPart - value)))
        : wprintf(L"%hs", AddCommas(ConvertTimestampToNs(value)));
}

int PrintTimeDelta(uint64_t value)
{
    return wprintf(L"%hs", value == 0 ? "0" : AddCommas(ConvertTimestampDeltaToNs(value)));
}


#if PRESENTMON_ENABLE_DEBUG_TRACE

namespace {

bool gVerboseTraceEnabled = false;

PresentEvent const* gModifiedPresent = nullptr;
PresentEvent gOriginalPresentValues;

void PrintU32(uint32_t value) { wprintf(L"%u", value); }
void PrintU64(uint64_t value) { wprintf(L"%llu", value); }
void PrintU64x(uint64_t value) { wprintf(L"0x%llx", value); }
void PrintBool(bool value) { wprintf(L"%hs", value ? "true" : "false"); }
void PrintString(std::string const& value) { wprintf(L"%hs", value.c_str()); }
void PrintWString(std::wstring const& value) { wprintf(L"%s", value.c_str()); }
void PrintRuntime(Runtime value)
{
    switch (value) {
    case Runtime::DXGI:  wprintf(L"DXGI");  break;
    case Runtime::D3D9:  wprintf(L"D3D9");  break;
    case Runtime::Other: wprintf(L"Other"); break;
    default:             wprintf(L"Unknown (%u)", value); assert(false); break;
    }
}
void PrintPresentMode(PresentMode value)
{
    switch (value) {
    case PresentMode::Unknown:                              wprintf(L"Unknown"); break;
    case PresentMode::Hardware_Legacy_Flip:                 wprintf(L"Hardware_Legacy_Flip"); break;
    case PresentMode::Hardware_Legacy_Copy_To_Front_Buffer: wprintf(L"Hardware_Legacy_Copy_To_Front_Buffer"); break;
    case PresentMode::Hardware_Independent_Flip:            wprintf(L"Hardware_Independent_Flip"); break;
    case PresentMode::Composed_Flip:                        wprintf(L"Composed_Flip"); break;
    case PresentMode::Composed_Copy_GPU_GDI:                wprintf(L"Composed_Copy_GPU_GDI"); break;
    case PresentMode::Composed_Copy_CPU_GDI:                wprintf(L"Composed_Copy_CPU_GDI"); break;
    case PresentMode::Hardware_Composed_Independent_Flip:   wprintf(L"Hardware_Composed_Independent_Flip"); break;
    default:                                                wprintf(L"Unknown (%u)", value); assert(false); break;
    }
}
void PrintPresentResult(PresentResult value)
{
    switch (value) {
    case PresentResult::Unknown:   wprintf(L"Unknown");   break;
    case PresentResult::Presented: wprintf(L"Presented"); break;
    case PresentResult::Discarded: wprintf(L"Discarded"); break;
    default:                       wprintf(L"Unknown (%u)", value); assert(false); break;
    }
}
void PrintPresentHistoryModel(uint32_t model)
{
    using namespace Microsoft_Windows_DxgKrnl;
    switch (model) {
    case PresentModel::D3DKMT_PM_UNINITIALIZED:          wprintf(L"UNINITIALIZED");          break;
    case PresentModel::D3DKMT_PM_REDIRECTED_GDI:         wprintf(L"REDIRECTED_GDI");         break;
    case PresentModel::D3DKMT_PM_REDIRECTED_FLIP:        wprintf(L"REDIRECTED_FLIP");        break;
    case PresentModel::D3DKMT_PM_REDIRECTED_BLT:         wprintf(L"REDIRECTED_BLT");         break;
    case PresentModel::D3DKMT_PM_REDIRECTED_VISTABLT:    wprintf(L"REDIRECTED_VISTABLT");    break;
    case PresentModel::D3DKMT_PM_SCREENCAPTUREFENCE:     wprintf(L"SCREENCAPTUREFENCE");     break;
    case PresentModel::D3DKMT_PM_REDIRECTED_GDI_SYSMEM:  wprintf(L"REDIRECTED_GDI_SYSMEM");  break;
    case PresentModel::D3DKMT_PM_REDIRECTED_COMPOSITION: wprintf(L"REDIRECTED_COMPOSITION"); break;
    case PresentModel::D3DKMT_PM_SURFACECOMPLETE:        wprintf(L"SURFACECOMPLETE");        break;
    case PresentModel::D3DKMT_PM_FLIPMANAGER:            wprintf(L"FLIPMANAGER");            break;
    default:                                             wprintf(L"Unknown (%u)", model); assert(false); break;
    }
}
void PrintQueuePacketType(uint32_t type)
{
    using namespace Microsoft_Windows_DxgKrnl;
    switch (type) {
    case QueuePacketType::DXGKETW_RENDER_COMMAND_BUFFER:   wprintf(L"RENDER"); break;
    case QueuePacketType::DXGKETW_DEFERRED_COMMAND_BUFFER: wprintf(L"DEFERRED"); break;
    case QueuePacketType::DXGKETW_SYSTEM_COMMAND_BUFFER:   wprintf(L"SYSTEM"); break;
    case QueuePacketType::DXGKETW_MMIOFLIP_COMMAND_BUFFER: wprintf(L"MMIOFLIP"); break;
    case QueuePacketType::DXGKETW_WAIT_COMMAND_BUFFER:     wprintf(L"WAIT"); break;
    case QueuePacketType::DXGKETW_SIGNAL_COMMAND_BUFFER:   wprintf(L"SIGNAL"); break;
    case QueuePacketType::DXGKETW_DEVICE_COMMAND_BUFFER:   wprintf(L"DEVICE"); break;
    case QueuePacketType::DXGKETW_SOFTWARE_COMMAND_BUFFER: wprintf(L"SOFTWARE"); break;
    case QueuePacketType::DXGKETW_PAGING_COMMAND_BUFFER:   wprintf(L"PAGING"); break;
    default:                                               wprintf(L"Unknown (%u)", type); assert(false); break;
    }
}
void PrintDmaPacketType(uint32_t type)
{
    using namespace Microsoft_Windows_DxgKrnl;
    switch (type) {
    case DmaPacketType::DXGKETW_CLIENT_RENDER_BUFFER:    wprintf(L"CLIENT_RENDER"); break;
    case DmaPacketType::DXGKETW_CLIENT_PAGING_BUFFER:    wprintf(L"CLIENT_PAGING"); break;
    case DmaPacketType::DXGKETW_SYSTEM_PAGING_BUFFER:    wprintf(L"SYSTEM_PAGING"); break;
    case DmaPacketType::DXGKETW_SYSTEM_PREEMTION_BUFFER: wprintf(L"SYSTEM_PREEMTION"); break;
    default:                                             wprintf(L"Unknown (%u)", type); assert(false); break;
    }
}
void PrintPresentFlags(uint32_t flags)
{
    if (flags & DXGI_PRESENT_TEST) wprintf(L"TEST");
}

void PrintEventHeader(EVENT_HEADER const& hdr)
{
    wprintf(L"%16hs %5u %5u ", AddCommas(ConvertTimestampToNs(hdr.TimeStamp.QuadPart)), hdr.ProcessId, hdr.ThreadId);
}

void PrintEventHeader(EVENT_HEADER const& hdr, char const* name)
{
    PrintEventHeader(hdr);
    wprintf(L"%hs\n", name);
}

void PrintEventHeader(EVENT_RECORD* eventRecord, EventMetadata* metadata, char const* name, std::initializer_list<void*> props)
{
    assert((props.size() % 2) == 0);

    PrintEventHeader(eventRecord->EventHeader);
    wprintf(L"%hs", name);
    for (auto ii = props.begin(), ie = props.end(); ii != ie; ++ii) {
        auto propName = (wchar_t const*) *ii; ++ii;
        auto propFunc = *ii;

        wprintf(L" %s=", propName);

             if (propFunc == PrintBool)                 PrintBool(metadata->GetEventData<uint32_t>(eventRecord, propName) != 0);
        else if (propFunc == PrintU32)                  PrintU32(metadata->GetEventData<uint32_t>(eventRecord, propName));
        else if (propFunc == PrintU64)                  PrintU64(metadata->GetEventData<uint64_t>(eventRecord, propName));
        else if (propFunc == PrintU64x)                 PrintU64x(metadata->GetEventData<uint64_t>(eventRecord, propName));
        else if (propFunc == PrintString)               PrintString(metadata->GetEventData<std::string>(eventRecord, propName));
        else if (propFunc == PrintWString)              PrintWString(metadata->GetEventData<std::wstring>(eventRecord, propName));
        else if (propFunc == PrintTime)                 PrintTime(metadata->GetEventData<uint64_t>(eventRecord, propName));
        else if (propFunc == PrintTimeDelta)            PrintTimeDelta(metadata->GetEventData<uint64_t>(eventRecord, propName));
        else if (propFunc == PrintQueuePacketType)      PrintQueuePacketType(metadata->GetEventData<uint32_t>(eventRecord, propName));
        else if (propFunc == PrintDmaPacketType)        PrintDmaPacketType(metadata->GetEventData<uint32_t>(eventRecord, propName));
        else if (propFunc == PrintPresentFlags)         PrintPresentFlags(metadata->GetEventData<uint32_t>(eventRecord, propName));
        else if (propFunc == PrintPresentHistoryModel)  PrintPresentHistoryModel(metadata->GetEventData<uint32_t>(eventRecord, propName));
        else assert(false);
    }
    wprintf(L"\n");
}

void FlushModifiedPresent()
{
    if (gModifiedPresent == nullptr) return;

    uint32_t changedCount = 0;

#define FLUSH_MEMBER(_Fn, _Name) \
    if (gModifiedPresent->_Name != gOriginalPresentValues._Name) { \
        if (changedCount++ == 0) { \
            wprintf(L"%*hsp%llu", 17 + 6 + 6, "", gModifiedPresent->Id); \
        } \
        wprintf(L" " L#_Name L"="); \
        _Fn(gOriginalPresentValues._Name); \
        wprintf(L"->"); \
        _Fn(gModifiedPresent->_Name); \
    }
    FLUSH_MEMBER(PrintTime,          PresentStopTime)
    FLUSH_MEMBER(PrintTime,          ReadyTime)
    FLUSH_MEMBER(PrintTime,          ScreenTime)
    FLUSH_MEMBER(PrintTime,          InputTime)
    FLUSH_MEMBER(PrintTime,          GPUStartTime)
    FLUSH_MEMBER(PrintTimeDelta,     GPUDuration)
    FLUSH_MEMBER(PrintTimeDelta,     GPUVideoDuration)
    FLUSH_MEMBER(PrintU64x,          SwapChainAddress)
    FLUSH_MEMBER(PrintU32,           SyncInterval)
    FLUSH_MEMBER(PrintU32,           PresentFlags)
    FLUSH_MEMBER(PrintU64x,          Hwnd)
    FLUSH_MEMBER(PrintU64x,          DxgkPresentHistoryToken)
    FLUSH_MEMBER(PrintU32,           QueueSubmitSequence)
    FLUSH_MEMBER(PrintU32,           DriverThreadId)
    FLUSH_MEMBER(PrintPresentMode,   PresentMode)
    FLUSH_MEMBER(PrintPresentResult, FinalState)
    FLUSH_MEMBER(PrintBool,          SupportsTearing)
    FLUSH_MEMBER(PrintBool,          WaitForFlipEvent)
    FLUSH_MEMBER(PrintBool,          WaitForMPOFlipEvent)
    FLUSH_MEMBER(PrintBool,          SeenDxgkPresent)
    FLUSH_MEMBER(PrintBool,          SeenWin32KEvents)
    FLUSH_MEMBER(PrintBool,          DwmNotified)
    FLUSH_MEMBER(PrintBool,          IsCompleted)
    FLUSH_MEMBER(PrintBool,          IsLost)
    FLUSH_MEMBER(PrintU32,           DeferredCompletionWaitCount)
#undef FLUSH_MEMBER

    if (changedCount > 0) {
        wprintf(L"\n");
    }

    gModifiedPresent = nullptr;
}

uint64_t LookupPresentId(
    PMTraceConsumer* pmConsumer,
    uint64_t CompositionSurfaceLuid,
    uint64_t PresentCount,
    uint64_t BindId)
{
    // pmConsumer can complete presents before they've seen all of
    // their TokenStateChanged_Info events, so we keep a copy of the
    // token->present id map here simply so we can print what present
    // the event refers to.
    static std::unordered_map<
        PMTraceConsumer::Win32KPresentHistoryToken,
        uint64_t,
        PMTraceConsumer::Win32KPresentHistoryTokenHash> tokenToIdMap;

    PMTraceConsumer::Win32KPresentHistoryToken key(CompositionSurfaceLuid, PresentCount, BindId);
    auto ii = pmConsumer->mPresentByWin32KPresentHistoryToken.find(key);
    if (ii != pmConsumer->mPresentByWin32KPresentHistoryToken.end()) {
        tokenToIdMap[key] = ii->second->Id;
        return ii->second->Id;
    }

    auto jj = tokenToIdMap.find(key);
    if (jj != tokenToIdMap.end()) {
        return jj->second;
    }

    return 0;
}

}

void EnableVerboseTrace(bool enable)
{
    gVerboseTraceEnabled = enable;
}

bool IsVerboseTraceEnabled()
{
    return gVerboseTraceEnabled;
}

void DebugAssertImpl(wchar_t const* msg, wchar_t const* file, int line)
{
    if (IsVerboseTraceEnabled()) {
        wprintf(L"ASSERTION FAILED: %s(%d): %s\n", file, line, msg);
    } else {
        #ifndef NDEBUG
        _wassert(msg, file, line);
        #endif
    }
}

void VerboseTraceEventImpl(PMTraceConsumer* pmConsumer, EVENT_RECORD* eventRecord, EventMetadata* metadata)
{
    auto const& hdr = eventRecord->EventHeader;

    static bool isFirstEventTraced = true;
    if (isFirstEventTraced) {
        isFirstEventTraced = false;
        wprintf(L"       Time (ns)   PID   TID EVENT\n");
    }

    FlushModifiedPresent();

    if (hdr.ProviderId == Microsoft_Windows_D3D9::GUID) {
        using namespace Microsoft_Windows_D3D9;
        switch (hdr.EventDescriptor.Id) {
        case Present_Start::Id: PrintEventHeader(hdr, "D3D9PresentStart"); break;
        case Present_Stop::Id:  PrintEventHeader(hdr, "D3D9PresentStop"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_DXGI::GUID) {
        using namespace Microsoft_Windows_DXGI;
        switch (hdr.EventDescriptor.Id) {
        case Present_Start::Id:                  PrintEventHeader(eventRecord, metadata, "DXGIPresent_Start",    { L"Flags", PrintPresentFlags, }); break;
        case PresentMultiplaneOverlay_Start::Id: PrintEventHeader(eventRecord, metadata, "DXGIPresentMPO_Start", { L"Flags", PrintPresentFlags, }); break;
        case Present_Stop::Id:                   PrintEventHeader(hdr, "DXGIPresent_Stop"); break;
        case PresentMultiplaneOverlay_Stop::Id:  PrintEventHeader(hdr, "DXGIPresentMPO_Stop"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::BLT_GUID)            { PrintEventHeader(hdr, "Win7::BLT"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::FLIP_GUID)           { PrintEventHeader(hdr, "Win7::FLIP"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::PRESENTHISTORY_GUID) { PrintEventHeader(hdr, "Win7::PRESENTHISTORY"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::QUEUEPACKET_GUID)    { PrintEventHeader(hdr, "Win7::QUEUEPACKET"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::VSYNCDPC_GUID)       { PrintEventHeader(hdr, "Win7::VSYNCDPC"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::MMIOFLIP_GUID)       { PrintEventHeader(hdr, "Win7::MMIOFLIP"); return; }

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::GUID) {
        using namespace Microsoft_Windows_DxgKrnl;
        switch (hdr.EventDescriptor.Id) {
        case Blit_Info::Id:                     PrintEventHeader(hdr, "Blit_Info"); break;
        case BlitCancel_Info::Id:               PrintEventHeader(hdr, "BlitCancel_Info"); break;
        case FlipMultiPlaneOverlay_Info::Id:    PrintEventHeader(hdr, "FlipMultiPlaneOverlay_Info"); break;
        case Present_Info::Id:                  PrintEventHeader(hdr, "DxgKrnl_Present_Info"); break;

        case MMIOFlip_Info::Id:                 PrintEventHeader(eventRecord, metadata, "MMIOFlip_Info",                { L"FlipSubmitSequence", PrintU64, }); break;
        case Flip_Info::Id:                     PrintEventHeader(eventRecord, metadata, "Flip_Info",                    { L"FlipInterval",   PrintU32,
                                                                                                                          L"MMIOFlip",       PrintBool, }); break;
        case IndependentFlip_Info::Id:          PrintEventHeader(eventRecord, metadata, "IndependentFlip_Info",         { L"SubmitSequence", PrintU32,
                                                                                                                          L"FlipInterval",   PrintU32, }); break;
        case PresentHistory_Start::Id:          PrintEventHeader(eventRecord, metadata, "PresentHistory_Start",         { L"Token",          PrintU64x,
                                                                                                                          L"Model",          PrintPresentHistoryModel, }); break;
        case PresentHistory_Info::Id:           PrintEventHeader(eventRecord, metadata, "PresentHistory_Info",          { L"Token",          PrintU64x,
                                                                                                                          L"Model",          PrintPresentHistoryModel, }); break;
        case PresentHistoryDetailed_Start::Id:  PrintEventHeader(eventRecord, metadata, "PresentHistoryDetailed_Start", { L"Token",          PrintU64x,
                                                                                                                          L"Model",          PrintPresentHistoryModel, }); break;
        case QueuePacket_Start::Id:             PrintEventHeader(eventRecord, metadata, "QueuePacket_Start",            { L"hContext",       PrintU64x,
                                                                                                                          L"SubmitSequence", PrintU32,
                                                                                                                          L"PacketType",     PrintQueuePacketType,
                                                                                                                          L"bPresent",       PrintU32, }); break;
        case QueuePacket_Start_2::Id:           PrintEventHeader(eventRecord, metadata, "QueuePacket_Start WAIT",       { L"hContext",       PrintU64x,
                                                                                                                          L"SubmitSequence", PrintU32, }); break;
        case QueuePacket_Stop::Id:              PrintEventHeader(eventRecord, metadata, "QueuePacket_Stop",             { L"hContext",       PrintU64x,
                                                                                                                          L"SubmitSequence", PrintU32, }); break;
        case Context_DCStart::Id:
        case Context_Start::Id:                 PrintEventHeader(eventRecord, metadata, "Context_Start",                { L"hContext",       PrintU64x,
                                                                                                                          L"hDevice",        PrintU64x,
                                                                                                                          L"NodeOrdinal",    PrintU32, }); break;
        case Context_Stop::Id:                  PrintEventHeader(eventRecord, metadata, "Context_Stop",                 { L"hContext",       PrintU64x, }); break;
        case Device_DCStart::Id:
        case Device_Start::Id:                  PrintEventHeader(eventRecord, metadata, "Device_Start",                 { L"hDevice",        PrintU64x,
                                                                                                                          L"pDxgAdapter",    PrintU64x, }); break;
        case Device_Stop::Id:                   PrintEventHeader(eventRecord, metadata, "Device_Stop",                  { L"hDevice",        PrintU64x, }); break;
        case HwQueue_DCStart::Id:
        case HwQueue_Start::Id:                 PrintEventHeader(eventRecord, metadata, "HwQueue_Start",                { L"hContext", PrintU64x,
                                                                                                                          L"hHwQueue", PrintU64x,
                                                                                                                          L"ParentDxgHwQueue", PrintU64x, }); break;
        case DmaPacket_Info::Id:                PrintEventHeader(eventRecord, metadata, "DmaPacket_Info",               { L"hContext",       PrintU64x,
                                                                                                                          L"ulQueueSubmitSequence", PrintU32,
                                                                                                                          L"PacketType",     PrintDmaPacketType, }); break;
        case DmaPacket_Start::Id:               PrintEventHeader(eventRecord, metadata, "DmaPacket_Start",              { L"hContext",       PrintU64x,
                                                                                                                          L"ulQueueSubmitSequence", PrintU32, }); break;
        case VSyncDPC_Info::Id: {
            auto FlipFenceId = metadata->GetEventData<uint64_t>(eventRecord, L"FlipFenceId");
            PrintEventHeader(hdr);
            wprintf(L"VSyncDPC_Info SubmitSequence=%llu FlipId=0x%llx\n",
                FlipFenceId >> 32,
                FlipFenceId & 0xffffffffll);
            break;
        }
        case HSyncDPCMultiPlane_Info::Id:
        case VSyncDPCMultiPlane_Info::Id: {
            EventDataDesc desc[] = {
                { L"FlipEntryCount" },
                { L"FlipSubmitSequence" },
            };

            metadata->GetEventData(eventRecord, desc, _countof(desc));
            auto FlipEntryCount     = desc[0].GetData<uint32_t>();
            auto FlipSubmitSequence = desc[1].GetArray<uint64_t>(FlipEntryCount);

            PrintEventHeader(hdr);
            wprintf(L"%hs", hdr.EventDescriptor.Id == HSyncDPCMultiPlane_Info::Id ? "HSyncDPCMultiPlane_Info" : "VSyncDPCMultiPlane_Info");
            for (uint32_t i = 0; i < FlipEntryCount; ++i) {
                if (i > 0) wprintf(L"\n                                                    ");
                wprintf(L" SubmitSequence[%u]=%llu FlipId[%u]=0x%llx",
                    i, FlipSubmitSequence[i] >> 32,
                    i, FlipSubmitSequence[i] & 0xffffffffll);
            }
            wprintf(L"\n");
            break;
        }
        case MMIOFlipMultiPlaneOverlay_Info::Id: {
            auto FlipSubmitSequence = metadata->GetEventData<uint64_t>(eventRecord, L"FlipSubmitSequence");
            PrintEventHeader(hdr);
            wprintf(L"DXGKrnl_MMIOFlipMultiPlaneOverlay_Info SubmitSequence=%llu FlipId=0x%llx",
                FlipSubmitSequence >> 32,
                FlipSubmitSequence & 0xffffffffll);
            if (hdr.EventDescriptor.Version >= 2) {
                switch (metadata->GetEventData<uint32_t>(eventRecord, L"FlipEntryStatusAfterFlip")) {
                case FlipEntryStatus::FlipWaitVSync:    wprintf(L" FlipWaitVSync"); break;
                case FlipEntryStatus::FlipWaitComplete: wprintf(L" FlipWaitComplete"); break;
                case FlipEntryStatus::FlipWaitHSync:    wprintf(L" FlipWaitHSync"); break;
                }
            }
            wprintf(L"\n");
            break;
        }
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Dwm_Core::GUID ||
        hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID) {
        using namespace Microsoft_Windows_Dwm_Core;
        switch (hdr.EventDescriptor.Id) {
        case MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id:
                                              PrintEventHeader(hdr, "DWM_GetPresentHistory"); break;
        case SCHEDULE_PRESENT_Start::Id:      PrintEventHeader(hdr, "DWM_SCHEDULE_PRESENT_Start"); break;
        case FlipChain_Pending::Id:           PrintEventHeader(hdr, "DWM_FlipChain_Pending"); break;
        case FlipChain_Complete::Id:          PrintEventHeader(hdr, "DWM_FlipChain_Complete"); break;
        case FlipChain_Dirty::Id:             PrintEventHeader(hdr, "DWM_FlipChain_Dirty"); break;
        case SCHEDULE_SURFACEUPDATE_Info::Id: PrintEventHeader(hdr, "DWM_Schedule_SurfaceUpdate"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Kernel_Process::GUID) {
        using namespace Microsoft_Windows_Kernel_Process;
        switch (hdr.EventDescriptor.Id) {
        case ProcessStart_Start::Id: PrintEventHeader(eventRecord, metadata, "ProcessStart", { L"ProcessID", PrintU32, L"ImageName", PrintWString }); break;
        case ProcessStop_Stop::Id:   PrintEventHeader(eventRecord, metadata, "ProcessStop",  { L"ProcessID", PrintU32, L"ImageName", PrintString }); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Win32k::GUID) {
        using namespace Microsoft_Windows_Win32k;
        switch (hdr.EventDescriptor.Id) {
        case TokenCompositionSurfaceObject_Info::Id: PrintEventHeader(hdr, "Win32k_TokenCompositionSurfaceObject"); break;
        case InputDeviceRead_Stop::Id:               PrintEventHeader(eventRecord, metadata, "Win32k_InputDeviceRead_Stop", { L"DeviceType", PrintU32, }); break;
        case RetrieveInputMessage_Info::Id:          PrintEventHeader(eventRecord, metadata, "Win32k_RetrieveInputMessage", { L"flags",      PrintU32, }); break;
        case TokenStateChanged_Info::Id: {
            EventDataDesc desc[] = {
                { L"CompositionSurfaceLuid" },
                { L"PresentCount" },
                { L"BindId" },
                { L"NewState" },
            };
            metadata->GetEventData(eventRecord, desc, _countof(desc));
            auto CompositionSurfaceLuid = desc[0].GetData<uint64_t>();
            auto PresentCount           = desc[1].GetData<uint32_t>();
            auto BindId                 = desc[2].GetData<uint64_t>();
            auto NewState               = desc[3].GetData<uint32_t>();

            PrintEventHeader(hdr);
            wprintf(L"Win32K_TokenStateChanged");

            auto presentId = LookupPresentId(pmConsumer, CompositionSurfaceLuid, PresentCount, BindId);
            if (presentId == 0) {
                wprintf(L" (unknown present)");
            } else {
                wprintf(L" p%llu", presentId);
            }

            switch (NewState) {
            case TokenState::Completed: wprintf(L" NewState=Completed"); break;
            case TokenState::InFrame:   wprintf(L" NewState=InFrame");   break;
            case TokenState::Confirmed: wprintf(L" NewState=Confirmed"); break;
            case TokenState::Retired:   wprintf(L" NewState=Retired");   break;
            case TokenState::Discarded: wprintf(L" NewState=Discarded"); break;
            default:                    wprintf(L" NewState=Unknown (%u)", NewState); assert(false); break;
            }

            wprintf(L"\n");
        }   break;
        }
        return;
    }

    if (hdr.ProviderId == NT_Process::GUID) {
        if (hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START ||
            hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_DC_START) {
            PrintEventHeader(eventRecord, metadata, "ProcessStart", { L"ProcessId",     PrintU32,
                                                                      L"ImageFileName", PrintString, });
        } else if (hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_END||
                   hdr.EventDescriptor.Opcode == EVENT_TRACE_TYPE_DC_END) {
            PrintEventHeader(eventRecord, metadata, "ProcessStop",  { L"ProcessId",     PrintU32 });
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Kernel_Process::GUID) {
        using namespace Microsoft_Windows_Kernel_Process;
        switch (hdr.EventDescriptor.Id) {
        case ProcessStart_Start::Id: PrintEventHeader(eventRecord, metadata, "ProcessStart", { L"ProcessID", PrintU32,
                                                                                               L"ImageName", PrintString, }); break;
        case ProcessStop_Stop::Id:   PrintEventHeader(eventRecord, metadata, "ProcessStop",  { L"ProcessID", PrintU32 }); break;
        }
        return;
    }
}

void VerboseTraceBeforeModifyingPresentImpl(PresentEvent const* p)
{
    if (gModifiedPresent != p) {
        FlushModifiedPresent();
        gModifiedPresent = p;
        if (p != nullptr) {
            gOriginalPresentValues = *p;
        }
    }
}

#endif
