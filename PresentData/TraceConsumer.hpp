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
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <tdh.h> // Must include after windows.h

struct EventMetadataKey {
    GUID guid_;
    EVENT_DESCRIPTOR desc_;
};

struct EventMetadataKeyHash { size_t operator()(EventMetadataKey const& k) const; }; 
struct EventMetadataKeyEqual { bool operator()(EventMetadataKey const& lhs, EventMetadataKey const& rhs) const; };

enum PropertyStatus {
    PROP_STATUS_NOT_FOUND       = 0,
    PROP_STATUS_FOUND           = 1 << 0,
    PROP_STATUS_CHAR_STRING     = 1 << 1,
    PROP_STATUS_WCHAR_STRING    = 1 << 2,
    PROP_STATUS_NULL_TERMINATED = 1 << 3,
};

struct EventDataDesc {
    wchar_t const* name_;   // Property name
    uint32_t arrayIndex_;   // Array index (optional)
    void* data_;            // OUT pointer to property data
    uint32_t size_;         // OUT size of property data
    uint32_t status_;       // OUT PropertyStatus result of search

    template<typename T> T GetData() const
    {
        assert(status_ & PROP_STATUS_FOUND);
        if (data_ == nullptr) {
            static bool first = true;
            if (first) {
                fprintf(stderr, "error: could not find event's %ls property.\n", name_);
                first = false;
            }
            assert(false);
            return T {};
        }
        if (size_ > sizeof(T)) {
            static bool first = true;
            if (first) {
                fprintf(stderr, "error: event's %ls property had unexpected size (%u > %zu).\n", name_, size_, sizeof(T));
                first = false;
            }
            assert(false);
            return *(T*) data_;
        }
        if (size_ < sizeof(T)) {

            // This is allowed and expected.  e.g., sometimes we want a
            // uint32_t promoted into uint64_t (for example to simplify pointer
            // handling).  It may also be a mistake though so we keep a warning
            // in DEBUG_VERBOSE.
#if DEBUG_VERBOSE
            static bool first = true;
            if (first) {
                fprintf(stderr, "warning: event's %ls property had unexpected size (%u < %zu).\n", name_, size_, sizeof(T));
                first = false;
            }
#endif
            T t {};
            memcpy(&t, data_, size_);
            return t;
        }

        return *(T*) data_;
    }

    template<typename T> T* GetString() const
    {
        assert(status_ & PROP_STATUS_FOUND);
        assert(status_ & (sizeof(T) == 1 ? PROP_STATUS_CHAR_STRING : PROP_STATUS_WCHAR_STRING));
        assert(status_ & PROP_STATUS_NULL_TERMINATED);
        assert(size_ >= sizeof(T) && (size_ % sizeof(T)) == 0);
        return (T*) data_;
    }
};

struct EventMetadata {
    std::unordered_map<EventMetadataKey, std::vector<uint8_t>, EventMetadataKeyHash, EventMetadataKeyEqual> metadata_;

    void AddMetadata(EVENT_RECORD* eventRecord);
    void GetEventData(EVENT_RECORD* eventRecord, EventDataDesc* desc, uint32_t descCount, uint32_t optionalCount=0);

    template<typename T> T GetEventData(EVENT_RECORD* eventRecord, wchar_t const* name, uint32_t arrayIndex = 0)
    {
        EventDataDesc desc = { name, arrayIndex, };
        GetEventData(eventRecord, &desc, 1);
        return desc.GetData<T>();
    }
};

template<> std::string EventMetadata::GetEventData<std::string>(EVENT_RECORD* eventRecord, wchar_t const* name, uint32_t arrayIndex);
template<> std::wstring EventMetadata::GetEventData<std::wstring>(EVENT_RECORD* eventRecord, wchar_t const* name, uint32_t arrayIndex);
