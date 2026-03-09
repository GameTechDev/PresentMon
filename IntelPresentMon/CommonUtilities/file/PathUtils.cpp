#include "PathUtils.h"
#include "../win/Utilities.h"


namespace pmon::util::file
{
	std::filesystem::path GetWorkingDirectory()
	{
		return std::filesystem::current_path();
	}

	void SetWorkingDirectory(const std::filesystem::path& path)
	{
		std::filesystem::current_path(path);
	}

	std::filesystem::path GetCurrentProcessModulePath()
	{
		return win::GetExecutableModulePath();
	}

	std::filesystem::path GetCurrentProcessModuleDirectory()
	{
		return GetCurrentProcessModulePath().parent_path();
	}

	ScopedWorkingDirectory::ScopedWorkingDirectory(const std::filesystem::path& path)
	{
		try {
			oldPath_ = GetWorkingDirectory();
			if (!path.empty()) {
				SetWorkingDirectory(path);
				changed_ = true;
			}
		}
		catch (...) {}
	}

	ScopedWorkingDirectory::~ScopedWorkingDirectory()
	{
		if (changed_) {
			try {
				SetWorkingDirectory(oldPath_);
			}
			catch (...) {}
		}
	}
}
