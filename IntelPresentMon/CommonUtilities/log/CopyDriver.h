#pragma once
#include "IDriver.h"
#include "IChannel.h"


namespace pmon::util::log
{
	class CopyDriver : public IDriver
	{
	public:
		explicit CopyDriver(IChannel* pChannel) noexcept;
		void Submit(const Entry&) override;
		void Flush() override;
	private:
		IChannel* pChannel_;
	};
}
