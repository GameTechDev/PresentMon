#pragma once
#include "../win/WinAPI.h"
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <deque>
#include <memory>

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

		// false when Close cancelled the wait. The caller does not own the mutex.
		as::awaitable<bool> Lock();
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
		friend as::awaitable<CoroLockGuard> CoroLock(CoroMutex& mtx);
		using CoroMutexType = CoroMutex;
	public:
		CoroLockGuard() = default;
		CoroLockGuard(const CoroLockGuard&) = delete;
		CoroLockGuard& operator=(const CoroLockGuard&) = delete;
		explicit operator bool() const { return pMtx_ != nullptr; }
		CoroLockGuard(CoroLockGuard&& other);
		CoroLockGuard& operator=(CoroLockGuard&& rhs);
		~CoroLockGuard();
	private:
		// functions
		CoroLockGuard(CoroMutexType& mtx) : pMtx_{ &mtx } {}
		// data
		CoroMutexType* pMtx_ = nullptr;
	};

	as::awaitable<CoroLockGuard> CoroLock(CoroMutex& mtx);
}