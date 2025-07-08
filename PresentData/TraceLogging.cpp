#include "TraceLogging.h"
#include "ETW\Nvidia_PCL.h"

using pmon::util::Except;


bool TraceLoggingContext::DecodeTraceLoggingEventRecord(EVENT_RECORD* pEventRecord)
{
    if (pEventRecord == nullptr) {
        pmlog_warn("null event record pointer");
        return false;
    }

    // We do not handle Windows Software Trace Preprocessor (WPP) events
    if ((pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_TRACE_MESSAGE) != 0)
    {
        return false;
    }

    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO &&
        pEventRecord->EventHeader.ProviderId == EventTraceGuid)
    {
        // TODO: determine if this necessary. As mentioned in the sample code
        // the first event in every ETL file contains the data from the file header.
        // This is the same data as was returned in the EVENT_TRACE_LOGFILEW by
        // OpenTrace. Since we've already seen this information, we'll skip this
        // event.
        // In PresentMon we process ETW events in both realtime and from ETL files.
        // Check how this works!!
        return false;
    }

    // Reset event state
    mpEventRecord = pEventRecord;
    mpEventData = static_cast<BYTE const*>(mpEventRecord->UserData);
    mpEventDataEnd = mpEventData + mpEventRecord->UserDataLength;
    if (mpEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) {
        mPointerSize = 4;
    }
    else if (mpEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) {
        mPointerSize = 8;
    }
    else {
        // Ambiguous, assume size of the decoder's pointer.
        mPointerSize = sizeof(void*);
    }

    return SetupTraceLoggingInfoBuffer();
}

bool TraceLoggingContext::SetupTraceLoggingInfoBuffer()
{
    ULONG status;
    ULONG cb;

    // Use TdhGetEventInformation to obtain information about this event.
    // We do not process WPP or classic ETW events in this class so we set
    // TdhContextCount to 0 and TdhContext to NULL in both calls to
    // TdhGetEventInformation
    cb = static_cast<ULONG>(mTraceEventInfoBuffer.size());
    status = TdhGetEventInformation(
        mpEventRecord,
        0,
        nullptr,
        reinterpret_cast<TRACE_EVENT_INFO*>(mTraceEventInfoBuffer.data()),
        &cb);
    if (status == ERROR_INSUFFICIENT_BUFFER) {
        mTraceEventInfoBuffer.resize(cb);
        status = TdhGetEventInformation(
            mpEventRecord,
            0,
            nullptr,
            reinterpret_cast<TRACE_EVENT_INFO*>(mTraceEventInfoBuffer.data()),
            &cb);
    }

    if (status != ERROR_SUCCESS)
    {
        pmlog_warn("Failure when calling TdhGetEventInformation").hr(status);
        return false;
    }

    mpTraceEventInfo = reinterpret_cast<TRACE_EVENT_INFO const*>(mTraceEventInfoBuffer.data());

    // Check to see if either the event name or the task name are available. If
    // not then we are unable to continue processing the event
    if (mpTraceEventInfo->EventNameOffset == 0 && mpTraceEventInfo->TaskNameOffset == 0) {
        pmlog_warn("Missing event or task name");
        return false;
    }

    return true;
}

std::optional<std::wstring> TraceLoggingContext::GetTraceEventInfoString(unsigned offset) const {
    if (mTraceEventInfoBuffer.empty()) {
        pmlog_warn("mTraceEventInfoBuffer is empty");
        return std::nullopt;
    }
    return std::wstring(reinterpret_cast<const wchar_t*>(mTraceEventInfoBuffer.data() + offset));
}

std::optional<std::wstring> TraceLoggingContext::GetEventName()
{
    if (mpTraceEventInfo == nullptr) {
        pmlog_warn("mTraceEventInfoBuffer not setup");
        return std::nullopt;
    }

    return GetTraceEventInfoString(mpTraceEventInfo->EventNameOffset);
}

size_t TraceLoggingContext::GetSkipSize(unsigned tdhType) const
{
    switch (tdhType)
    {
    case TDH_INTYPE_INT8:
    case TDH_INTYPE_UINT8:
        return 1;
    case TDH_INTYPE_INT16:
    case TDH_INTYPE_UINT16:
        return 2;
    case TDH_INTYPE_INT32:
    case TDH_INTYPE_UINT32:
    case TDH_INTYPE_HEXINT32:
        return 4;
    case TDH_INTYPE_INT64:
    case TDH_INTYPE_UINT64:
    case TDH_INTYPE_HEXINT64:
        return 8;
    case TDH_INTYPE_FLOAT:
        return sizeof(float);
    case TDH_INTYPE_DOUBLE:
        return sizeof(double);
    default:
        throw Except<TraceLoggingError>("Unknown TDH Type");
    }
}