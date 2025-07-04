// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include "../IntelPresentMon/CommonUtilities/log/Log.h"

#ifndef PRESENTMON_ENABLE_DEBUG_TRACE
#ifdef NDEBUG
#define PRESENTMON_ENABLE_DEBUG_TRACE 0
#else
#define PRESENTMON_ENABLE_DEBUG_TRACE 1
#endif
#endif

struct PresentEvent; // Can't include PresentMonTraceConsumer.hpp because it includes Debug.hpp (before defining PresentEvent)
struct PMTraceConsumer;
struct EventMetadata;
struct _EVENT_RECORD;
union _LARGE_INTEGER;

#if PRESENTMON_ENABLE_DEBUG_TRACE

// Enable/disable the verbose trace.
void EnableVerboseTrace(bool enable);
bool IsVerboseTraceEnabled();

// Assertions either written to verbose trace (if enabled) or routed to assert().
void DebugAssertImpl(wchar_t const* msg, wchar_t const* file, int line);
#define DebugAssertWide1(x) L##x
#define DebugAssertWide2(x) DebugAssertWide1(x)
#define DebugAssert(condition) !!(condition) || (DebugAssertImpl(L#condition, DebugAssertWide2(__FILE__), __LINE__), false)

// Should call this before modifying a PresentEvent member; causes those changes to be
// included in the verbose trace.
void VerboseTraceBeforeModifyingPresentImpl(PresentEvent const* p);
#define VerboseTraceBeforeModifyingPresent(p) !IsVerboseTraceEnabled() || (VerboseTraceBeforeModifyingPresentImpl(p), false)

// Print debug information about the handled event
void VerboseTraceEventImpl(PMTraceConsumer* pmConsumer, _EVENT_RECORD* eventRecord, EventMetadata* metadata);
#define VerboseTraceEvent(c, e, m) !IsVerboseTraceEnabled() || (VerboseTraceEventImpl(c, e, m), false)

#else

// When PRESENTMON_ENABLE_DEBUG_TRACE==0, all of the the verbose trace infrastructure should
// get optimized away.
#define IsVerboseTraceEnabled() false
#define DebugAssert(condition) !!(condition) || pmlog_warn("PresentData Assert Failed: "#condition)
#define VerboseTraceBeforeModifyingPresent(p)
#define VerboseTraceEvent(c, e, m)

#endif

// Print a time or time range.  You must call InitializeTimeStampInfo() before
// either of the PrintTime...() functions.
void InitializeTimestampInfo(_LARGE_INTEGER* firstTimestamp, _LARGE_INTEGER const& timestampFrequency);
int PrintTime(uint64_t timestampValue);
int PrintTimeDelta(uint64_t timestampValue);
