#pragma once
#include "FixedQuery.h"
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <IntelPresentMon/CommonUtilities//str/String.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/EnumMap.h>
#include "Session.h"
#include "BlobContainer.h"
#include <vector>
#include <ranges>
#include <array>

namespace pmapi
{
	const BlobContainer& FixedQueryContainer_::PeekBlobContainer() const
	{
		return blobs_;
	}

	BlobContainer FixedQueryContainer_::ExtractBlobContainer()
	{
		return std::move(blobs_);
	}

	void FixedQueryContainer_::InjectBlobContainer(BlobContainer blobs)
	{
		blobs_ = std::move(blobs);
	}

	void FixedQueryContainer_::SwapBlobContainers(BlobContainer& blobs)
	{
		std::swap(blobs, blobs_);
	}

	const uint8_t* FixedQueryContainer_::PeekActiveBlob() const
	{
		return blobs_[activeBlobIndex_];;
	}

	void FixedQueryContainer_::SetActiveBlobIndex(uint32_t blobIndex)
	{
		activeBlobIndex_ = blobIndex;
	}

	size_t FixedQueryContainer_::GetActiveBlobIndex() const
	{
		return activeBlobIndex_;
	}

	void FixedQueryContainer_::FinalizationPreprocess_()
	{
		// replace slot indexes with device ids
		for (auto& e : rawElements_) {
			e.deviceId = MapDeviceId_(e.deviceId);
		}
	}

	uint32_t FixedQueryContainer_::MapDeviceId_(uint32_t deviceSlot) const
	{
		if (deviceSlot == 0) {
			return 0;
		}
		else {
			return slotDeviceIds_[deviceSlot - 1];
		}
	}

	void FixedQueryContainer_::FinalizationPostprocess_(bool isPolled)
	{
		// complete smart query objects
		for (auto&& [raw, smart] : std::views::zip(rawElements_, smartElements_)) {
			smart->dataOffset_ = raw.dataOffset;
			const auto dti = pIntrospection_->FindMetric(raw.metric).GetDataTypeInfo();
			smart->dataType_ = isPolled ? dti.GetPolledType() : dti.GetFrameType();
			smart->enumId_ = dti.GetEnumId();
		}
		// cleanup temporary construction data
		smartElements_.clear();
		rawElements_.clear();
		slotDeviceIds_.clear();
		pSession_ = nullptr;
	}



	BlobContainer FixedDynamicQueryContainer::MakeBlobContainer(uint32_t nBlobs) const
	{
		return query_.MakeBlobContainer(nBlobs);
	}

	void FixedDynamicQueryContainer::Poll(ProcessTracker& tracker)
	{
		query_.Poll(tracker, blobs_);
	}

	void FixedDynamicQueryContainer::Finalize_()
	{
		FinalizationPreprocess_();

		// register query
		assert(pSession_);
		query_ = pSession_->RegisterDyanamicQuery(rawElements_, winSizeMs_, metricOffsetMs_);

		// make blobs
		blobs_ = query_.MakeBlobContainer(nBlobs_);

		FinalizationPostprocess_(true);
	}



	BlobContainer FixedFrameQueryContainer::MakeBlobContainer(uint32_t nBlobs) const
	{
		return query_.MakeBlobContainer(nBlobs);
	}

	void FixedFrameQueryContainer::Consume(ProcessTracker& tracker)
	{
		query_.Consume(tracker, blobs_);
	}

	size_t FixedFrameQueryContainer::ForEachConsume(ProcessTracker& tracker, std::function<void()> frameHandler)
	{
		size_t nFramesProcessed = 0;
		do {
			Consume(tracker);
			const auto nPopulated = blobs_.GetNumBlobsPopulated();
			for (uint32_t i = 0; i < nPopulated; i++) {
				SetActiveBlobIndex(i);
				frameHandler();
			}
			nFramesProcessed += nPopulated;
		} while (blobs_.AllBlobsPopulated());
		return nFramesProcessed;
	}

	void FixedFrameQueryContainer::Finalize_()
	{
		FinalizationPreprocess_();

		// register query
		assert(pSession_);
		query_ = pSession_->RegisterFrameQuery(rawElements_);

		// make blobs
		blobs_ = query_.MakeBlobContainer(nBlobs_);

		FinalizationPostprocess_(false);
	}



	FixedQueryElement::FixedQueryElement(FixedQueryContainer_* pContainer, PM_METRIC metric,
		PM_STAT stat, uint32_t deviceSlot, uint32_t index)
		:
		pContainer_{ pContainer }
	{
		const auto deviceId = pContainer_->MapDeviceId_(deviceSlot);
		// check whether the metric specified by this element is available
		bool available = false;
		for (auto&& dmi : pContainer_->pIntrospection_->FindMetric(metric).GetDeviceMetricInfo()) {
			if (dmi.GetDevice().GetId() == deviceId) {
				available = dmi.IsAvailable() && index < dmi.GetArraySize();
				break;
			}
		}
		// do not register element if not available
		// default datatype void will signal its omission
		// consider registering in some way while keeping out of rawElements
		// to enable enumeration etc. even for unavailable elements
		if (available) {
			pContainer->rawElements_.push_back(PM_QUERY_ELEMENT{
				.metric = metric,
				.stat = stat,
				.deviceId = deviceSlot,
				.arrayIndex = index,
				});
			pContainer->smartElements_.push_back(this);
		}
	}

	bool FixedQueryElement::IsAvailable() const
	{
		return dataType_ != PM_DATA_TYPE_VOID;
	}
}