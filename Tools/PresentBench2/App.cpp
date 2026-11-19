// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "App.h"
#include "Backend.h"
#include "Capabilities.h"
#include "ControlServer.h"
#include "FrameLoop.h"
#include "RunStatus.h"
#include "Window.h"
#include "markers/FrameMarkers.h"
#include "../../IntelPresentMon/CommonUtilities/log/Log.h"
#include <fstream>
#include <format>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>

namespace pb2
{
	namespace
	{
		class UniqueHandle
		{
		public:
			explicit UniqueHandle(HANDLE handle) : handle_(handle) {}
			~UniqueHandle()
			{
				if (handle_) {
					CloseHandle(handle_);
				}
			}
			UniqueHandle(const UniqueHandle&) = delete;
			UniqueHandle& operator=(const UniqueHandle&) = delete;
			HANDLE Get() const { return handle_; }

		private:
			HANDLE handle_ = nullptr;
		};

		void WriteResultFile(const std::string& path, const RunStatus& status)
		{
			if (path.empty()) {
				return;
			}
			std::ofstream out(path);
			if (!out) {
				throw std::runtime_error("failed to open result file: " + path);
			}
			nlohmann::json j = status;
			out << j.dump(2);
			if (!out) {
				throw std::runtime_error("failed to write result file: " + path);
			}
		}
	}

	App::App(BenchConfig config)
		:
		config_(std::move(config))
	{}

	int App::Run(HINSTANCE instance)
	{
		pmlog_info(std::format(
			"Starting PresentBench2: api={} mode={} adapter={} monitor={} size={}x{} "
			"buffers={} frames-in-flight={} frame-count={} frame-sleep-ms={} "
			"clears-per-frame={} stdio-control={}",
			ApiName(config_.api), ModeName(config_.mode), config_.adapter, config_.monitor,
			config_.swap.width, config_.swap.height, config_.swap.bufferCount,
			config_.framesInFlight,
			config_.frameCount ? std::to_string(*config_.frameCount) : "unbounded",
			config_.frameSleepMs, config_.workload.clearsPerFrame,
			config_.enableStdioControl ? "true" : "false"));

		RunStatusBoard board;
		board.Update([&](RunStatus& s) {
			s.pid = GetCurrentProcessId();
			s.state = RunState::Starting;
			s.api = ApiName(config_.api);
			s.mode = ModeName(config_.mode);
		});

		UniqueHandle quitEvent{ CreateEventW(nullptr, TRUE, FALSE, nullptr) };
		if (!quitEvent.Get()) {
			throw std::runtime_error("CreateEventW failed");
		}
		board.SetQuitEvent(quitEvent.Get());
		ControlServer control;
		std::optional<std::string> capabilitiesJson;

		if (config_.enableStdioControl) {
			control.SetHandlers({
				{ "ping", [](const std::string&) { return "ping-ok"; } },
				{ "status", [&](const std::string&) { return board.GetJson(); } },
				{ "capabilities", [&](const std::string&) -> std::string {
					if (!capabilitiesJson) {
						capabilitiesJson = CapabilitiesToJson(
							ProbeCapabilities(config_.adapter));
					}
					return *capabilitiesJson;
				} },
				{ "wait-for-frame", [&](const std::string& arg) -> std::string {
					const auto request = ParseWaitForFrameRequest(arg);
					if (!board.WaitForFrame(request.appFrameId, request.timeoutMs)) {
						const auto st = board.Get();
						if (st.state == RunState::Failed || st.state == RunState::Completed) {
							return "error: run ended before frame";
						}
						return "error: timeout";
					}
					return "wait-for-frame-ok";
				} },
				{ "quit", [&](const std::string&) -> std::string {
					board.RequestQuit();
					return "quit-ok";
				} },
			});
			control.SetDisconnectHandler([&] { board.RequestQuit(); });
			pmlog_info("Starting stdio control channel");
			control.StartStdio();
			pmlog_info("Stdio control channel started");
		}

		BenchWindow window;
		MarkerHub markers;
		std::unique_ptr<IRenderBackend> backend;
		int exitCode = 0;
		try {
			if (config_.workload.useXessFg) {
				const auto capabilities = ProbeCapabilities(config_.adapter);
				if (!capabilities.xessFg.feature.compiled ||
					!capabilities.xessFg.feature.runtime) {
					throw std::runtime_error(capabilities.xessFg.feature.reason);
				}
			}
			pmlog_info(std::format("Creating {} {} window", ApiName(config_.api),
				ModeName(config_.mode)));
			window.Create(instance, config_);
			pmlog_info(std::format("Window created: client-size={}x{}",
				window.GetClientWidth(), window.GetClientHeight()));
			if (config_.workload.emitProviderMarkers) {
				markers.Add(CreateProviderSink(!config_.workload.useXessFg));
			}
			if (config_.workload.emitPclMarkers) {
				markers.Add(CreatePclStatsSink());
			}

			pmlog_info(std::format("Creating {} render backend", ApiName(config_.api)));
			backend = CreateBackend(window.GetHwnd(), config_,
				window.GetClientWidth(), window.GetClientHeight());
			pmlog_info(std::format("{} render backend created", ApiName(config_.api)));
			if (auto* sink = backend->GetMarkerSink()) {
				markers.AddRaw(sink);
			}
			pmlog_info("Beginning frame loop");
			const auto loopResult = RunFrameLoop(config_, window, *backend, markers, board);
			exitCode = loopResult.ok ? 0 : 6;
			if (loopResult.ok) {
				pmlog_info(std::format("Frame loop completed: app-frames={} presents={}",
					loopResult.appFrames, loopResult.presents));
			}
			else {
				pmlog_error(std::format(
					"Frame loop failed after {} app frames and {} presents: {}",
					loopResult.appFrames, loopResult.presents, loopResult.error));
			}
		}
		catch (const std::exception& ex) {
			board.Update([&](RunStatus& s) {
				s.state = RunState::Failed;
				s.error = ex.what();
			});
			pmlog_error(std::format("PresentBench2 run failed: {}", ex.what()));
			exitCode = 6;
		}

		try {
			WriteResultFile(config_.resultJsonPath, board.Get());
			if (!config_.resultJsonPath.empty()) {
				pmlog_info(std::format("Wrote result JSON to {}",
					config_.resultJsonPath));
			}
		}
		catch (const std::exception& ex) {
			board.Update([&](RunStatus& s) {
				s.state = RunState::Failed;
				s.error = ex.what();
			});
			pmlog_error(ex.what());
			exitCode = 6;
		}

		if (config_.enableStdioControl) {
			// Stay alive until quit so agents can read final status.
			pmlog_info("Frame run ended; waiting for control-channel quit");
			while (!board.QuitRequested()) {
				if (!window.ProcessMessages()) {
					board.RequestQuit();
					break;
				}
				const HANDLE event = quitEvent.Get();
				const DWORD wait = MsgWaitForMultipleObjects(1, &event, FALSE, 50,
					QS_ALLINPUT);
				if (wait == WAIT_OBJECT_0) {
					break;
				}
			}
			pmlog_info("Stopping stdio control channel");
			control.Stop();
			pmlog_info("Stdio control channel stopped");
		}
		pmlog_info(std::format("PresentBench2 exiting with code {}", exitCode));
		return exitCode;
	}
}
