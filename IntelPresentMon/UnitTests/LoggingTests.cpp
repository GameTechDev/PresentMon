// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <CommonUtilities/win/WinAPI.h>

#include <CppUnitTest.h>

#include <CommonUtilities/log/Channel.h>
#include <CommonUtilities/log/Entry.h>
#include <CommonUtilities/log/IDriver.h>
#include <CommonUtilities/log/Log.h>
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

	}

	TEST_CLASS(LoggingChannelTests)
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
	};
}
