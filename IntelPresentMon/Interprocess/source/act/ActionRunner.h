#pragma once
#include "SymmetricActionConnector.h"
#include "../../../CommonUtilities/log/Log.h"
#include <cassert>
#include <exception>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace pmon::ipc::act
{
	// Plumbing shared by SymmetricActionClient and SymmetricActionServer. Each keeps its
	// io thread state in a State object (held by shared_ptr) that has lifecycle, admission,
	// ioctx and OnIoThread().

	// Name for the io thread: the pipe name without the \\.\pipe\ prefix.
	inline std::string MakeWorkerName(const std::string& pipeName)
	{
		constexpr std::string_view prefix = R"(\\.\pipe\)";
		if (pipeName.starts_with(prefix)) {
			return pipeName.substr(prefix.size());
		}
		return pipeName;
	}

	// Queues coro on the io thread if the lifecycle is still Running. The admission gate
	// counts it as in flight until it finishes, so shutdown waits for it. Returns false,
	// without queuing anything, once shutdown has begun.
	template<class State, class Coro>
	bool SpawnAdmitted(const std::shared_ptr<State>& state, Coro&& coro)
	{
		return state->admission.TryAdmit(state->lifecycle, [&] {
			as::co_spawn(state->ioctx, std::forward<Coro>(coro), [state](std::exception_ptr) {
				state->admission.CompleteOne();
			});
		});
	}

	// Destructor body for the client and server: begin shutdown, wait for Stopped, then
	// join the io thread. Every failure terminates instead of detaching the thread,
	// because it could still touch borrowed execution-context state after we return.
	template<class State>
	void StopAndJoinRunner(std::shared_ptr<State>& state, std::thread& runner, const char* owner)
	{
		if (!state) {
			return;
		}
		if (state->OnIoThread()) {
			pmlog_error(std::format("Destroying the {} on its io thread is unsupported", owner));
			assert(false && "Destroying an action client or server on its io thread");
			util::log::GetDefaultChannel()->Flush();
			std::terminate();
		}
		try {
			state->BeginShutdown();
		}
		catch (...) {
			pmlog_error(util::ReportException(std::format("Failure starting {} shutdown", owner)));
			util::log::GetDefaultChannel()->Flush();
			std::terminate();
		}
		if (!state->lifecycle.WaitFor(kShutdownTimeout)) {
			pmlog_error(std::format("{} runner did not stop after cancellation; refusing to detach", owner));
			util::log::GetDefaultChannel()->Flush();
			std::terminate();
		}
		if (runner.joinable()) {
			runner.join();
		}
		state.reset();
	}
}
