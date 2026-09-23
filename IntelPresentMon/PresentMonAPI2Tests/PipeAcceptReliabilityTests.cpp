// Copyright (C) 2022-2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace PipeAcceptReliabilityTests
{
	class ReliabilityFixture : public CommonTestFixture
	{
	protected:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-pipe-reliability-ctrl)",
				.shmNamePrefix = "pm_pipe_reliability_intro",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "MultiClient",
			};
			return args;
		}
	};

	TEST_CLASS(PipeAcceptReliability)
	{
		ReliabilityFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(SequentialReconnect)
		{
			constexpr int cycles = 40;
			for (int i = 0; i < cycles; ++i) {
				auto client = fixture_.LaunchClient();
				client.Quit();
			}
		}

		TEST_METHOD(ReconnectAfterMurder)
		{
			constexpr int cycles = 10;
			for (int i = 0; i < cycles; ++i) {
				auto client = fixture_.LaunchClient();
				client.Murder();
				std::this_thread::sleep_for(5ms);
				auto reconnect = fixture_.LaunchClient();
				reconnect.Quit();
			}
		}

		TEST_METHOD(TwoClientChurn)
		{
			auto client1 = fixture_.LaunchClient();
			auto client2 = fixture_.LaunchClient();
			client1.Quit();
			client2.Murder();
			std::this_thread::sleep_for(5ms);
			auto client3 = fixture_.LaunchClient();
			client3.Quit();
		}

		TEST_METHOD(AvailabilityAfterChurn)
		{
			for (int i = 0; i < 5; ++i) {
				auto client = fixture_.LaunchClient();
				client.Murder();
				std::this_thread::sleep_for(5ms);
			}
			Assert::IsTrue(
				util::pipe::DuplexPipe::WaitForAvailability(R"(\\.\pipe\pm-pipe-reliability-ctrl)", 250),
				L"Control pipe should accept new clients after churn");
		}
	};
}
