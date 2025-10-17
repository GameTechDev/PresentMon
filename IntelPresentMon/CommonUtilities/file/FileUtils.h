#pragma once
#include <filesystem>
#include <vector>


namespace pmon::util::file
{
	std::vector<std::filesystem::path> FindFilesMatchingPattern(const std::filesystem::path& dir, const std::string& regex);
}