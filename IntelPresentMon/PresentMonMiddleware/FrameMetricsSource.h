#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <boost/circular_buffer.hpp>
#include "../CommonUtilities/Qpc.h"
#include "../CommonUtilities/mc/MetricsCalculator.h"
#include "../CommonUtilities/mc/UnifiedSwapChain.h"
#include "../Interprocess/source/Interprocess.h"

namespace pmon::mid
{
	class SwapChainState
	{
	public:
		explicit SwapChainState(size_t capacity);
		bool HasPending() const;
		const util::metrics::FrameMetrics& Peek() const;
		void ConsumeNext();
		void ProcessFrame(const util::metrics::FrameData& frame, util::QpcConverter& qpc);

	private:
		void ClampCursor_();
		void PushMetrics_(const util::metrics::FrameMetrics& metrics);

		boost::circular_buffer<util::metrics::FrameMetrics> metrics_;
		util::metrics::UnifiedSwapChain unified_;
		size_t cursor_ = 0;
	};

	class FrameMetricsSource
	{
	public:
		FrameMetricsSource(ipc::MiddlewareComms& comms, uint32_t processId, size_t perSwapChainCapacity);
		~FrameMetricsSource();

		FrameMetricsSource(const FrameMetricsSource&) = delete;
		FrameMetricsSource& operator=(const FrameMetricsSource&) = delete;
		FrameMetricsSource(FrameMetricsSource&&) = delete;
		FrameMetricsSource& operator=(FrameMetricsSource&&) = delete;

		std::vector<util::metrics::FrameMetrics> Consume(size_t maxFrames);

	private:
		void ProcessNewFrames_();

		ipc::MiddlewareComms& comms_;
		const ipc::FrameDataStore* pStore_ = nullptr;
		uint32_t processId_ = 0;
		size_t perSwapChainCapacity_ = 0;
		size_t nextFrameSerial_ = 0;
		std::optional<util::QpcConverter> qpcConverter_;
		std::unordered_map<uint64_t, SwapChainState> swapChains_;
	};
}
