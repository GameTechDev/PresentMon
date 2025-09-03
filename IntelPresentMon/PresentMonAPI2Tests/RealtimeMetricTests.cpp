// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include <fstream>
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include <string>
#include <iostream>
#include <windows.h>
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapper/FixedQuery.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "tlhelp32.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process::v1;

namespace RealtimeMetricTests
{
	bool CaseInsensitiveCompare(std::wstring str1, std::wstring str2) {
		std::for_each(str1.begin(), str1.end(), [](wchar_t& c)
			{
				c = std::tolower(static_cast<wchar_t>(c));
			});
		std::for_each(str2.begin(), str2.end(), [](wchar_t& c)
			{
				c = std::tolower(static_cast<wchar_t>(c));
			});
		if (str1.compare(str2) == 0)
			return true;
		return false;
	}

	void GetProcessInformation(std::wstring processName, std::optional<unsigned int>& processId) {
		try
		{
			HANDLE processes_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
			if (processes_snapshot == INVALID_HANDLE_VALUE) {
				processId = std::nullopt;
				return;
			}

			PROCESSENTRY32 process_info;
			process_info.dwSize = sizeof(process_info);

			if (!Process32First(processes_snapshot, &process_info)) {
				// Unable to retrieve the first process
				CloseHandle(processes_snapshot);
				processId = std::nullopt;
				return;
			}

			do {
				if (CaseInsensitiveCompare(process_info.szExeFile, processName)) {
					CloseHandle(processes_snapshot);
					processId = process_info.th32ProcessID;
					processName = process_info.szExeFile;
					return;
				}
			} while (Process32Next(processes_snapshot, &process_info));

			CloseHandle(processes_snapshot);
		}
		catch (const std::exception& e) {
			std::cout << "Error: " << e.what() << std::endl;
			return;
		}
		catch (...) {
			std::cout << "Unknown Error" << std::endl;
			return;
		}
	}

	TEST_CLASS(RealtimeMetricTests)
	{
		std::optional<bp::child> oService;
		std::optional<bp::child> oApp;
	public:

		TEST_METHOD_CLEANUP(Cleanup)
		{
			if (oService) {
				oService->terminate();
				oService->wait();
				oService.reset();
			}
			if (oApp) {
				oApp->terminate();
				oApp->wait();
				oApp.reset();
			}
		}

		TEST_METHOD(RealtimeOpenSessionTest)
		{
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream serviceOut; // Stream for reading the process's output
			bp::opstream serviceIn;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etwSessionName = "RealtimeULTSession"s;

			oService.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etw-session-name"s, etwSessionName,
				bp::std_out > serviceOut, bp::std_in < serviceIn);

			std::this_thread::sleep_for(1000ms);

			bp::ipstream AppOut; // Stream for reading the app's output
			bp::opstream AppIn;  // Stream for writing to the app's input

			oApp.emplace("..\\..\\Tools\\PresentBench.exe"s,
				bp::std_out > AppOut, bp::std_in < AppIn);
			
			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);;
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}
		}

		TEST_METHOD(RealtimeTrackProcessTest)
		{
			using namespace std::string_literals;
			using namespace std::chrono_literals;

			bp::ipstream serviceOut; // Stream for reading the process's output
			bp::opstream serviceIn;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etwSessionName = "RealtimeULTSession"s;

			oService.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "10000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etw-session-name"s, etwSessionName,
				bp::std_out > serviceOut, bp::std_in < serviceIn);

			std::this_thread::sleep_for(1000ms);

			bp::ipstream AppOut; // Stream for reading the app's output
			bp::opstream AppIn;  // Stream for writing to the app's input

			oApp.emplace("..\\..\\Tools\\PresentBench.exe"s,
				bp::std_out > AppOut, bp::std_in < AppIn);

			std::this_thread::sleep_for(1000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);;
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe");
					return;
				}
			}

			std::wstring appName = L"PresentBench.exe";
			std::optional<unsigned int> appPid;
			GetProcessInformation(std::move(appName), appPid);
			if (!appPid.has_value())
			{
				Assert::AreEqual(false, true, L"*** Unable to detect PresentBench.exe.");
				return;
			}

			//pmapi::ProcessTracker processTracker;
			auto processTracker = pSession->TrackProcess(appPid.value());
			std::this_thread::sleep_for(5000ms);
			processTracker.Reset();
		}
		TEST_METHOD(RealtimeFrameMetricsTest)
		{
			using namespace std::string_literals;
			using namespace std::chrono_literals;
			using namespace pmapi;

			bp::ipstream serviceOut; // Stream for reading the process's output
			bp::opstream serviceIn;  // Stream for writing to the process's input

			const auto pipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
			const auto introName = "PM_intro_test_nsm_2"s;
			const auto etwSessionName = "RealtimeMetricSession"s;

			oService.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "20000"s,
				"--control-pipe"s, pipeName,
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName,
				"--etw-session-name"s, etwSessionName,
				bp::std_out > serviceOut, bp::std_in < serviceIn);

			std::this_thread::sleep_for(1000ms);

			bp::ipstream AppOut; // Stream for reading the app's output
			bp::opstream AppIn;  // Stream for writing to the app's input

			oApp.emplace("..\\..\\Tools\\PresentBench.exe"s,
				bp::std_out > AppOut, bp::std_in < AppIn);

			std::this_thread::sleep_for(2000ms);

			std::unique_ptr<pmapi::Session> pSession;
			{
				try
				{
					pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
					pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
					pSession = std::make_unique<pmapi::Session>(pipeName);;
				}
				catch (const std::exception& e) {
					std::cout << "Error: " << e.what() << std::endl;
					Assert::AreEqual(false, true, L"*** Connecting to service via named pipe.");
					return;
				}
			}

			std::wstring appName = L"PresentBench.exe";
			std::optional<unsigned int> appPid;
			GetProcessInformation(std::move(appName), appPid);
			if (!appPid.has_value())
			{
				Assert::AreEqual(false, true, L"*** Unable to detect PresentBench.exe.");
				return;
			}

			auto processTracker = pSession->TrackProcess(appPid.value());
			PM_BEGIN_FIXED_DYNAMIC_QUERY(MyDynamicQuery)
				FixedQueryElement qeCpuFtAvg{ this, PM_METRIC_CPU_FRAME_TIME, PM_STAT_AVG };
			PM_END_FIXED_QUERY dq{ *pSession, 1000, 0, 1, 1 };

			const int maxSamples = 10;
			for (int i = 0; i < maxSamples; i++)
			{
				dq.Poll(processTracker);
				if (dq.qeCpuFtAvg.As<double>() == 0.) {
					std::this_thread::sleep_for(500ms);
				}
				else
				{
					break;
				}
			}

			// Using the default frame wait for PresentBench it is reasonable to expect an
			// average CPU frame time of around 11ms. 
			double expectedFtAvg = 11.;
			// We are allowing a tolerance of up to 1ms for the CPU frame time
			double ftTolerance = 1.;
			if (fabs(expectedFtAvg - dq.qeCpuFtAvg.As<double>()) > ftTolerance) {
				Assert::AreEqual(expectedFtAvg, dq.qeCpuFtAvg.As<double>(), L"*** CPU Frame Time not within 1ms tolerance.");
			}
			processTracker.Reset();
		}
	};
}