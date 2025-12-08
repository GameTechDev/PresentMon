// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "TestProcess.h"
#include <string>
#include <ranges>
#include "Folders.h"
#include "JobManager.h"

#include "../PresentMonMiddleware/ActionClient.h"
#include "../Interprocess/source/Interprocess.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace vi = std::views;
using namespace std::literals;
using namespace pmon;

namespace InterimBroadcasterTests
{
	class TestFixture : public CommonTestFixture
	{
	public:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-intbroad-test-ctrl)",
				.shmNamePrefix = "pm_intborad_test_intro",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "NONE",
			};
			return args;
		}
	};

	TEST_CLASS(CommonFixtureTests)
	{
		TestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// verify service lifetime and status command functionality
		TEST_METHOD(ServiceStatusTest)
		{
			// verify initial status
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(0ull, status.nsmStreamedPids.size());
			Assert::AreEqual(16u, status.telemetryPeriodMs);
			Assert::IsTrue((bool)status.etwFlushPeriodMs);
			Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
		}
		// verify action system can connect
		TEST_METHOD(ActionConnect)
		{
			mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
			Assert::IsFalse(client.GetShmPrefix().empty());
			// there is a bit of a race condition on creating a service, immediately connecting
			// and then immediately terminating it via the test control module
			// not a concern for normal operation and is entirely synthetic; don't waste
			// effort on trying to rework this, just add a little wait for odd tests like this
			std::this_thread::sleep_for(150ms);
		}
		// verify comms work with introspection (no wrapper)
		TEST_METHOD(IntrospectionConnect)
		{
			mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
			auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
			auto pIntro = pComms->GetIntrospectionRoot();
			Assert::AreEqual(2ull, pIntro->pDevices->size);
			auto pDevice = static_cast<const PM_INTROSPECTION_DEVICE*>(pIntro->pDevices->pData[1]);
			Assert::AreEqual("NVIDIA GeForce RTX 2080 Ti", pDevice->pName->pData);
		}
	};
}