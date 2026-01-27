// Copyright (C) 2025 Intel Corporation
#include "FrameMetricsSource.h"

namespace pmon::mid
{
	SwapChainState::SwapChainState(size_t capacity)
		:
		metrics_{ capacity }
	{
	}

	bool SwapChainState::HasPending() const
	{
		return cursor_ < metrics_.size();
	}

	const util::metrics::FrameMetrics& SwapChainState::Peek() const
	{
		return metrics_[cursor_];
	}

	void SwapChainState::ConsumeNext()
	{
		if (cursor_ < metrics_.size()) {
			++cursor_;
		}
	}

	void SwapChainState::ProcessFrame(const util::metrics::FrameData& frame, util::QpcConverter& qpc)
	{
		auto ready = unified_.Enqueue(frame, util::metrics::MetricsVersion::V2);
		for (auto& item : ready) {
			auto& present = item.presentPtr ? *item.presentPtr : item.present;
			auto* nextPtr = item.nextDisplayedPtr;

			auto computed = util::metrics::ComputeMetricsForPresent(
				qpc,
				present,
				nextPtr,
				unified_.swapChain,
				util::metrics::MetricsVersion::V2);

			for (auto& cm : computed) {
				PushMetrics_(cm.metrics);
			}
		}
	}

	bool SwapChainState::Empty() const
	{
		return metrics_.empty();
	}

	size_t SwapChainState::Size() const
	{
		return metrics_.size();
	}

	const util::metrics::FrameMetrics& SwapChainState::At(size_t index) const
	{
		return metrics_[index];
	}

	size_t SwapChainState::LowerBoundIndex(uint64_t timestamp) const
	{
		return BoundIndex_(timestamp, BoundKind_::Lower);
	}

	size_t SwapChainState::UpperBoundIndex(uint64_t timestamp) const
	{
		return BoundIndex_(timestamp, BoundKind_::Upper);
	}

	size_t SwapChainState::NearestIndex(uint64_t timestamp) const
	{
		const size_t count = Size();
		if (count == 0) {
			return 0;
		}

		size_t index = LowerBoundIndex(timestamp);
		if (index >= count) {
			return count - 1;
		}

		if (index > 0) {
			const uint64_t nextTimestamp = TimestampOf_(At(index));
			const uint64_t prevTimestamp = TimestampOf_(At(index - 1));
			const uint64_t prevDelta = timestamp - prevTimestamp;
			const uint64_t nextDelta = nextTimestamp - timestamp;
			if (prevDelta <= nextDelta) {
				--index;
			}
		}

		return index;
	}

	size_t SwapChainState::CountInTimestampRange(uint64_t start, uint64_t end) const
	{
		const size_t count = Size();
		if (count == 0) {
			return 0;
		}

		const size_t first = LowerBoundIndex(start);
		const size_t last = UpperBoundIndex(end);
		if (last < first) {
			return 0;
		}
		return last - first;
	}

	size_t SwapChainState::BoundIndex_(uint64_t timestamp, BoundKind_ kind) const
	{
		const size_t count = Size();
		size_t lo = 0;
		size_t hi = count;
		while (lo < hi) {
			const size_t mid = lo + (hi - lo) / 2;
			const uint64_t midTimestamp = TimestampOf_(At(mid));
			if (kind == BoundKind_::Lower) {
				if (midTimestamp < timestamp) {
					lo = mid + 1;
				}
				else {
					hi = mid;
				}
			}
			else {
				if (midTimestamp <= timestamp) {
					lo = mid + 1;
				}
				else {
					hi = mid;
				}
			}
		}
		return lo;
	}

	uint64_t SwapChainState::TimestampOf_(const util::metrics::FrameMetrics& metrics)
	{
		return metrics.presentStartQpc;
	}

	void SwapChainState::PushMetrics_(const util::metrics::FrameMetrics& metrics)
	{
		if (metrics_.full() && cursor_ > 0) {
			--cursor_;
		}
		metrics_.push_back(metrics);
		ClampCursor_();
	}

	void SwapChainState::ClampCursor_()
	{
		if (cursor_ > metrics_.size()) {
			cursor_ = metrics_.size();
		}
	}

	FrameMetricsSource::FrameMetricsSource(ipc::MiddlewareComms& comms, uint32_t processId, size_t perSwapChainCapacity)
		:
		comms_{ comms },
		processId_{ processId },
		perSwapChainCapacity_{ perSwapChainCapacity == 0 ? size_t{ 1 } : perSwapChainCapacity }
	{
		// open the data store from ipc
		comms_.OpenFrameDataStore(processId_);
		pStore_ = &comms_.GetFrameDataStore(processId_);
		const auto range = pStore_->frameData.GetSerialRange();
		nextFrameSerial_ = range.first;
	}

