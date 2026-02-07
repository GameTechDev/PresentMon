#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../Interprocess/source/Interprocess.h"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include "../IntelPresentMon/CommonUtilities/mc/SwapChainState.h"

namespace pmapi::intro
{
	class Root;
}

namespace pmon::mid
{
	class FrameMetricsSource;

	class Middleware
	{
	public:
		Middleware(std::optional<std::string> pipeNameOverride = {});
		~Middleware();
		const PM_INTROSPECTION_ROOT* GetIntrospectionData();
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot);
		void StartTracking(uint32_t processId);
		void StartPlaybackTracking(uint32_t processId, bool isBackpressured);
		void StopTracking(uint32_t processId);
		void SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t timeMs);
		void SetEtwFlushPeriod(std::optional<uint32_t> periodMs);
		void FlushFrames(uint32_t processId);
		PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs);
		void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery);
		void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob,
			uint32_t* numSwapChains, std::optional<uint64_t> nowTimestamp = {});
		void PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob);
		PM_FRAME_QUERY* RegisterFrameEventQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t& blobSize);
		void FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery);
		void ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames);
		void StopPlayback();
		uint32_t StartEtlLogging();
		std::string FinishEtlLogging(uint32_t etlLogSessionHandle);
	private:
		struct QueryMetricKey
		{
			PM_METRIC metric;
			uint32_t deviceId;
			uint32_t arrayIndex;
		};
		// functions
		const pmapi::intro::Root& GetIntrospectionRoot_();
		FrameMetricsSource& GetFrameMetricSource_(uint32_t pid) const;
		void RegisterMetricUsage_(const void* queryHandle, std::span<const PM_QUERY_ELEMENT> queryElements);
		void UnregisterMetricUsage_(const void* queryHandle);
		void UpdateMetricUsage_();
		// data
		// action client connection to service RPC
		std::shared_ptr<class ActionClient> pActionClient_;
		// ipc shared memory for frame data, telemetry, and introspection
		std::unique_ptr<ipc::MiddlewareComms> pComms_;
		// cache of marshalled introspection data
		std::unique_ptr<pmapi::intro::Root> pIntroRoot_;
		// Frame metrics sources mapped to process id
		std::map<uint32_t, std::unique_ptr<FrameMetricsSource>> frameMetricsSources_;
		// Query handles mapped to their metric usage keys
		std::unordered_map<const void*, std::vector<QueryMetricKey>> queryMetricUsage_;
	};
}
