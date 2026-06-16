#pragma once
#include "CppUnitTest.h"
#include "../CommonUtilities/test/MachineExpectations.h"
#include "../Interprocess/source/Interprocess.h"
#include "../PresentMonMiddleware/ActionClient.h"
#include <chrono>
#include <cstdint>
#include <format>
#include <string>
#include <thread>

namespace pmon::tests
{
	inline constexpr std::chrono::seconds DefaultFirstFrameWaitLimit{ 5 };

	template<typename Ring>
	auto WaitForFirstFrameRange(
		const Ring& ring,
		const char* label,
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit)
	{
		waitLimit = util::test::ScaleWait(waitLimit);
		const auto warmupStart = std::chrono::steady_clock::now();
		auto warmupRange = ring.GetSerialRange();
		while (warmupRange.second == 0 &&
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
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit)
	{
		const auto warmupRange = WaitForFirstFrameRange(ring, label, waitLimit);
		Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(
			warmupRange.second > 0, L"Timed out waiting for first playback frame");
		return warmupRange;
	}

	inline bool TryWaitForFirstFrame(
		const std::string& ctrlPipe,
		uint32_t pid,
		const char* label,
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit)
	{
		mid::ActionClient client{ ctrlPipe };
		auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
		pComms->OpenProcessDataStore(pid);
		const auto warmupRange = WaitForFirstFrameRange(pComms->GetProcessDataStore(pid).frameData, label, waitLimit);
		return warmupRange.second > 0;
	}

	inline auto WaitForFirstFrame(
		const std::string& ctrlPipe,
		uint32_t pid,
		const char* label,
		std::chrono::milliseconds waitLimit = DefaultFirstFrameWaitLimit)
	{
		mid::ActionClient client{ ctrlPipe };
		auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
		pComms->OpenProcessDataStore(pid);
		return WaitForFirstFrame(pComms->GetProcessDataStore(pid).frameData, label, waitLimit);
	}
}
