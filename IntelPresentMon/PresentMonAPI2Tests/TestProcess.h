// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "JobManager.h"
#include "Logging.h"
#include "TestCommands.h"
#include "RawPipeClient.h"
#include "../CommonUtilities/file/FileUtils.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonService/GlobalIdentifiers.h"
#include <boost/process.hpp>
#include <cereal/archives/json.hpp>
#include <cctype>
#include <iostream>
#include <format>
#include <optional>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <vector>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace as = boost::asio;
namespace bp = boost::process;
namespace as = boost::asio;
namespace fs = std::filesystem;
using namespace std::literals;
using namespace pmon;

// Names used by child PresentMonService.exe instances under test.
// NEVER reuse production shared-service defaults (pipe, shm prefix, or ETW session
// name). Colliding with the installed PresentMonSharedService ETW session
// (default "PMService") stops that live session and leaves the installed service
// with a stale logger handle.
struct CommonProcessArgs
{
	std::string ctrlPipe;
	std::string shmNamePrefix;
	std::string logLevel;
	std::optional<std::string> logVerboseModules;
	std::string logFolder;
	std::string sampleClientMode;
	bool suppressService = false;
	// When false, kernel UI tests load about:blank. Set true to launch the app UI
	// so in-window error modals can be observed.
	bool launchAppUi = false;
	// Optional override. When empty, ServiceProcess derives "{shmNamePrefix}_etw".
	std::string etwSessionName;
};

inline bool EqualsIgnoreCaseAscii_(std::string_view a, std::string_view b)
{
	if (a.size() != b.size()) {
		return false;
	}
	for (size_t i = 0; i < a.size(); ++i) {
		if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) {
			return false;
		}
	}
	return true;
}

inline std::string NormalizeControlPipeBaseName_(std::string name)
{
	constexpr std::string_view prefixes[] = {
		R"(\\.\pipe\)",
		R"(//./pipe/)",
		R"(\\.\PIPE\)",
		R"(//./PIPE/)",
	};
	for (auto prefix : prefixes) {
		if (name.size() >= prefix.size()
			&& EqualsIgnoreCaseAscii_(name.substr(0, prefix.size()), prefix)) {
			name.erase(0, prefix.size());
			break;
		}
	}
	return name;
}

inline std::string ResolveTestEtwSessionName_(const CommonProcessArgs& common)
{
	if (!common.etwSessionName.empty()) {
		return common.etwSessionName;
	}
	Assert::IsFalse(common.shmNamePrefix.empty(),
		L"CommonProcessArgs.shmNamePrefix must be set so a non-production ETW session name can be derived");
	return common.shmNamePrefix + "_etw";
}

inline void AssertIsolatedFromProductionServiceNames_(const CommonProcessArgs& common)
{
	Assert::IsFalse(common.ctrlPipe.empty(),
		L"CommonProcessArgs.ctrlPipe must be set to a non-production test pipe name");
	Assert::IsFalse(common.shmNamePrefix.empty(),
		L"CommonProcessArgs.shmNamePrefix must be set to a non-production test shm prefix");

	const auto pipeBase = NormalizeControlPipeBaseName_(common.ctrlPipe);
	const auto productionPipeBase = NormalizeControlPipeBaseName_(gid::defaultControlPipeName);
	Assert::IsFalse(EqualsIgnoreCaseAscii_(pipeBase, productionPipeBase),
		L"Test control pipe must not use the production shared service pipe name");

	Assert::IsFalse(EqualsIgnoreCaseAscii_(common.shmNamePrefix, R"(Global\pm_svc_shm)"),
		L"Test shm prefix must not use the production shared service shm prefix");
	Assert::IsFalse(EqualsIgnoreCaseAscii_(common.shmNamePrefix, "pm_svc_shm"),
		L"Test shm prefix must not use the production shared service shm prefix");

	const auto etwName = ResolveTestEtwSessionName_(common);
	Assert::IsFalse(EqualsIgnoreCaseAscii_(etwName, "PMService"),
		L"Test ETW session name must not use the production shared service ETW session name");
}

