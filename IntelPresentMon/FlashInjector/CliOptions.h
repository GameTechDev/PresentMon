#pragma once
#include "../CommonUtilities/cli/CliFramework.h"
#include "../CommonUtilities/log/Level.h"

namespace clio
{
	using namespace pmon::util::cli;
	using namespace pmon::util::log;
	using namespace std::literals;
	struct Options : public OptionsBase<Options>
	{
		static constexpr const char* description = "Helper utility for drawing a rectangular flash within a target process via injection";
		static constexpr const char* name = "FlashInjector.exe";
	};
}