	FrameMetricsSource::~FrameMetricsSource()
	{
		// close the data store
		try {
			if (pStore_ == nullptr) {
				return;
			}
			comms_.CloseFrameDataStore(processId_);
			pStore_ = nullptr;
			swapChains_.clear();
		}
		catch (...) {
			pmlog_error(util::ReportException("Error closing frame data store"));
		}
	}

	void FrameMetricsSource::ProcessNewFrames_()
	{
		if (pStore_ == nullptr) {
			return;
		}

		const auto& ring = pStore_->frameData;
		const auto range = ring.GetSerialRange();

		if (range.first > nextFrameSerial_) {
			nextFrameSerial_ = range.first;
		}
		if (nextFrameSerial_ >= range.second) {
			return;
		}

		// deferred initialization of the qpc converter is required because
		// when the store is first created, start qpc is not yet populated (populates on
		// first frame that is broadcasted)
		if (!qpcConverter_) {
			// TODO: potentially source qpc frequency from store's bookkeeping
			qpcConverter_ = util::QpcConverter{ util::GetTimestampFrequencyUint64(), (uint64_t)pStore_->bookkeeping.startQpc };
		}

		for (size_t serial = nextFrameSerial_; serial < range.second; ++serial) {
			const auto& frame = ring.At(serial);
			auto [it, inserted] = swapChains_.try_emplace(frame.swapChainAddress, perSwapChainCapacity_);
			auto& state = it->second;
			state.ProcessFrame(frame, *qpcConverter_);
		}

		nextFrameSerial_ = range.second;
		ring.MarkNextRead(nextFrameSerial_);
	}

	void FrameMetricsSource::Update()
	{
		ProcessNewFrames_();
	}

	std::vector<util::metrics::FrameMetrics> FrameMetricsSource::Consume(size_t maxFrames)
	{
		std::vector<util::metrics::FrameMetrics> output;
		Update();

		if (maxFrames == 0) {
			return output;
		}

		output.reserve(maxFrames);

		for (size_t i = 0; i < maxFrames; ++i) {
			SwapChainState* selectedState = nullptr;
			const util::metrics::FrameMetrics* selectedMetrics = nullptr;
			uint64_t selectedSwapChain = 0;

			for (auto& [address, state] : swapChains_) {
				if (!state.HasPending()) {
					continue;
				}
				const auto& metrics = state.Peek();
				if (selectedMetrics == nullptr ||
					metrics.timeInSeconds < selectedMetrics->timeInSeconds ||
					(metrics.timeInSeconds == selectedMetrics->timeInSeconds && address < selectedSwapChain)) {
					selectedMetrics = &metrics;
					selectedState = &state;
					selectedSwapChain = address;
				}
			}

			if (selectedMetrics == nullptr || selectedState == nullptr) {
				break;
			}

			output.push_back(*selectedMetrics);
			selectedState->ConsumeNext();
		}

		return output;
	}

	const SwapChainState* FrameMetricsSource::GetActiveSwapChainState_(uint64_t start, uint64_t end) const
	{
		const SwapChainState* selectedState = nullptr;
		uint64_t selectedCount = 0;

		for (const auto& [address, state] : swapChains_) {
			if (state.Empty()) {
				continue;
			}
			const size_t count = state.CountInTimestampRange(start, end);
			if (selectedState == nullptr ||
				count > selectedCount) {
				selectedState = &state;
				selectedCount = count;
			}
		}

		return selectedState;
	}

	const util::metrics::FrameMetrics* FrameMetricsSource::FindNearestActive(uint64_t start, uint64_t end, uint64_t timestamp) const
	{
		const auto* state = GetActiveSwapChainState_(start, end);
		if (state == nullptr || state->Empty()) {
			return nullptr;
		}

		const size_t index = state->NearestIndex(timestamp);
		return &state->At(index);
	}

	bool FrameMetricsSource::HasActiveSwapChainSamples(uint64_t start, uint64_t end) const
	{
		const auto* state = GetActiveSwapChainState_(start, end);
		return state != nullptr && !state->Empty();
	}

	const util::QpcConverter& FrameMetricsSource::GetQpcConverter() const
	{
		assert(qpcConverter_);
		return *qpcConverter_;
	}

}
