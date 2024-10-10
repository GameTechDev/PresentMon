#include "ChannelFlusher.h"
#include "IChannel.h"
#include "../Exception.h"
#include "PanicLogger.h"
#include "../str/String.h"
#include <chrono>

namespace pmon::util::log
{
	using namespace std::literals;
	ChannelFlusher::ChannelFlusher(std::weak_ptr<IEntrySink> pChan)
		:
		pChan_{ std::move(pChan) }
	{
		worker_ = mt::Thread{ "log-flush", [this] {
			try {
				while (!win::WaitAnyEventFor(500ms, exitEvent_)) {
					pChan_.lock()->Flush();
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
}