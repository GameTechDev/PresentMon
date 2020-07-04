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
#define NOMINMAX
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <windows.h>
#include <tdh.h> // Must include after windows.h

#include <generated/version.h>

namespace {

// ----------------------------------------------------------------------------
// Helper functions

void usage()
{
    fprintf(stderr,
        "usage: etw_list.exe [options]\n"
        "options:\n"
        "    --provider=filter  List providers that match the filter, argument can be used more than once.\n"
        "                       filter can be a provider name or guid, and can include up to one '*'.\n"
        "    --sort=guid|name   Sort providers by specified element.\n"
        "    --show=property    Show specified property, argument can be used more than once.\n"
        "                       property can be 'events', 'params', 'keywords', 'levels', 'channels',\n"
        "                       or 'all'.\n"
        "    --event=filter     List events that match the filter, argument can be used more than once.\n"
        "                       filter is of the form Task::opcode, and can include up to one '*'.\n"
        "    --output=c++       Output in C++ format.\n"
        "build: %s\n", PRESENT_MON_VERSION);
}

struct Filter
{
    std::wstring part1_;
    std::wstring part2_;
    bool wildcard_;
    explicit Filter(wchar_t const* filter)
        : part1_(filter)
    {
        auto p = wcschr(filter, L'*');
        wildcard_ = p != nullptr;
        if (p != nullptr) {
            part1_.resize((uintptr_t) p - (uintptr_t) filter);
            part2_ = p + 1;
        }
    }
    bool Matches(wchar_t const* s) const
    {
        if (wildcard_) {
            auto s1 = s;
            auto s2 = s + wcslen(s) - part2_.size();
            return _wcsnicmp(s1, part1_.c_str(), part1_.size()) == 0 && _wcsicmp(s2, part2_.c_str()) == 0;
        }
        return _wcsicmp(s, part1_.c_str()) == 0;
    }
};

// Trace information (e.g., TRACE_PROVIDER_INFO) is provided in memory blocks
// where string members are specified as an offset from the base of the
// allocation.
//
// NOTE: in practice, some fields have tailing spaces (in particular
// Event::opcodeName_ and Event::layerName_ are typical) so we strip those here
// too.
wchar_t const* GetStringPtr(void* base, ULONG offset)
{
    auto s = (wchar_t*) ((uintptr_t) base + offset);
    for (auto n = wcslen(s); n-- && s[n] == L' '; ) {
        s[n] = '\0';
    }
    return s;
}


// ----------------------------------------------------------------------------
// Providers

struct Provider {
    GUID guid_;
    std::wstring guidStr_;
    std::wstring name_;
    bool manifest_;

    Provider() {}
    Provider(PROVIDER_ENUMERATION_INFO* enumInfo, TRACE_PROVIDER_INFO const& info)
        : guid_(info.ProviderGuid)
        , name_(GetStringPtr(enumInfo, info.ProviderNameOffset))
        , manifest_(info.SchemaSource == 0)
    {
        wchar_t* guidStr = nullptr;
        if (StringFromIID(info.ProviderGuid, &guidStr) == S_OK) {
            guidStr_ = guidStr;
            CoTaskMemFree(guidStr);
        }
    }
};

int EnumerateProviders(
    std::vector<Filter>* providerIds,
    std::vector<Provider>* providers)
{
    // Enumerate all providers on the system
    ULONG bufferSize = 0;
    auto status = TdhEnumerateProviders(nullptr, &bufferSize);
    if (status != ERROR_INSUFFICIENT_BUFFER) {
        fprintf(stderr, "error: could not enumerate providers (error=%u).\n", status);
        return 1;
    }

    auto enumInfo = (PROVIDER_ENUMERATION_INFO*) malloc(bufferSize);
    if (enumInfo == nullptr) {
        fprintf(stderr, "error: could not allocate memory for providers (%u bytes).\n", bufferSize);
        return 1;
    }

    status = TdhEnumerateProviders(enumInfo, &bufferSize);
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "error: could not enumerate providers (error=%u).\n", status);
        free(enumInfo);
        return 1;
    }

    auto providerCount = enumInfo->NumberOfProviders;
    providers->reserve(providerCount);
    for (ULONG i = 0; i < providerCount; ++i) {
        Provider provider(enumInfo, enumInfo->TraceProviderInfoArray[i]);

        for (size_t j = 0, n = providerIds->size(); j < n; ++j) {
            auto providerId = (*providerIds)[j];

            if (providerId.Matches(provider.name_.c_str()) ||
                providerId.Matches(provider.guidStr_.c_str())) {
                providers->emplace_back(provider);

                // Remove filter if we found an exact match
                if (!providerId.wildcard_) {
                    providerIds->erase(providerIds->begin() + j);
                }
                break;
            }
        }
    }

    free(enumInfo);

    // Add any full GUIDs provided by user, even if not enumerated by
    // TdhEnumerateProviders().  If we see events from this provider we'll try
    // to patch the name from the EVENT_INFO.
    for (auto providerId : *providerIds) {
        if (providerId.wildcard_) continue;

        GUID guid = {};
        if (IIDFromString(providerId.part1_.c_str(), &guid) == S_OK) {
            providers->emplace_back();
            providers->back().guid_ = guid;
            providers->back().guidStr_ = providerId.part1_;
            providers->back().name_ = L"Unknown";
            providers->back().manifest_ = true;
        }
    }

    return 0;
}


