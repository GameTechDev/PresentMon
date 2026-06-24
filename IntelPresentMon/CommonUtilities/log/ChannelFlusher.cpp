#include "ChannelFlusher.h"
#include "IChannel.h"
#include "../Exception.h"
#include "PanicLogger.h"
#include "../str/String.h"
#include <chrono>

namespace pmon::util::log
{
	using namespace std::literals;
	ChannelFlusher::ChannelFlusher(std::weak_ptr<IEntrySink> pChan, bool initiallyEnabled)
		:
		pChan_{ std::move(pChan) },
		enabled_{ initiallyEnabled },
		stateChangeEvent_{ false, false }
	{
		worker_ = mt::Thread{ "log-flush", [this] {
			try {
				auto flush = [this] {
					if (auto pChan = pChan_.lock()) {
						pChan->Flush();
						return true;
					}
					return false;
				};
				while (true) {
					if (!enabled_.load(std::memory_order_acquire)) {
						if (flushBeforeSuspend_.exchange(false, std::memory_order_acq_rel)) {
							if (!flush()) {
								return;
							}
						}
						if (win::WaitAnyEvent(exitEvent_, stateChangeEvent_) == 0) {
							return;
						}
						continue;
					}

					if (auto waitResult = win::WaitAnyEventFor(500ms, exitEvent_, stateChangeEvent_)) {
						if (*waitResult == 0) {
							return;
						}
						continue;
					}

					if (enabled_.load(std::memory_order_acquire)) {
						if (!flush()) {
							return;
						}
					}
				}
			}
			catch (...) {
				pmlog_panic_(ReportException());
			}
		} };
	}
	ChannelFlusher::~ChannelFlusher()
	{
		pmquell(exitEvent_.Set());
	}
	void ChannelFlusher::SetEnabled(bool enabled)
	{
		const auto wasEnabled = enabled_.exchange(enabled, std::memory_order_acq_rel);
		if (wasEnabled != enabled) {
			if (!enabled) {
				flushBeforeSuspend_.store(true, std::memory_order_release);
			}
			pmquell(stateChangeEvent_.Set());
		}
	}
}
