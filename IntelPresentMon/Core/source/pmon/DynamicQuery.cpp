#pragma once
#include "DynamicQuery.h"
#include <PresentMonAPIWrapper/PresentMonAPIWrapper.h>
#include <ranges>


namespace p2c::pmon
{
	DynamicQuery::DynamicQuery(pmapi::Session& session, double winSizeMs, double metricOffsetMs, std::span<const kern::QualifiedMetric> qmets)
	{
		for (auto& qmet : qmets) {
			elements.push_back(PM_QUERY_ELEMENT{
				.metric = (PM_METRIC)qmet.metricId,
				.stat = (PM_STAT)qmet.statId,
				.deviceId = qmet.deviceId,
				.arrayIndex = qmet.arrayIndex,
			});
		}
		query = session.RegisterDynamicQuery(elements, winSizeMs, metricOffsetMs);
		blobs = query.MakeBlobContainer(1u);
	}

	std::vector<PM_QUERY_ELEMENT> DynamicQuery::ExtractElements()
	{
		return std::move(elements);
	}

	void DynamicQuery::Poll(const pmapi::ProcessTracker& tracker)
	{
		if (query) {
			query.Poll(tracker, blobs);
		}
		else {
			pmlog_warn("Polling empty dynamic query");
		}
	}

	const uint8_t* DynamicQuery::GetBlobData() const
	{
		if (blobs.GetNumBlobsPopulated() == 0) {
			return nullptr;
		}
		return blobs.GetFirst();
	}
}