// ----------------------------------------------------------------------------
// Events

struct EventProperty : public EVENT_PROPERTY_INFO {
    std::wstring name_;
    std::wstring lengthName_;
    std::wstring countName_;
    std::vector<EventProperty> members_;

    EventProperty(TRACE_EVENT_INFO* eventInfo, EVENT_PROPERTY_INFO const& propInfo)
        : EVENT_PROPERTY_INFO(propInfo)
        , name_(GetStringPtr(eventInfo, propInfo.NameOffset))
    {
        if (propInfo.Flags & PropertyStruct) {
            auto propCount = propInfo.structType.NumOfStructMembers;
            members_.reserve(propCount);
            for (ULONG i = 0; i < propCount; ++i) {
                members_.emplace_back(eventInfo, eventInfo->EventPropertyInfoArray[propInfo.structType.StructStartIndex + i]);
            }
        }
        if (propInfo.Flags & PropertyParamLength) {
            lengthName_ = GetStringPtr(eventInfo, eventInfo->EventPropertyInfoArray[propInfo.lengthPropertyIndex].NameOffset);
        }
        if (propInfo.Flags & PropertyParamCount) {
            countName_ = GetStringPtr(eventInfo, eventInfo->EventPropertyInfoArray[propInfo.countPropertyIndex].NameOffset);
        }
    }
};

bool HasPointer(EventProperty const& prop);

bool HasPointer(std::vector<EventProperty> const& members)
{
    for (auto const& prop : members) {
        if (HasPointer(prop)) {
            return true;
        }
    }
    return false;
}

bool HasPointer(EventProperty const& prop)
{
    if (prop.Flags & PropertyStruct) {
        return HasPointer(prop.members_);
    }

    return prop.nonStructType.InType == TDH_INTYPE_POINTER;
}

struct Event : public EVENT_DESCRIPTOR {
    std::wstring taskName_;
    std::wstring levelName_;
    std::wstring opcodeName_;
    std::wstring message_;
    std::vector<EventProperty> properties_;

    Event(EVENT_DESCRIPTOR const& desc, TRACE_EVENT_INFO* eventInfo)
        : EVENT_DESCRIPTOR(desc)
    {
        // Note: MSDN doesn't say that task/opcode/level offsets can be zero
        // but there are cases of that
        if (eventInfo->TaskNameOffset == 0) {
            wchar_t b[126];
            _snwprintf_s(b, _TRUNCATE, L"Task_%u", desc.Task);
            taskName_ = b;
        } else {
            taskName_ = GetStringPtr(eventInfo, eventInfo->TaskNameOffset);
        }
        if (eventInfo->OpcodeNameOffset == 0) {
            wchar_t b[126];
            _snwprintf_s(b, _TRUNCATE, L"Opcode_%u", desc.Opcode);
            opcodeName_ = b;
        } else {
            opcodeName_ = GetStringPtr(eventInfo, eventInfo->OpcodeNameOffset);
        }
        if (eventInfo->LevelNameOffset == 0) {
            wchar_t b[126];
            _snwprintf_s(b, _TRUNCATE, L"Level_%u", desc.Level);
            levelName_ = b;
        } else {
            levelName_ = GetStringPtr(eventInfo, eventInfo->LevelNameOffset);
        }

        if (eventInfo->EventMessageOffset != 0) {
            message_ = GetStringPtr(eventInfo, eventInfo->EventMessageOffset);
        }

        auto propCount = eventInfo->TopLevelPropertyCount;
        properties_.reserve(propCount);
        for (ULONG i = 0; i < propCount; ++i) {
            properties_.emplace_back(eventInfo, eventInfo->EventPropertyInfoArray[i]);
        }
    }
};

