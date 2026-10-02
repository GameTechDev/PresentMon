#pragma once
#include <atomic>
#include <chrono>
#include <future>
#include <cstdint>

namespace pmon::ipc::act
{
	// Explicit session-owner lifecycle. io_context::stopped() is not this state:
	// it only reports an executor condition and cannot tell Stopping from Stopped.
	enum class LifecyclePhase : uint8_t
	{
		Running,
		Stopping,
		Stopped,
	};

	// The Running -> Stopping transition is owned by exactly one caller.
	// MarkStopped settles the shared future exactly once. A waiter that times out
	// leaves that future and the shutdown work with the shared state that owns them.
	class Lifecycle
	{
	public:
		Lifecycle()
			:
			stoppedFuture_{ stopped_.get_future() }
		{}
		Lifecycle(const Lifecycle&) = delete;
		Lifecycle& operator=(const Lifecycle&) = delete;

		bool TryBeginShutdown() noexcept
		{
			auto expected = LifecyclePhase::Running;
			return phase_.compare_exchange_strong(
				expected, LifecyclePhase::Stopping,
				std::memory_order_acq_rel, std::memory_order_acquire);
		}
		void MarkStopped()
		{
			phase_.store(LifecyclePhase::Stopped, std::memory_order_release);
			if (!published_.exchange(true, std::memory_order_acq_rel)) {
				stopped_.set_value();
			}
		}
		bool IsRunning() const noexcept
		{
			return Get() == LifecyclePhase::Running;
		}
		LifecyclePhase Get() const noexcept
		{
			return phase_.load(std::memory_order_acquire);
		}
		// True once Stopped has been published. False when the timeout elapses first;
		// the completion future stays owned by this object either way.
		bool WaitFor(std::chrono::milliseconds timeout) const
		{
			return stoppedFuture_.wait_for(timeout) == std::future_status::ready;
		}
	private:
		std::atomic<LifecyclePhase> phase_{ LifecyclePhase::Running };
		std::atomic<bool> published_{ false };
		std::promise<void> stopped_;
		std::shared_future<void> stoppedFuture_;
	};
}
