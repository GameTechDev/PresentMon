#pragma once
#include "IDriver.h"
#include <memory>

namespace pmon::util::log
{
	class ITextFormatter;

	class StdioDriver : public ITextDriver
	{
	public:
		StdioDriver(std::shared_ptr<ITextFormatter> pFormatter = {}, bool useStderr = true);
		void Submit(const Entry&) override;
		void SetFormatter(std::shared_ptr<ITextFormatter> pFormatter) override;
		std::shared_ptr<ITextFormatter> GetFormatter() const override;
		void Flush() override;
	private:
		std::shared_ptr<ITextFormatter> pFormatter_;
		bool useStderr_ = true;
	};
}