int EnumerateEvents(GUID const& providerGuid, std::vector<Event>* events, std::wstring* outProviderName)
{
    ULONG bufferSize = 0;
    auto status = TdhEnumerateManifestProviderEvents((LPGUID) &providerGuid, nullptr, &bufferSize);
    switch (status) {
    case ERROR_EMPTY:
        return 0;
    case ERROR_INSUFFICIENT_BUFFER:
        break;
    case ERROR_INVALID_DATA:
        fprintf(stderr, "error: could not enumerate events (ERROR_INVALID_DATA).\n");
        return 1;
    case ERROR_FILE_NOT_FOUND:
        fprintf(stderr, "error: could not enumerate events (provider meta data not found).\n");
        return 1;
    case ERROR_RESOURCE_TYPE_NOT_FOUND:
        fprintf(stderr, "error: could not enumerate events (ERROR_RESOURCE_TYPE_NOT_FOUND).\n");
        return 1;
    case ERROR_NOT_FOUND:
        fprintf(stderr, "error: could not enumerate events (provider schema information not found).\n");
        return 1;
    default:
        fprintf(stderr, "error: could not enumerate events (error=%u).\n", status);
        return 1;
    }

    auto enumInfo = (PROVIDER_EVENT_INFO*) malloc(bufferSize);
    if (enumInfo == nullptr) {
        fprintf(stderr, "error: could not allocate memory for events (%u bytes).\n", bufferSize);
        return 1;
    }

    status = TdhEnumerateManifestProviderEvents((LPGUID) &providerGuid, enumInfo, &bufferSize);
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "error: could not enumerate events (error=%u).\n", status);
        free(enumInfo);
        return 1;
    }

    int ret = 0;
    auto eventCount = enumInfo->NumberOfEvents;
    events->reserve(events->size() + eventCount);
    for (ULONG eventIndex = 0; eventIndex < eventCount; ++eventIndex) {
        auto const& desc = enumInfo->EventDescriptorsArray[eventIndex];

        bufferSize = 0;
        status = TdhGetManifestEventInformation((LPGUID) &providerGuid, (PEVENT_DESCRIPTOR) &desc, nullptr, &bufferSize);
        if (status != ERROR_INSUFFICIENT_BUFFER) {
            fprintf(stderr, "error: could not get manifest event information (error=%u).\n", status);
            ret += 1;
            continue;
        }

        auto eventInfo = (TRACE_EVENT_INFO*) malloc(bufferSize);
        if (eventInfo == nullptr) {
            fprintf(stderr, "error: could not allocate memory for event information (%u bytes).\n", bufferSize);
            ret += 1;
            continue;
        }

        status = TdhGetManifestEventInformation((LPGUID) &providerGuid, (PEVENT_DESCRIPTOR) &desc, eventInfo, &bufferSize);
        if (status != ERROR_SUCCESS) {
            fprintf(stderr, "error: could not get manifest event information (error=%u).\n", status);
            free(eventInfo);
            ret += 1;
            continue;
        }

        events->emplace_back(desc, eventInfo);

        // Patch provider name if we didn't find a name during provider
        // enumeration.
        //
        // Note: sometimes this can have different capitalization than the
        // name obtained during provider enumeration.
        auto providerName = GetStringPtr(eventInfo, eventInfo->ProviderNameOffset);
        if (outProviderName->empty()) {
            *outProviderName = providerName;
        }

        free(eventInfo);
    }

    free(enumInfo);
    return ret;
}


// ----------------------------------------------------------------------------
// Printing functions

wchar_t const* InTypeToString(USHORT intype) {
#define RETURN_INTYPE(_Type) if (intype == TDH_INTYPE_##_Type) return L#_Type
    RETURN_INTYPE(NULL);
    RETURN_INTYPE(UNICODESTRING);
    RETURN_INTYPE(ANSISTRING);
    RETURN_INTYPE(INT8);
    RETURN_INTYPE(UINT8);
    RETURN_INTYPE(INT16);
    RETURN_INTYPE(UINT16);
    RETURN_INTYPE(INT32);
    RETURN_INTYPE(UINT32);
    RETURN_INTYPE(INT64);
    RETURN_INTYPE(UINT64);
    RETURN_INTYPE(FLOAT);
    RETURN_INTYPE(DOUBLE);
    RETURN_INTYPE(BOOLEAN);
    RETURN_INTYPE(BINARY);
    RETURN_INTYPE(GUID);
    RETURN_INTYPE(POINTER);
    RETURN_INTYPE(FILETIME);
    RETURN_INTYPE(SYSTEMTIME);
    RETURN_INTYPE(SID);
    RETURN_INTYPE(HEXINT32);
    RETURN_INTYPE(HEXINT64);
#undef RETURN_INTYPE
    return L"Unknown intype";
}

