// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "FrameLoop.h"
#include "../../IntelPresentMon/CommonUtilities/IntervalWaiter.h"
#include "../../IntelPresentMon/CommonUtilities/Qpc.h"
#include <chrono>
#include <immintrin.h>
#include <stdexcept>
#include <thread>

namespace pb2
{
	namespace
	{
		void BusyWaitMs(uint32_t ms)
		{
			if (ms == 0) return;
			const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
			while (std::chrono::steady_clock::now() < end) {
				_mm_pause();
			}
		}

	}

	FrameLoopResult RunFrameLoop(const BenchConfig& config, IFrameLoopWindow& window,
		IRenderBackend& backend, MarkerHub& markers, RunStatusBoard& board)
	{
		FrameLoopResult result;
		const double intervalSec = config.frameSleepMs > 0 ? (config.frameSleepMs / 1000.0) : 0.0;
		pmon::util::IntervalWaiter::Options waitOpts{};
		if (!config.coarseFrameWait) {
			waitOpts.mechanism = pmon::util::WaitMechanism::HighPrecisionTimer;
			waitOpts.spinBufferSeconds = 0.0005;
		}
		pmon::util::IntervalWaiter waiter(intervalSec, waitOpts);
		pmon::util::QpcTimer sceneClock;

		board.Update([&](RunStatus& s) {
			s.state = RunState::Running;
			s.api = ApiName(config.api);
			s.mode = ModeName(config.mode);
		});

		const uint32_t frameCount = config.frameCount.value_or(UINT32_MAX);
		try {
			for (uint32_t frameIndex = 0;
				frameIndex < frameCount && !board.QuitRequested();) {
				if (!window.ProcessMessages()) {
					board.RequestQuit();
					break;
				}
				if (!backend.PrepareFrame(window.IsRenderActive())) {
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}

				const uint32_t frameId = frameIndex + 1;
				if (config.frameSleepMs > 0) {
					markers.Mark(FrameMarker::SleepStart, frameId);
					waiter.Wait();
					markers.Mark(FrameMarker::SleepEnd, frameId);
				}

				backend.WaitForFrameSlot();
				markers.BeginFrame(frameId);

				markers.Mark(FrameMarker::SimulationStart, frameId);
				bool mouseInjected = false;
				bool keyboardInjected = false;
				if (config.workload.injectMouseClick) {
					mouseInjected = window.InjectMouseClick();
					if (!mouseInjected) {
						throw std::runtime_error("mouse input injection failed");
					}
				}
				if (config.workload.injectKeyboard) {
					keyboardInjected = window.InjectKeyboardTap();
					if (!keyboardInjected) {
						throw std::runtime_error("keyboard input injection failed");
					}
				}
				if (mouseInjected || keyboardInjected) {
					markers.InputSample(frameId, mouseInjected, keyboardInjected);
				}
				BusyWaitMs(config.workload.cpuBusyMs);
				markers.Mark(FrameMarker::SimulationEnd, frameId);

				FrameInfo info{};
				info.frameId = frameId;
				info.clears = config.workload.clearsPerFrame;
				info.psoCompiles = config.workload.psoCompilesPerFrame;
				info.sceneTimeSec = (float)sceneClock.Peek();

				markers.Mark(FrameMarker::RenderSubmitStart, frameId);
				backend.Render(info);
				markers.Mark(FrameMarker::RenderSubmitEnd, frameId);

				markers.Mark(FrameMarker::PresentStart, frameId);
				const auto presentResult = backend.Present(info);
				markers.Mark(FrameMarker::PresentEnd, frameId);

				++result.appFrames;
				result.presents += presentResult.framesPresented;
				board.Update([&](RunStatus& s) {
					s.appFrames = result.appFrames;
					s.presents = result.presents;
					s.xessFgActive = presentResult.frameGenerationEnabled;
				});
				++frameIndex;
			}
		}
		catch (const std::exception& ex) {
			result.ok = false;
			result.error = ex.what();
		}

		board.Update([&](RunStatus& s) {
			s.appFrames = result.appFrames;
			s.presents = result.presents;
			s.state = result.ok ? RunState::Completed : RunState::Failed;
			s.error = result.error;
		});
		return result;
	}
}
