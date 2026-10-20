// Copyright (C) 2022-2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Listeners and session admission: acceptor churn, single-peer and addressed sends, the handshake
// deadline, the session cap, and refusing actions before OpenSession.
#include "TransportTestKit.h"

namespace TransportDemuxTests
{
	TEST_CLASS(SessionTests)
	{
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			RegisterActions_();
		}
		// an accept immediately yields a session and a replacement acceptor, so churn
		// cannot erode the pool the way unpaired half-connections used to
		TEST_METHOD(AcceptorPoolSurvivesConnectChurn)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			// Multi-peer: a closed client's session is removed on the server's thread, so it can
			// still be counted when the next client connects, and a single-peer server would
			// rightly refuse that client.
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };

			for (int i = 0; i < 25; i++) {
				auto pClient = ConnectClient_(pipeName, eventCount);
				const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 7, .pushCount = 0 });
				Assert::AreEqual(7u, res.value);
			}
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(0u, server.GetSessionCount());
			Assert::AreEqual(25u, disposeCount.load(), L"A disposed session was not cleaned up");
		}
		// The listener count can be observed directly: every client that connects and
		// vanishes costs one pipe instance, and every one of them must be replaced or
		// the server eventually has nothing left to accept on
		TEST_METHOD(ConnectAndCloseChurnKeepsListenerReserve)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			// Multi-peer: a vanished client can still be in the map for a moment, and
			// that must not keep the next real client from being admitted.
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };
			WaitForAcceptorCount_(server, TestServer<>::reservedAcceptors);

			for (int i = 0; i < 50; i++) {
				RawConnectAndClose_(pipeName);
				// a replacement is counted before the listener it replaces is released, so
				// the pool can read one high for an instant but must never read low
				Assert::IsTrue(server.GetAcceptorCount() >= TestServer<>::reservedAcceptors,
					L"A vanishing client consumed a listener from the pool");
			}
			WaitForAcceptorCount_(server, TestServer<>::reservedAcceptors);
			Assert::AreEqual(TestServer<>::reservedAcceptors, server.GetAcceptorCount(),
				L"Listener pool did not settle back to its reserve");
			// the pool is not just counted correctly, it still admits a real client
			auto pClient = ConnectClient_(pipeName, eventCount);
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 7, .pushCount = 0 });
			Assert::AreEqual(7u, res.value);
		}
		// the suffixed names are the shape of the reported defect; they must not come back
		TEST_METHOD(HalfPipeNamesAreNotExpressible)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			TestServer server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			AssertPipeAbsent_(pipeName + "-in");
			AssertPipeAbsent_(pipeName + "-out");
		}
		// regression for the dispose guard: cleanup used to be skipped entirely when a
		// session never reported a remote pid
		TEST_METHOD(DisposeRunsForSessionThatNeverOpened)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			{
				Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
				SymmetricActionClient<ClientExecutionContext> bareClient{
					pipeName, ClientExecutionContext{ .pEventCount = &eventCount } };
				WaitForSessionCount_(server, 1);
			}
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(1u, disposeCount.load(),
				L"Dispose did not run for a session that never sent OpenSession");
		}
		// The server runs nothing for a peer that has not opened its session: the action is
		// refused before it executes and the session ends as a protocol failure.
		TEST_METHOD(ActionBeforeOpenSessionEndsSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			SymmetricActionClient<ClientExecutionContext> bareClient{
				pipeName, ClientExecutionContext{ .pEventCount = &eventCount } };
			bool refused = false;
			try {
				// if this ran, its push would reach the client before the response
				bareClient.DispatchSync(EchoWithPush::Params{ .value = 7, .pushCount = 1 });
			}
			catch (...) {
				refused = true;
			}
			Assert::IsTrue(refused, L"An action sent before OpenSession was served");
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(1u, disposeCount.load(), L"The refused session was not disposed");
			Assert::AreEqual(0u, eventCount.load(), L"The refused action executed");
			std::lock_guard lock{ disposeMutex };
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::ProtocolFailure,
				L"An action before OpenSession did not end the session as a protocol failure");
		}
		// The client does not require an opened session: a server may push an event before
		// the client has recorded the OpenSession response, and that is legitimate.
		TEST_METHOD(EventBeforeEstablishSessionIsAccepted)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			SymmetricActionClient<ClientExecutionContext> bareClient{
				pipeName, ClientExecutionContext{ .pEventCount = &eventCount } };
			WaitForSessionCount_(server, 1);
			server.DispatchDetached(PushNotice::Params{ .value = 3 });
			for (int i = 0; i < 100 && eventCount.load() < 1; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1u, eventCount.load(), L"An event before EstablishSession_ was not delivered");
			Assert::IsTrue(bareClient.IsRunning(), L"An event before EstablishSession_ ended the client");
			Assert::AreEqual(1u, server.GetSessionCount(), L"An event before EstablishSession_ ended the session");
		}
		TEST_METHOD(SinglePeerRejectsSecondSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			bool refused = false;
			try {
				std::atomic<uint32_t> extraEvents = 0;
				auto pExtra = ConnectClient_(pipeName, extraEvents);
				for (int i = 0; i < 200 && pExtra->IsRunning(); i++) {
					std::this_thread::sleep_for(10ms);
				}
				refused = !pExtra->IsRunning();
			}
			catch (...) {
				refused = true;
			}
			Assert::IsTrue(refused, L"Single-peer server accepted a second session");
			Assert::AreEqual(1u, server.GetSessionCount());
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 6, .pushCount = 0 });
			Assert::AreEqual(6u, res.value);
		}
		TEST_METHOD(AddressedSendRequiresExplicitSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCountA = 0;
			std::atomic<uint32_t> eventCountB = 0;
			TestServer<AddressedMultiPeerServerPolicy> server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			TestClient clientA{ pipeName, eventCountA };
			TestClient clientB{ pipeName, eventCountB };
			const auto ids = server.CopySessionIds();
			Assert::AreEqual((size_t)2, ids.size());
			Assert::ExpectException<util::Exception>([&] {
				server.DispatchToSession(0xFFFFFFFFu, PushNotice::Params{ .value = 1 });
			});
			server.DispatchToSession(ids[0], PushNotice::Params{ .value = 2 });
			for (int i = 0; i < 200 && eventCountA.load() + eventCountB.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			std::this_thread::sleep_for(50ms);
			Assert::AreEqual(1u, eventCountA.load() + eventCountB.load());
			Assert::IsTrue(eventCountA.load() != eventCountB.load());
		}
		// A disposed session leaves both the session map and the connector map, so a send
		// addressed to it is refused while the surviving session is still reachable.
		TEST_METHOD(AddressedSendToDisposedSessionIsRefused)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCountA = 0;
			std::atomic<uint32_t> eventCountB = 0;
			TestServer<AddressedMultiPeerServerPolicy> server{ pipeName, disposeCount };
			auto pClientA = ConnectClient_(pipeName, eventCountA);
			const auto idsA = server.CopySessionIds();
			Assert::AreEqual((size_t)1, idsA.size());
			const auto idA = idsA.front();
			auto pClientB = ConnectClient_(pipeName, eventCountB);
			const auto idsAB = server.CopySessionIds();
			Assert::AreEqual((size_t)2, idsAB.size());
			const auto idB = idsAB[0] == idA ? idsAB[1] : idsAB[0];

			pClientA.reset();
			WaitForSessionCount_(server, 1);
			Assert::AreEqual(1u, server.GetSessionCount());
			Assert::AreEqual(1u, disposeCount.load());
			Assert::ExpectException<util::Exception>([&] {
				server.DispatchToSession(idA, PushNotice::Params{ .value = 1 });
			});

			server.DispatchToSession(idB, PushNotice::Params{ .value = 2 });
			for (int i = 0; i < 200 && eventCountB.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1u, eventCountB.load(), L"Surviving session did not receive its send");
			Assert::AreEqual(0u, eventCountA.load());
		}
		// After the only peer leaves, an unaddressed send must reach the next peer.
		TEST_METHOD(SinglePeerSendReachesReplacementPeer)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> firstEvents = 0;
			std::atomic<uint32_t> secondEvents = 0;
			TestServer server{ pipeName, disposeCount };
			ConnectClient_(pipeName, firstEvents).reset();
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(0u, server.GetSessionCount());

			auto pSecond = ConnectClient_(pipeName, secondEvents);
			Assert::AreEqual(1u, server.GetSessionCount());
			server.DispatchDetached(PushNotice::Params{ .value = 3 });
			for (int i = 0; i < 200 && secondEvents.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1u, secondEvents.load(), L"Send did not reach the replacement peer");
			Assert::AreEqual(0u, firstEvents.load());
		}
		// a connection that never opens its session is closed at the deadline, while an
		// opened session may stay idle past it
		TEST_METHOD(UnopenedSessionClosedAtHandshakeDeadline)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			std::atomic<uint32_t> eventCount = 0;
			constexpr auto deadline = 200ms;
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
				.limits = { .handshakeTimeoutMs = (uint32_t)deadline.count() },
			} };
			auto pClient = ConnectClient_(pipeName, eventCount);
			RawPipeClient raw{ pipeName };
			Assert::IsTrue(raw.IsConnected());
			const auto start = std::chrono::steady_clock::now();
			Assert::IsTrue(raw.WaitForServerClose(2s), L"An unopened session outlived the handshake deadline");
			Assert::IsTrue(std::chrono::steady_clock::now() - start >= deadline / 2,
				L"An unopened session was closed long before the handshake deadline");
			for (int i = 0; i < 200 && disposeCount.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			{
				std::lock_guard lock{ disposeMutex };
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::PeerStalled);
			}
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 10, .pushCount = 0 });
			Assert::AreEqual(10u, res.value, L"An opened session was closed by the handshake deadline");
			Assert::AreEqual(1u, server.GetSessionCount());
		}
		// the cap refuses sessions beyond it, and admits again once one ends
		TEST_METHOD(SessionCapRefusesUntilASessionEnds)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			constexpr uint32_t cap = 3;
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount, nullptr, nullptr, ServerTestHooks{
				.limits = { .maxSessions = cap },
			} };
			std::vector<std::unique_ptr<TestClient>> clients;
			for (uint32_t i = 0; i < cap; i++) {
				clients.push_back(ConnectClient_(pipeName, eventCount));
			}
			Assert::IsTrue(ConnectionIsRefused_(server, pipeName), L"A session beyond the cap was admitted");
			clients.front().reset();
			WaitForSessionCount_(server, cap - 1);
			auto pLate = ConnectClient_(pipeName, eventCount);
			const auto res = pLate->DispatchSync(EchoWithPush::Params{ .value = 12, .pushCount = 0 });
			Assert::AreEqual(12u, res.value);
		}
	};
}
