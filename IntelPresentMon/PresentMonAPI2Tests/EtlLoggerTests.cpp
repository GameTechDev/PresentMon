// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/file/FileUtils.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include <string>
#include <iostream>
#include <format>
#include <boost/process.hpp>
#include <cereal/archives/json.hpp>
#include <sstream>
#include <filesystem>
#include "TestCommands.h"
#include "Folders.h"
#include "JobManager.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process;
namespace as = boost::asio;
namespace fs = std::filesystem;
namespace vi = std::views;
using namespace std::literals;
using namespace pmon;

namespace EtlLoggerTests
{
	static constexpr const char* controlPipe_ = R"(\\.\pipe\pm-etllog-test-ctrl)";
	static constexpr const char* introNsm_ = "pm_etllog_test_intro";
	static constexpr const char* logLevel_ = "info";

	class TestProcess
	{
	public:
		TestProcess(as::io_context& ioctx, JobManager& jm, const std::string& executable, const std::vector<std::string>& args)
			:
			pipeFrom_{ ioctx },
			pipeTo_{ ioctx },
			process_{ ioctx, executable, args,
				bp::process_stdio{ pipeTo_, pipeFrom_, nullptr } }
		{
			jm.Attach(process_.native_handle());
			Logger::WriteMessage(std::format(" - Launched process {{{}}} [{}]\n",
				executable, process_.id()).c_str());
			Assert::AreEqual("ping-ok"s, Command("ping"));
		}
		~TestProcess()
		{
			if (process_.running()) {
				Quit();
			}
		}

		TestProcess(const TestProcess&) = delete;
		TestProcess& operator=(const TestProcess&) = delete;
		TestProcess(TestProcess&& other) noexcept = delete;
		TestProcess& operator=(TestProcess&& other) noexcept = delete;

		void Quit()
		{
			Assert::IsTrue(process_.running());
			Assert::AreEqual("quit-ok"s, Command("quit"));
			process_.wait();
		}
		void Murder()
		{
			Assert::IsTrue(process_.running());
			::TerminateProcess(process_.native_handle(), 0xDEAD);
			process_.wait();
		}
		std::string Command(const std::string command)
		{
			// send command
			as::write(pipeTo_, as::buffer(std::format("%{}\n", command)));

			// read through the start marker and drop it (and any leading junk)
			const auto n = as::read_until(pipeFrom_, readBufferFrom_, preamble_);
			readBufferFrom_.consume(n);

			// read through the end marker, m counts bytes up to and including postamble
			const auto m = as::read_until(pipeFrom_, readBufferFrom_, postamble_);

			// size string to accept payload
			constexpr auto postambleSize = std::size(postamble_) - 1;
			std::string payload;
			payload.resize(m - postambleSize);

			// read into sized string using stream wrapper and discard postamble
			std::istream is(&readBufferFrom_);
			is.read(&payload[0], static_cast<std::streamsize>(payload.size()));
			readBufferFrom_.consume(postambleSize);

			return payload;
		}
		uint32_t GetId() const
		{
			return process_.id();
		}
	private:
		constexpr static const char preamble_[] = "%%{";
		constexpr static const char postamble_[] = "}%%\r\n";
		as::readable_pipe pipeFrom_;
		as::streambuf readBufferFrom_;
		as::writable_pipe pipeTo_;
		bp::process process_;
	};

