#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
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
		bool Empty() const;
		size_t Size() const;
		const util::metrics::FrameMetrics& At(size_t index) const;
		size_t LowerBoundIndex(uint64_t timestamp) const;
		size_t UpperBoundIndex(uint64_t timestamp) const;
		size_t NearestIndex(uint64_t timestamp) const;
		size_t CountInTimestampRange(uint64_t start, uint64_t end) const;
		template<typename F>
		size_t ForEachInTimestampRange(uint64_t start, uint64_t end, F&& func) const
		{
			const size_t count = Size();
			if (count == 0) {
				return 0;
			}

			size_t index = LowerBoundIndex(start);
			size_t visited = 0;
			for (; index < count; ++index) {
				const auto& metrics = At(index);
				if (TimestampOf_(metrics) > end) {
					break;
				}
				std::forward<F>(func)(metrics);
				++visited;
			}
			return visited;
		}

	private:
		enum class BoundKind_
		{
			Lower,
			Upper
		};

		size_t BoundIndex_(uint64_t timestamp, BoundKind_ kind) const;
		static uint64_t TimestampOf_(const util::metrics::FrameMetrics& metrics);
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

		void Update();
		std::vector<util::metrics::FrameMetrics> Consume(size_t maxFrames);
		std::vector<uint64_t> GetSwapChainAddressesInTimestampRange(uint64_t start, uint64_t end) const;
		const SwapChainState* FindSwapChainState(uint64_t swapChainAddress) const;
		const util::QpcConverter& GetQpcConverter() const;

	private:
		void ProcessNewFrames_();

		ipc::MiddlewareComms& comms_;
		const ipc::FrameDataStore* pStore_ = nullptr;
		uint32_t processId_ = 0;
		size_t perSwapChainCapacity_ = 0;
		size_t nextFrameSerial_ = 0;
		// optional to defer creation until we are sure store is fully initialized (startQpc)
		std::optional<util::QpcConverter> qpcConverter_;
		std::unordered_map<uint64_t, SwapChainState> swapChains_;
	};
}
