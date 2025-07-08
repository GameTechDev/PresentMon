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

namespace pmapi::intro
{
	class Root;
}

namespace pmon::mid
{
	// Used to calculate correct start frame based on metric offset
	struct MetricOffsetData {
		uint64_t queryToFrameDataDelta = 0;
		uint64_t metricOffset = 0;
	};

	class InputToFsManager {
	public:
		void AddI2FsValueForProcess(uint32_t processId, uint64_t timeStamp, double value) {
			auto it = mProcessIdToI2FsValue.find(processId);
			if (it == mProcessIdToI2FsValue.end()) {
				auto firstEmaValue = pmon::util::CalculateEma(
					0.,
					value,
					mEmaAlpha);
				it = mProcessIdToI2FsValue.emplace(processId, I2FsValueContainer{ firstEmaValue, timeStamp }).first;
				return;
			}
			if (timeStamp > it->second.timestamp) {
				it->second.i2FsValue = pmon::util::CalculateEma(
					it->second.i2FsValue,
					value,
					mEmaAlpha);
				it->second.timestamp = timeStamp;
			}
		}

		// Retrieve the current input to frame start for a given process id.
		double GetI2FsForProcess(uint32_t processId) const {
			auto it = mProcessIdToI2FsValue.find(processId);
			if (it == mProcessIdToI2FsValue.end()) {
				return 0.f;
			}
			return it->second.i2FsValue;
		}

		// Remove a process from the map if it�s no longer needed.
		void RemoveProcess(uint32_t processId) {
			mProcessIdToI2FsValue.erase(processId);
		}

	private:
		struct I2FsValueContainer {
			double i2FsValue = 0.f;
			uint64_t timestamp = 0;
		};

		double const mEmaAlpha = 0.1;

		std::map<uint32_t, I2FsValueContainer> mProcessIdToI2FsValue; // Map from a process id to the current input to frame start value.
	};


    // Copied from: PresentMon/PresentMon.hpp
    // We store SwapChainData per process and per swapchain, where we maintain:
    // - information on previous presents needed for console output or to compute metrics for upcoming
    //   presents,
    // - pending presents whose metrics cannot be computed until future presents are received,
    // - exponential averages of key metrics displayed in console output.
	struct fpsSwapChainData {
        // Pending presents waiting for the next displayed present.
        std::vector<PmNsmPresentEvent> mPendingPresents;

        // The most recent present that has been processed (e.g., output into CSV and/or used for frame
        // statistics).
        PmNsmPresentEvent mLastPresent;
		// The most recent app present that has been processed (e.g., output into CSV and/or used for frame
		// statistics).
		PmNsmPresentEvent mLastAppPresent;

        bool mLastPresentIsValid = false;
		bool mLastAppPresentIsValid = false;

        // Whether to include frame data in the next PresentEvent's FrameMetrics.
        bool mIncludeFrameData = true;

        // IntelPresentMon specifics:
        std::vector<double> mCPUBusy;
        std::vector<double> mCPUWait;
        std::vector<double> mGPULatency;
        std::vector<double> mGPUBusy;
        std::vector<double> mVideoBusy;
        std::vector<double> mGPUWait;
        std::vector<double> mDisplayLatency;
        std::vector<double> mDisplayedTime;
        std::vector<double> mAppDisplayedTime;
		std::vector<double> mAnimationError;
        std::vector<double> mClickToPhotonLatency;
		std::vector<double> mAllInputToPhotonLatency;
        std::vector<double> mDropped;
		std::vector<double> mInstrumentedDisplayLatency;
		std::vector<double> mMsBetweenPresents;
        std::vector<double> mMsInPresentApi;
        std::vector<double> mMsUntilDisplayed;
        std::vector<double> mMsBetweenDisplayChange;
		std::vector<double> mMsUntilRenderComplete;
		std::vector<double> mMsBetweenSimStarts;
		std::vector<double> mMsPcLatency;

		std::vector<double> mInstrumentedSleep;
		std::vector<double> mInstrumentedRenderLatency;
		std::vector<double> mInstrumentedGpuLatency;
		std::vector<double> mInstrumentedReadyTimeToDisplayLatency;

		// QPC of last received input data that did not make it to the screen due 
		// to the Present() being dropped
		uint64_t mLastReceivedNotDisplayedAllInputTime = 0;
		uint64_t mLastReceivedNotDisplayedMouseClickTime = 0;
		// QPC of the last PC Latency simulation start
		uint64_t mLastReceivedNotDisplayedPclSimStart = 0;
		uint64_t mLastReceivedNotDisplayedPclInputTime = 0;

		// Animation error source. Start with CPU start QPC and switch if
		// we receive a valid PCL or App Provider simulation start time.
		AnimationErrorSource mAnimationErrorSource = AnimationErrorSource::CpuStart;

		// Accumulated PC latency input to frame start time due to the 
		// Present() being dropped
		double mAccumulatedInput2FrameStartTime = 0.f;

        // begin/end screen times to optimize average calculation:
		uint64_t mLastDisplayedScreenTime = 0;    // The last presented frame's ScreenTime (qpc)
		uint64_t mLastDisplayedAppScreenTime = 0; // The last presented app frame's ScreenTime (qpc)
		uint64_t display_0_screen_time = 0;       // The first presented frame's ScreenTime (qpc)
		uint64_t mLastDisplayedSimStartTime = 0;  // The simulation start of the last displayed frame
		uint32_t display_count = 0;               // The number of presented frames
		// QPC of the last simulation start time iregardless of whether it was displayed or not
		uint64_t mLastSimStartTime = 0;
	};

