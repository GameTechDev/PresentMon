// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../CefValues.h"
#include <CommonUtilities/win/Utilities.h>
#include <filesystem>

namespace p2c::client::util::async
{
	class ExploreLogFolder : public AsyncEndpoint
	{
	public:
		static constexpr std::string GetKey() { return "exploreLogFolder"; }
		ExploreLogFolder() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}

		Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor&) const override
		{
			const std::filesystem::path logFile{
				pArgObj->GetDictionary()->GetString("logFilePath").ToWString()
			};
			if (logFile.empty() || logFile.parent_path().empty()) {
				return MakeStringErrorResult(L"Invalid diagnostic log path");
			}
			::pmon::util::win::ExplorePath(logFile.parent_path());
			return Result{ true, CefValueNull() };
		}
	};
}
