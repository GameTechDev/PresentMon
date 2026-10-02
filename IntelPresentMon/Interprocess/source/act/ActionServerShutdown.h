// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../../CommonUtilities/log/Log.h"
#include <cstdlib>
#include <vector>

namespace pmon::ipc::act
{
	// Production service teardown and the transport test call this function.
	// Order: mark final teardown, optional mid step, stop admission and join,
	// destroy the server, record StopTraceSessions, then stop traces.
	// A timeout terminates. It does not detach the runner.
	template<class Server, class Mid, class Destroy, class StopTraces>
	void JoinActionServerThenStopTraces(Server& server, Mid&& mid, Destroy&& destroy,
		StopTraces&& stopTraces, std::vector<const char*>* events = nullptr)
	{
		server.EnterFinalTeardown();
		mid();
		server.BeginShutdown();
		if (!server.WaitForShutdown()) {
			pmlog_error("Action server runner did not stop while PresentMon and ETW were still alive");
			std::terminate();
		}
		destroy();
		if (events) {
			events->push_back("StopTraceSessions");
		}
		stopTraces();
	}
}