wchar_t const* OutTypeToString(USHORT outtype) {
#define RETURN_OUTTYPE(_Type) if (outtype == TDH_OUTTYPE_##_Type) return L#_Type
    RETURN_OUTTYPE(NULL);
    RETURN_OUTTYPE(STRING);
    RETURN_OUTTYPE(DATETIME);
    RETURN_OUTTYPE(BYTE);
    RETURN_OUTTYPE(UNSIGNEDBYTE);
    RETURN_OUTTYPE(SHORT);
    RETURN_OUTTYPE(UNSIGNEDSHORT);
    RETURN_OUTTYPE(INT);
    RETURN_OUTTYPE(UNSIGNEDINT);
    RETURN_OUTTYPE(LONG);
    RETURN_OUTTYPE(UNSIGNEDLONG);
    RETURN_OUTTYPE(FLOAT);
    RETURN_OUTTYPE(DOUBLE);
    RETURN_OUTTYPE(BOOLEAN);
    RETURN_OUTTYPE(GUID);
    RETURN_OUTTYPE(HEXBINARY);
    RETURN_OUTTYPE(HEXINT8);
    RETURN_OUTTYPE(HEXINT16);
    RETURN_OUTTYPE(HEXINT32);
    RETURN_OUTTYPE(HEXINT64);
    RETURN_OUTTYPE(PID);
    RETURN_OUTTYPE(TID);
    RETURN_OUTTYPE(PORT);
    RETURN_OUTTYPE(IPV4);
    RETURN_OUTTYPE(IPV6);
    RETURN_OUTTYPE(SOCKETADDRESS);
    RETURN_OUTTYPE(CIMDATETIME);
    RETURN_OUTTYPE(ETWTIME);
    RETURN_OUTTYPE(XML);
    RETURN_OUTTYPE(ERRORCODE);
    RETURN_OUTTYPE(WIN32ERROR);
    RETURN_OUTTYPE(NTSTATUS);
    RETURN_OUTTYPE(HRESULT);
    RETURN_OUTTYPE(CULTURE_INSENSITIVE_DATETIME);
    RETURN_OUTTYPE(JSON);
    //RETURN_OUTTYPE(UTF8);
   // RETURN_OUTTYPE(PKCS7_WITH_TYPE_INFO);
#undef RETURN_OUTTYPE
    return L"Unknown outtype";
}

std::wstring CppCondition(std::wstring s)
{
    for (size_t i = 0, n = s.length(); i < n; ++i) {
        if (s[i] == L' ' ||
            s[i] == L'-' ||
            s[i] == L'/' ||
            s[i] == L':' ||
            s[i] == L'.') {
            s[i] = L'_';
        }
    }
    return s;
}

std::wstring GetMemberStructName(std::wstring const& name, size_t memberIndex)
{
    wchar_t append[128] = {};
    _snwprintf_s(append, _TRUNCATE, L"_MemberStruct_%zu", memberIndex);
    return name + append;
}

