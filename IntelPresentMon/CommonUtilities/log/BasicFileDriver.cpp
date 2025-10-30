#include "BasicFileDriver.h"
#include "ITextFormatter.h"
#include "PanicLogger.h"
#include <cassert>

namespace pmon::util::log
{
	BasicFileDriver::BasicFileDriver(
		std::shared_ptr<ITextFormatter> pFormatter,
		std::shared_ptr<IFileStrategy> pFileStrategy)
		:
		pFormatter_{ std::move(pFormatter) },
		pFileStrategy_{ std::move(pFileStrategy) }
	{}
	void BasicFileDriver::Submit(const Entry& e)
	{
		if (pFormatter_ && pFileStrategy_) {
			auto pFile = pFileStrategy_->AddLine();
			*pFile << pFormatter_->Format(e);
		}
		else {
			if (!pFormatter_) {
				pmlog_panic_("BasicFileDriver submitted to without a formatter set");
			}
			if (!pFileStrategy_) {
				pmlog_panic_("BasicFileDriver submitted to without a file strategy set");
			}
		}
	}
	void BasicFileDriver::SetFormatter(std::shared_ptr<ITextFormatter> pFormatter)
	{
		pFormatter_ = std::move(pFormatter);
	}
	std::shared_ptr<ITextFormatter> BasicFileDriver::GetFormatter() const
	{
		return pFormatter_;
	}
	void BasicFileDriver::SetFileStrategy(std::shared_ptr<IFileStrategy> pFileStrategy)
	{
		pFileStrategy_ = std::move(pFileStrategy);
	}
	void BasicFileDriver::Flush()
	{
		if (pFileStrategy_) {
			if (auto pStream = pFileStrategy_->GetFileStream()) {
				pStream->flush();
			}
		}
		else {
			pmlog_panic_("BasicFileDriver flushed without a file strategy set");
		}
	}
}