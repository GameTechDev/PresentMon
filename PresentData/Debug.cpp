/*
Copyright 2019-2021 Intel Corporation

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

#include "PresentMonTraceConsumer.hpp"

#include "ETW/Microsoft_Windows_D3D9.h"
#include "ETW/Microsoft_Windows_Dwm_Core.h"
#include "ETW/Microsoft_Windows_DXGI.h"
#include "ETW/Microsoft_Windows_DxgKrnl.h"
#include "ETW/Microsoft_Windows_Win32k.h"

#include <assert.h>
#include <dxgi.h>
#include <initializer_list>

#if DEBUG_VERBOSE

namespace {

    FILE* pDebugFile = stdout;
    PresentEvent const* gModifiedPresent = nullptr;
    struct {
        uint64_t TimeTaken;
        uint64_t ReadyTime;
        uint64_t ScreenTime;

        uint64_t SwapChainAddress;
        int32_t SyncInterval;
        uint32_t PresentFlags;

        uint64_t Hwnd;
        uint64_t TokenPtr;
        uint32_t QueueSubmitSequence;
        uint32_t DriverBatchThreadId;
        PresentMode PresentMode;
        PresentResult FinalState;
        bool SupportsTearing;
        bool MMIO;
        bool SeenDxgkPresent;
        bool SeenWin32KEvents;
        bool DwmNotified;
        bool Completed;
    } gOriginalPresentValues;

    bool gDebugDone = false;
    bool gDebugTrace = false;
    LARGE_INTEGER* gFirstTimestamp = nullptr;
    LARGE_INTEGER gTimestampFrequency = {};

    uint64_t ConvertTimestampDeltaToNs(uint64_t timestampDelta)
    {
        return 1000000000ull * timestampDelta / gTimestampFrequency.QuadPart;
    }

    uint64_t ConvertTimestampToNs(LARGE_INTEGER timestamp)
    {
        return ConvertTimestampDeltaToNs(timestamp.QuadPart - gFirstTimestamp->QuadPart);
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

    void PrintU32(uint32_t value) { fprintf(pDebugFile, "%u", value); }
    void PrintU64(uint64_t value) { fprintf(pDebugFile, "%llu", value); }
    void PrintU64x(uint64_t value) { fprintf(pDebugFile, "%llx", value); }
    void PrintTimeDelta(uint64_t value) { fprintf(pDebugFile, "%s", AddCommas(ConvertTimestampDeltaToNs(value))); }
    void PrintBool(bool value) { fprintf(pDebugFile, "%s", value ? "true" : "false"); }
    void PrintRuntime(Runtime value)
    {
        switch (value) {
        case Runtime::DXGI:  fprintf(pDebugFile, "DXGI");  break;
        case Runtime::D3D9:  fprintf(pDebugFile, "D3D9");  break;
        case Runtime::Other: fprintf(pDebugFile, "Other"); break;
        default:             fprintf(pDebugFile, "ERROR"); break;
        }
    }
    void PrintPresentMode(PresentMode value)
    {
        switch (value) {
        case PresentMode::Unknown:                              fprintf(pDebugFile, "Unknown"); break;
        case PresentMode::Hardware_Legacy_Flip:                 fprintf(pDebugFile, "Hardware_Legacy_Flip"); break;
        case PresentMode::Hardware_Legacy_Copy_To_Front_Buffer: fprintf(pDebugFile, "Hardware_Legacy_Copy_To_Front_Buffer"); break;
        case PresentMode::Hardware_Independent_Flip:            fprintf(pDebugFile, "Hardware_Independent_Flip"); break;
        case PresentMode::Composed_Flip:                        fprintf(pDebugFile, "Composed_Flip"); break;
        case PresentMode::Composed_Copy_GPU_GDI:                fprintf(pDebugFile, "Composed_Copy_GPU_GDI"); break;
        case PresentMode::Composed_Copy_CPU_GDI:                fprintf(pDebugFile, "Composed_Copy_CPU_GDI"); break;
        case PresentMode::Composed_Composition_Atlas:           fprintf(pDebugFile, "Composed_Composition_Atlas"); break;
        case PresentMode::Hardware_Composed_Independent_Flip:   fprintf(pDebugFile, "Hardware_Composed_Independent_Flip"); break;
        default:                                                fprintf(pDebugFile, "ERROR"); break;
        }
    }
    void PrintPresentResult(PresentResult value)
    {
        switch (value) {
        case PresentResult::Unknown:   fprintf(pDebugFile, "Unknown");   break;
        case PresentResult::Presented: fprintf(pDebugFile, "Presented"); break;
        case PresentResult::Discarded: fprintf(pDebugFile, "Discarded"); break;
        case PresentResult::Error:     fprintf(pDebugFile, "Error");     break;
        default:                       fprintf(pDebugFile, "ERROR");     break;
        }
    }
    void PrintPresentHistoryModel(uint32_t model)
    {
        switch (model) {
        case D3DKMT_PM_UNINITIALIZED:          fprintf(pDebugFile, "UNINITIALIZED");          break;
        case D3DKMT_PM_REDIRECTED_GDI:         fprintf(pDebugFile, "REDIRECTED_GDI");         break;
        case D3DKMT_PM_REDIRECTED_FLIP:        fprintf(pDebugFile, "REDIRECTED_FLIP");        break;
        case D3DKMT_PM_REDIRECTED_BLT:         fprintf(pDebugFile, "REDIRECTED_BLT");         break;
        case D3DKMT_PM_REDIRECTED_VISTABLT:    fprintf(pDebugFile, "REDIRECTED_VISTABLT");    break;
        case D3DKMT_PM_SCREENCAPTUREFENCE:     fprintf(pDebugFile, "SCREENCAPTUREFENCE");     break;
        case D3DKMT_PM_REDIRECTED_GDI_SYSMEM:  fprintf(pDebugFile, "REDIRECTED_GDI_SYSMEM");  break;
        case D3DKMT_PM_REDIRECTED_COMPOSITION: fprintf(pDebugFile, "REDIRECTED_COMPOSITION"); break;
        default:                               fprintf(pDebugFile, "Unknown (%u)", model); assert(false); break;
        }
    }
    void PrintTokenState(uint32_t state)
    {
        switch (state) {
        case Microsoft_Windows_Win32k::TokenState::Completed: fprintf(pDebugFile, "Completed"); break;
        case Microsoft_Windows_Win32k::TokenState::InFrame:   fprintf(pDebugFile, "InFrame");   break;
        case Microsoft_Windows_Win32k::TokenState::Confirmed: fprintf(pDebugFile, "Confirmed"); break;
        case Microsoft_Windows_Win32k::TokenState::Retired:   fprintf(pDebugFile, "Retired");   break;
        case Microsoft_Windows_Win32k::TokenState::Discarded: fprintf(pDebugFile, "Discarded"); break;
        default:                                              fprintf(pDebugFile, "Unknown (%u)", state); assert(false); break;
        }
    }
    void PrintQueuePacketType(uint32_t type)
    {
        switch (type) {
        case DXGKETW_RENDER_COMMAND_BUFFER:   fprintf(pDebugFile, "RENDER"); break;
        case DXGKETW_DEFERRED_COMMAND_BUFFER: fprintf(pDebugFile, "DEFERRED"); break;
        case DXGKETW_SYSTEM_COMMAND_BUFFER:   fprintf(pDebugFile, "SYSTEM"); break;
        case DXGKETW_MMIOFLIP_COMMAND_BUFFER: fprintf(pDebugFile, "MMIOFLIP"); break;
        case DXGKETW_WAIT_COMMAND_BUFFER:     fprintf(pDebugFile, "WAIT"); break;
        case DXGKETW_SIGNAL_COMMAND_BUFFER:   fprintf(pDebugFile, "SIGNAL"); break;
        case DXGKETW_DEVICE_COMMAND_BUFFER:   fprintf(pDebugFile, "DEVICE"); break;
        case DXGKETW_SOFTWARE_COMMAND_BUFFER: fprintf(pDebugFile, "SOFTWARE"); break;
        case DXGKETW_PAGING_COMMAND_BUFFER:   fprintf(pDebugFile, "PAGING"); break;
        default:                              fprintf(pDebugFile, "Unknown (%u)", type); assert(false); break;
        }
    }
    void PrintPresentFlags(uint32_t flags)
    {
        if (flags & DXGI_PRESENT_TEST) fprintf(pDebugFile, "TEST");
    }

    void PrintEventHeader(EVENT_HEADER const& hdr)
    {
        fprintf(pDebugFile, "%16s %5u %5u ", AddCommas(ConvertTimestampToNs(hdr.TimeStamp)), hdr.ProcessId, hdr.ThreadId);
    }

    void PrintEventHeader(EVENT_HEADER const& hdr, char const* name)
    {
        PrintEventHeader(hdr);
        fprintf(pDebugFile, "%s\n", name);
    }

    void PrintEventHeader(EVENT_RECORD* eventRecord, EventMetadata* metadata, char const* name, std::initializer_list<void*> props)
    {
        assert((props.size() % 2) == 0);

        PrintEventHeader(eventRecord->EventHeader);
        fprintf(pDebugFile, "%s", name);
        for (auto ii = props.begin(), ie = props.end(); ii != ie; ++ii) {
            auto propName = (wchar_t const*)*ii; ++ii;
            auto propFunc = *ii;

            fprintf(pDebugFile, " %ls=", propName);

            if (propFunc == PrintU32)                  PrintU32(metadata->GetEventData<uint32_t>(eventRecord, propName));
            else if (propFunc == PrintU64x)                 PrintU64x(metadata->GetEventData<uint64_t>(eventRecord, propName));
            else if (propFunc == PrintTokenState)           PrintTokenState(metadata->GetEventData<uint32_t>(eventRecord, propName));
            else if (propFunc == PrintQueuePacketType)      PrintQueuePacketType(metadata->GetEventData<uint32_t>(eventRecord, propName));
            else if (propFunc == PrintPresentFlags)         PrintPresentFlags(metadata->GetEventData<uint32_t>(eventRecord, propName));
            else if (propFunc == PrintPresentHistoryModel)  PrintPresentHistoryModel(metadata->GetEventData<uint32_t>(eventRecord, propName));
            else assert(false);
        }
        fprintf(pDebugFile, "\n");
    }

    void PrintUpdateHeader(uint64_t id, int indent = 0)
    {
        fprintf(pDebugFile, "%*sp%llu", 17 + 6 + 6 + indent * 4, "", id);
    }

    void FlushModifiedPresent()
    {
        if (gModifiedPresent == nullptr) return;

        uint32_t changedCount = 0;
#define FLUSH_MEMBER(_Fn, _Name) \
    if (gModifiedPresent->_Name != gOriginalPresentValues._Name) { \
        if (changedCount++ == 0) PrintUpdateHeader(gModifiedPresent->Id); \
        fprintf( pDebugFile," " #_Name "="); \
        _Fn(gOriginalPresentValues._Name); \
        fprintf( pDebugFile,"->"); \
        _Fn(gModifiedPresent->_Name); \
    }
        FLUSH_MEMBER(PrintTimeDelta, TimeTaken)
            FLUSH_MEMBER(PrintTimeDelta, ReadyTime)
            FLUSH_MEMBER(PrintTimeDelta, ScreenTime)
            FLUSH_MEMBER(PrintU64x, SwapChainAddress)
            FLUSH_MEMBER(PrintU32, SyncInterval)
            FLUSH_MEMBER(PrintU32, PresentFlags)
            FLUSH_MEMBER(PrintU64x, Hwnd)
            FLUSH_MEMBER(PrintU64x, TokenPtr)
            FLUSH_MEMBER(PrintU32, QueueSubmitSequence)
            FLUSH_MEMBER(PrintU32, DriverBatchThreadId)
            FLUSH_MEMBER(PrintPresentMode, PresentMode)
            FLUSH_MEMBER(PrintPresentResult, FinalState)
            FLUSH_MEMBER(PrintBool, SupportsTearing)
            FLUSH_MEMBER(PrintBool, MMIO)
            FLUSH_MEMBER(PrintBool, SeenDxgkPresent)
            FLUSH_MEMBER(PrintBool, SeenWin32KEvents)
            FLUSH_MEMBER(PrintBool, DwmNotified)
            FLUSH_MEMBER(PrintBool, Completed)
#undef FLUSH_MEMBER
            if (changedCount > 0) {
                fprintf(pDebugFile, "\n");
            }

        gModifiedPresent = nullptr;
    }

}

void DebugInitialize(LARGE_INTEGER* firstTimestamp, LARGE_INTEGER timestampFrequency)
{
    gDebugDone = false;
    gFirstTimestamp = firstTimestamp;
    gTimestampFrequency = timestampFrequency;

    if (nullptr == pDebugFile)
    {
        fopen_s(&pDebugFile, "c:\\users\\blk\\Documents\\benchmark\\debug.txt", "wb");
        ffprintf(pDebugFile, pDebugFile, "       Time (ns)   PID   TID EVENT\n");
    }
}

bool DebugDone()
{
    return gDebugDone;
}

void DebugEvent(EVENT_RECORD* eventRecord, EventMetadata* metadata)
{
    auto const& hdr = eventRecord->EventHeader;
    auto id = hdr.EventDescriptor.Id;

    FlushModifiedPresent();

    auto t = ConvertTimestampToNs(hdr.TimeStamp);
    if (t >= DEBUG_START_TIME_NS) {
        gDebugTrace = true;
    }

    if (t >= DEBUG_STOP_TIME_NS) {
        gDebugTrace = false;
        gDebugDone = true;
    }

    if (!gDebugTrace) {
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_D3D9::GUID) {
        switch (id) {
        case Microsoft_Windows_D3D9::Present_Start::Id: PrintEventHeader(hdr, "D3D9PresentStart"); break;
        case Microsoft_Windows_D3D9::Present_Stop::Id:  PrintEventHeader(hdr, "D3D9PresentStop"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_DXGI::GUID) {
        switch (id) {
        case Microsoft_Windows_DXGI::Present_Start::Id:                     PrintEventHeader(eventRecord, metadata, "DXGIPresent_Start", {
                                                                                L"Flags", PrintPresentFlags,
            }); break;
        case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start::Id:    PrintEventHeader(eventRecord, metadata, "DXGIPresentMPO_Start", {
                                                                                L"Flags", PrintPresentFlags,
            }); break;
        case Microsoft_Windows_DXGI::Present_Stop::Id:                      PrintEventHeader(hdr, "DXGIPresent_Stop"); break;
        case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id:     PrintEventHeader(hdr, "DXGIPresentMPO_Stop"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::BLT_GUID) { PrintEventHeader(hdr, "Win7::BLT"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::FLIP_GUID) { PrintEventHeader(hdr, "Win7::FLIP"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::PRESENTHISTORY_GUID) { PrintEventHeader(hdr, "Win7::PRESENTHISTORY"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::QUEUEPACKET_GUID) { PrintEventHeader(hdr, "Win7::QUEUEPACKET"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::VSYNCDPC_GUID) { PrintEventHeader(hdr, "Win7::VSYNCDPC"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::MMIOFLIP_GUID) { PrintEventHeader(hdr, "Win7::MMIOFLIP"); return; }

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::GUID) {
        switch (id) {
        case Microsoft_Windows_DxgKrnl::Blit_Info::Id:                      PrintEventHeader(hdr, "DxgKrnl_Blit_Info"); break;
        case Microsoft_Windows_DxgKrnl::Flip_Info::Id:                      PrintEventHeader(hdr, "DxgKrnl_Flip_Info"); break;
        case Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info::Id:     PrintEventHeader(hdr, "DxgKrnl_FlipMultiPlaneOverlay_Info"); break;
        case Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id:        PrintEventHeader(hdr, "DxgKrnl_HSyncDPCMultiPlane_Info"); break;
        case Microsoft_Windows_DxgKrnl::MMIOFlip_Info::Id:                  PrintEventHeader(hdr, "DxgKrnl_MMIOFlip_Info"); break;
        case Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info::Id: PrintEventHeader(hdr, "DxgKrnl_MMIOFlipMultiPlaneOverlay_Info"); break;
        case Microsoft_Windows_DxgKrnl::Present_Info::Id:                   PrintEventHeader(hdr, "DxgKrnl_Present_Info"); break;
        case Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id:           PrintEventHeader(eventRecord, metadata, "PresentHistory_Start", {
                                                                                L"Token", PrintU64x,
                                                                                L"Model", PrintPresentHistoryModel,
            }); break;
        case Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id:            PrintEventHeader(eventRecord, metadata, "PresentHistory_Info", {
                                                                                L"Token", PrintU64x,
                                                                                L"Model", PrintPresentHistoryModel,
            }); break;
        case Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id:   PrintEventHeader(eventRecord, metadata, "PresentHistoryDetailed_Start", {
                                                                                L"Token", PrintU64x,
                                                                                L"Model", PrintPresentHistoryModel,
            }); break;
        case Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id:              PrintEventHeader(eventRecord, metadata, "DxgKrnl_QueuePacket_Start", {
                                                                                L"hContext", PrintU64x,
                                                                                L"SubmitSequence", PrintU32,
                                                                                L"PacketType", PrintQueuePacketType,
                                                                                L"bPresent", PrintU32,
            }); break;
        case Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id:               PrintEventHeader(eventRecord, metadata, "DxgKrnl_QueuePacket_Stop", {
                                                                                L"hContext", PrintU64x,
                                                                                L"SubmitSequence", PrintU32,
            }); break;
        case Microsoft_Windows_DxgKrnl::VSyncDPC_Info::Id:                  PrintEventHeader(hdr, "DxgKrnl_VSyncDPC_Info"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Dwm_Core::GUID ||
        hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID) {
        switch (id) {
        case Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id:
            PrintEventHeader(hdr, "DWM_GetPresentHistory"); break;
        case Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start::Id:      PrintEventHeader(hdr, "DWM_SCHEDULE_PRESENT_Start"); break;
        case Microsoft_Windows_Dwm_Core::FlipChain_Pending::Id:           PrintEventHeader(hdr, "DWM_FlipChain_Pending"); break;
        case Microsoft_Windows_Dwm_Core::FlipChain_Complete::Id:          PrintEventHeader(hdr, "DWM_FlipChain_Complete"); break;
        case Microsoft_Windows_Dwm_Core::FlipChain_Dirty::Id:             PrintEventHeader(hdr, "DWM_FlipChain_Dirty"); break;
        case Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info::Id: PrintEventHeader(hdr, "DWM_Schedule_SurfaceUpdate"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Win32k::GUID) {
        switch (id) {
        case Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info::Id:  PrintEventHeader(hdr, "Win32K_TokenCompositionSurfaceObject"); break;
        case Microsoft_Windows_Win32k::TokenStateChanged_Info::Id:              PrintEventHeader(eventRecord, metadata, "Win32K_TokenStateChanged", {
                                                                                    L"NewState", PrintTokenState,
            }); break;
        }
        return;
    }

    assert(false);
}

void DebugModifyPresent(PresentEvent const& p)
{
    if (!gDebugTrace) return;
    if (gModifiedPresent != &p) {
        FlushModifiedPresent();

        gModifiedPresent = &p;

        gOriginalPresentValues.TimeTaken = p.TimeTaken;
        gOriginalPresentValues.ReadyTime = p.ReadyTime;
        gOriginalPresentValues.ScreenTime = p.ScreenTime;
        gOriginalPresentValues.SwapChainAddress = p.SwapChainAddress;
        gOriginalPresentValues.SyncInterval = p.SyncInterval;
        gOriginalPresentValues.PresentFlags = p.PresentFlags;
        gOriginalPresentValues.Hwnd = p.Hwnd;
        gOriginalPresentValues.TokenPtr = p.TokenPtr;
        gOriginalPresentValues.QueueSubmitSequence = p.QueueSubmitSequence;
        gOriginalPresentValues.DriverBatchThreadId = p.DriverBatchThreadId;
        gOriginalPresentValues.PresentMode = p.PresentMode;
        gOriginalPresentValues.FinalState = p.FinalState;
        gOriginalPresentValues.SupportsTearing = p.SupportsTearing;
        gOriginalPresentValues.MMIO = p.MMIO;
        gOriginalPresentValues.SeenDxgkPresent = p.SeenDxgkPresent;
        gOriginalPresentValues.SeenWin32KEvents = p.SeenWin32KEvents;
        gOriginalPresentValues.DwmNotified = p.DwmNotified;
        gOriginalPresentValues.Completed = p.Completed;
    }
}

void DebugCreatePresent(PresentEvent const& p)
{
    if (!gDebugTrace) return;
    FlushModifiedPresent();
    PrintUpdateHeader(p.Id);
    fprintf(pDebugFile, " CreatePresent");
    fprintf(pDebugFile, " SwapChainAddress=%llx", p.SwapChainAddress);
    fprintf(pDebugFile, " PresentFlags=%x", p.PresentFlags);
    fprintf(pDebugFile, " SyncInterval=%u", p.SyncInterval);
    fprintf(pDebugFile, " Runtime=");
    PrintRuntime(p.Runtime);
    fprintf(pDebugFile, "\n");
}

void DebugCompletePresent(PresentEvent const& p, int indent)
{
    if (!gDebugTrace) return;
    FlushModifiedPresent();
    PrintUpdateHeader(p.Id, indent);
    fprintf(pDebugFile, " Completed=");
    PrintBool(p.Completed);
    fprintf(pDebugFile, "->");
    PrintBool(true);
    fprintf(pDebugFile, "\n");
}


void DebugLostPresent(PresentEvent const& p)
{
    if (!gDebugTrace) return;
    FlushModifiedPresent();
    PrintUpdateHeader(p.Id);
    fprintf(pDebugFile, " LostPresent");
    fprintf(pDebugFile, "\n");
}

#endif // if DEBUG_VERBOSE
