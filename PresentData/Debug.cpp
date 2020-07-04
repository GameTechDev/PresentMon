/*
Copyright 2019 Intel Corporation

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

#if DEBUG_VERBOSE

namespace {

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
    PresentMode PresentMode;
    PresentResult FinalState;
    bool SupportsTearing;
    bool MMIO;
    bool SeenDxgkPresent;
    bool SeenWin32KEvents;
    bool WasBatched;
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

void PrintEventHeader(EVENT_HEADER const& hdr)
{
    printf("%16s %5u %5u ", AddCommas(ConvertTimestampToNs(hdr.TimeStamp)), hdr.ProcessId, hdr.ThreadId);
}

void PrintUpdateHeader(uint64_t id, int indent=0)
{
    printf("%*sp%llu", 17 + 6 + 6 + indent*4, "", id);
}

void PrintU32(uint32_t value) { printf("%u", value); }
void PrintU64(uint64_t value) { printf("%llu", value); }
void PrintU64Ptr(uint64_t value) { printf("%llx", value); }
void PrintTimeDelta(uint64_t value) { printf("%s", AddCommas(ConvertTimestampDeltaToNs(value))); }
void PrintBool(bool value) { printf("%s", value ? "true" : "false"); }
void PrintRuntime(Runtime value)
{
    switch (value) {
    case Runtime::DXGI:  printf("DXGI");  break;
    case Runtime::D3D9:  printf("D3D9");  break;
    case Runtime::Other: printf("Other"); break;
    default:             printf("ERROR"); break;
    }
}
void PrintPresentMode(PresentMode value)
{
    switch (value) {
    case PresentMode::Unknown:                              printf("Unknown"); break;
    case PresentMode::Hardware_Legacy_Flip:                 printf("Hardware_Legacy_Flip"); break;
    case PresentMode::Hardware_Legacy_Copy_To_Front_Buffer: printf("Hardware_Legacy_Copy_To_Front_Buffer"); break;
    case PresentMode::Hardware_Independent_Flip:            printf("Hardware_Independent_Flip"); break;
    case PresentMode::Composed_Flip:                        printf("Composed_Flip"); break;
    case PresentMode::Composed_Copy_GPU_GDI:                printf("Composed_Copy_GPU_GDI"); break;
    case PresentMode::Composed_Copy_CPU_GDI:                printf("Composed_Copy_CPU_GDI"); break;
    case PresentMode::Composed_Composition_Atlas:           printf("Composed_Composition_Atlas"); break;
    case PresentMode::Hardware_Composed_Independent_Flip:   printf("Hardware_Composed_Independent_Flip"); break;
    default:                                                printf("ERROR"); break;
    }
}
void PrintPresentResult(PresentResult value)
{
    switch (value) {
    case PresentResult::Unknown:   printf("Unknown");   break;
    case PresentResult::Presented: printf("Presented"); break;
    case PresentResult::Discarded: printf("Discarded"); break;
    case PresentResult::Error:     printf("Error");     break;
    default:                       printf("ERROR");     break;
    }
}

void FlushModifiedPresent()
{
    if (gModifiedPresent == nullptr) return;

    uint32_t changedCount = 0;
#define FLUSH_MEMBER(_Fn, _Name) \
    if (gModifiedPresent->_Name != gOriginalPresentValues._Name) { \
        if (changedCount++ == 0) PrintUpdateHeader(gModifiedPresent->Id); \
        printf(" " #_Name "="); \
        _Fn(gOriginalPresentValues._Name); \
        printf("->"); \
        _Fn(gModifiedPresent->_Name); \
    }
    FLUSH_MEMBER(PrintTimeDelta,     TimeTaken)
    FLUSH_MEMBER(PrintTimeDelta,     ReadyTime)
    FLUSH_MEMBER(PrintTimeDelta,     ScreenTime)
    FLUSH_MEMBER(PrintU64Ptr,        SwapChainAddress)
    FLUSH_MEMBER(PrintU32,           SyncInterval)
    FLUSH_MEMBER(PrintU32,           PresentFlags)
    FLUSH_MEMBER(PrintU64Ptr,        Hwnd)
    FLUSH_MEMBER(PrintU64Ptr,        TokenPtr)
    FLUSH_MEMBER(PrintU32,           QueueSubmitSequence)
    FLUSH_MEMBER(PrintPresentMode,   PresentMode)
    FLUSH_MEMBER(PrintPresentResult, FinalState)
    FLUSH_MEMBER(PrintBool,          SupportsTearing)
    FLUSH_MEMBER(PrintBool,          MMIO)
    FLUSH_MEMBER(PrintBool,          SeenDxgkPresent)
    FLUSH_MEMBER(PrintBool,          SeenWin32KEvents)
    FLUSH_MEMBER(PrintBool,          WasBatched)
    FLUSH_MEMBER(PrintBool,          DwmNotified)
    FLUSH_MEMBER(PrintBool,          Completed)
#undef FLUSH_MEMBER
    if (changedCount > 0) {
        printf("\n");
    }

    gModifiedPresent = nullptr;
}

}

void DebugInitialize(LARGE_INTEGER* firstTimestamp, LARGE_INTEGER timestampFrequency)
{
    gDebugDone = false;
    gFirstTimestamp = firstTimestamp;
    gTimestampFrequency = timestampFrequency;

    printf("       Time (ns)   PID   TID EVENT\n");
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
        case Microsoft_Windows_D3D9::Present_Start::Id: PrintEventHeader(hdr); printf("D3D9PresentStart\n"); break;
        case Microsoft_Windows_D3D9::Present_Stop::Id:  PrintEventHeader(hdr); printf("D3D9PresentStop\n"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_DXGI::GUID) {
        switch (id) {
        case Microsoft_Windows_DXGI::Present_Start::Id:                  PrintEventHeader(hdr); printf("DXGIPresent_Start\n"); break;
        case Microsoft_Windows_DXGI::Present_Stop::Id:                   PrintEventHeader(hdr); printf("DXGIPresent_Stop\n"); break;
        case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start::Id: PrintEventHeader(hdr); printf("DXGIPresentMPO_Start\n"); break;
        case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id:  PrintEventHeader(hdr); printf("DXGIPresentMPO_Stop\n"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::BLT_GUID)            { PrintEventHeader(hdr); printf("Win7::BLT\n"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::FLIP_GUID)           { PrintEventHeader(hdr); printf("Win7::FLIP\n"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::PRESENTHISTORY_GUID) { PrintEventHeader(hdr); printf("Win7::PRESENTHISTORY\n"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::QUEUEPACKET_GUID)    { PrintEventHeader(hdr); printf("Win7::QUEUEPACKET\n"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::VSYNCDPC_GUID)       { PrintEventHeader(hdr); printf("Win7::VSYNCDPC\n"); return; }
    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::MMIOFLIP_GUID)       { PrintEventHeader(hdr); printf("Win7::MMIOFLIP\n"); return; }

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::GUID) {
        switch (id) {
        case Microsoft_Windows_DxgKrnl::Flip_Info::Id:                      PrintEventHeader(hdr); printf("DxgKrnl_Flip\n"); break;
        case Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info::Id:     PrintEventHeader(hdr); printf("DxgKrnl_FlipMPO\n"); break;
        case Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id:              PrintEventHeader(hdr); printf("DxgKrnl_QueuePacket_Start"); break;
        case Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id:               PrintEventHeader(hdr); printf("DxgKrnl_QueuePacket_Stop"); break;
        case Microsoft_Windows_DxgKrnl::MMIOFlip_Info::Id:                  PrintEventHeader(hdr); printf("DxgKrnl_MMIOFlip\n"); break;
        case Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info::Id: PrintEventHeader(hdr); printf("DxgKrnl_MMIOFlipMPO\n"); break;
        case Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id:        PrintEventHeader(hdr); printf("DxgKrnl_HSyncDPC\n"); break;
        case Microsoft_Windows_DxgKrnl::VSyncDPC_Info::Id:                  PrintEventHeader(hdr); printf("DxgKrnl_VSyncDPC\n"); break;
        case Microsoft_Windows_DxgKrnl::Present_Info::Id:                   PrintEventHeader(hdr); printf("DxgKrnl_Present\n"); break;
        case Microsoft_Windows_DxgKrnl::Blit_Info::Id:                      PrintEventHeader(hdr); printf("DxgKrnl_Blit\n"); break;
        case Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id:           PrintEventHeader(hdr); printf("DxgKrnl_PresentHistory_Start"); break;
        case Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id:            PrintEventHeader(hdr); printf("DxgKrnl_PresentHistory_Info"); break;
        case Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id:   PrintEventHeader(hdr); printf("DxgKrnl_PresentHistoryDetailed_Start"); break;
        }

        switch (id) {
        case Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id:
        case Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id:
            printf(" SubmitSequence=%u\n", metadata->GetEventData<uint32_t>(eventRecord, L"SubmitSequence"));
            break;
        // The first part of the event data is the same for all these (detailed
        // has other members after).
        case Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id:
        case Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id:
        case Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id:
            printf(" Token=%llx, Model=", metadata->GetEventData<uint64_t>(eventRecord, L"Token"));
            switch (metadata->GetEventData<uint32_t>(eventRecord, L"Model")) {
            case D3DKMT_PM_UNINITIALIZED:          printf("UNINITIALIZED");          break;
            case D3DKMT_PM_REDIRECTED_GDI:         printf("REDIRECTED_GDI");         break;
            case D3DKMT_PM_REDIRECTED_FLIP:        printf("REDIRECTED_FLIP");        break;
            case D3DKMT_PM_REDIRECTED_BLT:         printf("REDIRECTED_BLT");         break;
            case D3DKMT_PM_REDIRECTED_VISTABLT:    printf("REDIRECTED_VISTABLT");    break;
            case D3DKMT_PM_SCREENCAPTUREFENCE:     printf("SCREENCAPTUREFENCE");     break;
            case D3DKMT_PM_REDIRECTED_GDI_SYSMEM:  printf("REDIRECTED_GDI_SYSMEM");  break;
            case D3DKMT_PM_REDIRECTED_COMPOSITION: printf("REDIRECTED_COMPOSITION"); break;
            default:                               assert(false);                    break;
            }
            printf("\n");
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Dwm_Core::GUID ||
        hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID) {
        switch (id) {
        case Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id:
                                                                          PrintEventHeader(hdr); printf("DWM_GetPresentHistory\n"); break;
        case Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start::Id:      PrintEventHeader(hdr); printf("DWM_SCHEDULE_PRESENT_Start\n"); break;
        case Microsoft_Windows_Dwm_Core::FlipChain_Pending::Id:           PrintEventHeader(hdr); printf("DWM_FlipChain_Pending\n"); break;
        case Microsoft_Windows_Dwm_Core::FlipChain_Complete::Id:          PrintEventHeader(hdr); printf("DWM_FlipChain_Complete\n"); break;
        case Microsoft_Windows_Dwm_Core::FlipChain_Dirty::Id:             PrintEventHeader(hdr); printf("DWM_FlipChain_Dirty\n"); break;
        case Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info::Id: PrintEventHeader(hdr); printf("DWM_Schedule_SurfaceUpdate\n"); break;
        }
        return;
    }

    if (hdr.ProviderId == Microsoft_Windows_Win32k::GUID) {
        switch (id) {
        case Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info::Id:
            PrintEventHeader(hdr);
            printf("Win32K_TokenCompositionSurfaceObject\n");
            break;
        case Microsoft_Windows_Win32k::TokenStateChanged_Info::Id:
            PrintEventHeader(hdr);
            printf("Win32K_TokenStateChanged ");

            switch (metadata->GetEventData<uint32_t>(eventRecord, L"NewState")) {
            case Microsoft_Windows_Win32k::TokenState::Completed: printf("Completed\n"); break;
            case Microsoft_Windows_Win32k::TokenState::InFrame:   printf("InFrame\n");   break;
            case Microsoft_Windows_Win32k::TokenState::Confirmed: printf("Confirmed\n"); break;
            case Microsoft_Windows_Win32k::TokenState::Retired:   printf("Retired\n");   break;
            case Microsoft_Windows_Win32k::TokenState::Discarded: printf("Discarded\n"); break;
            default:                                              printf("Unknown (%u)\n", metadata->GetEventData<uint32_t>(eventRecord, L"NewState")); break;
            }
            break;
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

        gOriginalPresentValues.TimeTaken           = p.TimeTaken;
        gOriginalPresentValues.ReadyTime           = p.ReadyTime;
        gOriginalPresentValues.ScreenTime          = p.ScreenTime;
        gOriginalPresentValues.SwapChainAddress    = p.SwapChainAddress;
        gOriginalPresentValues.SyncInterval        = p.SyncInterval;
        gOriginalPresentValues.PresentFlags        = p.PresentFlags;
        gOriginalPresentValues.Hwnd                = p.Hwnd;
        gOriginalPresentValues.TokenPtr            = p.TokenPtr;
        gOriginalPresentValues.QueueSubmitSequence = p.QueueSubmitSequence;
        gOriginalPresentValues.PresentMode         = p.PresentMode;
        gOriginalPresentValues.FinalState          = p.FinalState;
        gOriginalPresentValues.SupportsTearing     = p.SupportsTearing;
        gOriginalPresentValues.MMIO                = p.MMIO;
        gOriginalPresentValues.SeenDxgkPresent     = p.SeenDxgkPresent;
        gOriginalPresentValues.SeenWin32KEvents    = p.SeenWin32KEvents;
        gOriginalPresentValues.WasBatched          = p.WasBatched;
        gOriginalPresentValues.DwmNotified         = p.DwmNotified;
        gOriginalPresentValues.Completed           = p.Completed;
    }
}

void DebugCreatePresent(PresentEvent const& p)
{
    if (!gDebugTrace) return;
    FlushModifiedPresent();
    PrintUpdateHeader(p.Id);
    printf(" CreatePresent");
    printf(" SwapChainAddress=%llx", p.SwapChainAddress);
    printf(" PresentFlags=%x", p.PresentFlags);
    printf(" SyncInterval=%u", p.SyncInterval);
    printf(" Runtime=");
    PrintRuntime(p.Runtime);
    printf("\n");
}

void DebugCompletePresent(PresentEvent const& p, int indent)
{
    if (!gDebugTrace) return;
    FlushModifiedPresent();
    PrintUpdateHeader(p.Id, indent);
    printf(" Completed=");
    PrintBool(p.Completed);
    printf("->");
    PrintBool(true);
    printf("\n");
}

#endif // if DEBUG_VERBOSE
