#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <evntrace.h>
#include <cstdint>
#include <string>
#include "../../PresentData/IFilterBuildListener.h"

namespace pmon::svc
{
	class EtwLogSession
	{
	public:
		EtwLogSession(const std::wstring& loggerName, const std::wstring& logFilePath);
		EtwLogSession(const EtwLogSession&) = delete;
		EtwLogSession & operator=(const EtwLogSession&) = delete;
		EtwLogSession(EtwLogSession&&) = delete;
		EtwLogSession & operator=(EtwLogSession&&) = delete;
		~EtwLogSession();
	private:
		// types
		struct TraceProperties_ : public EVENT_TRACE_PROPERTIES
		{
			wchar_t LoggerName[MAX_PATH];
			wchar_t LogFileName[MAX_PATH];
		};
		enum TimestampType_ : ULONG
		{
			TIMESTAMP_TYPE_QPC = 1,
			TIMESTAMP_TYPE_SYSTEM_TIME = 2,
			TIMESTAMP_TYPE_CPU_CYCLE_COUNTER = 3,
		};
		// data
		TRACEHANDLE hTraceSession_ = 0;
		TraceProperties_ traceProps_{};
	};
}