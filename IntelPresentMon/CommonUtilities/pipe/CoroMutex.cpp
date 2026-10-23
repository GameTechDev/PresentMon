#include "CoroMutex.h"
#include <algorithm>


namespace pmon::util::pipe
{
	as::awaitable<CoroMutex::LockResult> CoroMutex::Lock(std::optional<std::chrono::milliseconds> timeout)
	{
		if (counter_++ == 0) {
			co_return LockResult::Acquired;
		}

		auto ec = boost::system::error_code{};
		auto granted = std::make_shared<bool>(false);
		Timer timer{ ctx_ };
		if (timeout) {
			timer.expires_after(*timeout);
		}
		else {
			timer.expires_after(Timer::duration::max());
		}
		waiters_.push_back(Waiter{ &timer, granted });
		co_await timer.async_wait(as::redirect_error(as::use_awaitable, ec));
		// Granted wins even when the timeout fired too: Unlock already removed this waiter and
		// passed it the count, so it owns the mutex and its guard must release it.
		if (*granted) {
			holdoff_ = false;
			co_return LockResult::Acquired;
		}
		// Still queued means nobody removed it, so the timeout woke it: remove it and undo its
		// count here. Not queued means CancelWaiters removed it and already undid its count.
		if (const auto i = std::ranges::find(waiters_, &timer, &Waiter::timer); i != waiters_.end()) {
			waiters_.erase(i);
			--counter_;
			co_return LockResult::TimedOut;
		}
		co_return LockResult::Cancelled;
	}
	bool CoroMutex::TryLock()
	{
		if (counter_ == 0) {
			counter_++;
			return true;
		}
		return false;
	}
	void CoroMutex::Unlock()
	{
		// guard against unlocking twice from same coro or unlocked when there are no waiters
		if (counter_ == 0 || holdoff_) {
			return;
		}

		--counter_;

		if (waiters_.empty()) {
			return;
		}

		holdoff_ = true;
		if (waiters_.front().granted) {
			*waiters_.front().granted = true;
		}
		waiters_.front().timer->cancel();
		waiters_.pop_front();
	}
	void CoroMutex::CancelWaiters()
	{
		auto pending = std::move(waiters_);
		waiters_.clear();
		for (auto& waiter : pending) {
			if (counter_ > 0) {
				--counter_;
			}
			if (waiter.timer) {
				waiter.timer->cancel();
			}
		}
	}


	CoroLockGuard::CoroLockGuard(CoroLockGuard&& other)
		: pMtx_{ std::exchange(other.pMtx_, nullptr) },
		result_{ other.result_ }
	{}
	CoroLockGuard& CoroLockGuard::operator=(CoroLockGuard&& rhs)
	{
		if (this == &rhs) {
			return *this;
		}
		// release any mutex already held before taking ownership of rhs
		if (pMtx_) {
			pMtx_->Unlock();
		}
		pMtx_ = std::exchange(rhs.pMtx_, nullptr);
		result_ = rhs.result_;
		return *this;
	}
	CoroLockGuard::~CoroLockGuard()
	{
		if (pMtx_) {
			pMtx_->Unlock();
		}
	}


	as::awaitable<CoroLockGuard> CoroLock(CoroMutex& mtx, std::optional<std::chrono::milliseconds> timeout)
	{
		const auto result = co_await mtx.Lock(timeout);
		if (result != CoroMutex::LockResult::Acquired) {
			co_return CoroLockGuard{ result };
		}
		co_return CoroLockGuard{ mtx };
	}
}