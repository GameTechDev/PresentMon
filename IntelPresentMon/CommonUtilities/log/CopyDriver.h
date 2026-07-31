#pragma once
#include "IDriver.h"
#include "IChannel.h"


namespace pmon::util::log
{
	using HostLogEntryForwardFn = void(*)(IChannel*, const Entry&) noexcept;

	class CopyDriver : public IDriver
	{
	public:
		CopyDriver(IChannel* pChannel, HostLogEntryForwardFn forwardFn) noexcept;
		void Submit(const Entry&) override;
		void Flush() override;
	private:
		IChannel* pChannel_;
		HostLogEntryForwardFn forwardFn_;
	};
}
