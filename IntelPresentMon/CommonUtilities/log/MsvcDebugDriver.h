#pragma once
#include "IDriver.h"
#include <memory>

namespace pmon::util::log
{
	class ITextFormatter;

	class MsvcDebugDriver : public ITextDriver
	{
	public:
		MsvcDebugDriver(std::shared_ptr<ITextFormatter> pFormatter = {});
		void Submit(const Entry&) override;
		void SetFormatter(std::shared_ptr<ITextFormatter> pFormatter) override;
		std::shared_ptr<ITextFormatter> GetFormatter() const override;
		void Flush() override;
	private:
		std::shared_ptr<ITextFormatter> pFormatter_;
	};
}
