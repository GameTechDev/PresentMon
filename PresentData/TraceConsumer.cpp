// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include "TraceConsumer.hpp"
#include "ETW/Microsoft_Windows_EventMetadata.h"

namespace {

struct PropertyInfo
{
    uint32_t size_;
    uint32_t count_;
    uint32_t status_;
};

uint32_t GetPropertyDataOffset(TRACE_EVENT_INFO const& tei, EVENT_RECORD const& eventRecord, uint32_t index);

// If ((epi.Flags & PropertyParamLength) != 0), the epi.lengthPropertyIndex
// field contains the index of the property that contains the number of
// CHAR/WCHARs in the string.
//
// Else if ((epi.Flags & PropertyLength) != 0 || epi.length != 0), the
// epi.length field contains number of CHAR/WCHARs in the string.
//
// Else the string is terminated by (CHAR/WCHAR)0.
//
// Note that some event providers do not correctly null-terminate the last
// string field in the event. While this is technically invalid, event decoders
// may silently tolerate such behavior instead of rejecting the event as
// invalid.

template<typename T>
void GetStringPropertyInfo(TRACE_EVENT_INFO const& tei, EVENT_RECORD const& eventRecord, uint32_t index, uint32_t offset,
                           PropertyInfo* info)
{
    auto const& epi = tei.EventPropertyInfoArray[index];

    if ((epi.Flags & PropertyParamLength) != 0) {
        assert(false); // TODO: just not implemented yet
        info->size_ = 0;
    } else if (epi.length != 0) {
        info->size_ = epi.length * sizeof(T);
    } else {
        if (offset == UINT32_MAX) {
            offset = GetPropertyDataOffset(tei, eventRecord, index);
            assert(offset <= eventRecord.UserDataLength);
        }

        for (uint32_t size = 0;; size += sizeof(T)) {
            if (offset + size > eventRecord.UserDataLength) {
                // string ends at end of block, possibly ok (see note above)
                // paranoia check addressing coverity issue, likely not relevant
                assert(size >= sizeof(T));
                info->size_ = size - sizeof(T);
                break;
            }
            if (*(T const*) ((uintptr_t) eventRecord.UserData + offset + size) == (T) 0) {
                info->status_ |= PROP_STATUS_NULL_TERMINATED;
                info->size_ = size + sizeof(T);
                break;
            }
        }
    }
}

PropertyInfo GetPropertyInfo(TRACE_EVENT_INFO const& tei, EVENT_RECORD const& eventRecord, uint32_t index, uint32_t offset)
{
    // We don't handle all flags yet, these are the ones we do:
    auto const& epi = tei.EventPropertyInfoArray[index];
    assert((epi.Flags & ~(PropertyStruct | PropertyParamCount | PropertyParamFixedCount)) == 0);

    // Use the epi length and count by default.  There are cases where the count
    // is valid but (epi.Flags & PropertyParamFixedCount) == 0.
    PropertyInfo info;
    info.size_ = epi.length;
    info.count_ = epi.count;
    info.status_ = 0;

    if (epi.Flags & PropertyStruct) {
        info.size_ = 0;
        for (USHORT i = 0; i < epi.structType.NumOfStructMembers; ++i) {
            auto memberInfo = GetPropertyInfo(tei, eventRecord, epi.structType.StructStartIndex + i, UINT32_MAX);
            info.size_ += memberInfo.size_ * memberInfo.count_;
        }
    } else {
        switch (epi.nonStructType.InType) {
        case TDH_INTYPE_UNICODESTRING:
            info.status_ |= PROP_STATUS_WCHAR_STRING;
            GetStringPropertyInfo<wchar_t>(tei, eventRecord, index, offset, &info);
            break;
        case TDH_INTYPE_ANSISTRING:
            info.status_ |= PROP_STATUS_CHAR_STRING;
            GetStringPropertyInfo<char>(tei, eventRecord, index, offset, &info);
            break;

        case TDH_INTYPE_POINTER:    // TODO: Not sure this is needed, epi.length seems to be correct?
        case TDH_INTYPE_SIZET:
            info.status_ |= PROP_STATUS_POINTER_SIZE;
            info.size_ = (eventRecord.EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) ? 8 : 4;
            break;

        case TDH_INTYPE_SID:
        case TDH_INTYPE_WBEMSID:
            // TODO: can't figure out how to decode these... so reverting to TDH for now
            {
                PROPERTY_DATA_DESCRIPTOR descriptor{};
                descriptor.PropertyName = (ULONGLONG) &tei + epi.NameOffset;
                descriptor.ArrayIndex = UINT32_MAX;
                auto status = TdhGetPropertySize((EVENT_RECORD*) &eventRecord, 0, nullptr, 1, &descriptor, (ULONG*) &info.size_);
                (void) status;
            }
            break;
        }
    }

    if (epi.Flags & PropertyParamCount) {
        auto countIdx = epi.countPropertyIndex;
        auto addr = (uintptr_t) eventRecord.UserData + GetPropertyDataOffset(tei, eventRecord, countIdx);

        assert(tei.EventPropertyInfoArray[countIdx].Flags == 0);
        switch (tei.EventPropertyInfoArray[countIdx].nonStructType.InType) {
        case TDH_INTYPE_INT8:   info.count_ = *(int8_t const*) addr; break;
        case TDH_INTYPE_UINT8:  info.count_ = *(uint8_t const*) addr; break;
        case TDH_INTYPE_INT16:  info.count_ = *(int16_t const*) addr; break;
        case TDH_INTYPE_UINT16: info.count_ = *(uint16_t const*) addr; break;
        case TDH_INTYPE_INT32:  info.count_ = *(int32_t const*) addr; break;
        case TDH_INTYPE_UINT32: info.count_ = *(uint32_t const*) addr; break;
        default: assert(!"INTYPE not yet implemented for count.");
        }
    }

    // Note:
    // - info.size_ can be 0 for SIDs
    // - info.count_ can be 0 for array properties.

    return info;
}

uint32_t GetPropertyDataOffset(TRACE_EVENT_INFO const& tei, EVENT_RECORD const& eventRecord, uint32_t index)
{
    assert(index < tei.TopLevelPropertyCount);
    uint32_t offset = 0;
    for (uint32_t i = 0; i < index; ++i) {
        auto info = GetPropertyInfo(tei, eventRecord, i, offset);
        offset += info.size_ * info.count_;
    }
    return offset;
}

}

