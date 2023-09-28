// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ServiceBooter.h"
#include <Core/source/infra/svc/Services.h>
#include <Core/source/infra/opt/Options.h>
#include <Core/source/infra/util/errtl/HResult.h>
#include <Core/source/infra/util/errtl/PMStatus.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <Core/source/infra/log/Blacklist.h>
#include "CefProcessCompass.h"

void p2c::client::util::BootServices()
{
	using infra::svc::Services;
	auto&& cli = infra::opt::get();
	
	if (cli.filesWorking)
	{
		Services::Singleton<infra::util::FolderResolver>([] { return std::make_shared<infra::util::FolderResolver>(); });
	}
	else
	{
		Services::Singleton<infra::util::FolderResolver>([] { return std::make_shared<infra::util::FolderResolver>(L"Intel\\PresentMon", L"PresentMon"); });
	}
	Services::Singleton<CefProcessCompass>([] { return std::make_shared<CefProcessCompass>(); });
	Services::Singleton<infra::log::Blacklist>([] { return std::make_shared<infra::log::Blacklist>(); });
	// error code translators
	infra::util::errtl::HResult::RegisterService();
	infra::util::errtl::PMStatus::RegisterService();
	// configure logging
	{
#ifdef NDEBUG
		constexpr bool is_debug = false;
#else
		constexpr bool is_debug = true;
#endif

		if (!is_debug && !cli.verbose)
		{
			infra::log::GetDefaultChannel()->AddPolicy({ [](infra::log::EntryOutputBase& entry) {
				return entry.data.level == infra::log::Level::Verbose;
			} });
		}

		if (cli.logBlack)
		{
			auto black = Services::Resolve<infra::log::Blacklist>();
			// TODO: make this relative to appdata resolved
			black->Ingest(L"logging-blacklist.txt");
			if (!black->IsEmpty()) {
				infra::log::GetDefaultChannel()->AddPolicy(black->GetPolicy());
			}
		}
	}
}
