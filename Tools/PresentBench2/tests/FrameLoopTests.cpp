// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../Backend.h"
#include "../FrameLoop.h"
#include "../markers/FrameMarkers.h"
#include <algorithm>
#include <format>
#include <gtest/gtest.h>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
	class RecordingSink final : public pb2::IFrameMarkerSink
	{
	public:
		std::vector<std::string> events;
		void BeginFrame(uint32_t frameId) override
		{
			events.push_back(std::format("BeginFrame:{}", frameId));
		}
		void Mark(pb2::FrameMarker marker, uint32_t frameId) override
		{
			events.push_back(std::format("Mark:{}:{}", (int)marker, frameId));
		}
		void InputSample(uint32_t frameId, bool mouse, bool keyboard) override
		{
			events.push_back(std::format("Input:{}:{}:{}", frameId, mouse, keyboard));
		}
	};

	class FakeBackend final : public pb2::IRenderBackend
	{
	public:
		std::vector<std::string> events;
		bool fgActive = false;
		bool throwOnRender = false;
		uint32_t prepareFailures = 0;
		uint32_t prepareCalls = 0;
		bool PrepareFrame(bool windowActive) override
		{
			++prepareCalls;
			return windowActive && prepareCalls > prepareFailures;
		}
		void WaitForFrameSlot() override { events.push_back("WaitForFrameSlot"); }
		void Render(const pb2::FrameInfo& info) override
		{
			if (throwOnRender) {
				throw std::runtime_error("render failed");
			}
			events.push_back(std::format("Render:{}", info.frameId));
		}
		pb2::PresentResult Present(const pb2::FrameInfo& info) override
		{
			events.push_back(std::format("Present:{}", info.frameId));
			return { fgActive ? 2u : 1u, fgActive };
		}
	};

	class FakeWindow final : public pb2::IFrameLoopWindow
	{
	public:
		bool mouseResult = true;
		bool keyboardResult = true;
		bool renderActive = true;
		uint32_t processCalls = 0;
		uint32_t inactiveProcessCalls = 0;

		bool ProcessMessages() override
		{
			++processCalls;
			return true;
		}
		bool IsRenderActive() const override
		{
			return renderActive && processCalls > inactiveProcessCalls;
		}
		bool InjectMouseClick() override { return mouseResult; }
		bool InjectKeyboardTap() override { return keyboardResult; }
	};
}

TEST(FrameLoop, MarkerOrderAndWaitBeforeBegin)
{
	pb2::BenchConfig config;
	config.frameCount = 2;
	config.frameSleepMs = 0;

	FakeBackend backend;
	auto recording = std::make_unique<RecordingSink>();
	auto* sinkPtr = recording.get();
	pb2::MarkerHub hub;
	hub.Add(std::move(recording));
	pb2::RunStatusBoard board;
	FakeWindow window;

	const auto result = pb2::RunFrameLoop(config, window, backend, hub, board);
	EXPECT_TRUE(result.ok);
	EXPECT_EQ(result.appFrames, 2u);
	ASSERT_GE(backend.events.size(), 4u);
	EXPECT_EQ(backend.events[0], "WaitForFrameSlot");
	EXPECT_EQ(backend.events[1], "Render:1");
	EXPECT_EQ(backend.events[2], "Present:1");
	EXPECT_EQ(backend.events[3], "WaitForFrameSlot");

	ASSERT_GE(sinkPtr->events.size(), 3u);
	EXPECT_EQ(sinkPtr->events[0], "BeginFrame:1");
	// SimulationStart == enum value 2
	EXPECT_EQ(sinkPtr->events[1], "Mark:2:1");
	EXPECT_EQ(board.Get().state, pb2::RunState::Completed);
}

TEST(FrameLoop, FrameGenerationPresentCount)
{
	pb2::BenchConfig config;
	config.frameCount = 1;
	config.frameSleepMs = 0;
	FakeBackend backend;
	backend.fgActive = true;
	pb2::MarkerHub hub;
	pb2::RunStatusBoard board;
	FakeWindow window;
	const auto result = pb2::RunFrameLoop(config, window, backend, hub, board);
	EXPECT_TRUE(result.ok);
	EXPECT_EQ(result.presents, 2u);
	EXPECT_TRUE(board.Get().xessFgActive);
}

TEST(FrameLoop, BackendPauseDoesNotConsumeApplicationFrames)
{
	pb2::BenchConfig config;
	config.frameCount = 1;
	config.frameSleepMs = 0;
	FakeBackend backend;
	backend.prepareFailures = 2;
	pb2::MarkerHub hub;
	pb2::RunStatusBoard board;
	FakeWindow window;

	const auto result = pb2::RunFrameLoop(config, window, backend, hub, board);
	EXPECT_TRUE(result.ok);
	EXPECT_EQ(result.appFrames, 1u);
	EXPECT_EQ(result.presents, 1u);
	EXPECT_EQ(backend.prepareCalls, 3u);
	ASSERT_EQ(backend.events.size(), 3u);
	EXPECT_EQ(backend.events[0], "WaitForFrameSlot");
	EXPECT_EQ(backend.events[1], "Render:1");
	EXPECT_EQ(backend.events[2], "Present:1");
}