	class ServiceProcess : public TestProcess
	{
	public:
		ServiceProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs = {})
			:
			TestProcess{ ioctx, jm, "PresentMonService.exe"s, MakeArgs_(customArgs) }
		{
		}
		test::service::Status QueryStatus()
		{
			test::service::Status status;
			std::istringstream is{ Command("status") };
			cereal::JSONInputArchive{ is }(status);
			return status;
		}
	private:
		std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs)
		{
			std::vector<std::string> allArgs{
				"--control-pipe"s, controlPipe_,
				"--nsm-prefix"s, "pm_multi_test_nsm"s,
				"--intro-nsm"s, introNsm_,
				"--enable-test-control"s,
				"--log-dir"s, logFolder_,
				"--log-name-pid"s,
				"--log-level"s, std::string(logLevel_),
			};
			allArgs.append_range(customArgs);
			return allArgs;
		}
	};

	class ClientProcess : public TestProcess
	{
	public:
		ClientProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs = {})
			:
			TestProcess{ ioctx, jm, "SampleClient.exe"s, MakeArgs_(customArgs) }
		{
		}
		test::client::FrameResponse GetFrames()
		{
			test::client::FrameResponse resp;
			std::istringstream is{ Command("get-frames") };
			cereal::JSONInputArchive{ is }(resp);
			Assert::AreEqual("get-frames-ok"s, resp.status);
			return resp;
		}
	private:
		std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs)
		{
			std::vector<std::string> allArgs{
				"--control-pipe"s, controlPipe_,
				"--intro-nsm"s, introNsm_,
				"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
				"--log-folder"s, std::string(logFolder_),
				"--log-name-pid"s,
				"--log-level"s, std::string(logLevel_),
				"--mode"s, "EtlLogger"s,
			};
			allArgs.append_range(customArgs);
			return allArgs;
		}
	};

	class PresenterProcess
	{
	public:
		PresenterProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs = {})
			:
			process_{ ioctx, path_, customArgs }
		{
			jm.Attach(process_.native_handle());
			Logger::WriteMessage(std::format(" - Launched process {{{}}} [{}]\n",
				path_, process_.id()).c_str());
		}
		uint32_t GetId() const
		{
			return process_.id();
		}
	private:
		static constexpr const char* path_ = R"(..\..\Tools\PresentBench.exe)";
		bp::process process_;
	};

	class OpmProcess
	{
	public:
		OpmProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs = {})
			:
			exePath_{ util::file::FindFilesMatchingPattern(fs::current_path(), exePattern_).at(0) },
			process_{ ioctx, exePath_.string(), customArgs}
		{
			jm.Attach(process_.native_handle());
			Logger::WriteMessage(std::format(" - Launched process {{{}}} [{}]\n",
				exePath_.string(), process_.id()).c_str());
		}
		uint32_t GetId() const
		{
			return process_.id();
		}
		void Wait()
		{
			process_.wait();
		}
	private:
		// this will break on version up, needs a better solution
		static constexpr const char* exePattern_ = R"(^PresentMon-\d+\.\d+\.\d+-x64\.exe$)";
		fs::path exePath_;
		bp::process process_;
	};

	struct CommonTestFixture
	{
		JobManager jobMan;
		std::thread ioctxRunThread;
		as::io_context ioctx;
		std::optional<ServiceProcess> service;

		void Setup()
		{
			service.emplace(ioctx, jobMan);
			ioctxRunThread = std::thread{ [&] {pmquell(ioctx.run()); } };
			// wait before every test to ensure that service is available
			std::this_thread::sleep_for(50ms);
		}
		void Cleanup()
		{
			service.reset();
			ioctxRunThread.join();
			// sleep after every test to ensure that previous named pipe has vacated
			std::this_thread::sleep_for(50ms);
		}
		ClientProcess LaunchClient(std::vector<std::string> args = {})
		{
			return ClientProcess{ ioctx, jobMan, std::move(args) };
		}
		PresenterProcess LaunchPresenter(std::vector<std::string> args = {})
		{
			return PresenterProcess{ ioctx, jobMan, std::move(args) };
		}
		OpmProcess LaunchOpm(std::vector<std::string> args = {})
		{
			return OpmProcess{ ioctx, jobMan, std::move(args) };
		}
		std::unique_ptr<ClientProcess> LaunchClientAsPtr(std::vector<std::string> args = {})
		{
			return std::make_unique<ClientProcess>(ioctx, jobMan, std::move(args));
		}
	};

	TEST_CLASS(CommonFixtureTests)
	{
		CommonTestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// verify service lifetime and status command functionality
		TEST_METHOD(ServiceStatusTest)
		{
			// verify initial status
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(0ull, status.nsmStreamedPids.size());
			Assert::AreEqual(16u, status.telemetryPeriodMs);
			Assert::IsTrue((bool)status.etwFlushPeriodMs);
			Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
		}
		// verify client lifetime
		TEST_METHOD(ClientLaunchTest)
		{
			auto client = fixture_.LaunchClient();
		}
	};

	TEST_CLASS(RoundTripLoggerTest)
	{
		CommonTestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// verify etl can be captured and processed
		TEST_METHOD(RecordAndProcessEtl)
		{
			const auto etlFilePath = outFolder_ + "\\RecordAndProcessEtl.etl"s;
			const auto csvFilePath = outFolder_ + "\\RecordAndProcessEtl.csv"s;
			// launch target for tracking
			auto presenter = fixture_.LaunchPresenter();
			std::this_thread::sleep_for(150ms);
			// launch client
			auto client = fixture_.LaunchClient({
				"--run-time"s, "1.15"s,
				"--output-path"s, etlFilePath
			});
			// wait for completion
			client.Quit();
			// make sure .etl file was written
			Assert::IsTrue(std::filesystem::exists(etlFilePath));
			// process .etl file in opm
			fixture_.LaunchOpm({
				"--etl_file"s, etlFilePath,
				"--process_id"s, std::to_string(presenter.GetId()),
				"--output_file"s, csvFilePath,
			}).Wait();
			// verify that the csv has expected minimum size
			Logger::WriteMessage(std::format("Processed CSV size: {:.2f}kB\n",
				double(std::filesystem::file_size(csvFilePath)) / 1024.).c_str());
			Assert::IsTrue(std::filesystem::file_size(csvFilePath) > 10'000);
		}
	};
}