void PrintCppStruct(std::vector<EventProperty> const& members, std::wstring const& name)
{
    auto memberCount = members.size();
    if (memberCount == 0) {
        return;
    }

    // First print any member struct dependencies
    {
        size_t memberIndex = 1;
        for (auto const& member : members) {
            if (member.Flags & PropertyStruct) {
                PrintCppStruct(member.members_, GetMemberStructName(name, memberIndex));
                memberIndex += 1;
            }
        }
    }

    // Break the struct up into parts at any variable-sized member
    std::vector<std::pair<size_t, bool> > parts;
    {
        bool hasPointerMember = false;
        for (size_t i = 0; ; ++i) {
            auto const& member = members[i];
            if (HasPointer(member)) {
                hasPointerMember = true;
            }
            if (i == memberCount - 1) break;
            // If is variable length ...
            if (((member.Flags & (PropertyParamLength | PropertyParamCount)) != 0) ||
                ((member.Flags & (PropertyWBEMXmlFragment | PropertyHasCustomSchema | PropertyParamFixedLength | PropertyParamFixedCount)) == 0 &&
                 (member.nonStructType.InType == TDH_INTYPE_UNICODESTRING ||
                  member.nonStructType.InType == TDH_INTYPE_ANSISTRING ||
                  member.nonStructType.InType == TDH_INTYPE_SID))) {
                parts.emplace_back(i + 1, hasPointerMember);
                hasPointerMember = false;
            }
        }
        parts.emplace_back(memberCount, hasPointerMember);
    }

    for (size_t memberIndex = 0, structMemberIndex = 1, partIndex = 0, partCount = parts.size(); partIndex < partCount; ++partIndex) {
        auto const& part = parts[partIndex];
        auto partEnd = part.first;
        auto partHasPointerMembers = part.second;

        // Start the struct for this part
        if (partHasPointerMembers) {
            printf("template<typename PointerT>\n");
        }
        printf("struct %ls_Struct", name.c_str());
        if (partCount > 1) {
            printf("_Part%zu", partIndex + 1);
        }
        printf(" {\n");

        // Add the members for this part
        for ( ; memberIndex < partEnd; ++memberIndex) {
            auto const& member = members[memberIndex];

            auto isStruct           = (member.Flags & PropertyStruct) != 0;
            auto isNonStruct        = (member.Flags & (PropertyWBEMXmlFragment | PropertyHasCustomSchema)) == 0;
            auto isParamLength      = (member.Flags & PropertyParamLength) != 0;
            auto isParamCount       = (member.Flags & PropertyParamCount) != 0;
            auto isParamFixedLength = (member.Flags & PropertyParamFixedLength) != 0;
            auto isParamFixedCount  = (member.Flags & PropertyParamFixedCount) != 0;

            auto fixedCount = isParamFixedLength ? member.length : member.count;
            auto ending = L";";

            assert(isStruct || isNonStruct);
            (void) isNonStruct;

            std::wstring stype;
            wchar_t const* type = L"unsupported_type";
            if (isStruct) {
                stype = std::wstring(L"struct ") + GetMemberStructName(name, structMemberIndex) + L"_Struct";
                if (HasPointer(member.members_)) {
                    stype += L"<PointerT>";
                }
                type = stype.c_str();
                structMemberIndex += 1;
            } else {
                switch (member.nonStructType.InType) {
                case TDH_INTYPE_INT8:     type = L"int8_t"; break;
                case TDH_INTYPE_UINT8:    type = L"uint8_t"; break;
                case TDH_INTYPE_INT16:    type = L"int16_t"; break;
                case TDH_INTYPE_UINT16:   type = L"uint16_t"; break;
                case TDH_INTYPE_INT32:    type = L"int32_t"; break;
                case TDH_INTYPE_BOOLEAN:
                case TDH_INTYPE_HEXINT32:
                case TDH_INTYPE_UINT32:   type = L"uint32_t"; break;
                case TDH_INTYPE_INT64:    type = L"int64_t"; break;
                case TDH_INTYPE_HEXINT64:
                case TDH_INTYPE_UINT64:   type = L"uint64_t"; break;
                case TDH_INTYPE_FLOAT:    type = L"float"; break;
                case TDH_INTYPE_DOUBLE:   type = L"double"; break;

                case TDH_INTYPE_POINTER:
                    // If eventRecord.EventHeader.Flags has
                    // EVENT_HEADER_FLAG_32_BIT_HEADER set, the field size is 4
                    // bytes.  If the EVENT_HEADER_FLAG_64_BIT_HEADER is set the
                    // field size is 8 bytes.
                    type = L"PointerT";
                    break;

                case TDH_INTYPE_UNICODESTRING:
                    type = L"wchar_t";
                    if (!isParamLength && !isParamFixedLength) {
                        ending = L"[]; // null-terminated";
                    }
                    break;

                case TDH_INTYPE_ANSISTRING:
                    type = L"uint8_t";
                    if (!isParamLength && !isParamFixedLength) {
                        ending = L"[]; // null-terminated";
                    }
                    break;

                case TDH_INTYPE_BINARY:
                    type = L"uint8_t";
                    if (member.nonStructType.OutType == TDH_OUTTYPE_IPV6) {
                        isParamFixedCount = true;
                        fixedCount = 16;
                    }
                    break;

                case TDH_INTYPE_GUID:       type = L"uint8_t"; isParamFixedCount = true; fixedCount = 16; break;
                case TDH_INTYPE_FILETIME:   type = L"uint8_t"; isParamFixedCount = true; fixedCount = 8; break;
                case TDH_INTYPE_SYSTEMTIME: type = L"uint8_t"; isParamFixedCount = true; fixedCount = 16; break;

                case TDH_INTYPE_SID:
                    type = L"uint8_t";
                    ending = L"[]; // Field size is determined by reading the first few bytes of the field value.";
                    break;

                default: assert(0);
                }
            }

            printf("    %-*ls %ls", 11, type, CppCondition(member.name_).c_str());

            // Array count
            if (isParamLength || isParamCount) {
                printf("[]; // Count provided by %ls.\n", (isParamLength ? member.lengthName_ : member.countName_).c_str());
            } else if (isParamFixedLength || isParamFixedCount) {
                printf("[%u];\n", fixedCount);
            } else {
                printf("%ls\n", ending);
            }
        }

        // End the struct for this part
        printf("};\n");
    }

    printf("\n");
}

