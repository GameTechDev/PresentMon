// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "LogSetup.h"
#include "../../IntelPresentMon/CommonUtilities/log/Log.h"
#include "../../IntelPresentMon/CommonUtilities/log/Channel.h"
#include "../../IntelPresentMon/CommonUtilities/log/MsvcDebugDriver.h"
#include "../../IntelPresentMon/CommonUtilities/log/BasicFileDriver.h"
#include "../../IntelPresentMon/CommonUtilities/log/StdioDriver.h"
#include "../../IntelPresentMon/CommonUtilities/log/TextFormatter.h"
#include "../../IntelPresentMon/CommonUtilities/log/SimpleFileStrategy.h"
#include "../../IntelPresentMon/CommonUtilities/log/LinePolicy.h"
#include "../../IntelPresentMon/CommonUtilities/log/ErrorCodeResolvePolicy.h"
#include "../../IntelPresentMon/CommonUtilities/log/ErrorCodeResolver.h"
#include "../../IntelPresentMon/CommonUtilities/win/HrErrorCodeProvider.h"
#include <memory>
#include <string>

using namespace std::literals;

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			auto pChannel = std::shared_ptr<IChannel>{ new Channel{}, [](Channel* p) { delete p; } };
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
			pErrPolicy->SetResolver(std::move(pErrorResolver));
			pChannel->AttachComponent(std::move(pErrPolicy));
			const auto pFormatter = std::make_shared<TextFormatter>();
			pChannel->AttachComponent(std::make_shared<LinePolicy>());
			pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter));
			pChannel->AttachComponent(
				std::make_shared<StdioDriver>(pFormatter, true, true), "drv:std");
			return pChannel;
		}
	}

	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel);
	}
}

namespace pb2log
{
	using namespace pmon::util::log;

	LogChannelManager::LogChannelManager() noexcept
	{
		GetDefaultChannel();
	}

	LogChannelManager::~LogChannelManager()
	{
		FlushEntryPoint();
	}

	void ConfigureLogging(const std::string& logPath) noexcept
	{
		try {
			auto pChan = GetDefaultChannel();
			if (!logPath.empty()) {
				pChan->AttachComponent(std::make_shared<BasicFileDriver>(
					std::make_shared<TextFormatter>(),
					std::make_shared<SimpleFileStrategy>(logPath)), "drv:file");
			}
		}
		catch (...) {}
	}
}
