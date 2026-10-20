// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Exercises the connected pipe pair: both ends created by one process, so no other
// process can connect to it. The kernel uses one per UI launch.
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include <atomic>
#include <chrono>
#include <format>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace PipePairTests
{
	using namespace pmon::util;
	namespace as = boost::asio;

	struct Note
	{
		uint32_t value = 0;
		template<class A> void serialize(A& ar) { ar(value); }
	};

	// larger than the pipe's buffer, so writing it needs the peer to read
	struct Bulk
	{
		std::string bytes;
		template<class A> void serialize(A& ar) { ar(bytes); }
	};

	std::string MakeUniquePipeName_()
	{
		static std::atomic<uint32_t> nextId = 0;
		return std::format(R"(\\.\pipe\pm-pair-test-{}-{})", GetCurrentProcessId(), nextId++);
	}

	// runs a coroutine on ioctx to completion and rethrows anything it threw
	template<class F>
	void RunToCompletion_(as::io_context& ioctx, F&& coro)
	{
		auto done = as::co_spawn(ioctx, std::forward<F>(coro), as::use_future);
		ioctx.run();
		done.get();
	}

	TEST_CLASS(PipePairTests)
	{
	public:
		TEST_METHOD(PairCarriesPacketsBothWays)
		{
			as::io_context ioctx;
			auto pair = pipe::DuplexPipe::MakeConnectedPair(MakeUniquePipeName_(), ioctx, {});
			auto pClient = pipe::DuplexPipe::AdoptConnectedClientAsPtr(pair.client.Release(), ioctx);
			Assert::AreEqual((uint32_t)GetCurrentProcessId(), pair.pServer->GetClientProcessId(),
				L"The pair's client was not this process");
			Note atClient[2];
			Note atServer[2];
			RunToCompletion_(ioctx, [&]() -> as::awaitable<void> {
				co_await pair.pServer->WritePacket(Note{ 1 }, Note{ 2 });
				atClient[0] = co_await pClient->ReadPacketConsumeHeader<Note>();
				atClient[1] = pClient->ConsumePacketPayload<Note>();
				co_await pClient->WritePacket(Note{ 3 }, Note{ 4 });
				atServer[0] = co_await pair.pServer->ReadPacketConsumeHeader<Note>();
				atServer[1] = pair.pServer->ConsumePacketPayload<Note>();
			});
			Assert::AreEqual(1u, atClient[0].value);
			Assert::AreEqual(2u, atClient[1].value);
			Assert::AreEqual(3u, atServer[0].value);
			Assert::AreEqual(4u, atServer[1].value);
		}
		// The pair has its only instance connected, so the name refuses everyone else.
		TEST_METHOD(PairNameRefusesAnotherClient)
		{
			as::io_context ioctx;
			const auto name = MakeUniquePipeName_();
			auto pair = pipe::DuplexPipe::MakeConnectedPair(name, ioctx, {});
			const win::Handle intruder{ CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
				nullptr, OPEN_EXISTING, 0, nullptr) };
			const auto error = GetLastError();
			Assert::IsFalse((bool)intruder, L"Another client connected to the pair's name");
			Assert::AreEqual((DWORD)ERROR_PIPE_BUSY, error);
		}
		// A name someone created first must not be used: they would own the other instance.
		TEST_METHOD(PairRefusesNameAlreadyInUse)
		{
			as::io_context ioctx;
			const auto name = MakeUniquePipeName_();
			auto squatter = pipe::DuplexPipe::MakeAsPtr(name, ioctx);
			Assert::ExpectException<pipe::PipeError>([&] {
				pipe::DuplexPipe::MakeConnectedPair(name, ioctx, {});
			}, L"A pair was created on a name already in use");
		}
		// A peer that stops reading cannot hold a write forever: the deadline throws
		// PipeWriteTimeout, and the unsent bytes are dropped so the next writer starts clean.
		TEST_METHOD(WriteTimesOutWhenPeerStopsReading)
		{
			as::io_context ioctx;
			auto pair = pipe::DuplexPipe::MakeConnectedPair(MakeUniquePipeName_(), ioctx, {});
			// the client end is adopted but never read
			auto pClient = pipe::DuplexPipe::AdoptConnectedClientAsPtr(pair.client.Release(), ioctx);
			bool timedOut = false;
			const auto start = std::chrono::steady_clock::now();
			RunToCompletion_(ioctx, [&]() -> as::awaitable<void> {
				try {
					co_await pair.pServer->WritePacket(Note{ 1 }, Bulk{ std::string(512 * 1024, 'x') }, 50u);
				}
				catch (const pipe::PipeWriteTimeout&) {
					timedOut = true;
				}
			});
			const auto elapsed = std::chrono::steady_clock::now() - start;
			Assert::IsTrue(timedOut, L"A write to a peer that does not read did not time out");
			Assert::IsTrue(elapsed < 2s, L"The write timeout took far longer than its deadline");
			Assert::AreEqual((size_t)0, pair.pServer->GetWriteBufferPending(),
				L"A timed-out write left unsent bytes in the write buffer");
		}
		// This is how the kernel sees its UI exit: the only client handle closes.
		TEST_METHOD(ClosingClientEndEndsServerReads)
		{
			as::io_context ioctx;
			auto pair = pipe::DuplexPipe::MakeConnectedPair(MakeUniquePipeName_(), ioctx, {});
			auto pClient = pipe::DuplexPipe::AdoptConnectedClientAsPtr(pair.client.Release(), ioctx);
			pClient.reset();
			bool broken = false;
			RunToCompletion_(ioctx, [&]() -> as::awaitable<void> {
				try {
					co_await pair.pServer->ReadPacketConsumeHeader<Note>();
				}
				catch (const pipe::PipeBroken&) {
					broken = true;
				}
			});
			Assert::IsTrue(broken, L"Closing the client end did not end the server's read");
		}
		// Closing the pipe while a write with a deadline is queued for the write lock wakes it
		// as cancelled, promptly, rather than letting it run to its timeout.
		TEST_METHOD(CloseWakesQueuedTimedWrite)
		{
			as::io_context ioctx;
			auto pair = pipe::DuplexPipe::MakeConnectedPair(MakeUniquePipeName_(), ioctx, {});
			// the client end is adopted but never read, so the first write holds the lock
			auto pClient = pipe::DuplexPipe::AdoptConnectedClientAsPtr(pair.client.Release(), ioctx);
			bool firstDone = false;
			bool queuedDone = false;
			bool queuedCancelled = false;
			std::chrono::steady_clock::duration queuedWaited{};
			RunToCompletion_(ioctx, [&]() -> as::awaitable<void> {
				const auto ex = co_await as::this_coro::executor;
				as::co_spawn(ex, [&]() -> as::awaitable<void> {
					try {
						co_await pair.pServer->WritePacket(Note{ 1 }, Bulk{ std::string(512 * 1024, 'x') }, 5000u);
					}
					catch (...) {
					}
					firstDone = true;
				}, as::detached);
				as::steady_timer pause{ ex };
				pause.expires_after(20ms);
				co_await pause.async_wait(as::use_awaitable);
				const auto start = std::chrono::steady_clock::now();
				as::co_spawn(ex, [&]() -> as::awaitable<void> {
					try {
						co_await pair.pServer->WritePacket(Note{ 2 }, Note{ 3 }, 5000u);
					}
					catch (const pipe::PipeOperationCanceled&) {
						queuedCancelled = true;
					}
					catch (...) {
					}
					queuedWaited = std::chrono::steady_clock::now() - start;
					queuedDone = true;
				}, as::detached);
				pause.expires_after(20ms);
				co_await pause.async_wait(as::use_awaitable);
				pair.pServer->Close();
				while (!firstDone || !queuedDone) {
					co_await as::post(ex, as::use_awaitable);
				}
			});
			Assert::IsTrue(queuedCancelled, L"Closing the pipe did not cancel the write queued for the lock");
			Assert::IsTrue(queuedWaited < 1s, L"The queued write ran on toward its timeout after the pipe closed");
		}
	};

	TEST_CLASS(CoroMutexTests)
	{
	public:
		// A timed waiter that gives up leaves the queue and its count, so the mutex is free
		// as soon as the holder releases it.
		TEST_METHOD(TimedWaiterTimesOutAndLeavesNoCount)
		{
			as::io_context ioctx;
			pipe::CoroMutex mtx{ ioctx };
			auto result = pipe::CoroMutex::LockResult::Acquired;
			std::chrono::steady_clock::duration waited{};
			bool heldWhileHolding = false;
			bool freeAfterRelease = false;
			RunToCompletion_(ioctx, [&]() -> as::awaitable<void> {
				auto holder = co_await pipe::CoroLock(mtx);
				const auto start = std::chrono::steady_clock::now();
				const auto waiter = co_await pipe::CoroLock(mtx, 50ms);
				waited = std::chrono::steady_clock::now() - start;
				result = waiter.GetResult();
				heldWhileHolding = !mtx.TryLock();
				if (!heldWhileHolding) {
					mtx.Unlock();
				}
				holder = pipe::CoroLockGuard{};
				freeAfterRelease = mtx.TryLock();
				if (freeAfterRelease) {
					mtx.Unlock();
				}
			});
			Assert::IsTrue(result == pipe::CoroMutex::LockResult::TimedOut, L"The waiter did not time out");
			Assert::IsTrue(waited < 1s, L"The timed wait ran far past its timeout");
			Assert::IsTrue(heldWhileHolding, L"The timed-out waiter's count was undone twice");
			Assert::IsTrue(freeAfterRelease, L"A timed-out waiter left its count on the mutex");
		}
		// Unlock and the timeout can wake a waiter at the same moment. Whichever wins, an
		// acquired waiter owns the mutex and releases it through its guard, a timed-out waiter
		// never owns it, and the mutex ends up free.
		TEST_METHOD(GrantRacingTimeoutKeepsOwnershipConsistent)
		{
			as::io_context ioctx;
			pipe::CoroMutex mtx{ ioctx };
			int acquired = 0;
			int timedOut = 0;
			int leftHeld = 0;
			constexpr int rounds = 200;
			RunToCompletion_(ioctx, [&]() -> as::awaitable<void> {
				const auto ex = co_await as::this_coro::executor;
				for (int i = 0; i < rounds; i++) {
					auto holder = co_await pipe::CoroLock(mtx);
					bool done = false;
					as::co_spawn(ex, [&]() -> as::awaitable<void> {
						const auto waiter = co_await pipe::CoroLock(mtx, 1ms);
						if (waiter.GetResult() == pipe::CoroMutex::LockResult::Acquired) {
							acquired++;
						}
						else if (waiter.GetResult() == pipe::CoroMutex::LockResult::TimedOut) {
							timedOut++;
						}
						done = true;
					}, as::detached);
					// release on a timer due at the same moment as the waiter's timeout
					as::steady_timer release{ ex };
					release.expires_after(1ms);
					co_await release.async_wait(as::use_awaitable);
					holder = pipe::CoroLockGuard{};
					while (!done) {
						co_await as::post(ex, as::use_awaitable);
					}
					if (mtx.TryLock()) {
						mtx.Unlock();
					}
					else {
						leftHeld++;
					}
				}
			});
			Assert::AreEqual(rounds, acquired + timedOut, L"A waiter was neither granted nor timed out");
			Assert::AreEqual(0, leftHeld, L"The mutex was left held after a grant raced a timeout");
		}
		// Closing the pipe (CancelWaiters) wakes a timed waiter as Cancelled. Its count is undone
		// once, by CancelWaiters, not again when its timer would have expired.
		TEST_METHOD(CancelWaitersWakesTimedWaiterOnce)
		{
			as::io_context ioctx;
			pipe::CoroMutex mtx{ ioctx };
			auto result = pipe::CoroMutex::LockResult::Acquired;
			bool heldWhileHolding = false;
			bool freeAfterRelease = false;
			RunToCompletion_(ioctx, [&]() -> as::awaitable<void> {
				const auto ex = co_await as::this_coro::executor;
				auto holder = co_await pipe::CoroLock(mtx);
				bool done = false;
				as::co_spawn(ex, [&]() -> as::awaitable<void> {
					const auto waiter = co_await pipe::CoroLock(mtx, 5000ms);
					result = waiter.GetResult();
					done = true;
				}, as::detached);
				// let the waiter queue
				co_await as::post(ex, as::use_awaitable);
				co_await as::post(ex, as::use_awaitable);
				mtx.CancelWaiters();
				while (!done) {
					co_await as::post(ex, as::use_awaitable);
				}
				// the holder still holds it: a second decrement would make it look free
				heldWhileHolding = !mtx.TryLock();
				if (!heldWhileHolding) {
					mtx.Unlock();
				}
				holder = pipe::CoroLockGuard{};
				freeAfterRelease = mtx.TryLock();
				if (freeAfterRelease) {
					mtx.Unlock();
				}
			});
			Assert::IsTrue(result == pipe::CoroMutex::LockResult::Cancelled, L"CancelWaiters did not wake the timed waiter as cancelled");
			Assert::IsTrue(heldWhileHolding, L"The cancelled waiter's count was undone twice");
			Assert::IsTrue(freeAfterRelease, L"The mutex count was wrong after cancelling a timed waiter");
		}
	};
}
