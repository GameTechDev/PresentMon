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
#include "../CommonUtilities/log/Verbose.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/win/HrErrorCodeProvider.h"
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/Exception.h"
#include "../PresentMonAPIWrapperCommon/PmErrorCodeProvider.h"
#include "../PresentMonAPI2/Internal.h"

#include <cctype>
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

		std::vector<std::string> SplitVerboseModules_(const std::string& raw)
		{
			std::vector<std::string> tokens;
			std::string token;
			for (unsigned char ch : raw) {
				if (ch == ',' || std::isspace(ch)) {
					if (!token.empty()) {
						tokens.push_back(token);
						token.clear();
					}
					continue;
				}
				token.push_back(static_cast<char>(ch));
			}
			if (!token.empty()) {
				tokens.push_back(token);
			}
			return tokens;
		}

		std::vector<util::log::V> ParseVerboseModules_(const std::string& raw)
		{
			std::vector<util::log::V> modules;
			if (raw.empty()) {
				return modules;
			}
			const auto map = util::log::GetVerboseModuleMapNarrow();
			for (const auto& token : SplitVerboseModules_(raw)) {
				const auto key = util::str::ToLower(token);
				if (auto it = map.find(key); it != map.end()) {
					modules.push_back(it->second);
				}
			}
			return modules;
		}
	}

	void SetupTestLogging(const std::string& logFolder,
		const std::string& logLevel,
		const std::optional<std::string>& logVerboseModules) noexcept
	{
		try {
			util::log::IdentificationTable::AddThisProcess("ms-test");
			util::log::IdentificationTable::AddThisThread("exec");

			const auto verboseModules =
				logVerboseModules ? ParseVerboseModules_(*logVerboseModules) : std::vector<util::log::V>{};
			auto pChannel = util::log::GetDefaultChannel();
			if (!pChannel) {
				return;
			}

			const auto level = ParseLogLevel_(logLevel);
			auto& policy = util::log::GlobalPolicy::Get();
			policy.SetLogLevel(level);
			policy.SetTraceLevel(util::log::Level::Error);
			policy.SetExceptionTrace(false);
			for (auto mod : verboseModules) {
				policy.ActivateVerboseModule(mod);
			}

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
				for (auto mod : verboseModules) {
					dllPolicy.ActivateVerboseModule(mod);
				}
			}
		}
		catch (...) {
		}
	}

	LogChannelManager::LogChannelManager() noexcept
	{
		util::InstallSehTranslator();
		util::log::BootDefaultChannelEager();
	}

	LogChannelManager::~LogChannelManager()
	{
		pmFlushEntryPoint_();
		util::log::FlushEntryPoint();
	}
}
