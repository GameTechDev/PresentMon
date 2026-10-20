#pragma once
#include "../win/WinAPI.h"
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <chrono>
#include <deque>
#include <memory>
#include <optional>

namespace pmon::util::pipe
{
	namespace as = boost::asio;

	class CoroMutex
	{
	public:
		using Timer = as::steady_timer;

		CoroMutex(as::io_context& ctx) : ctx_{ ctx } {}
		CoroMutex(const CoroMutex&) = delete;
		CoroMutex& operator=(const CoroMutex&) = delete;
		CoroMutex(CoroMutex&&) = default;
		CoroMutex& operator=(CoroMutex&&) = default;
		~CoroMutex() = default;

		enum class LockResult
		{
			// the caller owns the mutex and must Unlock it (CoroLockGuard does)
			Acquired,
			// CancelWaiters woke the wait (the pipe was closed); the caller does not own it
			Cancelled,
			// the timeout passed first; the caller does not own it
			TimedOut,
		};
		// Waits for the mutex, up to timeout when one is given.
		// counter_ counts the holder plus every queued waiter. A waiter is removed from waiters_
		// exactly once, and whoever removes it accounts for its count: Unlock hands the count to
		// it as the new holder, CancelWaiters and a timed-out waiter decrement it.
		as::awaitable<LockResult> Lock(std::optional<std::chrono::milliseconds> timeout = {});
		bool TryLock();
		void Unlock();
		// Wakes every coroutine blocked in Lock without granting the mutex.
		void CancelWaiters();
	private:
		struct Waiter
		{
			Timer* timer = nullptr;
			std::shared_ptr<bool> granted;
		};
		as::io_context& ctx_;
		std::deque<Waiter> waiters_;
		int counter_ = 0;
		// used to check for the the same coro unlocking 2+ times
		bool holdoff_ = false;
	};

	class CoroLockGuard
	{
		friend as::awaitable<CoroLockGuard> CoroLock(CoroMutex& mtx, std::optional<std::chrono::milliseconds> timeout);
		using CoroMutexType = CoroMutex;
	public:
		CoroLockGuard() = default;
		CoroLockGuard(const CoroLockGuard&) = delete;
		CoroLockGuard& operator=(const CoroLockGuard&) = delete;
		// true when the mutex is held, that is when the result was Acquired
		explicit operator bool() const { return pMtx_ != nullptr; }
		CoroMutex::LockResult GetResult() const { return result_; }
		CoroLockGuard(CoroLockGuard&& other);
		CoroLockGuard& operator=(CoroLockGuard&& rhs);
		~CoroLockGuard();
	private:
		// functions
		CoroLockGuard(CoroMutexType& mtx) : pMtx_{ &mtx }, result_{ CoroMutex::LockResult::Acquired } {}
		CoroLockGuard(CoroMutex::LockResult failure) : result_{ failure } {}
		// data
		CoroMutexType* pMtx_ = nullptr;
		CoroMutex::LockResult result_ = CoroMutex::LockResult::Cancelled;
	};

	// The guard owns the mutex only when GetResult() is Acquired, and then releases it when
	// destroyed. With a timeout the wait gives up at that deadline (TimedOut).
	as::awaitable<CoroLockGuard> CoroLock(CoroMutex& mtx, std::optional<std::chrono::milliseconds> timeout = {});
}