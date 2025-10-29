#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include <filesystem>
#include "Folders.h"

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
	WipeAndRecreate(MultiClientTests::logFolder_);
	WipeAndRecreate(EtlLoggerTests::logFolder_);
	WipeAndRecreate(EtlLoggerTests::outFolder_);
	WipeAndRecreate(PacedPolling::logFolder_);
	WipeAndRecreate(PacedPolling::outFolder_);
}