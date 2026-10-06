#pragma once
#include "Lifecycle.h"
#include <atomic>
#include <mutex>

namespace pmon::ipc::act
{
	// Dispatch acceptance and the Running -> Stopping transition share this mutex.
	// The coroutine is queued before the lock is released, so StopSequence cannot
	// be ordered ahead of an accepted dispatch, and a rejected dispatch is never queued.
	class AdmissionGate
	{
	public:
		template<class Queue>
		bool TryAdmit(const Lifecycle& lifecycle, Queue&& queue)
		{
			std::lock_guard lock{ mutex_ };
			if (!lifecycle.IsRunning()) {
				return false;
			}
			if (hook_) {
				hook_();
			}
			inflight_.fetch_add(1, std::memory_order_acq_rel);
			try {
				queue();
			}
			catch (...) {
				inflight_.fetch_sub(1, std::memory_order_acq_rel);
				throw;
			}
			return true;
		}
		template<class F>
		void Lock(F&& fn)
		{
			std::lock_guard lock{ mutex_ };
			fn();
		}
		void CompleteOne() noexcept
		{
			inflight_.fetch_sub(1, std::memory_order_acq_rel);
		}
		int Inflight() const noexcept
		{
			return inflight_.load(std::memory_order_acquire);
		}
		// Test seam. Invoked on the admitting thread while the gate is held and the
		// phase is still Running, before the dispatch coroutine is queued.
		// The hook must not re-enter the gate.
		void SetHookForTest(void (*hook)())
		{
			hook_ = hook;
		}
	private:
		std::mutex mutex_;
		std::atomic<int> inflight_{ 0 };
		void (*hook_)() = nullptr;
	};
}
