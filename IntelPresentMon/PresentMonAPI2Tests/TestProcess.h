// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "JobManager.h"
#include "TestCommands.h"
#include <boost/process.hpp>
#include <cereal/archives/json.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <format>
#include <sstream>
#include <filesystem>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace as = boost::asio;
namespace bp = boost::process;
namespace as = boost::asio;
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
	virtual ~TestProcess()
	{
		if (process_.running()) {
			Quit();
		}
	}

	TestProcess(const TestProcess&) = delete;
	TestProcess& operator=(const TestProcess&) = delete;
	TestProcess(TestProcess&& other) noexcept = delete;
	TestProcess& operator=(TestProcess&& other) noexcept = delete;

	virtual void Quit() {}
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
	void Quit() override
	{
		Assert::IsTrue(process_.running());
		Assert::AreEqual("quit-ok"s, Command("quit"));
		process_.wait();
	}
	void Ping()
	{
		Assert::AreEqual("ping-ok"s, Command("ping"));
	}
protected:
	std::string GetCommandPrefix_() const override { return "%"; }
	std::string GetCommandResponsePreamble_() const override { return "%%{"; }
	std::string GetCommandResponsePostamble_() const override { return "}%%\r\n"; }
};

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
	{
	}
};

class CommonTestFixture
{
public:
	std::optional<ServiceProcess> service;

	CommonTestFixture() = default;
	CommonTestFixture(const CommonTestFixture&) = delete;
	CommonTestFixture& operator=(const CommonTestFixture&) = delete;
	CommonTestFixture(CommonTestFixture&&) = delete;
	CommonTestFixture& operator=(CommonTestFixture&&) = delete;
	virtual ~CommonTestFixture() = default;

	void Setup(const std::vector<std::string>& args = {})
	{
		service.emplace(ioctx, jobMan, args, GetCommonArgs_());
		ioctxRunThread = std::thread{ [&] {try{ioctx.run();}catch(...){}} };
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
	ClientProcess LaunchClient(const std::vector<std::string>& args = {})
	{
		return ClientProcess{ ioctx, jobMan, args, GetCommonArgs_() };
	}
	std::unique_ptr<ClientProcess> LaunchClientAsPtr(const std::vector<std::string>& args = {})
	{
		return std::make_unique<ClientProcess>(ioctx, jobMan, args, GetCommonArgs_());
	}
	PresenterProcess LaunchPresenter(const std::vector<std::string>& args = {})
	{
		return PresenterProcess{ ioctx, jobMan, args };
	}
protected:
	virtual const CommonProcessArgs& GetCommonArgs_() const = 0;
private:
	JobManager jobMan;
	std::thread ioctxRunThread;
	as::io_context ioctx;
};