void PrintEventProperty(EventProperty const& prop, uint32_t indentCount=0, uint32_t indentWidth=4)
{
    // Name
    printf("%*s%-30ls", indentCount * indentWidth, "", prop.name_.c_str());

    // Type
    if (prop.Flags & PropertyStruct) {
        printf(" {\n");
        for (auto const& subProp : prop.members_) {
            PrintEventProperty(subProp, indentCount + 1, indentWidth);
        }
        printf("%*s}", indentCount * indentWidth, "");
    } else if (prop.Flags & PropertyHasCustomSchema) {
        // TODO
        printf(" <custom schema, not implemented>");
    } else {
        printf(" %ls -> %ls", InTypeToString(prop.nonStructType.InType), OutTypeToString(prop.nonStructType.OutType));
    }

    // TODO: PropertyWBEMXmlFragment

    // Array count
    if (prop.Flags & PropertyParamLength) {
        printf(" (%ls)", prop.lengthName_.c_str());
    }
    if (prop.Flags & PropertyParamFixedLength) {
        printf(" (%u)", prop.length);
    }
    if (prop.Flags & PropertyParamCount) {
        printf(" [%ls]", prop.countName_.c_str());
    }
    if (prop.Flags & PropertyParamFixedCount) {
        printf(" [%u]", prop.count);
    }

    // Tags
    if (prop.Flags & PropertyHasTags) {
        printf(" @0x%07x", prop.Tags);
    }

    // Done
    printf("\n");
}

}

