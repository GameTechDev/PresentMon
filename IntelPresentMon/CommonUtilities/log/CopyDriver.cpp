#include "CopyDriver.h"

namespace pmon::util::log
{
	CopyDriver::CopyDriver(IChannel* pChannel, HostLogEntryForwardFn forwardFn) noexcept
		:
		pChannel_{ pChannel },
		forwardFn_{ forwardFn }
	{}
	void CopyDriver::Submit(const Entry& e)
	{
		if (pChannel_) {
			if (forwardFn_) {
				forwardFn_(pChannel_, e);
			}
			else {
				pChannel_->Submit(e);
			}
		}
	}
	void CopyDriver::Flush()
	{
		if (pChannel_) {
			pChannel_->Flush();
		}
	}
}