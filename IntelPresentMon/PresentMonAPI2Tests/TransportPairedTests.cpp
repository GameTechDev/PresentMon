// Copyright (C) 2022-2026 Intel Corporation
// SPDX-License-Identifier: MIT
// The paired server: no listeners, each session is a pipe pair the server creates and hands one
// end of to a process it launches.
#include "TransportTestKit.h"

namespace TransportDemuxTests
{
	TEST_CLASS(PairedTests)
	{
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			RegisterActions_();
		}
		// --- paired server: no listeners, each session is a pipe pair it creates ---
		TEST_METHOD(PairedSessionServesRequestsAndEvents)
		{
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			PairedTestServer server{ disposeCount };
			PairedTestClient client{ server.OpenPairedSession(), eventCount };
			const auto res = client.DispatchSync(EchoWithPush::Params{ .value = 5, .pushCount = 1 });
			Assert::AreEqual(5u, res.value, L"A request over a pipe pair was not answered");
			for (int i = 0; i < 100 && eventCount.load() < 1; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1u, eventCount.load(), L"An event over a pipe pair was not delivered");
			Assert::AreEqual(1u, server.GetSessionCount());
		}
		TEST_METHOD(PairedActionBeforeOpenSessionEndsSession)
		{
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			PairedTestServer server{ disposeCount, &disposeMutex, &reasons };
			SymmetricActionClient<ClientExecutionContext> bareClient{ server.OpenPairedSession(), "pair-test",
				ClientExecutionContext{ .pEventCount = &eventCount } };
			bool refused = false;
			try {
				bareClient.DispatchSync(EchoWithPush::Params{ .value = 7, .pushCount = 1 });
			}
			catch (...) {
				refused = true;
			}
			Assert::IsTrue(refused, L"An action sent before OpenSession on a pair was served");
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(0u, eventCount.load(), L"The refused action executed");
			std::lock_guard lock{ disposeMutex };
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::ProtocolFailure);
		}
		// This is how the kernel sees its UI exit.
		TEST_METHOD(PairedClientDisconnectEndsSession)
		{
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			PairedTestServer server{ disposeCount, &disposeMutex, &reasons };
			auto pClient = std::make_unique<PairedTestClient>(server.OpenPairedSession(), eventCount);
			Assert::AreEqual(1u, server.GetSessionCount());
			pClient.reset();
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(0u, server.GetSessionCount(), L"Closing the client did not end the session");
			std::lock_guard lock{ disposeMutex };
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected);
		}
		// Two opens at once: neither may wait for the session the other one creates to close.
		// Whether the calls actually overlap depends on scheduling, so this guards the fix
		// without catching a regression on every run; it never fails spuriously.
		TEST_METHOD(ConcurrentPairOpensBothReturn)
		{
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			PairedTestServer server{ disposeCount };
			// a connected session makes both opens wait for it to drain
			PairedTestClient first{ server.OpenPairedSession(), eventCount };
			util::win::Handle clientA;
			util::win::Handle clientB;
			std::atomic<int> returned = 0;
			// a call that throws still counts as returned; the handle check below catches it
			auto open = [&](util::win::Handle& out) {
				try {
					out = server.OpenPairedSession();
				}
				catch (...) {
				}
				returned.fetch_add(1);
			};
			{
				std::jthread openA{ [&] { open(clientA); } };
				std::jthread openB{ [&] { open(clientB); } };
				for (int i = 0; i < 500 && returned.load() < 2; i++) {
					std::this_thread::sleep_for(10ms);
				}
				if (returned.load() < 2) {
					// unblock the waiting call so the threads can be joined
					server.BeginShutdown();
				}
			}
			Assert::AreEqual(2, returned.load(), L"A concurrent OpenPairedSession waited for the other's new session to close");
			Assert::IsTrue((bool)clientA && (bool)clientB, L"A concurrent OpenPairedSession failed");
			WaitForSessionCount_(server, 1);
			Assert::AreEqual(1u, server.GetSessionCount(), L"Concurrent opens left other than one session");
		}
		TEST_METHOD(NewPairEndsPreviousSession)
		{
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			PairedTestServer server{ disposeCount };
			PairedTestClient first{ server.OpenPairedSession(), eventCount };
			PairedTestClient second{ server.OpenPairedSession(), eventCount };
			Assert::AreEqual(1u, disposeCount.load(), L"A new pair did not end the previous session");
			Assert::AreEqual(1u, server.GetSessionCount());
			const auto res = second.DispatchSync(EchoWithPush::Params{ .value = 9, .pushCount = 0 });
			Assert::AreEqual(9u, res.value, L"The new pair was not served");
			bool firstRefused = false;
			try {
				first.DispatchSync(EchoWithPush::Params{ .value = 1, .pushCount = 0 });
			}
			catch (...) {
				firstRefused = true;
			}
			Assert::IsTrue(firstRefused, L"The replaced session was still served");
		}
		TEST_METHOD(PairedServerShutsDownIdleAndWithSession)
		{
			{
				std::atomic<uint32_t> disposeCount = 0;
				PairedTestServer server{ disposeCount };
				// with no listener and no session, only the idle guard keeps the server running
				std::this_thread::sleep_for(100ms);
				Assert::IsTrue(server.IsRunning(), L"An idle paired server stopped by itself");
				server.BeginShutdown();
				Assert::IsTrue(server.WaitForShutdown(), L"An idle paired server did not shut down");
			}
			{
				std::atomic<uint32_t> disposeCount = 0;
				std::atomic<uint32_t> eventCount = 0;
				PairedTestServer server{ disposeCount };
				PairedTestClient client{ server.OpenPairedSession(), eventCount };
				server.BeginShutdown();
				Assert::IsTrue(server.WaitForShutdown(), L"A paired server with a session did not shut down");
				Assert::AreEqual(1u, disposeCount.load());
			}
		}
		// The kernel hands the client end to the UI it launches: only the listed handle is
		// inherited, and the session lives exactly as long as the child holding it.
		TEST_METHOD(PairedHandleInheritedByChildKeepsSession)
		{
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			PairedTestServer server{ disposeCount, &disposeMutex, &reasons };
			auto clientEnd = server.OpenPairedSession();
			Assert::IsTrue((bool)SetHandleInformation(clientEnd, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT));
			wchar_t systemDir[MAX_PATH]{};
			Assert::IsTrue(GetSystemDirectoryW(systemDir, MAX_PATH) > 0);
			boost::asio::io_context procCtx;
			boost::process::v2::windows::default_launcher launcher;
			launcher.inherited_handles.push_back(clientEnd.Get());
			// the child does nothing with the pipe; it only holds the handle for about two seconds
			auto child = launcher(procCtx, std::filesystem::path{ systemDir } / L"cmd.exe",
				std::vector<std::string>{ "/c", "ping", "-n", "3", "127.0.0.1" });
			clientEnd = util::win::Handle{};
			std::this_thread::sleep_for(500ms);
			Assert::AreEqual(1u, server.GetSessionCount(), L"The session ended while the child still held the handle");
			child.wait();
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(0u, server.GetSessionCount(), L"The session outlived the child holding the handle");
			std::lock_guard lock{ disposeMutex };
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected);
		}
	};
}
