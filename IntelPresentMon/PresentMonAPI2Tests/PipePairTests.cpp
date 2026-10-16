// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Exercises the connected pipe pair: both ends created by one process, so no other
// process can connect to it. The kernel uses one per UI launch.
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include <atomic>
#include <format>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PipePairTests
{
	using namespace pmon::util;
	namespace as = boost::asio;

	struct Note
	{
		uint32_t value = 0;
		template<class A> void serialize(A& ar) { ar(value); }
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
	};
}
