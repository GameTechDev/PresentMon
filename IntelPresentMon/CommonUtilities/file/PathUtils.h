#pragma once
#include <filesystem>


namespace pmon::util::file
{
	std::filesystem::path GetWorkingDirectory();
	void SetWorkingDirectory(const std::filesystem::path& path);
	std::filesystem::path GetCurrentProcessModulePath();
	std::filesystem::path GetCurrentProcessModuleDirectory();

	class ScopedWorkingDirectory
	{
	public:
		explicit ScopedWorkingDirectory(const std::filesystem::path& path);
		ScopedWorkingDirectory(const ScopedWorkingDirectory&) = delete;
		ScopedWorkingDirectory& operator=(const ScopedWorkingDirectory&) = delete;
		ScopedWorkingDirectory(ScopedWorkingDirectory&&) = delete;
		ScopedWorkingDirectory& operator=(ScopedWorkingDirectory&&) = delete;
		~ScopedWorkingDirectory();

	private:
		std::filesystem::path oldPath_;
		bool changed_ = false;
	};
}
