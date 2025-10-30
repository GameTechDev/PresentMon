#pragma once
#include "IChannelComponent.h"
#include <memory>

namespace pmon::util::log
{
	struct Entry;
	class ITextFormatter;

	class IDriver : public IChannelComponent
	{
	public:
		virtual void Submit(const Entry&) = 0;
		virtual void Flush() = 0;
	};

	class ITextDriver : public IDriver
	{
	public:
		virtual void SetFormatter(std::shared_ptr<ITextFormatter>) = 0;
		virtual std::shared_ptr<ITextFormatter> GetFormatter() const = 0;
	};
}