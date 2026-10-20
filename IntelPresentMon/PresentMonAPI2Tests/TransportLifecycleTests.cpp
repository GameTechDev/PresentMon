// Copyright (C) 2022-2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Shutdown, session disposal and its reasons, admission racing shutdown, runner joins and final
// teardown.
#include "TransportTestKit.h"

namespace TransportDemuxTests
{
	TEST_CLASS(LifecycleTests)
	{
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			RegisterActions_();
		}
		TEST_METHOD(ClientIsNotRunningAfterServerDisconnect)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			std::unique_ptr<TestClient> pClient;
			{
				TestServer server{ pipeName, disposeCount };
				pClient = ConnectClient_(pipeName, eventCount);
				Assert::IsTrue(pClient->IsRunning(), L"Client should be running while session is active");
			}
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning(),
				L"Client io context should stop when the session reader strand exits");
		}
		// A continuation shuts down its own client while another request on that
		// client is still pending. Ownership stays with this test until both settle.
		TEST_METHOD(PendingRequestFailsWhenShutdownFromContinuationOnIoThread)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			std::atomic<bool> pendingFailed = false;
			std::atomic<bool> pendingFinished = false;
			std::jthread pendingRequester{ [&] {
				try {
					pClient->DispatchSync(DropRequest::Params{});
				}
				catch (...) {
					pendingFailed = true;
				}
				pendingFinished = true;
			} };
			std::this_thread::sleep_for(100ms);

			std::atomic<int> continuationCalls = 0;
			std::atomic<bool> shutdownPosted = false;
			pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 1, .pushCount = 0 },
				[&](EchoWithPush::Response&&, std::exception_ptr) {
					continuationCalls.fetch_add(1);
					pClient->BeginShutdown();
					shutdownPosted = true;
				});

			for (int i = 0; i < 300 && (!pendingFinished || !shutdownPosted); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1, continuationCalls.load(), L"Continuation did not run exactly once");
			Assert::IsTrue(shutdownPosted.load(), L"Shutdown was not initiated from the continuation");
			Assert::IsTrue(pendingFinished.load(), L"Same-client shutdown left a pending request hung");
			Assert::IsTrue(pendingFailed.load(), L"A pending request during same-client shutdown did not fail");
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(PendingRequestFailsWhenClientShutsDown)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			std::atomic<bool> failed = false;
			std::atomic<bool> finished = false;
			std::jthread requester{ [&] {
				try {
					pClient->DispatchSync(DropRequest::Params{});
				}
				catch (...) {
					failed = true;
				}
				finished = true;
			} };
			// let the responder consume the request without answering it
			std::this_thread::sleep_for(100ms);
			pClient->BeginShutdown();
			for (int i = 0; i < 300 && !finished; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsTrue(finished.load(), L"Shutdown left a request waiting for a response");
			Assert::IsTrue(failed.load(), L"A request that can never be answered reported success");
		}
		// shutdown cleanup must not become a stall: a client whose server is already gone,
		// and one whose server is still there, both have to tear down promptly
		TEST_METHOD(ClientShutdownIsPrompt)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			{
				TestServer server{ pipeName, disposeCount };
				auto pClient = ConnectClient_(pipeName, eventCount);
				AssertTeardownIsPrompt_(pClient, L"with a live server");
				WaitForSessionCount_(server, 0);
				Assert::AreEqual(1u, disposeCount.load(), L"Live server did not dispose the torn-down client");
				Assert::AreEqual(TestServer<>::reservedAcceptors, server.GetAcceptorCount());
			}
			AssertPipeAbsent_(pipeName);
			std::unique_ptr<TestClient> pOrphan;
			{
				TestServer server{ pipeName, disposeCount };
				try {
					pOrphan = ConnectClient_(pipeName, eventCount);
				}
				catch (const std::exception& ex) {
					Assert::Fail(util::str::ToWide(std::format(
						"second connect failed sessions={} acceptors={} what={}",
						server.GetSessionCount(), server.GetAcceptorCount(), ex.what())).c_str());
				}
			}
			AssertTeardownIsPrompt_(pOrphan, L"after the server dropped");
		}
		TEST_METHOD(RepeatedShutdownIsIdempotent)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			pClient->BeginShutdown();
			pClient->BeginShutdown();
			pClient->WaitForShutdown();
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(DestructionAfterAsyncShutdownIsPrompt)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			pClient->BeginShutdown();
			const auto start = std::chrono::steady_clock::now();
			pClient.reset();
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"Destroying a client after BeginShutdown stalled");
		}
		TEST_METHOD(ShutdownAfterPeerDisconnectReachesStopped)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			std::unique_ptr<TestClient> pClient;
			{
				TestServer server{ pipeName, disposeCount };
				pClient = ConnectClient_(pipeName, eventCount);
			}
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning());
			pClient->BeginShutdown();
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(DispatchRacingShutdownSettlesOrRejects)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			std::atomic<uint32_t> completed = 0;
			std::atomic<uint32_t> rejected = 0;
			std::vector<std::jthread> threads;
			for (int t = 0; t < 4; t++) {
				threads.emplace_back([&] {
					const auto deadline = std::chrono::steady_clock::now() + 3s;
					while (std::chrono::steady_clock::now() < deadline) {
						try {
							const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 3, .pushCount = 0 });
							if (res.value == 3) {
								completed.fetch_add(1);
							}
						}
						catch (...) {
							rejected.fetch_add(1);
							return;
						}
					}
				});
			}
			std::this_thread::sleep_for(30ms);
			pClient->BeginShutdown();
			threads.clear();
			Assert::IsTrue(rejected.load() > 0, L"Shutdown never rejected or failed a dispatch");
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(ContinuationRunsExactlyOnce)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			std::atomic<int> calls = 0;
			pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 9, .pushCount = 0 },
				[&](EchoWithPush::Response&& res, std::exception_ptr error) {
					calls.fetch_add(1);
					Assert::IsTrue(error == nullptr);
					Assert::AreEqual(9u, res.value);
				});
			for (int i = 0; i < 200 && calls.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			std::this_thread::sleep_for(50ms);
			Assert::AreEqual(1, calls.load());
		}
		TEST_METHOD(DisposeOnceOnDisconnectProtocolFailureAndLocalShutdown)
		{
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::atomic<uint32_t> eventCount = 0;
				TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount, &disposeMutex, &reasons };
				{
					auto pClient = ConnectClient_(pipeName, eventCount);
					WaitForSessionCount_(server, 1);
				}
				WaitForSessionCount_(server, 0);
				Assert::AreEqual(1u, disposeCount.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected);
			}
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::atomic<uint32_t> eventCountA = 0;
				std::atomic<uint32_t> eventCountB = 0;
				TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount, &disposeMutex, &reasons };
				auto pOther = ConnectClient_(pipeName, eventCountB);
				(void)eventCountA;
				std::ostringstream body;
				{
					cereal::BinaryOutputArchive archive{ body };
					PacketHeader header{
						.identifier = "Nope",
						.commandToken = 0xA11F0000u,
						.packetType = PacketType::ActionResponse,
						.headerVersion = kHeaderVersion,
					};
					archive(header, EmptyPayload{});
				}
				const auto payload = body.str();
				const uint32_t payloadSize = (uint32_t)payload.size();
				util::win::Handle raw;
				for (int i = 0; i < 200 && !raw; i++) {
					raw = util::win::Handle{ CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE,
						0, nullptr, OPEN_EXISTING, 0, nullptr) };
					if (!raw) {
						std::this_thread::sleep_for(5ms);
					}
				}
				Assert::IsTrue((bool)raw, L"Could not open a raw session to inject an unknown response");
				DWORD written = 0;
				Assert::IsTrue(WriteFile(raw, &payloadSize, sizeof(payloadSize), &written, nullptr) != FALSE);
				Assert::IsTrue(WriteFile(raw, payload.data(), (DWORD)payload.size(), &written, nullptr) != FALSE);
				for (int i = 0; i < 200 && disposeCount.load() == 0; i++) {
					std::this_thread::sleep_for(10ms);
				}
				Assert::AreEqual(1u, server.GetSessionCount());
				const auto res = pOther->DispatchSync(EchoWithPush::Params{ .value = 4, .pushCount = 0 });
				Assert::AreEqual(4u, res.value);
				Assert::AreEqual(1u, disposeCount.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::ProtocolFailure);
			}
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::atomic<uint32_t> eventCount = 0;
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons };
				auto pClient = ConnectClient_(pipeName, eventCount);
				WaitForSessionCount_(server, 1);
				server.BeginShutdown();
				server.WaitForShutdown();
				Assert::IsTrue(server.GetLifecyclePhase() == LifecyclePhase::Stopped);
				Assert::AreEqual(1u, disposeCount.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::LocalShutdown);
			}
		}
		TEST_METHOD(DispatchAcceptanceIsDecidedWithScheduling)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			AdmissionHookState_ hook;
			hook.client = pClient.get();
			gAdmissionHook_ = &hook;
			pClient->SetAdmissionHookForTest(&AdmissionHookFn_);
			std::atomic<bool> dispatchDone{ false };
			std::jthread dispatcher{ [&] {
				try {
					(void)pClient->DispatchSync(EchoWithPush::Params{ .value = 4, .pushCount = 0 });
				}
				catch (...) {
				}
				dispatchDone.store(true);
			} };
			{
				std::unique_lock lock{ hook.mutex };
				Assert::IsTrue(hook.cv.wait_for(lock, 2s, [&] { return hook.entered; }),
					L"Dispatch never entered the admission gate");
			}
			Assert::IsTrue(hook.sawRunning.load(), L"Admission hook ran after the phase left Running");
			std::jthread shutdown{ [&] { pClient->BeginShutdown(); } };
			std::this_thread::sleep_for(50ms);
			{
				std::lock_guard lock{ hook.mutex };
				hook.release = true;
			}
			hook.cv.notify_all();
			dispatcher.join();
			shutdown.join();
			pClient->SetAdmissionHookForTest(nullptr);
			gAdmissionHook_ = nullptr;
			Assert::IsTrue(dispatchDone.load(), L"Accepted dispatch did not finish");
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
			std::atomic<int> calls = 0;
			Assert::ExpectException<ServerDroppedError>([&] {
				pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 5, .pushCount = 0 },
					[&](EchoWithPush::Response&&, std::exception_ptr) { calls.fetch_add(1); });
			});
			std::this_thread::sleep_for(30ms);
			Assert::AreEqual(0, calls.load(), L"Rejected continuation was invoked");
		}
		TEST_METHOD(ReaderExitAdmissionIsDecidedWithScheduling)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			AdmissionHookState_ hook;
			hook.client = pClient.get();
			gAdmissionHook_ = &hook;
			pClient->SetAdmissionHookForTest(&AdmissionHookFn_);
			std::atomic<bool> dispatchDone{ false };
			std::jthread dispatcher{ [&] {
				try {
					(void)pClient->DispatchSync(EchoWithPush::Params{ .value = 4, .pushCount = 0 });
				}
				catch (...) {
				}
				dispatchDone.store(true);
			} };
			{
				std::unique_lock lock{ hook.mutex };
				Assert::IsTrue(hook.cv.wait_for(lock, 2s, [&] { return hook.entered; }),
					L"Dispatch never entered the admission gate");
			}
			Assert::IsTrue(hook.sawRunning.load(), L"Admission hook ran after the phase left Running");
			server.BeginShutdown();
			for (int i = 0; i < 200 && !pClient->ReaderExitShutdownEntered(); i++) {
				std::this_thread::sleep_for(5ms);
			}
			Assert::IsTrue(pClient->ReaderExitShutdownEntered(), L"Reader exit did not reach shutdown");
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Running,
				L"Reader exit left Running before the admitted dispatch was queued");
			{
				std::lock_guard lock{ hook.mutex };
				hook.release = true;
			}
			hook.cv.notify_all();
			const auto start = std::chrono::steady_clock::now();
			dispatcher.join();
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"Dispatch in the reader-exit gap did not finish");
			Assert::IsTrue(dispatchDone.load(), L"Accepted dispatch did not finish");
			pClient->SetAdmissionHookForTest(nullptr);
			gAdmissionHook_ = nullptr;
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
			std::atomic<int> calls = 0;
			Assert::ExpectException<ServerDroppedError>([&] {
				pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 5, .pushCount = 0 },
					[&](EchoWithPush::Response&&, std::exception_ptr) { calls.fetch_add(1); });
			});
			std::this_thread::sleep_for(30ms);
			Assert::AreEqual(0, calls.load(), L"Rejected continuation was invoked");
		}
		TEST_METHOD(RunnerJoinsBeforeBorrowedDependencyDies)
		{
			auto borrowedAlive = std::make_shared<std::atomic<bool>>(true);
			std::vector<const char*> events;
			std::atomic<uint32_t> disposeCount = 0;
			const auto pipeName = MakeUniquePipeName_();
			{
				struct BorrowMark
				{
					std::shared_ptr<std::atomic<bool>> alive;
					~BorrowMark() { alive->store(false, std::memory_order_release); }
				} borrow{ borrowedAlive };
				{
					TestServer server{ pipeName, disposeCount, nullptr, nullptr, ServerTestHooks{
						.events = &events,
						.borrowedAlive = borrowedAlive,
					} };
					auto pClient = ConnectClient_(pipeName, disposeCount);
					(void)pClient;
				}
				Assert::IsTrue(borrowedAlive->load(), L"Borrowed dependency died before the runner joined");
				bool sawDestroyed = false;
				bool sawDeadTouch = false;
				for (auto event : events) {
					if (std::string_view{ event } == "ExecCtxDestroyed") {
						sawDestroyed = true;
					}
					if (std::string_view{ event } == "ExecCtxTouchedDeadBorrow") {
						sawDeadTouch = true;
					}
				}
				Assert::IsTrue(sawDestroyed, L"Execution context outlived the joined runner");
				Assert::IsFalse(sawDeadTouch, L"Runner still touched a borrowed dependency after it died");
			}
		}
		TEST_METHOD(ContinuationBeginShutdownThenExternalDestroy)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			std::atomic<bool> continued{ false };
			pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 6, .pushCount = 0 },
				[&](EchoWithPush::Response&&, std::exception_ptr) {
					pClient->BeginShutdown();
					continued.store(true);
				});
			for (int i = 0; i < 200 && !continued.load(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsTrue(continued.load(), L"Continuation did not run");
			const auto start = std::chrono::steady_clock::now();
			pClient.reset();
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"External destroy after continuation shutdown did not join");
		}
		TEST_METHOD(FinalTeardownDisposesBeforeTraceStopAndPreservesReasons)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			std::vector<SessionCleanupMode> modes;
			std::atomic<uint32_t> updateTracking = 0;
			std::atomic<uint32_t> startEtw = 0;
			std::vector<const char*> events;
			std::atomic<uint32_t> eventCount = 0;
			auto borrowedAlive = std::make_shared<std::atomic<bool>>(true);
			bool tracesStoppedWhileBorrowed = false;
			{
				auto pServer = std::make_unique<TestServer<>>(pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
					.startEtw = &startEtw,
					.events = &events,
					.borrowedAlive = borrowedAlive,
				});
				auto pClient = ConnectClient_(pipeName, eventCount);
				JoinActionServerThenStopTraces(*pServer, [&] {
					pClient.reset();
					for (int i = 0; i < 200 && reasons.empty(); i++) {
						std::this_thread::sleep_for(10ms);
					}
				}, [&] {
					Assert::IsFalse(pServer->IsRunning());
					pServer.reset();
				}, [&] {
					tracesStoppedWhileBorrowed = borrowedAlive->load(std::memory_order_acquire);
				}, &events);
			}
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected,
				L"Final teardown overwrote a peer disconnect");
			Assert::AreEqual((size_t)1, modes.size());
			Assert::IsTrue(modes[0] == SessionCleanupMode::FinalTeardown);
			Assert::AreEqual(0u, updateTracking.load(), L"UpdateTracking ran during final teardown");
			Assert::AreEqual(0u, startEtw.load(), L"StartEtwSession ran during final teardown");
			Assert::IsTrue(tracesStoppedWhileBorrowed, L"PresentMon stand-in died before trace stop");
			AssertPipeAbsent_(pipeName);
			bool disposed = false;
			bool ctxDestroyed = false;
			for (auto event : events) {
				if (std::string_view{ event } == "Dispose") {
					Assert::IsFalse(ctxDestroyed, L"Session disposal ran after the execution context died");
					disposed = true;
				}
				if (std::string_view{ event } == "ExecCtxDestroyed") {
					ctxDestroyed = true;
				}
			}
			Assert::IsTrue(disposed);
			Assert::IsTrue(ctxDestroyed);
			size_t disposeAt = events.size();
			size_t destroyedAt = events.size();
			size_t stopAt = events.size();
			for (size_t i = 0; i < events.size(); i++) {
				const std::string_view name{ events[i] };
				if (name == "Dispose") {
					disposeAt = i;
				}
				if (name == "ExecCtxDestroyed") {
					destroyedAt = i;
				}
				if (name == "StopTraceSessions") {
					stopAt = i;
				}
			}
			Assert::IsTrue(stopAt < events.size(), L"Teardown did not record StopTraceSessions");
			Assert::IsTrue(disposeAt < stopAt, L"Disposal did not happen before StopTraceSessions");
			Assert::IsTrue(destroyedAt < stopAt, L"Action server was not joined before PresentMon teardown");
		}
		TEST_METHOD(NormalDisconnectStillRecomputes)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			std::vector<SessionCleanupMode> modes;
			std::atomic<uint32_t> updateTracking = 0;
			std::atomic<uint32_t> eventCount = 0;
			{
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
				} };
				auto pClient = ConnectClient_(pipeName, eventCount);
				pClient.reset();
				for (int i = 0; i < 200 && reasons.empty(); i++) {
					std::this_thread::sleep_for(10ms);
				}
			}
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected);
			Assert::IsTrue(modes[0] == SessionCleanupMode::NormalOperation);
			Assert::AreEqual(1u, updateTracking.load(), L"A normal disconnect skipped global recomputation");
		}
		TEST_METHOD(ProtocolFaultAndBodyTimeoutKeepReasonDuringFinalTeardown)
		{
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::vector<SessionCleanupMode> modes;
				std::atomic<uint32_t> updateTracking = 0;
				std::atomic<uint32_t> eventCount = 0;
				ServerExecutionContext::ProtocolGate gate;
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
					.protocolGate = &gate,
				} };
				auto pClient = ConnectClient_(pipeName, eventCount);
				server.EnterFinalTeardown();
				std::atomic<bool> dispatchDone{ false };
				std::jthread dispatcher{ [&] {
					try {
						pClient->DispatchSync(ProtocolFault::Params{});
					}
					catch (...) {
					}
					dispatchDone.store(true);
				} };
				for (int i = 0; i < 200 && !gate.entered.load(); i++) {
					std::this_thread::sleep_for(5ms);
				}
				Assert::IsTrue(gate.entered.load(), L"Protocol fault action did not reach the gate");
				server.BeginShutdown();
				gate.release.store(true, std::memory_order_release);
				dispatcher.join();
				server.WaitForShutdown();
				Assert::IsTrue(dispatchDone.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::ProtocolFailure,
					L"Final teardown replaced a protocol failure");
				Assert::IsTrue(modes[0] == SessionCleanupMode::FinalTeardown);
				Assert::AreEqual(0u, updateTracking.load());
			}
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::vector<SessionCleanupMode> modes;
				std::atomic<uint32_t> updateTracking = 0;
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
					.bodyReadTimeoutMs = 50,
				} };
				server.EnterFinalTeardown();
				Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
				util::win::Handle raw;
				for (int i = 0; i < 200 && !raw; i++) {
					raw = util::win::Handle{ CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE,
						0, nullptr, OPEN_EXISTING, 0, nullptr) };
					if (!raw) {
						std::this_thread::sleep_for(5ms);
					}
				}
				Assert::IsTrue((bool)raw, L"Could not open a raw session for the body-read timeout");
				WaitForSessionCount_(server, 1);
				const uint32_t declared = 4096;
				DWORD written = 0;
				Assert::IsTrue(WriteFile(raw, &declared, sizeof(declared), &written, nullptr) != FALSE);
				for (int i = 0; i < 200 && reasons.empty(); i++) {
					std::this_thread::sleep_for(10ms);
				}
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::PeerStalled,
					L"Body-read timeout was relabeled");
				Assert::IsTrue(modes[0] == SessionCleanupMode::FinalTeardown);
				Assert::AreEqual(0u, updateTracking.load());
				server.BeginShutdown();
				server.WaitForShutdown();
				Assert::AreEqual((size_t)1, reasons.size(), L"Shutdown added a second disposal");
				Assert::IsTrue(reasons[0] == SessionEndReason::PeerStalled);
			}
		}
		TEST_METHOD(NoActionAdmittedAfterFinalTeardownStopping)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			server.EnterFinalTeardown();
			server.BeginShutdown();
			Assert::IsFalse(server.IsRunning());
			const auto start = std::chrono::steady_clock::now();
			server.DispatchDetached(PushNotice::Params{ .value = 1 });
			try {
				pClient->DispatchSync(EchoWithPush::Params{ .value = 1, .pushCount = 0 });
			}
			catch (...) {
			}
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"A dispatch was admitted or blocked after Stopping");
			server.WaitForShutdown();
			Assert::AreEqual(0u, server.GetSessionCount());
		}
	};
}
