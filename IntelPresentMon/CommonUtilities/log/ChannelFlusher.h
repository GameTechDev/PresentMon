#pragma once
#include <memory>
#include "../mt/Thread.h"
#include "../win/Event.h"
#include "IChannelObject.h"

namespace pmon::util::log
{
	class IEntrySink;

	class ChannelFlusher : public IChannelObject
	{
	public:
		ChannelFlusher(std::weak_ptr<IEntrySink> pChan);
		~ChannelFlusher();

		ChannelFlusher(const ChannelFlusher&) = delete;
		ChannelFlusher & operator=(const ChannelFlusher&) = delete;
		ChannelFlusher(ChannelFlusher&&) = delete;
		ChannelFlusher & operator=(ChannelFlusher&&) = delete;

	private:
		std::weak_ptr<IEntrySink> pChan_;
		mt::Thread worker_;
		win::Event exitEvent_;
	};
}

