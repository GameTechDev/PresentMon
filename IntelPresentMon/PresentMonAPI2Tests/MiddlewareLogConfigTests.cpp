// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "../CommonUtilities/file/TempFile.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;
namespace fs = std::filesystem;
namespace api2 = pmon::api2;

namespace MiddlewareLogConfigTests
{
	namespace
	{
		// caller struct from a hypothetical newer build with an extra trailing field
		struct MiddlewareLogPolicyNext_
		{
			api2::MiddlewareLogPolicy v1;
			uint64_t futureField = 0xDEADBEEF;
		};

		fs::path MakeTempFolder_()
		{
			auto folder = fs::temp_directory_path() / pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(folder);
			return folder;
		}

		std::string ReadAll_(const fs::path& path)
		{
			std::ifstream file{ path };
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		}

		// restores middleware policy captured at construction
		class PolicyRestorer_
		{
		public:
			PolicyRestorer_()
			{
				Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmGetMiddlewareLogPolicy_(&saved_));
			}
			~PolicyRestorer_()
			{
				pmSetMiddlewareLogPolicy_(&saved_);
			}
		private:
			api2::MiddlewareLogPolicy saved_;
		};
	}

	TEST_CLASS(MiddlewareLogPolicyTests)
	{
	public:
		TEST_METHOD(SetGetRoundTrip)
		{
			PolicyRestorer_ restore;
			api2::MiddlewareLogPolicy policy;
			policy.logLevel = PM_DIAGNOSTIC_LEVEL_VERBOSE2;
			policy.traceLevel = PM_DIAGNOSTIC_LEVEL_VERBOSE2;
			policy.exceptionTrace = 1;
			policy.sehTrace = 0;
			policy.resolveTraceInClientThread = 0;
			policy.verboseModuleBitset = 0b1010;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogPolicy_(&policy));

			api2::MiddlewareLogPolicy readBack;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmGetMiddlewareLogPolicy_(&readBack));
			Assert::AreEqual((uint32_t)sizeof(api2::MiddlewareLogPolicy), readBack.size);
			Assert::AreEqual((int)PM_DIAGNOSTIC_LEVEL_VERBOSE2, (int)readBack.logLevel);
			Assert::AreEqual((int)PM_DIAGNOSTIC_LEVEL_VERBOSE2, (int)readBack.traceLevel);
			Assert::AreEqual((int)1, (int)readBack.exceptionTrace);
			Assert::AreEqual((int)0, (int)readBack.sehTrace);
			Assert::AreEqual((uint64_t)0b1010, readBack.verboseModuleBitset);
		}

		TEST_METHOD(RejectsBadArgumentsWithoutApplying)
		{
			PolicyRestorer_ restore;
			api2::MiddlewareLogPolicy before;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmGetMiddlewareLogPolicy_(&before));

			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogPolicy_(nullptr));
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmGetMiddlewareLogPolicy_(nullptr));

			api2::MiddlewareLogPolicy tooSmall;
			tooSmall.size = api2::kMiddlewareLogPolicySizeV1 - 1;
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogPolicy_(&tooSmall));

			// valid log level paired with invalid trace level must apply nothing
			api2::MiddlewareLogPolicy invalid = before;
			invalid.logLevel = before.logLevel == PM_DIAGNOSTIC_LEVEL_VERBOSE
				? PM_DIAGNOSTIC_LEVEL_ERROR : PM_DIAGNOSTIC_LEVEL_VERBOSE;
			invalid.traceLevel = (PM_DIAGNOSTIC_LEVEL)15;
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogPolicy_(&invalid));

			api2::MiddlewareLogPolicy after;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmGetMiddlewareLogPolicy_(&after));
			Assert::AreEqual((int)before.logLevel, (int)after.logLevel);
			Assert::AreEqual((int)before.traceLevel, (int)after.traceLevel);
		}

		TEST_METHOD(AcceptsNewerCallerSize)
		{
			PolicyRestorer_ restore;
			MiddlewareLogPolicyNext_ next;
			next.v1.size = sizeof(MiddlewareLogPolicyNext_);
			next.v1.logLevel = PM_DIAGNOSTIC_LEVEL_DEBUG;
			next.v1.traceLevel = PM_DIAGNOSTIC_LEVEL_ERROR;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogPolicy_(&next.v1));

			// get reports only the bytes this middleware knows and leaves the rest alone
			MiddlewareLogPolicyNext_ readBack;
			readBack.v1.size = sizeof(MiddlewareLogPolicyNext_);
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmGetMiddlewareLogPolicy_(&readBack.v1));
			Assert::AreEqual((uint32_t)sizeof(api2::MiddlewareLogPolicy), readBack.v1.size);
			Assert::AreEqual((int)PM_DIAGNOSTIC_LEVEL_DEBUG, (int)readBack.v1.logLevel);
			Assert::AreEqual((uint64_t)0xDEADBEEF, readBack.futureField);
		}

		TEST_METHOD(NonTerminalFlushPreservesPolicy)
		{
			PolicyRestorer_ restore;
			api2::MiddlewareLogPolicy policy;
			policy.logLevel = PM_DIAGNOSTIC_LEVEL_DEBUG;
			policy.traceLevel = PM_DIAGNOSTIC_LEVEL_WARNING;
			policy.resolveTraceInClientThread = 0;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogPolicy_(&policy));
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFlushMiddlewareLog_(2000));
			// 0 waits indefinitely (matching pmDiagnosticWaitForMessage) rather than timing out
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFlushMiddlewareLog_(0));

			api2::MiddlewareLogPolicy readBack;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmGetMiddlewareLogPolicy_(&readBack));
			Assert::AreEqual((int)PM_DIAGNOSTIC_LEVEL_WARNING, (int)readBack.traceLevel);
			Assert::AreEqual((int)0, (int)readBack.resolveTraceInClientThread);
		}
	};

	TEST_CLASS(MiddlewareLogOutputTests)
	{
	public:
		TEST_METHOD(RejectsBadArguments)
		{
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogOutput_(nullptr));
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmGetMiddlewareLogOutput_(nullptr));
			api2::MiddlewareLogOutput tooSmall;
			tooSmall.size = api2::kMiddlewareLogOutputSizeV1 - 1;
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogOutput_(&tooSmall));
			api2::MiddlewareLogOutputState state;
			state.size = api2::kMiddlewareLogOutputStateSizeV1 - 1;
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmGetMiddlewareLogOutput_(&state));

			const auto missing = (fs::temp_directory_path() /
				pmon::util::file::TempFile::MakeRandomName() / "mw.txt").wstring();
			api2::MiddlewareLogOutput output;
			output.debuggerOutput = 1;
			output.pFilePath = missing.c_str();
			Assert::AreEqual((int)PM_STATUS_NONEXISTENT_FILE_PATH, (int)pmSetMiddlewareLogOutput_(&output));
		}

		TEST_METHOD(FileOutputReceivesMiddlewareEntries)
		{
			PolicyRestorer_ restore;
			api2::MiddlewareLogPolicy policy;
			policy.logLevel = PM_DIAGNOSTIC_LEVEL_DEBUG;
			policy.traceLevel = PM_DIAGNOSTIC_LEVEL_NONE;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogPolicy_(&policy));

			const auto folder = MakeTempFolder_();
			const auto filePath = folder / L"mw-\u65e5.txt";
			const auto filePathWide = filePath.wstring();
			api2::MiddlewareLogOutput output;
			output.debuggerOutput = 1;
			output.pFilePath = filePathWide.c_str();
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogOutput_(&output));
			// same path again is a no-op and must succeed
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogOutput_(&output));
			{
				api2::MiddlewareLogOutputState state;
				Assert::AreEqual((int)PM_STATUS_INSUFFICIENT_BUFFER,
					(int)pmGetMiddlewareLogOutput_(&state));
				std::vector<wchar_t> path((size_t)state.filePathLength + 1);
				state.filePathCapacity = (uint32_t)path.size();
				state.pFilePath = path.data();
				Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmGetMiddlewareLogOutput_(&state));
				Assert::AreEqual(filePath.c_str(), path.data());
				Assert::AreEqual((int)1, (int)state.debuggerOutput);
			}

			// provoke a middleware log entry
			Assert::AreNotEqual((int)PM_STATUS_SUCCESS,
				(int)pmCloseSession(reinterpret_cast<PM_SESSION_HANDLE>(0x1234)));
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmFlushMiddlewareLog_(2000));

			// detach file (keep debugger output, matching module setup) to flush and close
			output.pFilePath = nullptr;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogOutput_(&output));
			// no active file: zero-capacity query succeeds with zero length
			api2::MiddlewareLogOutputState detached;
			Assert::AreEqual((int)PM_STATUS_SUCCESS,
				(int)pmGetMiddlewareLogOutput_(&detached));
			Assert::AreEqual((uint32_t)sizeof(api2::MiddlewareLogOutputState), detached.size);
			Assert::AreEqual(0u, detached.filePathLength);
			Assert::AreEqual((int)1, (int)detached.debuggerOutput);

			const auto content = ReadAll_(filePath);
			Assert::IsTrue(content.find("pmCloseSession") != std::string::npos,
				L"middleware log file lacks expected entry");
			std::error_code ec;
			fs::remove_all(folder, ec);
		}
	};

	TEST_CLASS(MiddlewareLogFilterTests)
	{
	public:
		TEST_METHOD(RejectsBadArguments)
		{
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogFilter_(nullptr));

			api2::MiddlewareLogFilter filter;
			filter.size = api2::kMiddlewareLogFilterSizeV1 - 1;
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogFilter_(&filter));

			// None with a path, and Deny without a path, are invalid
			filter.size = sizeof(filter);
			filter.listMode = api2::MiddlewareLogListMode::None;
			filter.pListPath = L"list.txt";
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogFilter_(&filter));
			filter.listMode = api2::MiddlewareLogListMode::Deny;
			filter.pListPath = nullptr;
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogFilter_(&filter));
			filter.listMode = (api2::MiddlewareLogListMode)7;
			Assert::AreEqual((int)PM_STATUS_BAD_ARGUMENT, (int)pmSetMiddlewareLogFilter_(&filter));

			const auto missing = (fs::temp_directory_path() /
				pmon::util::file::TempFile::MakeRandomName()).wstring();
			filter.listMode = api2::MiddlewareLogListMode::Allow;
			filter.pListPath = missing.c_str();
			Assert::AreEqual((int)PM_STATUS_NONEXISTENT_FILE_PATH, (int)pmSetMiddlewareLogFilter_(&filter));

			const auto folder = MakeTempFolder_();
			const auto malformedPath = folder / "malformed.txt";
			{
				std::ofstream list{ malformedPath };
				list << "C:\\valid.cpp(1)\n";
				list << "C:\\invalid.cpp(999999999999999999999999)\n";
			}
			const auto malformedPathWide = malformedPath.wstring();
			filter.pListPath = malformedPathWide.c_str();
			Assert::AreEqual((int)PM_STATUS_FAILURE, (int)pmSetMiddlewareLogFilter_(&filter));
			std::error_code ec;
			fs::remove_all(folder, ec);
		}

		TEST_METHOD(IngestsListAndClears)
		{
			const auto folder = MakeTempFolder_();
			const auto listPath = folder / L"deny-\u65e5.txt";
			{
				std::ofstream list{ listPath };
				list << "C:\\nonexistent\\MiddlewareLogConfigTests.cpp(1)\n";
			}
			const auto listPathWide = listPath.wstring();
			api2::MiddlewareLogFilter filter;
			filter.listMode = api2::MiddlewareLogListMode::Deny;
			filter.pListPath = listPathWide.c_str();
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogFilter_(&filter));

			// restore: disable list filtering
			api2::MiddlewareLogFilter clear;
			Assert::AreEqual((int)PM_STATUS_SUCCESS, (int)pmSetMiddlewareLogFilter_(&clear));
			std::error_code ec;
			fs::remove_all(folder, ec);
		}
	};
}
