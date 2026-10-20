#pragma once
#include "../win/WinAPI.h"
#include "../win/Event.h"
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <chrono>
#include <optional>


namespace pmon::util::pipe
{
	namespace as = boost::asio;

	class ManualAsyncEvent
	{
		using Timer = as::steady_timer;
	public:
		ManualAsyncEvent(as::io_context& ctx)
			:
			timer_{ ctx, Timer::duration::max() }
		{}
		// Returns true once signalled, false only if the optional timeout elapsed first.
		// Returns immediately if already signalled, so a waiter cannot miss a signal
		// raised before it started waiting.
		as::awaitable<bool> AsyncWait(std::optional<std::chrono::milliseconds> timeout = {})
		{
			if (timeout) {
				timer_.expires_after(*timeout);
			}
			while (!signalled_) {
				auto ec = boost::system::error_code{};
				co_await timer_.async_wait(as::redirect_error(as::use_awaitable, ec));
				// Signal() cancels the timer; an abort with signalled_ set is success.
				if (signalled_) {
					co_return true;
				}
				if (ec) {
					throw boost::system::system_error{ ec };
				}
				if (timeout) {
					co_return false;
				}
				// without a deadline the timer is parked at time max, so a clean
				// completion is a spurious wake and is simply waited out again
			}
			co_return true;
		}
		void Signal()
		{
			signalled_ = true;
			timer_.cancel();
		}
	private:
		Timer timer_;
		bool signalled_ = false;
	};
}