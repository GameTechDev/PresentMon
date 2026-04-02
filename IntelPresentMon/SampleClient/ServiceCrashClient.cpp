#include "ServiceCrashClient.h"
#include "CliOptions.h"
#include <PresentMonAPIWrapper/FixedQuery.h>
#include <PresentMonAPI2Tests/TestCommands.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std::literals;
using namespace pmon::test::client;

namespace
{
	CrashPhase ClampPhase(int rawPhase)
	{
		if (rawPhase <= static_cast<int>(CrashPhase::SessionOpen)) {
			return CrashPhase::SessionOpen;
		}
		if (rawPhase >= static_cast<int>(CrashPhase::QueryPolling)) {
			return CrashPhase::QueryPolling;
		}
		return static_cast<CrashPhase>(rawPhase);
	}
}

int ServiceCrashClientTest(std::unique_ptr<pmapi::Session> pSession)
{
	auto& opt = clio::Options::Get();
	const auto phase = ClampPhase(*opt.submode);

	const auto WaitForPing = [] {
		std::string line;
		if (!std::getline(std::cin, line) || line != "%ping") {
			std::cout << "%%{ping-error}%%" << std::endl;
			return false;
		}
		std::cout << "%%{ping-ok}%%" << std::endl;
		return true;
	};

	const auto WaitForExit = [] {
		std::string line;
		while (std::getline(std::cin, line)) {
			if (line == "%exit") {
				std::cout << "%%{exit-ack}%%" << std::endl;
				return true;
			}
			if (line == "%quit") {
				std::cout << "%%{quit-ok}%%" << std::endl;
				return true;
			}
			std::cout << "%%{err-bad-command}%%" << std::endl;
		}
		return false;
	};

	if (phase >= CrashPhase::QueryRegistered) {
		pmapi::ProcessTracker tracker;

		PM_BEGIN_FIXED_FRAME_QUERY(CrashFrameQuery)
			pmapi::FixedQueryElement cpuStartTime{ this, PM_METRIC_CPU_START_TIME };
		PM_END_FIXED_QUERY query{ *pSession, 32 };

		if (phase >= CrashPhase::TargetTracked) {
			if (!opt.processId) {
				std::cout << "%%{err-missing-process-id}%%" << std::endl;
				return -1;
			}
			tracker = pSession->TrackProcess(*opt.processId);
		}

		if (!WaitForPing()) {
			return -1;
		}

		if (phase >= CrashPhase::QueryPolling && tracker) {
			std::atomic<bool> exitRequested{ false };
			std::atomic<bool> exitOk{ false };

			std::jthread commandThread{ [&] {
				exitOk = WaitForExit();
				exitRequested = true;
			} };

			while (!exitRequested) {
				query.ForEachConsume(tracker, [] {});
				std::this_thread::sleep_for(50ms);
			}

			if (!exitOk) {
				return -1;
			}
		}
		else if (!WaitForExit()) {
			return -1;
		}
	}
	else {
		if (!WaitForPing()) {
			return -1;
		}
		if (!WaitForExit()) {
			return -1;
		}
	}

	std::this_thread::sleep_for(10ms);
	return 0;
}
