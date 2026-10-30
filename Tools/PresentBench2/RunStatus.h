// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>

namespace pb2
{
	enum class RunState
	{
		Starting,
		Running,
		Completed,
		Failed,
	};

	struct RunStatus
	{
		uint32_t pid = 0;
		RunState state = RunState::Starting;
		std::string api;
		std::string mode;
		uint32_t appFrames = 0;
		uint32_t presents = 0;
		bool xessFgActive = false;
		std::string error;
	};

	inline void to_json(nlohmann::json& j, const RunStatus& s)
	{
		const char* stateName = "Starting";
		switch (s.state) {
		case RunState::Starting: stateName = "Starting"; break;
		case RunState::Running: stateName = "Running"; break;
		case RunState::Completed: stateName = "Completed"; break;
		case RunState::Failed: stateName = "Failed"; break;
		}
		j = nlohmann::json{
			{ "pid", s.pid },
			{ "state", stateName },
			{ "api", s.api },
			{ "mode", s.mode },
			{ "appFrames", s.appFrames },
			{ "presents", s.presents },
			{ "xessFgActive", s.xessFgActive },
			{ "error", s.error },
			{ "ok", s.state == RunState::Completed },
		};
	}

	class RunStatusBoard
	{
	public:
		void Update(std::function<void(RunStatus&)> fn)
		{
			{
				std::lock_guard lock(mutex_);
				fn(status_);
			}
			cv_.notify_all();
		}

		RunStatus Get() const
		{
			std::lock_guard lock(mutex_);
			return status_;
		}

		std::string GetJson() const
		{
			nlohmann::json j = Get();
			return j.dump();
		}

		bool WaitForFrame(uint32_t appFrameId, uint32_t timeoutMs)
		{
			std::unique_lock lock(mutex_);
			cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] {
				const bool terminal = status_.state == RunState::Completed ||
					status_.state == RunState::Failed || quit_.load();
				return status_.appFrames >= appFrameId || terminal;
			});
			return status_.appFrames >= appFrameId;
		}

		void RequestQuit()
		{
			{
				std::lock_guard lock(mutex_);
				quit_ = true;
			}
			cv_.notify_all();
			if (quitEvent_) {
				SetEvent(quitEvent_);
			}
		}

		bool QuitRequested() const { return quit_.load(); }

		void SetQuitEvent(HANDLE event) { quitEvent_ = event; }

	private:
		mutable std::mutex mutex_;
		std::condition_variable cv_;
		RunStatus status_;
		std::atomic<bool> quit_{ false };
		HANDLE quitEvent_ = nullptr;
	};
}