size_t EventMetadataKeyHash::operator()(EventMetadataKey const& key) const
{
    static_assert((sizeof(key) % sizeof(size_t)) == 0, "sizeof(EventMetadataKey) must be multiple of sizeof(size_t)");
    auto p = (size_t const*) &key;
    auto h = (size_t) 0;
    for (size_t i = 0; i < sizeof(key) / sizeof(size_t); ++i) {
        h ^= p[i];
    }
    return h;
}

bool EventMetadataKeyEqual::operator()(EventMetadataKey const& lhs, EventMetadataKey const& rhs) const
{
    return memcmp(&lhs, &rhs, sizeof(EventMetadataKey)) == 0;
}

void EventMetadata::AddMetadata(EVENT_RECORD* eventRecord)
{
    if (eventRecord->EventHeader.EventDescriptor.Opcode == Microsoft_Windows_EventMetadata::EventInfo::Opcode) {
        auto userData = (uint8_t const*) eventRecord->UserData;
        auto tei = (TRACE_EVENT_INFO const*) userData;

        if (tei->DecodingSource == DecodingSourceTlg || tei->EventDescriptor.Channel == 0xB) {
            return; // Don't store tracelogging metadata
        }

        // Store metadata (overwriting any previous)
        EventMetadataKey key;
        key.guid_ = tei->ProviderGuid;
        key.desc_ = tei->EventDescriptor;
        metadata_[key].assign(userData, userData + eventRecord->UserDataLength);
    }
}

// Look up metadata for this provider/event and use it to look up the property.
// If the metadata isn't found look it up using TDH.  Then, look up each
// property in the metadata to obtain it's data pointer and size.
void EventMetadata::GetEventData(EVENT_RECORD* eventRecord, EventDataDesc* desc, uint32_t descCount, uint32_t optionalCount /*=0*/)
{
    [[maybe_unused]] auto foundCount = GetEventDataWithCount(eventRecord, desc, descCount);
    assert(foundCount >= descCount - optionalCount);
    (void)optionalCount;
}

