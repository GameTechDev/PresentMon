#include "Logging.h"

#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/Channel.h"
#include "../CommonUtilities/log/MsvcDebugDriver.h"
#include "../CommonUtilities/log/BasicFileDriver.h"
#include "../CommonUtilities/log/TextFormatter.h"
#include "../CommonUtilities/log/SimpleFileStrategy.h"
#include "../CommonUtilities/log/LinePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolvePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolver.h"
#include "../CommonUtilities/log/IdentificationTable.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/win/HrErrorCodeProvider.h"
#include "../CommonUtilities/win/WinAPI.h"
#include "../PresentMonAPIWrapperCommon/PmErrorCodeProvider.h"
#include "../PresentMonAPI2/Internal.h"

#include <filesystem>
#include <format>
#include <memory>
#include <mutex>

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel_()
		{
			// channel (use custom deleter to ensure deletion in this module's heap)
			auto pChannel = std::shared_ptr<IChannel>{ new Channel{}, [](Channel* p) { delete p; } };
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			pErrorResolver->AddProvider(std::make_unique<pmapi::PmErrorCodeProvider>());
			// error resolving policy
			auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
			pErrPolicy->SetResolver(std::move(pErrorResolver));
			pChannel->AttachComponent(std::move(pErrPolicy));
			// make and add the line-tracking policy
			pChannel->AttachComponent(std::make_shared<LinePolicy>());
			// attach debugger output by default
			const auto pFormatter = std::make_shared<TextFormatter>();
			pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
			return pChannel;
		}
	}

	// this is injected into to util::log namespace and hooks into that system
	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel_);
	}
}

namespace pmon::test
{
	namespace
	{
		struct LogLinkState
		{
			std::mutex mtx;
			bool linked = false;
			LoggingSingletons getters{};
		};

		LogLinkState& GetLogLinkState_()
		{
			static LogLinkState state;
			return state;
		}

		util::log::Level ParseLogLevel_(const std::string& logLevel)
		{
			if (logLevel.empty()) {
				return util::log::Level::Debug;
			}
			const auto levelMap = util::log::GetLevelMapNarrow();
			const auto key = util::str::ToLower(logLevel);
			if (auto it = levelMap.find(key); it != levelMap.end()) {
				return it->second;
			}
			return util::log::Level::Debug;
		}

		std::string BuildLogFileName_(const std::filesystem::path& folder)
		{
			return std::format("test-harness-{}.txt", GetCurrentProcessId());
		}
	}

	void SetupTestLogging(const std::string& logFolder, const std::string& logLevel) noexcept
	{
		try {
			auto pChannel = util::log::GetDefaultChannel();
			if (!pChannel) {
				return;
			}

			const auto level = ParseLogLevel_(logLevel);
			auto& policy = util::log::GlobalPolicy::Get();
			policy.SetLogLevel(level);
			policy.SetTraceLevel(util::log::Level::Error);
			policy.SetExceptionTrace(false);

			if (!logFolder.empty()) {
				std::filesystem::path folderPath{ logFolder };
				std::error_code ec;
				std::filesystem::create_directories(folderPath, ec);
				const auto filePath = folderPath / BuildLogFileName_(folderPath);
				auto pFormatter = std::make_shared<util::log::TextFormatter>();
				pChannel->AttachComponent(std::make_shared<util::log::BasicFileDriver>(
					pFormatter,
					std::make_shared<util::log::SimpleFileStrategy>(std::move(filePath))),
					"drv:file");
			}

			auto& linkState = GetLogLinkState_();
			LoggingSingletons gettersCopy{};
			{
				std::lock_guard lock{ linkState.mtx };
				if (!linkState.linked) {
					linkState.getters = pmLinkLogging_(pChannel, []() -> util::log::IdentificationTable& {
						return util::log::IdentificationTable::Get_();
						});
					linkState.linked = true;
				}
				gettersCopy = linkState.getters;
			}

			if (gettersCopy) {
				auto& dllPolicy = gettersCopy.getGlobalPolicy();
				dllPolicy.SetLogLevel(level);
				dllPolicy.SetTraceLevel(util::log::Level::Error);
				dllPolicy.SetExceptionTrace(false);
			}
		}
		catch (...) {
		}
	}
}
