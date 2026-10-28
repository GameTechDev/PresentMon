#pragma once
#include "CppUnitTest.h"
#include "../CommonUtilities/test/MachineExpectations.h"
#include "../Interprocess/source/Interprocess.h"
#include "../PresentMonMiddleware/ActionClient.h"
#include <chrono>
#include <cstdint>
#include <format>
#include <map>
#include <string>
#include <thread>

namespace pmon::tests
{
	inline constexpr std::chrono::seconds DefaultFirstFrameWaitLimit{ 5 };

	inline size_t FrameCountInRange_(const auto& range)
	{
		return range.second > range.first ? (size_t)(range.second - range.first) : 0u;
	}

	inline std::string RequireFrameSegmentName(
		const std::map<uint32_t, std::string>& frameSegmentNames,
		uint32_t pid)
	{
		const auto it = frameSegmentNames.find(pid);
		Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(
			it != frameSegmentNames.end(),
			L"Expected frame segment name for tracked pid");
		return it->second;
	}

	template<typename Ring>
	auto WaitForFirstFrameRange(
		const Ring& ring,
		const char* label,
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit,
		size_t minFrameCount = 1)
	{
		waitLimit = util::test::ScaleWait(waitLimit);
		const auto warmupStart = std::chrono::steady_clock::now();
		auto warmupRange = ring.GetSerialRange();
		while (FrameCountInRange_(warmupRange) < minFrameCount &&
			std::chrono::steady_clock::now() - warmupStart < waitLimit) {
			std::this_thread::sleep_for(std::chrono::milliseconds{ 25 });
			warmupRange = ring.GetSerialRange();
		}

		const auto warmupElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - warmupStart).count();
		Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(std::format("{} warmup range [{},{}), elapsedMs={}\n",
			label, warmupRange.first, warmupRange.second, warmupElapsedMs).c_str());
		return warmupRange;
	}

	template<typename Ring>
	auto WaitForFirstFrame(
		const Ring& ring,
		const char* label,
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit,
		size_t minFrameCount = 1)
	{
		const auto warmupRange = WaitForFirstFrameRange(ring, label, waitLimit, minFrameCount);
		Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(
			FrameCountInRange_(warmupRange) >= minFrameCount,
			L"Timed out waiting for playback frames");
		return warmupRange;
	}

	// Observes an already-tracked pid's segment. Callers must StartTracking / TrackProcess
	// first and supply the segment name from that track response or QueryStatus.
	inline bool TryWaitForFirstFrame(
		const std::string& ctrlPipe,
		uint32_t pid,
		const std::string& frameSegmentName,
		const char* label,
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit,
		size_t minFrameCount = 1)
	{
		mid::ActionClient client{ ctrlPipe };
		auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
		pComms->OpenFrameDataStore(pid, frameSegmentName);
		const auto warmupRange = WaitForFirstFrameRange(pComms->GetFrameDataStore(pid).frameData, label, waitLimit, minFrameCount);
		return FrameCountInRange_(warmupRange) >= minFrameCount;
	}

	inline auto WaitForFirstFrame(
		const std::string& ctrlPipe,
		uint32_t pid,
		const std::string& frameSegmentName,
		const char* label,
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit,
		size_t minFrameCount = 1)
	{
		mid::ActionClient client{ ctrlPipe };
		auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
		pComms->OpenFrameDataStore(pid, frameSegmentName);
		return WaitForFirstFrame(pComms->GetFrameDataStore(pid).frameData, label, waitLimit, minFrameCount);
	}
}
