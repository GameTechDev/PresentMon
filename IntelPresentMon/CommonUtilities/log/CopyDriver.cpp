#include "CopyDriver.h"

namespace pmon::util::log
{
	CopyDriver::CopyDriver(IChannel* pChannel) noexcept
		:
		pChannel_{ pChannel }
	{}
	void CopyDriver::Submit(const Entry& e)
	{
		if (pChannel_) {
			pChannel_->Submit(e);
		}
	}
	void CopyDriver::Flush()
	{
		if (pChannel_) {
			pChannel_->Flush();
		}
	}
}