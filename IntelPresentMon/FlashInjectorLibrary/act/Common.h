#pragma once
#include <string>
#include <cstdint>
#include <format>

namespace inj::act
{
	inline std::string MakePipeName(uint32_t pid)
	{
		return std::format(R"(\\.\pipe\ipm-injection-{})", pid);
	}
}