#include "SimpleFileStrategy.h"

namespace pmon::util::log
{
	SimpleFileStrategy::SimpleFileStrategy(std::filesystem::path path)
		:
		path_{ std::move(path) }
	{}
	std::shared_ptr<std::ostream> SimpleFileStrategy::AddLine()
	{
		CreateFile_();
		return pFileStream_;
	}
	std::shared_ptr<std::ostream> SimpleFileStrategy::GetFileStream() const
	{
		return pFileStream_;
	}
	void SimpleFileStrategy::Cleanup()
	{}
	void SimpleFileStrategy::CreateFile_()
	{
		if (!pFileStream_) {
			// create any directories in the path that don't yet exist
			if (const auto dirs = path_.parent_path(); !dirs.empty()) {
				std::filesystem::create_directories(dirs);
			}
			// open file, append if already exists
			pFileStream_ = std::make_shared<std::ofstream>(path_, std::ios::out | std::ios::app);
		}
	}
}