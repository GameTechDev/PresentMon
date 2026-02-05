#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "Middleware.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Streamer/StreamClient.h"
#include <optional>
#include <string>
#include <queue>
#include "../CommonUtilities/Hash.h"
#include "../CommonUtilities/Math.h"
#include "FrameTimingData.h"
#include "../IntelPresentMon/CommonUtilities/mc/SwapChainState.h"

namespace pmapi::intro
{
	class Root;
}

namespace pmon::mid
{
	class FrameMetricsSource;

	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware(std::optional<std::string> pipeNameOverride = {});
		~ConcreteMiddleware() override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() override;
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) override;
		PM_STATUS StartStreaming(uint32_t processId) override;
		PM_STATUS StartPlaybackTracking(uint32_t processId, bool isBackpressured) override;
		PM_STATUS StopStreaming(uint32_t processId) override;
		PM_STATUS SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t timeMs) override;
		PM_STATUS SetEtwFlushPeriod(std::optional<uint32_t> periodMs) override;
		PM_STATUS FlushFrames(uint32_t processId) override;
		PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs) override;
		void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery) override {}
		void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob,
			uint32_t* numSwapChains, std::optional<uint64_t> nowTimestamp = {}) override;
		void PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob) override;
		PM_FRAME_QUERY* RegisterFrameEventQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t& blobSize) override;
		void FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery) override;
		void ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames) override;
		void StopPlayback() override;
		uint32_t StartEtlLogging() override;
		std::string FinishEtlLogging(uint32_t etlLogSessionHandle) override;
	private:
		// functions
		const pmapi::intro::Root& GetIntrospectionRoot_();
		FrameMetricsSource& GetFrameMetricSource_(uint32_t pid) const;
		// data
		// action client connection to service RPC
		std::shared_ptr<class ActionClient> pActionClient;
		// ipc shared memory for frame data, telemetry, and introspection
		std::unique_ptr<ipc::MiddlewareComms> pComms;
		// cache of marshalled introspection data
		std::unique_ptr<pmapi::intro::Root> pIntroRoot;
		// Frame metrics sources mapped to process id
		std::map<uint32_t, std::unique_ptr<FrameMetricsSource>> frameMetricsSources;
	};
}
