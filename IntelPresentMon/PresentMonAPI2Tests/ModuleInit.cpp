#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include <filesystem>
#include "Folders.h"
#include "../CommonUtilities/test/MachineExpectations.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "../PresentMonAPI2/Internal.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;

void WipeAndRecreate(const fs::path& path)
{
	// Wipe the folder before any tests run
	try {
		if (fs::exists(path)) {
			fs::remove_all(path);
		}
		fs::create_directories(path);
	}
	catch (const std::exception& ex) {
		Logger::WriteMessage(std::format("Failed to wipe/create folder [{}]: {}\n", path.string(), ex.what()).c_str());
		throw; // let MSTest see this as a test infrastructure error
	}
}

TEST_MODULE_INITIALIZE(Api2TestModuleInit)
{
	// initialize c-api setting for in-module operation
	pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
	pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
	// setup folders
	WipeAndRecreate(MultiClientTests::logFolder_);
	WipeAndRecreate(EtlLoggerTests::logFolder_);
	WipeAndRecreate(EtlLoggerTests::outFolder_);
	WipeAndRecreate(EtlTests::logFolder_);
	WipeAndRecreate(EtlTests::outFolder_);
	WipeAndRecreate(PacedPolling::logFolder_);
	WipeAndRecreate(PacedPolling::outFolder_);
	WipeAndRecreate(PacedFrame::logFolder_);
	WipeAndRecreate(PacedFrame::outFolder_);
	WipeAndRecreate(InterimBroadcasterTests::logFolder_);
	WipeAndRecreate(InterimBroadcasterTests::outFolder_);
	WipeAndRecreate(IpcMcIntegrationTests::logFolder_);
	WipeAndRecreate(pmon::util::test::MachineExpectationOutputFolder);
	WipeAndRecreate(RealtimeMetricTests::logFolder_);
	WipeAndRecreate(RealtimeMetricTests::outFolder_);
	WipeAndRecreate(LoggingTests::logFolder_);
	WipeAndRecreate(MockTelemetryTests::logFolder_);
}
