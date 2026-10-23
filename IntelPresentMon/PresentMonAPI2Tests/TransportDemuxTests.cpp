// Copyright (C) 2022-2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Request, response and push demultiplexing on one handle, and command tokens.
#include "TransportTestKit.h"

namespace TransportDemuxTests
{
	TEST_CLASS(DemuxTests)
	{
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			RegisterActions_();
		}
		// Walkthrough: one client, one request, no pushes
		TEST_METHOD(WalkthroughOneRequest)
		{
            const auto pipeName = MakeUniquePipeName_();
            std::atomic<uint32_t> disposeCount = 0;
            std::atomic<uint32_t> eventCount = 0;
            TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
            // a 10-minute response timeout so pausing in the debugger does not trigger a timeout
            const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 42, .pushCount = 0 }, 600000);
            Assert::AreEqual(42u, res.value);
		}
		// the framing floor must match what the protocol actually serializes, otherwise
		// the pipe layer rejects legitimate minimal packets or lets junk through
		TEST_METHOD(MinimalPacketBodySizeMatchesProtocolFloor)
		{
			std::ostringstream body;
			{
				cereal::BinaryOutputArchive archive{ body };
				archive(PacketHeader{}, EmptyPayload{});
			}
			Assert::AreEqual((size_t)kMinPacketBodyBytes, body.str().size(),
				L"kMinPacketBodyBytes no longer matches the serialized size of an empty header");
		}
		// the whole point of the demux: an event pushed while a request is in flight must
		// not be mistaken for that request's response
		TEST_METHOD(PushDuringRequestResolvesCorrectResponse)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			constexpr uint32_t requestCount = 200;
			constexpr uint32_t pushesPerRequest = 3;
			for (uint32_t i = 0; i < requestCount; i++) {
				const auto res = pClient->DispatchSync(EchoWithPush::Params{
					.value = i, .pushCount = pushesPerRequest });
				Assert::AreEqual(i, res.value, L"Response did not match its own request");
				Assert::AreEqual((uint32_t)GetCurrentProcessId(), res.remotePid);
			}
			// pushes are detached, so allow them to drain before counting
			for (int i = 0; i < 100 && eventCount.load() < requestCount * pushesPerRequest; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(requestCount * pushesPerRequest, eventCount.load(),
				L"Pushed events were lost while requests were in flight");
		}
		// Every packet body read races a timer. When the read wins, the cancelled timer
		// must finish without throwing, so healthy traffic raises no C++ exceptions at all.
		TEST_METHOD(HealthyTrafficThrowsNoExceptions)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			constexpr uint32_t requestCount = 50;
			uint32_t throws = 0;
			{
				CxxThrowCounter_ counter;
				for (uint32_t i = 0; i < requestCount; i++) {
					const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = i, .pushCount = 1 });
					Assert::AreEqual(i, res.value);
				}
				for (int i = 0; i < 100 && eventCount.load() < requestCount; i++) {
					std::this_thread::sleep_for(10ms);
				}
				throws = counter.Count();
			}
			Assert::AreEqual(requestCount, eventCount.load(), L"Pushed events were lost");
			Assert::AreEqual(0u, throws, L"Healthy request, response and event traffic raised C++ exceptions");
		}
		// concurrent callers on one session share a reader loop; each must get its own answer
		TEST_METHOD(ConcurrentRequestsOnOneSessionDoNotCrossTalk)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			constexpr uint32_t threadCount = 4;
			constexpr uint32_t perThread = 50;
			std::atomic<uint32_t> mismatches = 0;
			std::vector<std::jthread> threads;
			for (uint32_t t = 0; t < threadCount; t++) {
				threads.emplace_back([&, t] {
					for (uint32_t i = 0; i < perThread; i++) {
						const auto value = t * 1000 + i;
						const auto res = pClient->DispatchSync(EchoWithPush::Params{
							.value = value, .pushCount = 1 });
						if (res.value != value) {
							mismatches.fetch_add(1);
						}
					}
				});
			}
			threads.clear();
			Assert::AreEqual(0u, mismatches.load(), L"A response was delivered to the wrong caller");
		}
		// two sessions from one process exercise the per-session pending-response maps,
		// where the two token counters are free to collide in value
		TEST_METHOD(MultipleSessionsFromOneProcess)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCountA = 0;
			std::atomic<uint32_t> eventCountB = 0;
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };
			auto pClientA = ConnectClient_(pipeName, eventCountA);
			auto pClientB = ConnectClient_(pipeName, eventCountB);
			Assert::AreEqual(2u, server.GetSessionCount());

			for (uint32_t i = 0; i < 50; i++) {
				const auto resA = pClientA->DispatchSync(EchoWithPush::Params{ .value = i, .pushCount = 0 });
				const auto resB = pClientB->DispatchSync(EchoWithPush::Params{ .value = i + 500, .pushCount = 0 });
				Assert::AreEqual(i, resA.value);
				Assert::AreEqual(i + 500, resB.value);
			}
		}
		TEST_METHOD(DuplicateResponseAfterLateResponseKillsSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons };
			auto pClient = ConnectClient_(pipeName, eventCount);
			Assert::ExpectException<ResponseTimeout>([&] {
				pClient->DispatchSync(LateDouble::Params{ .value = 5 }, 40);
			});
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning(), L"A second response for a consumed expired token left the session up");
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(1u, disposeCount.load());
		}
		TEST_METHOD(ExpirationEvictionTreatsForgottenTokenAsUnknown)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			for (size_t i = 0; i < kMaxExpiredResponseTokens + 1; i++) {
				Assert::ExpectException<ResponseTimeout>([&] {
					pClient->DispatchSync(DropRequest::Params{}, 30);
				});
			}
			const auto kept = pClient->DispatchSync(SpoofRequest::Params{ .token = 2 });
			Assert::AreEqual(2u, kept.token);
			Assert::IsTrue(pClient->IsRunning());
			try {
				pClient->DispatchSync(SpoofRequest::Params{ .token = 1 });
			}
			catch (...) {
			}
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning(), L"A response for an evicted token did not end the session");
		}
		TEST_METHOD(CommandTokenAllocatorDefaultStartsAtZero)
		{
			CommandTokenAllocator allocator;
			const auto unavailable = [](uint32_t) { return false; };
			Assert::AreEqual(0u, allocator.Allocate(unavailable));
			Assert::AreEqual(1u, allocator.Allocate(unavailable));
		}
		TEST_METHOD(CommandTokenAllocatorSkipsUnavailableNearWrap)
		{
			CommandTokenAllocator allocator{ 0xFFFFFFFFu };
			const std::unordered_set<uint32_t> pending{ 0 };
			const std::unordered_set<uint32_t> expired{ 1 };
			const auto unavailable = [&](uint32_t token) {
				return pending.contains(token) || expired.contains(token);
			};
			Assert::AreEqual(0xFFFFFFFFu, allocator.Allocate(unavailable));
			Assert::AreEqual(2u, allocator.Allocate(unavailable));
		}
		TEST_METHOD(LiveTokenAllocationSkipsPendingAndExpiredOnWrap)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			class ProbeClient : public SymmetricActionClient<ClientExecutionContext>
			{
			public:
				ProbeClient(const std::string& pipe, std::atomic<uint32_t>& events, CommandTokenAllocator tokens)
					:
					SymmetricActionClient{ pipe, ClientExecutionContext{ .pEventCount = &events }, std::move(tokens) }
				{
					const auto res = DispatchSync(OpenSession::Params{ .clientPid = GetCurrentProcessId() });
					EstablishSession_(res.serverPid);
				}
				void Probe(std::atomic<uint32_t>& skippedPending, std::atomic<uint32_t>& skippedExpired, std::atomic<int>& done)
				{
					DispatchWithContinuation(EchoWithPush::Params{ .value = 1, .pushCount = 0 },
						[this, &skippedPending, &skippedExpired, &done](EchoWithPush::Response&&, std::exception_ptr error) {
							if (!error) {
								struct FakePending : PendingResponse
								{
									void OnComplete_(const PacketHeader&, util::pipe::DuplexPipe&) override {}
									void OnFail_(std::exception_ptr) override {}
								} fake;
								SessionConnector_().ExpirePending(0);
								skippedExpired = SessionConnector_().AllocateCommandToken();
								SessionConnector_().RegisterPending(2, fake);
								skippedPending = SessionConnector_().AllocateCommandToken();
								SessionConnector_().ExpirePending(2);
							}
							done.fetch_add(1);
						});
				}
			};
			ProbeClient client{ pipeName, eventCount, CommandTokenAllocator{ 0xFFFFFFFEu } };
			std::atomic<uint32_t> skippedPending = 0;
			std::atomic<uint32_t> skippedExpired = 0;
			std::atomic<int> done = 0;
			client.Probe(skippedPending, skippedExpired, done);
			for (int i = 0; i < 200 && done.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1, done.load());
			Assert::AreEqual(1u, skippedExpired.load(), L"Wrap landed on a retained expiration");
			Assert::AreEqual(3u, skippedPending.load(), L"Wrap landed on a pending token");
		}
		TEST_METHOD(LiveRequestsWrapCommandTokens)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			class WrapClient : public SymmetricActionClient<ClientExecutionContext>
			{
			public:
				WrapClient(const std::string& pipe, std::atomic<uint32_t>& events)
					:
					SymmetricActionClient{ pipe, ClientExecutionContext{ .pEventCount = &events },
						CommandTokenAllocator{ 0xFFFFFFFEu } }
				{
					const auto res = DispatchSync(OpenSession::Params{ .clientPid = GetCurrentProcessId() });
					EstablishSession_(res.serverPid);
				}
			};
			WrapClient client{ pipeName, eventCount };
			const auto first = client.DispatchSync(EchoToken::Params{ .value = 10 });
			const auto second = client.DispatchSync(EchoToken::Params{ .value = 11 });
			const auto third = client.DispatchSync(EchoToken::Params{ .value = 12 });
			Assert::AreEqual(10u, first.value);
			Assert::AreEqual(0xFFFFFFFFu, first.token);
			Assert::AreEqual(11u, second.value);
			Assert::AreEqual(0u, second.token);
			Assert::AreEqual(12u, third.value);
			Assert::AreEqual(1u, third.token);
		}
		// remotePid is only diagnostic: a session opened under another pid is reported in
		// the log but still served
		TEST_METHOD(OpenSessionUnderAnotherPidIsStillServed)
		{
			AssertOpenSessionUnderPidIsServed_<RequestResponseServerPolicy>(GetCurrentProcessId() + 4);
			AssertOpenSessionUnderPidIsServed_<SinglePeerServerPolicy>(GetCurrentProcessId() + 4);
		}
	private:
		template<class Policy>
		static void AssertOpenSessionUnderPidIsServed_(uint32_t claimedPid)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer<Policy> server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000),
				L"Timed out waiting for the test transport pipe");
			TestClient client{ pipeName, eventCount, claimedPid };
			const auto res = client.DispatchSync(EchoWithPush::Params{ .value = 14, .pushCount = 0 });
			Assert::AreEqual(14u, res.value, L"A session opened under another pid was not served");
			Assert::AreEqual(claimedPid, res.remotePid, L"The session did not keep the pid it claimed");
			Assert::AreEqual(0u, disposeCount.load(), L"A session opened under another pid was ended");
		}
	};
}