inline std::vector<std::string> SplitVerboseModulesArgs_(const std::string& raw)
{
	std::vector<std::string> modules;
	std::string token;
	for (unsigned char ch : raw) {
		if (ch == ',' || std::isspace(ch)) {
			if (!token.empty()) {
				modules.push_back(token);
				token.clear();
			}
			continue;
		}
		token.push_back(static_cast<char>(ch));
	}
	if (!token.empty()) {
		modules.push_back(token);
	}
	return modules;
}

inline void AppendVerboseModulesArgs_(std::vector<std::string>& args,
	const std::optional<std::string>& modules,
	const char* flag)
{
	if (!modules || modules->empty()) {
		return;
	}
	const auto values = SplitVerboseModulesArgs_(*modules);
	if (values.empty()) {
		return;
	}
	args.push_back(flag);
	args.insert(args.end(), values.begin(), values.end());
}

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
	int GetExitCode() const
	{
		return process_.exit_code();
	}
	bool IsRunning()
	{
		return process_.running();
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
		static constexpr auto kServiceExitTimeout = 15s;
		if (!WaitForExit(kServiceExitTimeout)) {
			Logger::WriteMessage(std::format(
				"Service did not exit within {}s after quit; terminating\n",
				kServiceExitTimeout.count()).c_str());
			Murder();
			return;
		}
	}
	void Ping()
	{
		Assert::AreEqual("ping-ok"s, Command("ping"));
	}
	~ConnectedTestProcess() override
	{
		if (process_.running()) {
			try { Quit(); }
			catch (...) {
				Logger::WriteMessage(util::ReportException("ConnectedTestProcess dtor").first.c_str());
			}
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
		AssertIsolatedFromProductionServiceNames_(common);
		std::vector<std::string> allArgs{
			"--control-pipe"s, common.ctrlPipe,
			"--shm-name-prefix"s, common.shmNamePrefix,
			"--enable-test-control"s,
			"--log-dir"s, common.logFolder,
			"--log-name-pid"s,
			"--log-level"s, common.logLevel,
			"--enable-debugger-log"s,
		};
		AppendVerboseModulesArgs_(allArgs, common.logVerboseModules, "--log-verbose-modules");
		allArgs.append_range(customArgs);
		// Always force a non-production ETW session name last so customArgs cannot
		// omit it and fall through to the production default ("PMService").
		allArgs.push_back("--etw-session-name"s);
		allArgs.push_back(ResolveTestEtwSessionName_(common));
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
			"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
			"--log-folder"s, common.logFolder,
			"--log-name-pid"s,
			"--log-level"s, common.logLevel,
			"--mode"s, common.sampleClientMode,
		};
		AppendVerboseModulesArgs_(allArgs, common.logVerboseModules, "--log-verbose-modules");
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

// PresentMonUI child process used for UI process guard coverage
class UiProcess : public TestProcess
{
public:
	UiProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
		:
		TestProcess{ ioctx, jm, "PresentMonUI.exe"s, MakeArgs_(customArgs, common) }
	{}
private:
	std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
	{
		std::vector<std::string> allArgs{
			"--p2c-log-folder"s, common.logFolder,
			"--p2c-log-level"s, common.logLevel,
			"--p2c-url"s, "about:blank"s,
			"--p2c-no-net-fail"s,
		};
		AppendVerboseModulesArgs_(allArgs, common.logVerboseModules, "--p2c-log-verbose-modules");
		allArgs.append_range(customArgs);
		return allArgs;
	}
};

// PresentMon kernel process used to verify second launch behavior before UI spawn
class KernelProcess : public TestProcess
{
public:
	KernelProcess(as::io_context& ioctx, JobManager& jm, const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
		:
		TestProcess{ ioctx, jm, "PresentMon.exe"s, MakeArgs_(customArgs, common) }
	{}
private:
	std::vector<std::string> MakeArgs_(const std::vector<std::string>& customArgs,
		const CommonProcessArgs& common)
	{
		std::vector<std::string> allArgs{
			"--files-working"s,
			"--log-folder"s, common.logFolder,
			"--log-level"s, common.logLevel,
			"--control-pipe"s, common.ctrlPipe,
			"--shm-name-prefix"s, common.shmNamePrefix,
			"--middleware-dll-path"s, "PresentMonAPI2.dll"s,
		};
		if (!common.launchAppUi) {
			allArgs.append_range(std::vector<std::string>{
				"--ui-option"s, "url"s, "about:blank"s,
			});
		}
		AppendVerboseModulesArgs_(allArgs, common.logVerboseModules, "--log-verbose-modules");
		allArgs.append_range(customArgs);
		return allArgs;
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
		if (!logManager_) {
			logManager_.emplace();
		}
		Logger::WriteMessage(std::format("Test log directory: {}\n",
			fs::absolute(GetCommonArgs().logFolder).string()).c_str());
		pmon::test::SetupTestLogging(GetCommonArgs().logFolder, GetCommonArgs().logLevel,
			GetCommonArgs().logVerboseModules);
		svcArgs_ = std::move(args);
		if (!GetCommonArgs().suppressService) {
			StartService_(svcArgs_, GetCommonArgs());
			serviceStarted_ = true;
		}
		else {
			serviceStarted_ = false;
		}
	}
	void Cleanup()
	{
		if (serviceStarted_) {
			StopService_(GetCommonArgs());
			serviceStarted_ = false;
		}
		if (ioctxRunThread_.joinable()) {
			ioctxRunThread_.join();
		}
		logManager_.reset();
	}
	void StopService()
	{
		StopService_(GetCommonArgs());
	}
	void RebootService(std::optional<std::vector<std::string>> newArgs = {})
	{
		auto& common = GetCommonArgs();
		if (common.suppressService) {
			return;
		}
		auto& svcArgs = newArgs ? *newArgs : svcArgs_;
		StopService_(common);
		StartService_(svcArgs, common);
		svcArgs_ = std::move(svcArgs);
		serviceStarted_ = true;
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
	UiProcess LaunchUi(const std::vector<std::string>& args = {})
	{
		return UiProcess{ ioctx_, jobMan_, args, GetCommonArgs() };
	}
	std::unique_ptr<UiProcess> LaunchUiAsPtr(const std::vector<std::string>& args = {})
	{
		return std::make_unique<UiProcess>(ioctx_, jobMan_, args, GetCommonArgs());
	}
	KernelProcess LaunchKernel(const std::vector<std::string>& args = {})
	{
		return KernelProcess{ ioctx_, jobMan_, args, GetCommonArgs() };
	}
	std::unique_ptr<KernelProcess> LaunchKernelAsPtr(const std::vector<std::string>& args = {})
	{
		return std::make_unique<KernelProcess>(ioctx_, jobMan_, args, GetCommonArgs());
	}
	const std::string& GetControlPipeName() const
	{
		return GetCommonArgs().ctrlPipe;
	}
	virtual const CommonProcessArgs& GetCommonArgs() const = 0;
private:
	// functions
	void StartService_(const std::vector<std::string>& args, const CommonProcessArgs& common)
	{
		AssertIsolatedFromProductionServiceNames_(common);
		// make sure ioctx thread is running and keep it running until service launches
		auto workGuard = ReserveIoctxThread_();
		Assert::IsTrue(util::pipe::DuplexPipe::WaitForVacancy(common.ctrlPipe, svcPipeTimeout_),
			L"Timed out waiting for control pipe vacancy before service start");
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
	bool serviceStarted_ = false;
	std::thread ioctxRunThread_;
	as::io_context ioctx_;
	std::optional<pmon::test::LogChannelManager> logManager_;
};
