/*
Copyright 2017 Intel Corporation

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

#include <windows.h>
#include <stdio.h>
#include <string>
#include <tdh.h>

void PrintEventInformation(FILE* fp, EVENT_RECORD* pEventRecord);

template <typename T>
bool GetEventData(EVENT_RECORD* pEventRecord, wchar_t const* name, T* out)
{
    PROPERTY_DATA_DESCRIPTOR descriptor;
    descriptor.PropertyName = (ULONGLONG) name;
    descriptor.ArrayIndex = ULONG_MAX;

    auto status = TdhGetProperty(pEventRecord, 0, nullptr, 1, &descriptor, sizeof(T), (BYTE*) out);
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "error: could not get event %ls property (error=%u).\n", name, status);
        PrintEventInformation(stderr, pEventRecord);
        return false;
    }

    return true;
}

template <typename T>
T GetEventData(EVENT_RECORD* pEventRecord, wchar_t const* name)
{
    T value = {};
    auto ok = GetEventData(pEventRecord, name, &value);
    (void) ok;
    return value;
}

template <> bool GetEventData<std::string>(EVENT_RECORD* pEventRecord, wchar_t const* name, std::string* out);
