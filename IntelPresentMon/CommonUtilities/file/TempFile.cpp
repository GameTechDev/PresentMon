#include "TempFile.h"
#include "../win/WinAPI.h"
#include "../win/Utilities.h"
#include "../str/String.h"
#include "../Exception.h"
#include <fstream>
#include <rpc.h>

#pragma comment(lib, "Rpcrt4.lib")

namespace fs = std::filesystem;

namespace pmon::util::file
{
	TempFile::~TempFile()
	{
		if (!Empty()) {
			fs::remove(path_);
		}
	}
	TempFile TempFile::Create(const std::string& nameIn)
	{
		return CreateAt(fs::temp_directory_path());
	}
	TempFile TempFile::CreateAt(const std::filesystem::path& path, const std::string& nameIn)
	{
		TempFile tmp;
		std::string name = nameIn;
		if (name.empty()) {
			name = MakeRandomName();
		}
		tmp.path_ = path / name;
		// don't allow overwrite etc.
		if (fs::exists(tmp.path_)) {
			throw Except<Exception>("Temp file creation failed: already exists");
		}
		// create an empty file (or truncate existing)
		std::ofstream{ tmp.path_, std::ios::trunc };
		return tmp;
	}
	TempFile TempFile::AdoptExisting(const std::filesystem::path& path)
	{
		TempFile tmp;
		tmp.path_ = path;
		// make sure actually exists
		if (!fs::exists(tmp.path_)) {
			throw Except<Exception>("Temp file adoption failed: does not exist");
		}
		return tmp;
	}
    void TempFile::MoveTo(const std::filesystem::path& dest)
    {
        if (path_.empty()) {
            throw Except<Exception>("MoveTo failed: source path is empty");
        }
        if (dest.empty()) {
            throw Except<Exception>("MoveTo failed: destination folder is empty");
        }

        // Maintain same filename; build final target inside dest folder.
        const auto leafName = path_.filename();
        const auto target = dest / leafName;

        // Destination must be a directory
        if (!fs::exists(dest) || !fs::is_directory(dest)) {
            throw Except<Exception>("MoveTo failed: destination folder does not exist");
        }

		// move (rename) the file
		fs::rename(path_, target);

        // Update our stored path on success
        path_ = target;
    }

	void TempFile::Ascend()
	{
        const auto oneLevelUp = path_.parent_path().parent_path();
        if (oneLevelUp.empty()) {
            throw Except<Exception>("Ascend failed: already in volume root");
        }
        MoveTo(oneLevelUp);
	}
    const std::filesystem::path& TempFile::GetPath() const
    {
        return path_;
    }
	std::string TempFile::MakeRandomName()
	{
		UUID uuid;
		if (UuidCreate(&uuid) != RPC_S_OK) {
			throw Except<Exception>("Failed creating uuid");
		}
		// TODO: fallback to <random>
		return str::ToNarrow(win::GuidToString(uuid));
	}
	std::filesystem::path TempFile::Release()
	{
		auto p = std::move(path_);
		path_.clear();
		return p;
	}
}