// Some events have been evolving over time but not incrementing the version number. As an example DXGI::SwapChain::Start.
// Use this version of GetEventData when a data param might be available based on the version of the event
// being processed. Be sure to check the returned event cound to ensure the expected number of descriptions have
// been found. 
void EventMetadata::GetEventData(EVENT_RECORD* eventRecord, EventDataDesc* desc, uint32_t* descCount)
{
    auto foundCount = GetEventDataWithCount(eventRecord, desc, *descCount);
    *descCount = foundCount;
    return;
}

uint32_t EventMetadata::GetEventDataWithCount(EVENT_RECORD* eventRecord, EventDataDesc* desc, uint32_t descCount)
{
    // Look up stored metadata.  If not found, look up metadata using TDH and
    // cache it for future events.
    EventMetadataKey key;
    key.guid_ = eventRecord->EventHeader.ProviderId;
    key.desc_ = eventRecord->EventHeader.EventDescriptor;
    auto ii = metadata_.find(key);
    if (ii == metadata_.end()) {
        ULONG bufferSize = 0;
        auto status = TdhGetEventInformation(eventRecord, 0, nullptr, nullptr, &bufferSize);
        if (status == ERROR_INSUFFICIENT_BUFFER) {
            ii = metadata_.emplace(key, std::vector<uint8_t>(bufferSize, 0)).first;

            status = TdhGetEventInformation(eventRecord, 0, nullptr, (TRACE_EVENT_INFO*) ii->second.data(), &bufferSize);
            assert(status == ERROR_SUCCESS);
        } else {
            // No schema registered with system, nor ETL-embedded metadata.
            ii = metadata_.emplace(key, std::vector<uint8_t>(sizeof(TRACE_EVENT_INFO), 0)).first;
            assert(false);
        }
}

    auto tei = (TRACE_EVENT_INFO*) ii->second.data();

    // Lookup properties in metadata
    uint32_t foundCount = 0;

#if 0 /* Helper to see all property names while debugging */
    std::vector<wchar_t const*> props(tei->TopLevelPropertyCount, nullptr);
    for (uint32_t i = 0; i < tei->TopLevelPropertyCount; ++i) {
        props[i] = TEI_PROPERTY_NAME(tei, &tei->EventPropertyInfoArray[i]);
    }
#endif

    for (uint32_t i = 0, offset = 0; i < tei->TopLevelPropertyCount; ++i) {
        auto info = GetPropertyInfo(*tei, *eventRecord, i, offset);

        auto propName = TEI_PROPERTY_NAME(tei, &tei->EventPropertyInfoArray[i]);
        if (propName != nullptr) {
            for (uint32_t j = 0; j < descCount; ++j) {
                if (desc[j].status_ == PROP_STATUS_NOT_FOUND && wcscmp(propName, desc[j].name_) == 0) {
                    desc[j].data_   = (void*) ((uintptr_t) eventRecord->UserData + offset);
                    desc[j].size_   = info.size_;
                    desc[j].count_  = info.count_;
                    desc[j].status_ = info.status_ | PROP_STATUS_FOUND;

                    foundCount += 1;
                    if (foundCount == descCount) {
                        return foundCount;
                    }
                }
            }
        }

        offset += info.size_ * info.count_;
    }

    return foundCount;
}

namespace {

template <typename T>
T GetEventString(EventDataDesc const& desc)
{
    assert(desc.status_ & PROP_STATUS_FOUND);
    assert(desc.status_ & (std::is_same<char, typename T::value_type>::value ? PROP_STATUS_CHAR_STRING : PROP_STATUS_WCHAR_STRING));
    assert(desc.status_ & PROP_STATUS_NULL_TERMINATED);
    assert((desc.size_ % sizeof(typename T::value_type)) == 0);

    auto start = (typename T::value_type*) desc.data_;
    auto end   = (typename T::value_type*) ((uintptr_t) desc.data_ + desc.size_);

    // Don't include null termination character
    if (desc.status_ & PROP_STATUS_NULL_TERMINATED) {
        assert(end > start);
        end -= 1;
    }

    return T(start, end);
}

}

template <>
std::string EventDataDesc::GetData<std::string>() const
{
    return GetEventString<std::string>(*this);
}

template <>
std::wstring EventDataDesc::GetData<std::wstring>() const
{
    return GetEventString<std::wstring>(*this);
}

template<> bool EventDataDesc::GetData<bool>() const
{
    return (bool)GetData<uint32_t>();
}
