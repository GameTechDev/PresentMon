#pragma once
#include <set>
#include <optional>
#include <cereal/types/set.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>


namespace pmon::test
{
	namespace service
	{
		struct Status
		{
			std::set<uint32_t> nsmStreamedPids;
			uint32_t activeAdapterId;
			uint32_t telemetryPeriodMs;
			std::optional<uint32_t> etwFlushPeriodMs;

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(nsmStreamedPids, activeAdapterId, telemetryPeriodMs, etwFlushPeriodMs);
			}
		};
	}

	namespace client
	{
		struct Frame
		{
			double receivedTime;
			double cpuStartTime;
			double msBetweenPresents;
			double msUntilDisplayed;
			double msGpuBusy;

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(receivedTime, cpuStartTime, msBetweenPresents, msUntilDisplayed, msGpuBusy);
			}
		};

		struct FrameResponse
		{
			std::string status;
			std::vector<Frame> frames;

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(status, frames);
			}
		};
	}
}