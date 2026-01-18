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
		// TODO: potentially source qpc frequency from store's bookkeeping
		qpcConverter_ = util::QpcConverter{ util::GetTimestampFrequencyUint64(), (uint64_t)pStore_->bookkeeping.startQpc};
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

		for (size_t serial = nextFrameSerial_; serial < range.second; ++serial) {
			const auto& frame = ring.At(serial);
			auto [it, inserted] = swapChains_.try_emplace(frame.swapChainAddress, perSwapChainCapacity_);
			auto& state = it->second;
			state.ProcessFrame(frame, *qpcConverter_);
		}

		nextFrameSerial_ = range.second;
		ring.MarkNextRead(nextFrameSerial_);
	}

	std::vector<util::metrics::FrameMetrics> FrameMetricsSource::Consume(size_t maxFrames)
	{
		std::vector<util::metrics::FrameMetrics> output;
		ProcessNewFrames_();

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

	const util::QpcConverter& FrameMetricsSource::GetQpcConverter() const
	{
		assert(qpcConverter_);
		return *qpcConverter_;
	}

}
