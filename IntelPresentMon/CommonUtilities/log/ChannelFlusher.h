#pragma once
#include <memory>
#include <atomic>
#include "../mt/Thread.h"
#include "../win/Event.h"
#include "IChannelObject.h"

namespace pmon::util::log
{
	class IEntrySink;

	class ChannelFlusher : public IChannelObject
	{
	public:
		ChannelFlusher(std::weak_ptr<IEntrySink> pChan, bool initiallyEnabled = true);
		~ChannelFlusher();

		ChannelFlusher(const ChannelFlusher&) = delete;
		ChannelFlusher & operator=(const ChannelFlusher&) = delete;
		ChannelFlusher(ChannelFlusher&&) = delete;
		ChannelFlusher & operator=(ChannelFlusher&&) = delete;

		void SetEnabled(bool enabled);

	private:
		std::weak_ptr<IEntrySink> pChan_;
		std::atomic<bool> enabled_;
		std::atomic<bool> flushBeforeSuspend_ = false;
		mt::Thread worker_;
		win::Event exitEvent_;
		win::Event stateChangeEvent_;
	};
}
