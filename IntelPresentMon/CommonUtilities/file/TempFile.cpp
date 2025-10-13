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
    void TempFile::SecureMoveTo(const std::filesystem::path& dest)
    {
        if (path_.empty()) {
            throw Except<Exception>("SecureMoveTo failed: source path is empty");
        }
        if (dest.empty()) {
            throw Except<Exception>("SecureMoveTo failed: destination folder is empty");
        }

        // Maintain same filename; build final target inside dest folder.
        const fs::path leafName = path_.filename();
        const fs::path target = dest / leafName;

        // Do not overwrite an existing file
        if (fs::exists(target)) {
            throw Except<Exception>("SecureMoveTo failed: destination file already exists");
        }

        // Destination must be a directory
        if (!fs::exists(dest) || !fs::is_directory(dest)) {
            throw Except<Exception>("SecureMoveTo failed: destination folder does not exist");
        }

        const fs::path parent = dest; // rename-by-handle into this directory

        // 1) Open the source file for rename-by-handle
        HANDLE hFile = ::CreateFileW(
            path_.c_str(),
            FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            throw Except<Exception>("SecureMoveTo failed: unable to open source file");
        }

        // 2) Open the destination directory (do not follow reparse points)
        HANDLE hParent = ::CreateFileW(
            parent.c_str(),
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
            nullptr);
        if (hParent == INVALID_HANDLE_VALUE) {
            ::CloseHandle(hFile);
            throw Except<Exception>("SecureMoveTo failed: unable to open destination folder");
        }

        // 3) Build FILE_RENAME_INFO with RootDirectory = dest folder handle and leaf name only
        const std::wstring leaf = leafName.wstring();
        const DWORD nameBytes = static_cast<DWORD>(leaf.size() * sizeof(WCHAR));
        const size_t renameSize = offsetof(FILE_RENAME_INFO, FileName) + nameBytes;

        std::vector<BYTE> buf(renameSize, 0);
        auto* fri = reinterpret_cast<FILE_RENAME_INFO*>(buf.data());
        fri->ReplaceIfExists = FALSE;                // no overwrite
        fri->RootDirectory = hParent;              // folder handle
        fri->FileNameLength = nameBytes;
        std::memcpy(fri->FileName, leaf.data(), nameBytes);

        // 4) Rename-by-handle (atomic within the same volume)
        if (!::SetFileInformationByHandle(hFile, FileRenameInfo, fri, static_cast<DWORD>(buf.size()))) {
            DWORD err = ::GetLastError();
            ::CloseHandle(hParent);
            ::CloseHandle(hFile);

            // Minimal, pragmatic fallback for cross-volume moves
            if (err == ERROR_NOT_SAME_DEVICE) {
                if (!::MoveFileExW(path_.c_str(), target.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
                    throw Except<Exception>("SecureMoveTo failed: cross-volume move error");
                }
            }
            else {
                throw Except<Exception>("SecureMoveTo failed: rename-by-handle error");
            }
        }
        else {
            ::CloseHandle(hParent);
            ::CloseHandle(hFile);
        }

        // 5) Update our stored path on success
        path_ = target;
    }

	void TempFile::SecureAscend()
	{
        const auto oneLevelUp = path_.parent_path().parent_path();
        if (oneLevelUp.empty()) {
            throw Except<Exception>("SecureAscend failed: already in volume root");
        }
        SecureMoveTo(oneLevelUp);
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
}