// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Exercises SpawnAdmitted, the admission step shared by the action client and server.
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "../Interprocess/source/act/ActionRunner.h"
#include "../Interprocess/source/act/AdmissionGate.h"
#include "../Interprocess/source/act/Lifecycle.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace ActionRunnerTests
{
	using namespace pmon::ipc::act;
	namespace as = boost::asio;

	// the members SpawnAdmitted uses
	struct TestState
	{
		Lifecycle lifecycle;
		AdmissionGate admission;
		as::io_context ioctx;
	};

	TEST_CLASS(SpawnAdmittedTests)
	{
	public:
		// Admitted from the io thread itself, the coroutine must not start until SpawnAdmitted
		// has returned. Started any earlier, it would run while the admission gate is held, and
		// a failure continuation calling BeginShutdown or dispatching again would deadlock.
		TEST_METHOD(AdmittedWorkStartsAfterTheGateIsReleased)
		{
			auto state = std::make_shared<TestState>();
			auto keepRunning = as::make_work_guard(state->ioctx);
			std::jthread io{ [state] { state->ioctx.run(); } };
			// -1 not started yet, 0 started before SpawnAdmitted returned, 1 started after
			auto startedAfterReturn = std::make_shared<std::atomic<int>>(-1);
			auto spawnReturned = std::make_shared<std::atomic<bool>>(false);
			as::post(state->ioctx, [state, startedAfterReturn, spawnReturned] {
				const bool accepted = SpawnAdmitted(state, [startedAfterReturn, spawnReturned]() -> as::awaitable<void> {
					startedAfterReturn->store(spawnReturned->load() ? 1 : 0);
					co_return;
				});
				spawnReturned->store(accepted);
			});
			for (int i = 0; i < 200 && startedAfterReturn->load() < 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			for (int i = 0; i < 200 && state->admission.Inflight() > 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			keepRunning.reset();
			state->ioctx.stop();
			io.join();
			Assert::AreEqual(1, startedAfterReturn->load(),
				L"Work admitted from the io thread started before SpawnAdmitted returned");
			Assert::AreEqual(0, state->admission.Inflight(), L"The admitted work was not completed");
		}
	};
}
