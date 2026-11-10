// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <CommonUtilities/win/WinAPI.h>

#include <CppUnitTest.h>

#include <CommonUtilities/log/Channel.h>
#include <CommonUtilities/log/Entry.h>
#include <CommonUtilities/log/IDriver.h>
#include <CommonUtilities/log/LineTable.h>
#include <CommonUtilities/log/Log.h>
#include <CommonUtilities/file/TempFile.h>
#include <chrono>
#include <filesystem>
#include <fstream>
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

		class LineTableRestorer_
		{
		public:
			LineTableRestorer_()
			{
				LineTable::ClearList();
			}
			~LineTableRestorer_()
			{
				LineTable::ClearList();
			}
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

	TEST_CLASS(LineTableTests)
	{
	public:
		TEST_METHOD(ReplaceListClearsPriorMembershipAndTraceOverrides)
		{
			LineTableRestorer_ restore;
			const auto folder = std::filesystem::temp_directory_path() /
				pmon::util::file::TempFile::MakeRandomName();
			std::filesystem::create_directories(folder);
			const auto firstPath = folder / L"first-\u65e5.txt";
			{
				std::ofstream list{ firstPath };
				list << "C:\\first.cpp(10)\n";
				list << "C:\\trace.cpp(20)$\n";
			}
			LineTable::ReplaceList(firstPath, LineTable::ListMode::Black);
			Assert::AreEqual((int)LineTable::ListMode::Black, (int)LineTable::GetListMode());
			Assert::IsTrue(LineTable::GetTraceOverride());
			Assert::IsTrue(LineTable::TryLookup("C:\\first.cpp", 10)->isListed_);
			Assert::AreEqual((int)LineTable::TraceOverride::ForceOn,
				(int)LineTable::TryLookup("C:\\trace.cpp", 20)->traceOverride_);

			const auto secondPath = folder / "second.txt";
			{
				std::ofstream list{ secondPath };
				list << "C:\\second.cpp(30)\n";
			}
			LineTable::ReplaceList(secondPath, LineTable::ListMode::White);
			Assert::AreEqual((int)LineTable::ListMode::White, (int)LineTable::GetListMode());
			Assert::IsFalse(LineTable::GetTraceOverride());
			Assert::IsFalse(LineTable::TryLookup("C:\\first.cpp", 10)->isListed_);
			Assert::AreEqual((int)LineTable::TraceOverride::None,
				(int)LineTable::TryLookup("C:\\trace.cpp", 20)->traceOverride_);
			Assert::IsTrue(LineTable::TryLookup("C:\\second.cpp", 30)->isListed_);

			std::error_code ec;
			std::filesystem::remove_all(folder, ec);
		}

		TEST_METHOD(IngestListDoesNotCarryTraceMarkerToLaterLines)
		{
			LineTableRestorer_ restore;
			const auto folder = std::filesystem::temp_directory_path() /
				pmon::util::file::TempFile::MakeRandomName();
			std::filesystem::create_directories(folder);
			const auto listPath = folder / "mixed.txt";
			{
				std::ofstream list{ listPath };
				list << "C:\\trace.cpp(20)$\n";
				list << "C:\\listed.cpp(30)\n";
			}
			Assert::IsTrue(LineTable::IngestList(listPath.string(), false));
			Assert::AreEqual((int)LineTable::ListMode::White, (int)LineTable::GetListMode());
			Assert::IsTrue(LineTable::TryLookup("C:\\listed.cpp", 30)->isListed_);
			Assert::AreEqual((int)LineTable::TraceOverride::None,
				(int)LineTable::TryLookup("C:\\listed.cpp", 30)->traceOverride_);

			std::error_code ec;
			std::filesystem::remove_all(folder, ec);
		}

		TEST_METHOD(ReplaceListWithoutListedLinesLeavesFilteringOff)
		{
			LineTableRestorer_ restore;
			const auto folder = std::filesystem::temp_directory_path() /
				pmon::util::file::TempFile::MakeRandomName();
			std::filesystem::create_directories(folder);
			// trace-only allow list must not enable allow mode (which would drop everything)
			const auto tracePath = folder / "trace-only.txt";
			{
				std::ofstream list{ tracePath };
				list << "C:\\trace.cpp(20)$\n";
				list << "C:\\quiet.cpp(21)%\n";
			}
			LineTable::ReplaceList(tracePath, LineTable::ListMode::White);
			Assert::AreEqual((int)LineTable::ListMode::None, (int)LineTable::GetListMode());
			Assert::IsTrue(LineTable::GetTraceOverride());
			Assert::AreEqual((int)LineTable::TraceOverride::ForceOn,
				(int)LineTable::TryLookup("C:\\trace.cpp", 20)->traceOverride_);

			// empty / unparsable list replaces a prior list and leaves filtering off
			const auto listedPath = folder / "listed.txt";
			{
				std::ofstream list{ listedPath };
				list << "C:\\listed.cpp(30)\n";
			}
			LineTable::ReplaceList(listedPath, LineTable::ListMode::White);
			Assert::AreEqual((int)LineTable::ListMode::White, (int)LineTable::GetListMode());
			const auto emptyPath = folder / "empty.txt";
			{
				std::ofstream list{ emptyPath };
				list << "not a line reference\n";
			}
			LineTable::ReplaceList(emptyPath, LineTable::ListMode::White);
			Assert::AreEqual((int)LineTable::ListMode::None, (int)LineTable::GetListMode());
			Assert::IsFalse(LineTable::GetTraceOverride());
			Assert::IsFalse(LineTable::TryLookup("C:\\listed.cpp", 30)->isListed_);

			std::error_code ec;
			std::filesystem::remove_all(folder, ec);
		}
	};
}
