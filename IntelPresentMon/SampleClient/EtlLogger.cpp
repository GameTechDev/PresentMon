#include "EtlLogger.h"
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

int EtlLoggerTest(std::unique_ptr<pmapi::Session> pSession)
{
	auto& opt = clio::Options::Get();

	std::string line;

	// ping gate to sync on init finished
	std::getline(std::cin, line);
	if (line != "%ping") {
		std::cout << "%%{ping-error}%%" << std::endl;
		return -1;
	}
	std::cout << "%%{ping-ok}%%" << std::endl;

	std::optional<PM_STATUS> errorStatus;
	pmapi::EtlLogger logger;

	try {
		if (opt.runTime && opt.outputPath) {
			logger = pSession->StartEtlLogging();
			std::this_thread::sleep_for(1s * *opt.runTime);
			logger.Finish(*opt.outputPath);
		}
	}
	catch (const pmapi::ApiErrorException& e) {
		if (!opt.testExpectError) {
			throw;
		}
		errorStatus = e.GetCode();
	}

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

	if (opt.runTime) {
		std::this_thread::sleep_for(1ms * *opt.runTime);
	}

	// wait for command
	while (std::getline(std::cin, line)) {
		if (line == "%quit") {
			std::cout << "%%{quit-ok}%%" << std::endl;
			std::this_thread::sleep_for(25ms);
			return 0;
		}
		else {
			std::cout << "%%{err-bad-command}%%" << std::endl;
		}
	}

	return -1;
}