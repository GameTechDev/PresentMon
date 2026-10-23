// Copyright (C) 2022-2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Deadlines on sending and on waiting for responses: a peer that stops reading or answering
// cannot hold a request past its timeout.
#include "TransportTestKit.h"

namespace TransportDemuxTests
{
	TEST_CLASS(WriteDeadlineTests)
	{
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			RegisterActions_();
		}
		// a responder that goes quiet must cost the requester a bounded wait, and only
		// that request: the session itself stays usable
		TEST_METHOD(ResponseTimeoutFailsOnlyTheRequest)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			Assert::ExpectException<ResponseTimeout>([&] {
				pClient->DispatchSync(StallForClient::Params{ .durationMs = 1000 }, 100);
			}, L"A request that was never answered did not time out");
			// the abandoned response arrives late and is dropped, leaving the stream usable
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 11, .pushCount = 0 });
			Assert::AreEqual(11u, res.value, L"A timed out request poisoned its session");
			Assert::AreEqual(1u, server.GetSessionCount(), L"A timed out request dropped its session");
		}
		// A relaunched UI asks for a new pair, possibly before the old pipe's close was seen.
		// A server that stops reading cannot hold a request past its timeout: sending shares the
		// request's deadline, the session then ends, and a request queued behind the stalled
		// write fails promptly instead of waiting for it.
		TEST_METHOD(StalledPeerBoundsRequestSend)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			// the server's reader awaits this stall, so it reads nothing for 2 s
			std::jthread staller{ [&] {
				try {
					pClient->DispatchSync(StallForClient::Params{ .durationMs = 2000 }, 5000u);
				}
				catch (...) {
				}
			} };
			std::this_thread::sleep_for(100ms);
			std::atomic<int> finished = 0;
			auto send = [&](auto params) {
				try {
					pClient->DispatchSync(std::move(params), 50u);
				}
				catch (...) {
				}
				finished.fetch_add(1);
			};
			const auto start = std::chrono::steady_clock::now();
			{
				std::jthread bulk{ [&] { send(BulkRequest::Params{ .bytes = std::string(512 * 1024, 'x') }); } };
				std::this_thread::sleep_for(10ms);
				// waits for the write lock the bulk request holds
				std::jthread queued{ [&] { send(EchoWithPush::Params{ .value = 1, .pushCount = 0 }); } };
				for (int i = 0; i < 300 && finished.load() < 2; i++) {
					std::this_thread::sleep_for(10ms);
				}
			}
			const auto elapsed = std::chrono::steady_clock::now() - start;
			// without a send deadline the bulk write would last until the stall ends
			Assert::IsTrue(elapsed < 1000ms, L"A request to a peer that stopped reading outlived its timeout");
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning(), L"The session survived a write that timed out");
			// the stall waits on a timer that closing the session cannot interrupt, so let it end
			// before the server is destroyed; otherwise its shutdown would exceed its timeout
			WaitForSessionCount_(server, 0);
		}
		// A request queued behind a stalled write keeps its own deadline: a 50 ms request behind
		// a 1,000 ms one fails at about 50 ms, not when the other gives up. Nothing of it was
		// sent, so it does not end the session; the stalled writer does, at its own deadline.
		TEST_METHOD(QueuedRequestKeepsItsOwnDeadline)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			// the server's reader awaits this stall, so it reads nothing for 2.5 s
			std::jthread staller{ [&] {
				try {
					pClient->DispatchSync(StallForClient::Params{ .durationMs = 2500 }, 5000u);
				}
				catch (...) {
				}
			} };
			std::this_thread::sleep_for(100ms);
			std::atomic<bool> bulkDone = false;
			std::jthread bulk{ [&] {
				try {
					pClient->DispatchSync(BulkRequest::Params{ .bytes = std::string(512 * 1024, 'x') }, 1000u);
				}
				catch (...) {
				}
				bulkDone.store(true);
			} };
			std::this_thread::sleep_for(20ms);
			// waits for the write lock the bulk request holds
			const auto start = std::chrono::steady_clock::now();
			try {
				pClient->DispatchSync(EchoWithPush::Params{ .value = 1, .pushCount = 0 }, 50u);
			}
			catch (...) {
			}
			const auto queuedElapsed = std::chrono::steady_clock::now() - start;
			const bool sessionUpAfterQueuedTimeout = pClient->IsRunning() && !bulkDone.load();
			bulk.join();
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			const bool sessionEndedByStall = !pClient->IsRunning();
			// the stall waits on a timer that closing the session cannot interrupt, so let it end
			// before the server is destroyed; otherwise its shutdown would exceed its timeout
			WaitForSessionCount_(server, 0);
			Assert::IsTrue(queuedElapsed < 300ms, L"A queued request waited past its own deadline for the stalled write");
			Assert::IsTrue(sessionUpAfterQueuedTimeout, L"A queued request's timeout ended the session");
			Assert::IsTrue(sessionEndedByStall, L"The stalled write did not end the session at its own deadline");
		}
		// A response whose deadline passes while it waits for the write lock has sent nothing,
		// so only that response is dropped: the requester times out on its own and the session
		// stays up for the requests after it.
		TEST_METHOD(QueuedResponseTimeoutKeepsSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount, nullptr, nullptr,
				ServerTestHooks{ .responseWriteTimeoutMs = 100u } };
			auto pClient = ConnectClient_(pipeName, eventCount);
			// the client's reader awaits this stall, so it reads nothing for 1.5 s
			server.DispatchDetached(StallReaderNotice::Params{ .durationMs = 1500 });
			// this push then holds the server's write lock until the stall ends
			server.DispatchDetached(BulkNotice::Params{ .bytes = std::string(512 * 1024, 'x') });
			std::this_thread::sleep_for(100ms);
			// the response waits for that lock past its 100 ms deadline
			Assert::ExpectException<ResponseTimeout>([&] {
				pClient->DispatchSync(EchoWithPush::Params{ .value = 1, .pushCount = 0 }, 500u);
			}, L"A request whose response was dropped did not time out");
			// the server's view: the client cannot see a session end while its reader is stalled
			Assert::AreEqual(1u, server.GetSessionCount(), L"A response that missed its deadline in the write queue ended the session");
			// once the client has read the push, the lock is free again
			for (int i = 0; i < 500 && eventCount.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1u, eventCount.load(), L"The client never received the push");
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 2, .pushCount = 0 }, 5000u);
			Assert::AreEqual(2u, res.value, L"The session did not serve a request after a dropped response");
			Assert::AreEqual(1u, server.GetSessionCount(), L"A dropped response ended its session");
		}
	};
}
