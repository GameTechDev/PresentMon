#include "CoroMutex.h"


namespace pmon::util::pipe
{
	as::awaitable<bool> CoroMutex::Lock()
	{
		if (counter_++ == 0) {
			co_return true;
		}

		auto ec = boost::system::error_code{};
		auto granted = std::make_shared<bool>(false);
		Timer timer{ ctx_ };
		timer.expires_after(Timer::duration::max());
		waiters_.push_back(Waiter{ &timer, granted });
		co_await timer.async_wait(as::redirect_error(as::use_awaitable, ec));
		if (!*granted) {
			co_return false;
		}
		holdoff_ = false;
		co_return true;
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
		: pMtx_{ std::exchange(other.pMtx_, nullptr) }
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
		return *this;
	}
	CoroLockGuard::~CoroLockGuard()
	{
		if (pMtx_) {
			pMtx_->Unlock();
		}
	}


	as::awaitable<CoroLockGuard> CoroLock(CoroMutex& mtx)
	{
		if (!co_await mtx.Lock()) {
			co_return CoroLockGuard{};
		}
		co_return CoroLockGuard{ mtx };
	}
}