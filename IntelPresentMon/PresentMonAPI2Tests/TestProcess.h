// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "JobManager.h"
#include "TestCommands.h"
#include "../CommonUtilities/file/FileUtils.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include <boost/process.hpp>
#include <cereal/archives/json.hpp>
#include <iostream>
#include <format>
#include <sstream>
#include <filesystem>
#include <chrono>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace as = boost::asio;
namespace bp = boost::process;
namespace as = boost::asio;
namespace fs = std::filesystem;
using namespace std::literals;
using namespace pmon;

struct CommonProcessArgs
{
	std::string ctrlPipe;
	std::string introNsm;
	std::string frameNsm;
	std::string logLevel;
	std::string logFolder;
	std::string sampleClientMode;
};

// base class to represent child processes launched by test cases
class TestProcess
{
public:
	TestProcess(as::io_context& ioctx, JobManager& jm, const std::string& executable,
		const std::vector<std::string>& args)
		:
		pipeFrom_{ ioctx },
		pipeTo_{ ioctx },
		process_{ ioctx, executable, args,
			bp::process_stdio{ pipeTo_, pipeFrom_, nullptr } }
	{
		jm.Attach(process_.native_handle());
		Logger::WriteMessage(std::format(" - Launched process {{{}}} [{}]\n",
			executable, process_.id()).c_str());
	}

	TestProcess(const TestProcess&) = delete;
	TestProcess& operator=(const TestProcess&) = delete;
	TestProcess(TestProcess&& other) noexcept = delete;
	TestProcess& operator=(TestProcess&& other) noexcept = delete;
	virtual ~TestProcess() noexcept = default;

	void Murder()
	{
		Assert::IsTrue(process_.running());
		::TerminateProcess(process_.native_handle(), 0xDEAD);
		process_.wait();
	}
	uint32_t GetId() const
	{
		return process_.id();
	}
	void Wait()
	{
		process_.wait();
	}
	bool WaitForExit(std::chrono::milliseconds timeout)
	{
		if (!process_.running()) {
			return true;
		}
		const auto waitResult = WaitForSingleObject(process_.native_handle(),
			static_cast<DWORD>(timeout.count()));
		if (waitResult == WAIT_OBJECT_0) {
			process_.wait();
			return true;
		}
		return false;
	}
	std::string Command(const std::string& command)
	{
		const auto prefix = GetCommandPrefix_();
		const auto preamble = GetCommandResponsePreamble_();
		const auto postamble = GetCommandResponsePostamble_();

		// send command
		as::write(pipeTo_, as::buffer(std::format("{}{}\n", prefix, command)));

		// read through the start marker and drop it (and any leading junk)
		const auto n = as::read_until(pipeFrom_, readBufferFrom_, preamble);
		readBufferFrom_.consume(n);

		// read through the end marker, m counts bytes up to and including postamble
		const auto m = as::read_until(pipeFrom_, readBufferFrom_, postamble);

		// size string to accept payload
		std::string payload;
		payload.resize(m - postamble.size());

		// read into sized string using stream wrapper and discard postamble
		std::istream is(&readBufferFrom_);
		is.read(&payload[0], static_cast<std::streamsize>(payload.size()));
		readBufferFrom_.consume(postamble.size());

		return payload;
	}
private:
	as::readable_pipe pipeFrom_;
	as::streambuf readBufferFrom_;
	as::writable_pipe pipeTo_;
protected:
	virtual std::string GetCommandPrefix_() const { return ""; }
	virtual std::string GetCommandResponsePreamble_() const { return ""; }
	virtual std::string GetCommandResponsePostamble_() const { return "\r\n"; }
	bp::process process_;
};

// test process that has connection-oriented session with ping and quit lifecycle commands
class ConnectedTestProcess : public TestProcess
{
public:
	ConnectedTestProcess(as::io_context& ioctx, JobManager& jm, const std::string& executable,
		const std::vector<std::string>& args)
		:
		TestProcess{ ioctx, jm, executable, args }
	{
		Ping();
	}
	void Quit()
	{
		Assert::IsTrue(process_.running());
		Assert::AreEqual("quit-ok"s, Command("quit"));
		process_.wait();
	}
	void Ping()
	{
		Assert::AreEqual("ping-ok"s, Command("ping"));
	}
	~ConnectedTestProcess() override
	{
		if (process_.running()) {
			Quit();
		}
	}
protected:
	std::string GetCommandPrefix_() const override { return "%"; }
	std::string GetCommandResponsePreamble_() const override { return "%%{"; }
	std::string GetCommandResponsePostamble_() const override { return "}%%\r\n"; }
};

// service as child
class ServiceProcess : public ConnectedTestProcess
{
public:
	ServiceProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
		:
		ConnectedTestProcess{ ioctx, jm, "PresentMonService.exe"s, MakeArgs_(customArgs, common) }
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
	std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
	{
		std::vector<std::string> allArgs{
			"--control-pipe"s, common.ctrlPipe,
			"--nsm-prefix"s, common.frameNsm,
			"--intro-nsm"s, common.introNsm,
			"--enable-test-control"s,
			"--log-dir"s, common.logFolder,
			"--log-name-pid"s,
			"--log-level"s, common.logLevel,
		};
		allArgs.append_range(customArgs);
		return allArgs;
	}
};

