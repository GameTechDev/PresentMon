// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <filesystem>
#include <format>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

namespace
{
	int RunPresentBench2(const std::string& args)
	{
		namespace fs = std::filesystem;
		fs::path exe = "PresentBench2.exe";
		if (!fs::exists(exe)) {
			throw std::runtime_error("PresentBench2.exe not found beside tests");
		}
		const auto cmd = std::format("\"{}\" {}", exe.string(), args);
		return std::system(cmd.c_str());
	}
}

TEST(CliReject, InvalidApiCombinations)
{
	const std::string bounded = " --frame-count 1 --frame-sleep 0";
	constexpr int cliValidationError = 108;
	constexpr int configValidationError = 1;
	EXPECT_EQ(RunPresentBench2("--vk --swap-effect FlipDiscard" + bounded),
		cliValidationError);
	EXPECT_EQ(RunPresentBench2("--vk --fullscreen" + bounded), cliValidationError);
	EXPECT_EQ(RunPresentBench2("--dx11 --use-xess-fg" + bounded), cliValidationError);
	EXPECT_EQ(RunPresentBench2("--width 100 --borderless" + bounded), cliValidationError);
	EXPECT_EQ(RunPresentBench2("--dx9 --enable-debug-layer" + bounded), cliValidationError);
	EXPECT_EQ(RunPresentBench2("--dx9 --sync-interval 2" + bounded), configValidationError);
	EXPECT_EQ(RunPresentBench2("--dx9 --buffer-count 5" + bounded), configValidationError);
	EXPECT_EQ(RunPresentBench2("--dx12 --swap-effect Discard" + bounded),
		configValidationError);
	EXPECT_EQ(RunPresentBench2(
		"--vk --enable-debug-layer --enable-stdio-control" + bounded),
		configValidationError);
}

TEST(CliReject, HelpSucceeds)
{
	const int code = RunPresentBench2("--help");
	// CLI11 help exits with 0.
	EXPECT_EQ(code, 0);
}

TEST(CliReject, ValidEnumValueIsAccepted)
{
	EXPECT_EQ(RunPresentBench2(
		"--describe-capabilities --dx12 --swap-effect FlipDiscard"), 0);
}
