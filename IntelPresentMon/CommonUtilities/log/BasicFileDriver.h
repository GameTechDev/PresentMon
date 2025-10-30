#pragma once
#include "IDriver.h"
#include "IFileStrategy.h"
#include <memory>

namespace pmon::util::log
{
	class BasicFileDriver : public ITextDriver
	{
	public:
		BasicFileDriver(std::shared_ptr<ITextFormatter> pFormatter = {},
			std::shared_ptr<IFileStrategy> pFileStrategy = {});
		void Submit(const Entry&) override;
		void SetFormatter(std::shared_ptr<ITextFormatter> pFormatter) override;
		std::shared_ptr<ITextFormatter> GetFormatter() const override;
		void SetFileStrategy(std::shared_ptr<IFileStrategy> pFileStrategy);
		void Flush() override;
	private:
		std::shared_ptr<IFileStrategy> pFileStrategy_;
		std::shared_ptr<ITextFormatter> pFormatter_;
	};
}

