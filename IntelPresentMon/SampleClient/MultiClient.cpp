#include "MultiClient.h"
#include "CliOptions.h"
#include <PresentMonAPIWrapperCommon/EnumMap.h>
#include <PresentMonAPIWrapper/FixedQuery.h>
#include <chrono>
#include <iostream>
#include <PresentMonAPI2Tests/TestCommands.h>
#include <cereal/archives/json.hpp>

using namespace std::literals;
using namespace pmon::test::client;
using Clock = std::chrono::steady_clock;

int MultiClientTest(std::unique_ptr<pmapi::Session> pSession)
{
	auto& opt = clio::Options::Get();

	std::optional<PM_STATUS> errorStatus;
	pmapi::ProcessTracker tracker;

	try {
		if (opt.telemetryPeriodMs) {
			pSession->SetTelemetryPollingPeriod(0, *opt.telemetryPeriodMs);
		}
		if (opt.etwFlushPeriodMs) {
			pSession->SetEtwFlushPeriod(*opt.etwFlushPeriodMs);
		}
		if (opt.processId) {
			tracker = pSession->TrackProcess(*opt.processId);
		}
	}
	catch (const pmapi::ApiErrorException& e) {
		if (!opt.testExpectError) {
			throw;
		}
		errorStatus = e.GetCode();
	}

	std::string line;

	// ping gate to sync on init finished
	std::getline(std::cin, line);
	if (line != "%ping") {
		std::cout << "%%{ping-error}%%" << std::endl;
		return -1;
	}
	std::cout << "%%{ping-ok}%%" << std::endl;

	// if we captured an error, wait here for error ack
	if (errorStatus) {
		std::getline(std::cin, line);
		if (line != "%err-check") {
			std::cout << "%%{err-check-error}%%" << std::endl;
			return -1;
		}
		auto&& err = pmapi::EnumMap::GetKeyMap(PM_ENUM_STATUS)->at(*errorStatus).narrowSymbol;
		std::cout << "%%{err-check-ok:" << err << "}%%" << std::endl;
	}

	// capture frames if requested
	std::vector<Frame> frames;
	if (opt.runTime) {
		PM_BEGIN_FIXED_FRAME_QUERY(FrameQuery)
			pmapi::FixedQueryElement cpuStartTime{ this, PM_METRIC_CPU_START_TIME };
			pmapi::FixedQueryElement msBetweenPresents{ this, PM_METRIC_BETWEEN_PRESENTS };
			pmapi::FixedQueryElement msUntilDisplayed{ this, PM_METRIC_UNTIL_DISPLAYED };
			pmapi::FixedQueryElement msGpuBusy{ this, PM_METRIC_GPU_BUSY };
		PM_END_FIXED_QUERY query{ *pSession, 1'000 };
		
		const auto start = Clock::now();
		while (Clock::now() - start < 1s * *opt.runTime) {
			query.ForEachConsume(tracker, [&] {
				frames.push_back(Frame{
					.receivedTime = std::chrono::duration<double>(Clock::now() - start).count(),
					.cpuStartTime = query.cpuStartTime,
					.msBetweenPresents = query.msBetweenPresents,
					.msUntilDisplayed = query.msUntilDisplayed,
					.msGpuBusy = query.msGpuBusy,
				});
			});
			std::this_thread::sleep_for(5ms);
		}
	}

	// wait for command
	while (std::getline(std::cin, line)) {
		if (line == "%quit") {
			std::cout << "%%{quit-ok}%%" << std::endl;
			std::this_thread::sleep_for(25ms);
			return 0;
		}
		else if (line == "%get-frames") {
			FrameResponse resp;
			if (!opt.runTime) {
				resp.status = "get-frames-err:not-recorded";
			}
			else {
				resp.status = "get-frames-ok";
				resp.frames = frames;
			}
			std::ostringstream oss;
			cereal::JSONOutputArchive{ oss }(resp);
			std::cout << "%%{" << oss.str() << "}%%" << std::endl;
		}
		else {
			std::cout << "%%{err-bad-command}%%" << std::endl;
		}
	}

	return -1;
}