#pragma once

// What the animation error calculation is based on
enum class AnimationErrorSource {
	CpuStart,
	AppProvider,
	PCLatency,
};

struct FlipDelayData {
	uint64_t flipDelay = 0;
	uint64_t displayQpc = 0;
};

struct FrameTimingData {
	// QPC of the very first AppSimStartTime. This will either be from
	// the app provider or the PCL simulation start time.
	uint64_t firstAppSimStartTime = 0;
	// QPC of the latest AppSimStartTime. This needs to be tracked because
	// not every present is guaranteed to have an app provider or
	// PCL simulation start time attached to it. Used when calculating
	// ms between simulation starts
	uint64_t lastAppSimStartTime = 0;
	// QPC of the last displayed AppSimStartTime. Similar to lastAppSimStartTime
	// but this is only updated when the present is displayed. Used in calculating
	// animation error and time.
	uint64_t lastDisplayedAppSimStartTime = 0;
    uint64_t lastDisplayedAppScreenTime = 0;
	AnimationErrorSource animationErrorSource = AnimationErrorSource::CpuStart;
	// NVIDIA Flip Delay related data
	std::unordered_map<uint32_t, FlipDelayData> flipDelayDataMap{};
    uint32_t lastDisplayedFrameId = 0;
};