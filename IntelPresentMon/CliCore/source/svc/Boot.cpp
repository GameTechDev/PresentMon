#include "Boot.h"
#include <Core/source/infra/svc/Services.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <Core/source/infra/util/errtl/HResult.h>
#include <Core/source/infra/util/errtl/PMStatus.h>
#include <Core/source/infra/log/DefaultChannel.h>
#include <Core/source/infra/log/drv/SimpleFileDriver.h>
#include <Core/source/infra/log/drv/DebugOutDriver.h>
#include <PresentMonAPI/PresentMonAPI.h>
#include <filesystem>

namespace p2c::cli::svc
{
	void Boot(bool logging, std::optional<std::string> logPath, std::optional<int> logLevel)
	{
		using Services = infra::svc::Services;

#ifdef NDEBUG
		constexpr bool is_debug = false;
#else
		constexpr bool is_debug = true;
#endif

		// appfolder and docfolder resolve to cwd
		Services::Bind<infra::util::FolderResolver>(
			[] { return std::make_shared<infra::util::FolderResolver>(L"", L"", false); }
		);

		if (!is_debug) {
			const auto level = infra::log::Level(std::clamp(logLevel.value_or(0), 0, 3));
			infra::log::SetDefaultChannelFactory([=] {
				auto pDefaultChannel = std::make_shared<infra::log::Channel>();
				if (logging) {
					auto logFilePath = logPath
						.transform([](auto& p) {return p + "\\pm-cli.log"; })
						.value_or("pm-cli.log");
					pDefaultChannel->AddDriver(std::make_unique<infra::log::drv::SimpleFileDriver>(std::move(logFilePath)));
					pDefaultChannel->AddPolicy({ [=](infra::log::EntryOutputBase& entry) {
						return (int)entry.data.level <= (int)level;
					} });
				}
				return pDefaultChannel;
			});
			const auto glogLevel = [&] {
				switch (level) {
				case infra::log::Level::Error: return 2;
				case infra::log::Level::Warning: return 1;
				case infra::log::Level::Info: return 0;
				case infra::log::Level::Debug: return 0;
				default: return 2;
				}
			}();
			if (logging && logPath) {
				std::filesystem::create_directories(*logPath);
			}
			if (logging) {
				pmInitializeLogging(
					logPath.transform(&std::string::c_str).value_or("."),
					"pm-sdk",
					".log",
					glogLevel
				);
			}
			else {
				pmInitializeLogging(nullptr, nullptr, nullptr, glogLevel);
			}
		}
		else {
			std::filesystem::create_directories("logs");
			pmInitializeLogging("logs", "pm-sdk", ".log", 0);
		}

		// error code translators
		infra::util::errtl::HResult::RegisterService();
		infra::util::errtl::PMStatus::RegisterService();
	}
}