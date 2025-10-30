#include "StdioDriver.h"
#include "ITextFormatter.h"
#include "PanicLogger.h"
#include <iostream>

namespace pmon::util::log
{
	StdioDriver::StdioDriver(std::shared_ptr<ITextFormatter> pFormatter, bool useStderr)
		:
		pFormatter_{ std::move(pFormatter) },
		useStderr_{ useStderr }
	{}
	void StdioDriver::Submit(const Entry& e)
	{
		if (pFormatter_) {
			if (useStderr_) {
				std::cerr << pFormatter_->Format(e);
			}
			else {
				std::cout << pFormatter_->Format(e);
			}
		}
		else {
			pmlog_panic_("BasicFileDriver submitted to without a formatter set");
		}
	}
	void StdioDriver::SetFormatter(std::shared_ptr<ITextFormatter> pFormatter)
	{
		pFormatter_ = std::move(pFormatter);
	}
	std::shared_ptr<ITextFormatter> StdioDriver::GetFormatter() const
	{
		return pFormatter_;
	}
	void StdioDriver::Flush()
	{
		if (useStderr_) {
			std::cerr << std::flush;
		}
		else {
			std::cout << std::flush;
		}
	}
}