int wmain(
    int argc,
    wchar_t** argv)
{
    // Parse command line arguments
    uint32_t errorCount = 0;
    std::vector<Filter> providerIds;
    std::vector<Filter> eventIds;
    auto sortByName = false;
    auto sortByGuid = false;
    auto showAll = false;
    auto showKeywords = false;
    auto showLevels = false;
    auto showChannels = false;
    auto showEvents = false;
    auto showEventParams = false;
    auto cppFormat = false;
    for (int i = 1; i < argc; ++i) {
        if (wcsncmp(argv[i], L"--provider=", 11) == 0) {
            providerIds.emplace_back(argv[i] + 11);
            continue;
        }

        if (wcsncmp(argv[i], L"--event=", 8) == 0) {
            eventIds.emplace_back(argv[i] + 8);
            continue;
        }

        if (wcscmp(argv[i], L"--sort=guid") == 0) {
            sortByName = false;
            sortByGuid = true;
            continue;
        }

        if (wcscmp(argv[i], L"--sort=name") == 0) {
            sortByName = true;
            sortByGuid = false;
            continue;
        }

        if (wcscmp(argv[i], L"--show=all") == 0) {
            showAll = true;
            continue;
        }

        if (wcscmp(argv[i], L"--show=keywords") == 0) {
            showKeywords = true;
            continue;
        }

        if (wcscmp(argv[i], L"--show=levels") == 0) {
            showLevels = true;
            continue;
        }

        if (wcscmp(argv[i], L"--show=channels") == 0) {
            showChannels = true;
            continue;
        }

        if (wcscmp(argv[i], L"--show=events") == 0) {
            showEvents = true;
            continue;
        }

        if (wcscmp(argv[i], L"--show=params") == 0) {
            showEventParams = true;
            continue;
        }

        if (wcscmp(argv[i], L"--output=c++") == 0) {
            cppFormat = true;
            continue;
        }

        fprintf(stderr, "error: unrecognized argument '%ls'.\n", argv[i]);
        usage();
        return 1;
    }

    if (providerIds.empty()) {
        fprintf(stderr, "error: nothing to list, --provider argument is required.\n");
        usage();
        return 1;
    }

    if (cppFormat && showEventParams) {
        fprintf(stderr, "warning: cannot show params in C++ format, ignoring --show=params.\n");
    }

    if (showAll) {
        showKeywords = true;
        showLevels = true;
        showChannels = true;
        showEvents = true;
        showEventParams = true;
    }

    if (cppFormat) {
        showEventParams = false;
    }

    if (eventIds.empty()) {
        eventIds.emplace_back(L"*");
    }

    // Enumerate all providers that match providerIds
    std::vector<Provider> providers;
    errorCount += EnumerateProviders(&providerIds, &providers);

    // Sort providers
    if (sortByName) {
        std::sort(providers.begin(), providers.end(), [](Provider const& a, Provider const& b) {
            return _wcsicmp(a.name_.c_str(), b.name_.c_str()) < 0;
        });
    } else if (sortByGuid) {
        std::sort(providers.begin(), providers.end(), [](Provider const& a, Provider const& b) {
            return _wcsicmp(a.guidStr_.c_str(), b.guidStr_.c_str()) < 0;
        });
    }

    // List providers
    if (cppFormat) {
        printf(
            "#pragma once\n"
            "// This file originally generated by etw_list\n"
            "//     version:    %s\n"
            "//     parameters:",
            PRESENT_MON_VERSION);
        for (int i = 1; i < argc; ++i) {
            printf(" %ls", argv[i]);
        }
        printf("\n\n");
    } else {
        printf("Providers (%zu):\n", providers.size());
    }
    for (auto& provider : providers) {
        // Enumerate provider events first (which may patch provider.name_)
        std::map<std::wstring, std::vector<Event> > events;
        if (showEvents) {
            if (provider.manifest_) {
                std::vector<Event> allEvents;
                errorCount += EnumerateEvents(provider.guid_, &allEvents, &provider.name_);

                for (auto const& event : allEvents) {
                    auto id = event.taskName_ + L"::" + event.opcodeName_;
                    for (auto const& eventId : eventIds) {
                        if (eventId.Matches(id.c_str())) {
                            events.emplace(event.taskName_, std::vector<Event>()).first->second.emplace_back(event);
                            break;
                        }
                    }
                }
            } else {
                // TODO
            }
        }

        // Print provider name/guid
        if (cppFormat) {
            printf("namespace %ls {\n", CppCondition(provider.name_).c_str());
            printf("\n");
            printf("struct __declspec(uuid(\"%ls\")) GUID_STRUCT;\n", provider.guidStr_.c_str());
            printf("static const auto GUID = __uuidof(GUID_STRUCT);\n");
            printf("\n");
        } else {
            printf("    %ls %ls\n",
                provider.guidStr_.c_str(),
                provider.name_.c_str());
        }

        // Print field information
        {
            std::vector<EVENT_FIELD_TYPE> fieldTypes;
            if (showKeywords) fieldTypes.emplace_back(EventKeywordInformation);
            if (showLevels)   fieldTypes.emplace_back(EventLevelInformation);
            if (showChannels) fieldTypes.emplace_back(EventChannelInformation);
            for (auto fieldType : fieldTypes) {
                ULONG size = 0;
                auto hr = TdhEnumerateProviderFieldInformation(&provider.guid_, fieldType, nullptr, &size);
                if (hr == ERROR_NOT_SUPPORTED ||
                    hr == ERROR_NOT_FOUND) {
                    continue;
                }

                char const* eventFieldTypeStr = nullptr;
                char const* eventFieldTypeTypeStr = nullptr;
                switch (fieldType) {
                case EventKeywordInformation: eventFieldTypeStr = "Keyword"; eventFieldTypeTypeStr = "uint64_t"; break;
                case EventLevelInformation:   eventFieldTypeStr = "Level";   eventFieldTypeTypeStr = "uint8_t"; break;
                case EventChannelInformation: eventFieldTypeStr = "Channel"; eventFieldTypeTypeStr = "uint8_t"; break;
                }

                if (hr != ERROR_INSUFFICIENT_BUFFER) {
                    fprintf(stderr, "error: failed to enumerate provider %s information (%u)\n", eventFieldTypeStr, hr);
                    continue;
                }

                if (cppFormat) {
                    printf("enum class %s : %s {\n", eventFieldTypeStr, eventFieldTypeTypeStr);
                } else {
                    printf("        %ss\n", eventFieldTypeStr);
                }

                auto providerFieldInfo = (PROVIDER_FIELD_INFOARRAY*) malloc(size);
                hr = TdhEnumerateProviderFieldInformation(&provider.guid_, fieldType, providerFieldInfo, &size);
                if (hr != ERROR_SUCCESS) {
                    fprintf(stderr, "error: failed to enumerate provider %s information (%u)\n", eventFieldTypeStr, hr);
                    free(providerFieldInfo);
                    continue;
                }

                std::vector<std::wstring> names;
                names.reserve(providerFieldInfo->NumberOfElements);
                int maxWidth = 0;
                for (ULONG i = 0; i < providerFieldInfo->NumberOfElements; ++i) {
                    auto name = GetStringPtr(providerFieldInfo, providerFieldInfo->FieldInfoArray[i].NameOffset);
                    if (cppFormat) {
                        names.emplace_back(CppCondition(name));
                        maxWidth = std::max(maxWidth, (int) names.back().length());
                    } else {
                        names.emplace_back(name);
                    }
                }

                for (ULONG i = 0; i < providerFieldInfo->NumberOfElements; ++i) {
                    if (cppFormat) {
                        printf("    %-*ls = 0x%llx,\n", maxWidth, names[i].c_str(), providerFieldInfo->FieldInfoArray[i].Value);
                    } else {
                        printf("            0x%llx: %ls\n", providerFieldInfo->FieldInfoArray[i].Value, names[i].c_str());
                    }
                }

                free(providerFieldInfo);

                if (cppFormat) {
                    printf("};\n\n");
                }
            }
        }

        // Print events ordered by task
        if (showEvents) {
            if (!provider.manifest_) {
                printf("%swarning: etw_list can't enumerate events from WMI MOF class-based providers.\n", cppFormat ? "// " : "        ");
            }
            if (cppFormat) {
                if (!events.empty()) {
                    std::vector<std::wstring> eventName;
                    int maxWidth = 0;
                    for (auto const& pair : events) {
                        for (auto const& event : pair.second) {
                            auto baseName = CppCondition(pair.first) + L'_' + event.opcodeName_;
                            auto name = baseName;
                            for (int version = 2; std::find(eventName.begin(), eventName.end(), name) != eventName.end(); ++version) {
                                wchar_t versionStr[128] = {};
                                _snwprintf_s(versionStr, _TRUNCATE, L"_%d", version);
                                name = baseName + versionStr;
                            }
                            eventName.emplace_back(name);
                            maxWidth = std::max(maxWidth, (int) name.length());
                        }
                    }

                    printf(
                        "// Event descriptors:\n"
                        "#define EVENT_DESCRIPTOR_DECL(name_, id_, version_, channel_, level_, opcode_, task_, keyword_) "
                        "struct name_ { \\\n"
                        "    static uint16_t const Id      = id_; \\\n"
                        "    static uint8_t  const Version = version_; \\\n"
                        "    static uint8_t  const Channel = channel_; \\\n"
                        "    static uint8_t  const Level   = level_; \\\n"
                        "    static uint8_t  const Opcode  = opcode_; \\\n"
                        "    static uint16_t const Task    = task_; \\\n"
                        "    static %s const Keyword = %skeyword_; \\\n"
                        "};\n"
                        "\n",
                        showKeywords ? "Keyword " : "uint64_t",
                        showKeywords ? "(Keyword) " : "");

                    size_t eventIdx = 0;
                    for (auto const& pair : events) {
                        for (auto const& event : pair.second) {
                            printf("EVENT_DESCRIPTOR_DECL(%-*ls, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%04x, 0x%016llx)\n",
                                maxWidth,
                                eventName[eventIdx].c_str(),
                                event.Id,
                                event.Version,
                                event.Channel,
                                event.Level,
                                event.Opcode,
                                event.Task,
                                event.Keyword);
                            eventIdx += 1;
                        }
                    }

                    printf(
                        "\n"
                        "#undef EVENT_DESCRIPTOR_DECL\n"
                        "\n"
                        "#pragma warning(push)\n"
                        "#pragma warning(disable: 4200) // nonstandard extension used: zero-sized array in struct\n"
                        "\n"
                        "#pragma pack(push)\n"
                        "#pragma pack(1)\n"
                        "\n");
                    eventIdx = 0;
                    for (auto const& pair : events) {
                        for (auto const& event : pair.second) {
                            PrintCppStruct(event.properties_, eventName[eventIdx]);
                            eventIdx += 1;
                        }
                    }
                    printf(
                        "#pragma pack(pop)\n"
                        "#pragma warning(pop)\n"
                        "\n");
                }
            } else {
                for (auto& pair : events) {
                    printf("        %ls::\n", pair.first.c_str());
                    for (auto& event : pair.second) {
                        printf("            %ls", event.opcodeName_.c_str());
                        if (event.Level != 0) {
                            printf(" (%ls)", event.levelName_.c_str());
                        }
                        printf("\n");

                        if (showEventParams) {
                            printf("                %04x %02x %02x %04x",
                                event.Id,
                                event.Version,
                                event.Opcode,
                                event.Task);
                            if (showChannels) printf(" %02x", event.Channel);
                            if (showLevels)   printf(" %02x", event.Level);
                            if (showKeywords) printf(" %016llx", event.Keyword);
                            printf("\n");

                            if (event.message_[0] != L'\0') {
                                printf("                '%ls'\n", event.message_.c_str());
                            }

                            for (auto const& prop : event.properties_) {
                                PrintEventProperty(prop, 5, 4);
                            }
                        }
                    }
                }
            }
        }

        if (cppFormat) {
            printf("}\n");
        }
    }

    // Done
    if (errorCount > 0) {
        fprintf(stderr, "error: there were %u errors\n", errorCount);
        return 1;
    }

    return 0;
}