	struct DeviceInfo
	{
		PM_DEVICE_VENDOR deviceVendor;
		std::string deviceName;
		uint32_t deviceId;
		std::optional<uint32_t> adapterId;
		std::optional<double> gpuSustainedPowerLimit;
		std::optional<uint64_t> gpuMemorySize;
		std::optional<uint64_t> gpuMemoryMaxBandwidth;
		std::optional<double> cpuPowerLimit;
	};

	struct MetricInfo
	{
		// Map of array indices to associated data
		std::unordered_map<uint32_t, std::vector<double>> data;
	};

	class ConcreteMiddleware : public Middleware
	{
	public:
		ConcreteMiddleware(std::optional<std::string> pipeNameOverride = {});
		~ConcreteMiddleware() override;
		const PM_INTROSPECTION_ROOT* GetIntrospectionData() override;
		void FreeIntrospectionData(const PM_INTROSPECTION_ROOT* pRoot) override;
		PM_STATUS StartStreaming(uint32_t processId) override;
		PM_STATUS StopStreaming(uint32_t processId) override;
		PM_STATUS SetTelemetryPollingPeriod(uint32_t deviceId, uint32_t timeMs) override;
		PM_STATUS SetEtwFlushPeriod(std::optional<uint32_t> periodMs) override;
		PM_DYNAMIC_QUERY* RegisterDynamicQuery(std::span<PM_QUERY_ELEMENT> queryElements, double windowSizeMs, double metricOffsetMs) override;
		void FreeDynamicQuery(const PM_DYNAMIC_QUERY* pQuery) override {}
		void PollDynamicQuery(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains) override;
		void PollStaticQuery(const PM_QUERY_ELEMENT& element, uint32_t processId, uint8_t* pBlob) override;
		PM_FRAME_QUERY* RegisterFrameEventQuery(std::span<PM_QUERY_ELEMENT> queryElements, uint32_t& blobSize) override;
		void FreeFrameEventQuery(const PM_FRAME_QUERY* pQuery) override;
		void ConsumeFrameEvents(const PM_FRAME_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t& numFrames) override;
		void StopPlayback() override;
	private:
		PmNsmFrameData* GetFrameDataStart(StreamClient* client, uint64_t& index, uint64_t dataOffset, uint64_t& queryFrameDataDelta, double& windowSampleSizeMs);
		uint64_t GetAdjustedQpc(uint64_t current_qpc, uint64_t frame_data_qpc, uint64_t queryMetricsOffset, LARGE_INTEGER frequency, uint64_t& queryFrameDataDelta);
		bool DecrementIndex(NamedSharedMem* nsm_view, uint64_t& index);
		PM_STATUS SetActiveGraphicsAdapter(uint32_t deviceId);
		void GetStaticGpuMetrics();

		void CalculateFpsMetric(fpsSwapChainData& swapChain, const PM_QUERY_ELEMENT& element, uint8_t* pBlob, LARGE_INTEGER qpcFrequency);
		void CalculateGpuCpuMetric(std::unordered_map<PM_METRIC, MetricInfo>& metricInfo, const PM_QUERY_ELEMENT& element, uint8_t* pBlob);
		double CalculateStatistic(std::vector<double>& inData, PM_STAT stat, bool invert = false) const;
		double CalculatePercentile(std::vector<double>& inData, double percentile, bool invert) const;
		bool GetGpuMetricData(size_t telemetry_item_bit, PresentMonPowerTelemetryInfo& power_telemetry_info, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo);
		bool GetCpuMetricData(size_t telemetryBit, CpuTelemetryInfo& cpuTelemetry, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo);
		void GetStaticCpuMetrics();
		std::string GetProcessName(uint32_t processId);
		void CopyStaticMetricData(PM_METRIC metric, uint32_t deviceId, uint8_t* pBlob, uint64_t blobOffset, size_t sizeInBytes = 0);

		void CalculateMetrics(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob, uint32_t* numSwapChains, LARGE_INTEGER qpcFrequency, std::unordered_map<uint64_t, fpsSwapChainData>& swapChainData, std::unordered_map<PM_METRIC, MetricInfo>& metricInfo);
		void SaveMetricCache(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob);
		void CopyMetricCacheToBlob(const PM_DYNAMIC_QUERY* pQuery, uint32_t processId, uint8_t* pBlob);

		std::optional<size_t> GetCachedGpuInfoIndex(uint32_t deviceId);

		const pmapi::intro::Root& GetIntrospectionRoot();

		std::shared_ptr<class ActionClient> pActionClient;
		uint32_t clientProcessId = 0;
		// Stream clients mapping to process id
		std::map<uint32_t, std::unique_ptr<StreamClient>> presentMonStreamClients;
		// Frame timing data for each process id
		std::map<uint32_t, FrameTimingData> frameTimingData;
		std::unique_ptr<ipc::MiddlewareComms> pComms;
		// Dynamic query handle to frame data delta
		std::unordered_map<std::pair<const PM_DYNAMIC_QUERY*, uint32_t>, uint64_t> queryFrameDataDeltas;
		// Dynamic query handle to cache data
		std::unordered_map<std::pair<const PM_DYNAMIC_QUERY*, uint32_t>, std::unique_ptr<uint8_t[]>> cachedMetricDatas;
		std::vector<DeviceInfo> cachedGpuInfo;
		std::vector<DeviceInfo> cachedCpuInfo;
		uint32_t currentGpuInfoIndex = UINT32_MAX;
		std::optional<uint32_t> activeDevice;
		std::unique_ptr<pmapi::intro::Root> pIntroRoot;
		InputToFsManager mPclI2FsManager;
	};
}
