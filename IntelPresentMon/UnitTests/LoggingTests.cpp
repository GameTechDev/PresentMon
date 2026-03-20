// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <CommonUtilities/win/WinAPI.h>

#include <CppUnitTest.h>

#include <CommonUtilities/log/Channel.h>
#include <CommonUtilities/log/CrashFlushHooks.h>
#include <CommonUtilities/log/Entry.h>
#include <CommonUtilities/log/IDriver.h>
#include <CommonUtilities/log/Log.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <semaphore>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::chrono_literals;

namespace LoggingTests
{
	using namespace pmon::util::log;

	namespace
	{
		Entry MakeEntry_()
		{
			Entry e;
			e.level_ = Level::Info;
			e.note_ = "test-entry";
			e.sourceStrings_ = Entry::StaticSourceStrings{
				.file_ = __FILE__,
				.functionName_ = __FUNCTION__,
			};
			e.sourceLine_ = __LINE__;
			e.timestamp_ = std::chrono::system_clock::now();
			e.pid_ = GetCurrentProcessId();
			e.tid_ = GetCurrentThreadId();
			return e;
		}

		class BlockingDriver_ : public IDriver
		{
		public:
			void Submit(const Entry&) override
			{
				entered.release();
				release.acquire();
			}
			void Flush() override {}

			std::binary_semaphore entered{ 0 };
			std::binary_semaphore release{ 0 };
		};

		class StubChannel_ : public IChannel
		{
		public:
			void Submit(Entry&&) noexcept override {}
			void Submit(const Entry&) noexcept override {}
			void Flush() override {}
			bool TryFlushFor(std::chrono::milliseconds timeout) noexcept override
			{
				lastTimeoutMs.store((long long)timeout.count());
				flushCalls.fetch_add(1);
				return flushResult.load();
			}
			void AttachComponent(std::shared_ptr<IChannelComponent>, std::string = {}) override {}
			std::shared_ptr<IChannelComponent> GetComponent(std::string) const override { return {}; }
			void FlushEntryPointExit() override {}

			std::atomic<bool> flushResult = true;
			std::atomic<int> flushCalls = 0;
			std::atomic<long long> lastTimeoutMs = -1;
		};

		class DefaultChannelGuard_
		{
		public:
			DefaultChannelGuard_()
				:
				pOriginal_{ GetDefaultChannel() }
			{}
			~DefaultChannelGuard_()
			{
				InjectDefaultChannel(std::move(pOriginal_));
			}
		private:
			std::shared_ptr<IChannel> pOriginal_;
		};

		class CrashHookGuard_
		{
		public:
			~CrashHookGuard_()
			{
				UninstallCrashFlushHooks();
			}
		};
	}

	TEST_CLASS(LoggingCrashTests)
	{
	public:
		TEST_METHOD(ChannelTryFlushForTimesOutWhenWorkerBlocked)
		{
			auto pChannel = std::make_shared<Channel>();
			auto pDriver = std::make_shared<BlockingDriver_>();
			pChannel->AttachComponent(pDriver, "drv:block");

			pChannel->Submit(MakeEntry_());
			pDriver->entered.acquire();

			const auto timedOut = pChannel->TryFlushFor(10ms);
			Assert::IsFalse(timedOut);

			pDriver->release.release();
			const auto flushed = pChannel->TryFlushFor(500ms);
			Assert::IsTrue(flushed);
		}

		TEST_METHOD(CrashFlushUsesConfiguredTimeout)
		{
			DefaultChannelGuard_ channelGuard;
			CrashHookGuard_ hookGuard;

			auto pStub = std::make_shared<StubChannel_>();
			InjectDefaultChannel(pStub);

			CrashFlushConfig config;
			config.flushTimeout = 123ms;
			config.installTerminateHandler = false;
			config.installUnhandledExceptionFilter = false;
			InstallCrashFlushHooks(config);

			const auto success = ExecuteCrashFlushNowForTesting();
			Assert::IsTrue(success);
			Assert::AreEqual(1, pStub->flushCalls.load());
			Assert::AreEqual(123ll, pStub->lastTimeoutMs.load());
		}

		TEST_METHOD(CrashFlushPropagatesFlushFailure)
		{
			DefaultChannelGuard_ channelGuard;
			CrashHookGuard_ hookGuard;

			auto pStub = std::make_shared<StubChannel_>();
			pStub->flushResult.store(false);
			InjectDefaultChannel(pStub);

			CrashFlushConfig config;
			config.flushTimeout = 42ms;
			config.installTerminateHandler = false;
			config.installUnhandledExceptionFilter = false;
			InstallCrashFlushHooks(config);

			const auto success = ExecuteCrashFlushNowForTesting();
			Assert::IsFalse(success);
			Assert::AreEqual(1, pStub->flushCalls.load());
			Assert::AreEqual(42ll, pStub->lastTimeoutMs.load());
		}

		TEST_METHOD(UnhandledFilterHookTriggersCrashFlushPath)
		{
			DefaultChannelGuard_ channelGuard;
			CrashHookGuard_ hookGuard;

			auto pStub = std::make_shared<StubChannel_>();
			InjectDefaultChannel(pStub);

			CrashFlushConfig config;
			config.flushTimeout = 77ms;
			config.installTerminateHandler = false;
			config.installUnhandledExceptionFilter = false;
			InstallCrashFlushHooks(config);

			const auto code = ExecuteUnhandledExceptionFilterForTesting(nullptr);
			Assert::AreEqual((long)EXCEPTION_CONTINUE_SEARCH, code);
			Assert::AreEqual(1, pStub->flushCalls.load());
			Assert::AreEqual(77ll, pStub->lastTimeoutMs.load());
		}
	};
}
