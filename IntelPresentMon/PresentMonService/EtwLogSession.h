#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <evntrace.h>
#include <cstdint>
#include <string>
#include <vector>
#include <span>
#include "../../PresentData/IFilterBuildListener.h"

namespace pmon::svc
{
    class EtwLogProviderListener : public IFilterBuildListener
    {
    public:
        // types
        struct ProviderDescription
        {
            std::vector<uint16_t> events;
            uint64_t anyKeyMask;
            uint64_t allKeyMask;
            uint8_t maxLevel;
            GUID providerGuid;
            uint32_t controlCode;
        };
        // functions
        void EventAdded(uint16_t id) override;
        void ProviderEnabled(const GUID& providerGuid, uint64_t anyKey, uint64_t allKey, uint8_t maxLevel,
            uint32_t controlCode) override;
		void ClearEvents() override;
		std::span<const ProviderDescription> GetProviderDescriptions() const;
    private:
        std::vector<uint16_t> eventsOnDeck_;
        std::vector<ProviderDescription> providerDescriptions_;
    };

	class EtwLogSession
	{
	public:
		EtwLogSession(const std::wstring& loggerName, const std::wstring& logFilePath,
			std::span<const EtwLogProviderListener::ProviderDescription> providers);
		EtwLogSession(const EtwLogSession&) = delete;
		EtwLogSession& operator=(const EtwLogSession&) = delete;
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