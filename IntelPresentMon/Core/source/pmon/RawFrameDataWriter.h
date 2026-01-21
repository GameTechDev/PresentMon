// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include "StatisticsTracker.h"
#include <PresentMonAPI2/PresentMonAPI.h>
#include <PresentMonAPIWrapper/Session.h>
#include <PresentMonAPIWrapper/BlobContainer.h>
#include <Core/source/infra/Logging.h>

namespace p2c::pmon
{
	class QueryElementContainer_;

	class RawFrameDataWriter
	{
	public:
        RawFrameDataWriter(std::wstring path, const pmapi::ProcessTracker& procTracker, uint32_t activeDeviceId,
			pmapi::Session& session, std::optional<std::wstring> frameStatsPath, const pmapi::intro::Root& introRoot);
		RawFrameDataWriter(const RawFrameDataWriter&) = delete;
		RawFrameDataWriter& operator=(const RawFrameDataWriter&) = delete;
		void Process();
		~RawFrameDataWriter();
	private:
		// functions
		double GetDuration_() const;
		void WriteStats_();
		// data
		static constexpr uint32_t numberOfBlobs = 150u;
		const pmapi::ProcessTracker& procTracker;
		std::unique_ptr<QueryElementContainer_> pQueryElementContainer;
		std::optional<std::wstring> frameStatsPath;
		std::unique_ptr<StatisticsTracker> pStatsTracker;
		std::unique_ptr<StatisticsTracker> pAnimationErrorTracker;
		pmapi::BlobContainer blobs;
		double startTime = -1.;
		double endTime = -1.;
		std::ofstream file;
	};
}
