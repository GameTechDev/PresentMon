#pragma once

#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <string>

namespace LibraryInject
{
	void Attach(uint32_t processId, const std::filesystem::path& dllPath);
}