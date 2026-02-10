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
			// new ipc tracking
			std::set<uint32_t> trackedPids;
			std::set<uint32_t> frameStorePids;
			uint32_t telemetryPeriodMs;
			std::optional<uint32_t> etwFlushPeriodMs;

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(trackedPids, frameStorePids, telemetryPeriodMs, etwFlushPeriodMs);
			}
		};
	}

	namespace client
	{
		enum class CrashPhase
		{
			SessionOpen = 0,
			QueryRegistered = 1,
			TargetTracked = 2,
			QueryPolling = 3,
		};

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
