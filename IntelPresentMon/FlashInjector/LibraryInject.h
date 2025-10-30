#pragma once

#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <string>

namespace LibraryInject
{
	using ProcessMap = std::unordered_map<uint32_t, std::string>;
	void Attach(uint32_t processId, const std::filesystem::path& dllPath);
	ProcessMap GetProcessNames();
}