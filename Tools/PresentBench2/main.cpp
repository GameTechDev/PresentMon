// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "App.h"
#include "Capabilities.h"
#include "CliOptions.h"
#include "LogSetup.h"
#include "../../IntelPresentMon/CommonUtilities/log/Log.h"
#include <cstdio>
#include <iostream>

int main(int argc, char** argv)
{
	pb2log::LogChannelManager logManager;

	if (auto code = clio::Options::Init(argc, argv, true)) {
		std::cerr << clio::Options::GetDiagnostics();
		return *code;
	}

	pb2::BenchConfig config;
	try {
		config = clio::Options::Get().ToConfig();
	}
	catch (const std::exception& ex) {
		std::cerr << "error: " << ex.what() << "\n";
		return 1;
	}

	pb2log::ConfigureLogging(config.logPath);

	try {
		if (config.describeCapabilities) {
			const auto json = pb2::CapabilitiesToJson(
				pb2::ProbeCapabilities(config.adapter));
			std::fwrite(json.data(), 1, json.size(), stdout);
			std::fputc('\n', stdout);
			return 0;
		}
		pb2::App app(std::move(config));
		return app.Run(GetModuleHandleW(nullptr));
	}
	catch (const std::exception& ex) {
		pmlog_error(ex.what());
		std::cerr << "error: " << ex.what() << "\n";
		return 2;
	}
}
