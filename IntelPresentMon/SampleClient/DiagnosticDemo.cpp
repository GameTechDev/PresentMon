#include "DiagnosticDemo.h"
#include <iostream>
#include <format>
#include "CliOptions.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapper/DiagnosticHandler.h"
#include "../CommonUtilities/Exception.h"

using namespace pmon::util;

const char* GetLevelName(PM_DIAGNOSTIC_LEVEL lvl) {
	switch (lvl) {
	case PM_DIAGNOSTIC_LEVEL_NONE: return "None";
	case PM_DIAGNOSTIC_LEVEL_FATAL: return "Fatal";
	case PM_DIAGNOSTIC_LEVEL_ERROR: return "Error";
	case PM_DIAGNOSTIC_LEVEL_WARNING: return "Warning";
	case PM_DIAGNOSTIC_LEVEL_INFO: return "Info";
	case PM_DIAGNOSTIC_LEVEL_PERFORMANCE: return "Performance";
	case PM_DIAGNOSTIC_LEVEL_DEBUG: return "Debug";
	case PM_DIAGNOSTIC_LEVEL_VERBOSE: return "Verbose";
	default: return "Unknown";
	}
}

void RunDiagnosticDemo(int mode)
{
	// example of setting up diagnostic layer and custom diagnostic message handling
	pmapi::DiagnosticHandler dh{
		PM_DIAGNOSTIC_LEVEL_INFO,
		PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER | PM_DIAGNOSTIC_OUTPUT_FLAGS_QUEUE,
		[](const PM_DIAGNOSTIC_MESSAGE& msg) { std::cout <<
			std::format("[PMON {}] <{}> {}\n", GetLevelName(msg.level),
				msg.pTimestamp ? msg.pTimestamp : "", msg.pText);
		}
	};

	pmapi::Session sesh;

	// basic log info w/ message
	if (mode == 0) {
		std::string timeStamp = std::format("{}", std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() });
		PM_DIAGNOSTIC_MESSAGE dm{
			.level = PM_DIAGNOSTIC_LEVEL_WARNING,
			.system = PM_DIAGNOSTIC_SUBSYSTEM(PM_DIAGNOSTIC_SUBSYSTEM_USER + 2),
			.pText = "@#$ test test test",
			.pTimestamp = timeStamp.c_str(),
		};
		pmDiagnosticEnqueueMessage(&dm);
	}
	else if (mode > 255) {
		// mode is pid
		auto proc = sesh.TrackProcess(mode);
		// fail on purpose (non-static metric in static query call)
		try {
			pmapi::PollStatic(sesh, proc, PM_METRIC_APPLICATION_FPS);
		}
		catch (...) {
			std::cout << std::format("ERROR) {}\n", ReportException().first);
		}
		// silently fail to register dynamic query properly
		{
			PM_QUERY_ELEMENT elements[]{
				{ .metric = PM_METRIC_CPU_START_TIME, .stat = PM_STAT_AVG },
				{ .metric = PM_METRIC_GPU_FREQUENCY, .stat = PM_STAT_AVG, .deviceId = 42 },
			};
			sesh.RegisterDynamicQuery(elements);
		}
		// hard fail to register dynamic query with bogus metric id
		try {
			PM_QUERY_ELEMENT elements[]{
				{ .metric = PM_METRIC(42069), .stat = PM_STAT_AVG }
			};
			sesh.RegisterDynamicQuery(elements);
		}
		catch (...) {
			std::cout << std::format("ERROR) {}\n", ReportException().first);
		}
	}
	else {
		std::cout << "Unknown mode" << std::endl;
	}
}