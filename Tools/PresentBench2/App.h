// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "Config.h"
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"

namespace pb2
{
	class App
	{
	public:
		explicit App(BenchConfig config);
		int Run(HINSTANCE instance);

	private:
		BenchConfig config_;
	};
}
