// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../ControlServer.h"
#include <gtest/gtest.h>
#include <map>
#include <stdexcept>
#include <string>

TEST(ControlServer, FramingAndDispatch)
{
	pb2::ControlServer server;
	server.SetHandlers({
		{ "ping", [](const std::string&) { return "ping-ok"; } },
		{ "echo", [](const std::string& arg) { return arg.empty() ? "empty" : arg; } },
	});

	EXPECT_EQ(server.HandleCommand("%ping"), "ping-ok");
	EXPECT_EQ(server.HandleCommand("ping\r\n"), "ping-ok");
	EXPECT_EQ(server.HandleCommand("%echo {\"a\":1}"), "{\"a\":1}");
	EXPECT_EQ(server.HandleCommand("%unknown"), "error: unknown command unknown");
	EXPECT_EQ(pb2::ControlServer::FormatResponse("ping-ok"), "%%{ping-ok}%%\n");
}

TEST(ControlServer, HandlerExceptionBecomesError)
{
	pb2::ControlServer server;
	server.SetHandlers({
		{ "boom", [](const std::string&) -> std::string {
			throw std::runtime_error("bad json");
		} },
	});
	EXPECT_EQ(server.HandleCommand("%boom"), "error: bad json");
}

TEST(ControlServer, WaitForFrameUsesDefaultTimeout)
{
	const auto request = pb2::ParseWaitForFrameRequest(R"({"appFrameId":42})");
	EXPECT_EQ(request.appFrameId, 42u);
	EXPECT_EQ(request.timeoutMs, 5000u);
}

TEST(ControlServer, WaitForFrameUsesNormalTimeout)
{
	const auto request = pb2::ParseWaitForFrameRequest(
		R"({"appFrameId":42,"timeoutMs":1250})");
	EXPECT_EQ(request.appFrameId, 42u);
	EXPECT_EQ(request.timeoutMs, 1250u);
}

TEST(ControlServer, WaitForFrameCapsOversizedTimeout)
{
	const auto request = pb2::ParseWaitForFrameRequest(
		R"({"appFrameId":42,"timeoutMs":90000})");
	EXPECT_EQ(request.appFrameId, 42u);
	EXPECT_EQ(request.timeoutMs, 5000u);
}