// SampleClient as a driver for interacting with service test child
class ClientProcess : public ConnectedTestProcess
{
public:
	ClientProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
		:
		ConnectedTestProcess{ ioctx, jm, "SampleClient.exe"s, MakeArgs_(customArgs, common) }
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
	std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
	{
		std::vector<std::string> allArgs{
			"--control-pipe"s, common.ctrlPipe,
			"--intro-nsm"s, common.introNsm,
			"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
			"--log-folder"s, common.logFolder,
			"--log-name-pid"s,
			"--log-level"s, common.logLevel,
			"--mode"s, common.sampleClientMode,
		};
		allArgs.append_range(customArgs);
		return allArgs;
	}
};

// PresentBench child process for a reliable presenting target process
class PresenterProcess : public TestProcess
{
public:
	PresenterProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs)
		:
		TestProcess{ ioctx, jm, R"(..\..\Tools\PresentBench.exe)", customArgs }
	{}
};

// original presentmon console application
class OpmProcess : public TestProcess
{
public:
	OpmProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs)
		:
		TestProcess{ ioctx, jm, LocateExecutable_(), customArgs }
	{}
private:
	static std::string LocateExecutable_()
	{
		const auto pattern = R"(^PresentMon-\d+\.\d+\.\d+-x64\.exe$)";
		try {
			return util::file::FindFilesMatchingPattern(fs::current_path(), pattern).at(0).string();
		}
		catch (...) {
			Logger::WriteMessage(std::format("Failed to find executable matching: [{}]", pattern).c_str());
			Assert::IsTrue(false);
			return "";
		}
	}
};

// fixture to embed into each test class to give common setup/cleanup/child management
class CommonTestFixture
{
public:
	std::optional<ServiceProcess> service;

	CommonTestFixture() = default;
	CommonTestFixture(const CommonTestFixture&) = delete;
	CommonTestFixture& operator=(const CommonTestFixture&) = delete;
	CommonTestFixture(CommonTestFixture&&) = delete;
	CommonTestFixture& operator=(CommonTestFixture&&) = delete;
	virtual ~CommonTestFixture() noexcept = default;

	void Setup(std::vector<std::string> args = {})
	{
		StartService_(args, GetCommonArgs());
		svcArgs_ = std::move(args);
	}
	void Cleanup()
	{
		StopService_(GetCommonArgs());
		ioctxRunThread_.join();
	}
	void StopService()
	{
		StopService_(GetCommonArgs());
	}
	void RebootService(std::optional<std::vector<std::string>> newArgs = {})
	{
		auto& common = GetCommonArgs();
		auto& svcArgs = newArgs ? *newArgs : svcArgs_;
		StopService_(common);
		StartService_(svcArgs, common);
		svcArgs_ = std::move(svcArgs);
	}
	ClientProcess LaunchClient(const std::vector<std::string>& args = {})
	{
		return ClientProcess{ ioctx_, jobMan_, args, GetCommonArgs() };
	}
	std::unique_ptr<ClientProcess> LaunchClientAsPtr(const std::vector<std::string>& args = {})
	{
		return std::make_unique<ClientProcess>(ioctx_, jobMan_, args, GetCommonArgs());
	}
	PresenterProcess LaunchPresenter(const std::vector<std::string>& args = {})
	{
		return PresenterProcess{ ioctx_, jobMan_, args };
	}
	OpmProcess LaunchOpm(const std::vector<std::string>& args = {})
	{
		return OpmProcess{ ioctx_, jobMan_, args };
	}
	virtual const CommonProcessArgs& GetCommonArgs() const = 0;
private:
	// functions
	void StartService_(const std::vector<std::string>& args, const CommonProcessArgs& common)
	{
		// make sure ioctx thread is running and keep it running until service launches
		auto workGuard = ReserveIoctxThread_();
		// launch the service
		service.emplace(ioctx_, jobMan_, args, common);
		// ensure that service pipe is available
		Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(common.ctrlPipe, svcPipeTimeout_),
			L"Timed out waiting for pipe availability");
	}
	void StopService_(const CommonProcessArgs& common)
	{
		service.reset();
		// ensure that service pipe has vacated
		Assert::IsTrue(util::pipe::DuplexPipe::WaitForVacancy(common.ctrlPipe, svcPipeTimeout_),
			L"Timed out waiting for pipe vacancy");
	}
	as::executor_work_guard<as::io_context::executor_type> ReserveIoctxThread_()
	{
		auto workGuard = as::executor_work_guard<as::io_context::executor_type>{ ioctx_.get_executor() };
		if (!ioctxRunThread_.joinable()) {
			ioctxRunThread_ = std::thread{ [&] {try { ioctx_.run(); } catch (...) {}} };
		}
		return workGuard;
	}
	// data
	static constexpr int svcPipeTimeout_ = 250;
	std::vector<std::string> svcArgs_;
	JobManager jobMan_;
	std::thread ioctxRunThread_;
	as::io_context ioctx_;
};
