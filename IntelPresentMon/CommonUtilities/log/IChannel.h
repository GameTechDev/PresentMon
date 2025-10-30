#pragma once
#include <memory>
#include <string>

namespace pmon::util::log
{
	struct Entry;
	class IChannelComponent;

	class IEntrySink
	{
	public:
		virtual ~IEntrySink() = default;
		virtual void Submit(Entry&&) noexcept = 0;
		virtual void Submit(const Entry&) noexcept = 0;
		virtual void Flush() = 0;
	};

	class IChannel : public IEntrySink
	{
	public:
		virtual void AttachComponent(std::shared_ptr<IChannelComponent>, std::string = {}) = 0;
		virtual std::shared_ptr<IChannelComponent> GetComponent(std::string tag) const = 0;
		virtual void FlushEntryPointExit() = 0;
	};
}