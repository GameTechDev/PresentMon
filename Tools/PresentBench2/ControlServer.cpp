// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "ControlServer.h"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace pb2
{
	namespace
	{
		struct ParsedCommand
		{
			std::string name;
			std::string arg;
		};

		ParsedCommand ParseCommand(std::string cmd)
		{
			while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == '\n' || cmd.back() == ' ')) {
				cmd.pop_back();
			}
			if (!cmd.empty() && cmd.front() == '%') {
				cmd.erase(cmd.begin());
			}
			const auto sp = cmd.find(' ');
			if (sp == std::string::npos) {
				return { std::move(cmd), {} };
			}
			return { cmd.substr(0, sp), cmd.substr(sp + 1) };
		}
	}

	WaitForFrameRequest ParseWaitForFrameRequest(const std::string& arg)
	{
		const auto j = nlohmann::json::parse(arg.empty() ? "{}" : arg);
		WaitForFrameRequest request;
		request.appFrameId = j.value("appFrameId", request.appFrameId);
		request.timeoutMs = (std::min)(j.value("timeoutMs", request.timeoutMs), 5000u);
		return request;
	}

	ControlServer::~ControlServer()
	{
		Stop();
	}

	void ControlServer::SetHandlers(std::map<std::string, ControlHandler> handlers)
	{
		handlers_ = std::move(handlers);
	}

	void ControlServer::SetDisconnectHandler(std::function<void()> handler)
	{
		disconnectHandler_ = std::move(handler);
	}

	std::string ControlServer::FormatResponse(const std::string& payload)
	{
		// stdout is in text mode, so the CRT expands '\n' to "\r\n" for pipes.
		return std::string("%%{") + payload + "}%%\n";
	}

	void ControlServer::StartStdio()
	{
		stop_ = false;
		exited_ = false;
		stdioThread_ = std::thread([this] { StdioThread_(); });
	}

	void ControlServer::Stop()
	{
		stop_ = true;
		if (stdioThread_.joinable()) {
			const HANDLE thread = (HANDLE)stdioThread_.native_handle();
			while (!exited_ && WaitForSingleObject(thread, 10) == WAIT_TIMEOUT) {
				CancelSynchronousIo((HANDLE)stdioThread_.native_handle());
			}
			stdioThread_.join();
		}
	}

	std::string ControlServer::HandleCommand(const std::string& commandLine)
	{
		const auto command = ParseCommand(commandLine);
		try {
			const auto it = handlers_.find(command.name);
			if (it == handlers_.end()) {
				return std::string("error: unknown command ") + command.name;
			}
			return it->second(command.arg);
		}
		catch (const std::exception& ex) {
			return std::string("error: ") + ex.what();
		}
	}

	void ControlServer::StdioThread_()
	{
		std::string line;
		while (!stop_) {
			if (!std::getline(std::cin, line)) {
				if (!stop_ && disconnectHandler_) {
					disconnectHandler_();
				}
				break;
			}
			if (line.empty()) {
				continue;
			}
			const auto response = HandleCommand(line);
			std::cout << FormatResponse(response) << std::flush;
			if (ParseCommand(line).name == "quit") {
				break;
			}
		}
		exited_ = true;
	}
}