TEST(FrameLoop, InactiveFullscreenPauseDoesNotConsumeApplicationFrames)
{
	pb2::BenchConfig config;
	config.frameCount = 1;
	config.frameSleepMs = 0;
	FakeBackend backend;
	pb2::MarkerHub hub;
	pb2::RunStatusBoard board;
	FakeWindow window;
	window.inactiveProcessCalls = 1;

	const auto result = pb2::RunFrameLoop(config, window, backend, hub, board);
	EXPECT_TRUE(result.ok);
	EXPECT_EQ(result.appFrames, 1u);
	EXPECT_EQ(backend.prepareCalls, 2u);
	EXPECT_EQ(window.processCalls, 2u);
}

TEST(FrameLoop, SleepAndInputMarkersAreEmitted)
{
	pb2::BenchConfig config;
	config.frameCount = 1;
	config.frameSleepMs = 1;
	config.coarseFrameWait = true;
	config.workload.injectMouseClick = true;
	config.workload.injectKeyboard = true;
	FakeBackend backend;
	auto recording = std::make_unique<RecordingSink>();
	auto* sinkPtr = recording.get();
	pb2::MarkerHub hub;
	hub.Add(std::move(recording));
	pb2::RunStatusBoard board;
	FakeWindow window;

	EXPECT_TRUE(pb2::RunFrameLoop(config, window, backend, hub, board).ok);
	ASSERT_GE(sinkPtr->events.size(), 4u);
	EXPECT_EQ(sinkPtr->events[0], "Mark:0:1");
	EXPECT_EQ(sinkPtr->events[1], "Mark:1:1");
	EXPECT_NE(std::find(sinkPtr->events.begin(), sinkPtr->events.end(),
		"Input:1:true:true"), sinkPtr->events.end());
}

TEST(FrameLoop, BackendFailureUpdatesStatus)
{
	pb2::BenchConfig config;
	config.frameCount = 1;
	config.frameSleepMs = 0;
	FakeBackend backend;
	backend.throwOnRender = true;
	pb2::MarkerHub hub;
	pb2::RunStatusBoard board;
	FakeWindow window;
	const auto result = pb2::RunFrameLoop(config, window, backend, hub, board);
	EXPECT_FALSE(result.ok);
	EXPECT_EQ(result.error, "render failed");
	EXPECT_EQ(board.Get().state, pb2::RunState::Failed);
	EXPECT_EQ(board.Get().error, "render failed");
}

TEST(FrameLoop, InputInjectionFailureFailsRunWithoutInputMarker)
{
	pb2::BenchConfig config;
	config.frameCount = 1;
	config.frameSleepMs = 0;
	config.workload.injectMouseClick = true;
	FakeBackend backend;
	FakeWindow window;
	window.mouseResult = false;
	auto recording = std::make_unique<RecordingSink>();
	auto* sinkPtr = recording.get();
	pb2::MarkerHub hub;
	hub.Add(std::move(recording));
	pb2::RunStatusBoard board;

	const auto result = pb2::RunFrameLoop(config, window, backend, hub, board);
	EXPECT_FALSE(result.ok);
	EXPECT_EQ(result.error, "mouse input injection failed");
	EXPECT_EQ(std::find_if(sinkPtr->events.begin(), sinkPtr->events.end(),
		[](const std::string& event) { return event.starts_with("Input:"); }),
		sinkPtr->events.end());
}

TEST(RunStatusBoard, WaitForFrameReturnsFalseWhenRunEndsFirst)
{
	pb2::RunStatusBoard board;
	std::thread finish([&] {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		board.Update([](pb2::RunStatus& status) {
			status.state = pb2::RunState::Completed;
			status.appFrames = 2;
		});
	});
	EXPECT_FALSE(board.WaitForFrame(3, 1000));
	finish.join();
}

TEST(RunStatusBoard, WaitForFrameReturnsTrueAtTarget)
{
	pb2::RunStatusBoard board;
	board.Update([](pb2::RunStatus& status) {
		status.state = pb2::RunState::Running;
		status.appFrames = 3;
	});
	EXPECT_TRUE(board.WaitForFrame(3, 0));
}

TEST(RunStatusBoard, QuitWakesFrameWaiter)
{
	pb2::RunStatusBoard board;
	bool reached = true;
	std::thread waiter([&] {
		reached = board.WaitForFrame(1, 5000);
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	board.RequestQuit();
	waiter.join();
	EXPECT_FALSE(reached);
}
