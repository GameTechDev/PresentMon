// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>

namespace pb2
{
	using ControlHandler = std::function<std::string(const std::string& argJson)>;

	struct WaitForFrameRequest
	{
		uint32_t appFrameId = 1;
		uint32_t timeoutMs = 5000;
	};

	WaitForFrameRequest ParseWaitForFrameRequest(const std::string& arg);

	class ControlServer
	{
	public:
		ControlServer() = default;
		~ControlServer();

		void SetHandlers(std::map<std::string, ControlHandler> handlers);
		void SetDisconnectHandler(std::function<void()> handler);
		void StartStdio();
		void Stop();

		static std::string FormatResponse(const std::string& payload);
		std::string HandleCommand(const std::string& commandLine);

	private:
		void StdioThread_();
		std::map<std::string, ControlHandler> handlers_;
		std::function<void()> disconnectHandler_;
		std::atomic<bool> stop_{ false };
		std::atomic<bool> exited_{ true };
		std::thread stdioThread_;
	};
}
