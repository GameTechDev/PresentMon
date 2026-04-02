#include "TempFile.h"
#include "../win/WinAPI.h"
#include "../win/Utilities.h"
#include "../win/HrError.h"
#include "../str/String.h"
#include "../Exception.h"
#include "../Memory.h"
#include "../log/Log.h"
#include <fstream>
#include <random>
#include <format>
#include <rpc.h>
#include <sddl.h>
#include <aclapi.h>

#pragma comment(lib, "Rpcrt4.lib")

namespace fs = std::filesystem;

namespace pmon::util::file
{
	TempFile::~TempFile()
	{
		if (!Empty()) {
			try { fs::remove(path_); }
			catch (...) {
				pmlog_warn(util::ReportException("TempFile dtor cleanup"));
			}
		}
	}
	TempFile TempFile::Create()
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
	TempFile& TempFile::MoveTo(const std::filesystem::path& dest)
    {
        if (Empty()) {
            throw Except<Exception>("MoveTo failed: this object is empty");
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

		return *this;
    }

	TempFile& TempFile::Ascend()
	{
        const auto oneLevelUp = path_.parent_path().parent_path();
        if (oneLevelUp.empty()) {
            throw Except<Exception>("Ascend failed: already in volume root");
        }
        return MoveTo(oneLevelUp);
	}
    const std::filesystem::path& TempFile::GetPath() const
    {
        return path_;
    }
	std::string TempFile::MakeRandomName()
	{
		UUID uuid;
		if (UuidCreate(&uuid) == RPC_S_OK) {
			return str::ToNarrow(win::GuidToString(uuid));
		}

		// Fallback: generate a pseudo-random GUID-like string.
		static thread_local std::mt19937_64 rng{ std::random_device{}() };
		std::uniform_int_distribution<uint32_t> dist32;
		std::uniform_int_distribution<uint16_t> dist16;

		return std::format(
			"{{{:08x}-{:04x}-{:04x}-{:04x}-{:012x}}}",
			dist32(rng),                                       // 32 bits
			dist16(rng),                                       // 16 bits
			(dist16(rng) & 0x0FFF) | 0x4000,                   // version 4
			(dist16(rng) & 0x3FFF) | 0x8000,                   // variant 1
			(static_cast<uint64_t>(dist32(rng)) << 32) | dist32(rng)  // 48 bits from 64
		);
	}
	bool TempFile::Empty() const
	{
		return path_.empty();
	}
	TempFile::operator bool() const
	{
		return !Empty();
	}
	TempFile& TempFile::MakePublic()
	{
		if (Empty()) {
			throw Except<Exception>("No file to make public");
		}

		// Build ACEs
		EXPLICIT_ACCESSA ea[3]{};

		// SYSTEM full
		BuildExplicitAccessWithNameA(&ea[0], (LPSTR)"SYSTEM",
			GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);

		// Administrators full
		BuildExplicitAccessWithNameA(&ea[1], (LPSTR)"BUILTIN\\Administrators",
			GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);

		// Authenticated Users modify (RWX + delete)
		DWORD modifyMask = FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE;
		BuildExplicitAccessWithNameA(&ea[2], (LPSTR)"Authenticated Users",
			modifyMask, SET_ACCESS, NO_INHERITANCE);

		UniqueLocalPtr<ACL> pNewDacl;
		if (auto res = SetEntriesInAclA(3, ea, nullptr, OutPtr(pNewDacl)); res != ERROR_SUCCESS) {
			throw Except<win::HrError>(HRESULT(res), "SetEntriesInAcl failed");
		}

		if (auto res = SetNamedSecurityInfoA(
			(LPSTR)path_.string().c_str(),
			SE_FILE_OBJECT,
			DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
			nullptr, nullptr, pNewDacl.get(), nullptr);
			res != ERROR_SUCCESS) {
			throw Except<win::HrError>(HRESULT(res), "SetNamedSecurityInfo failed");
		}

		return *this;
	}
	std::filesystem::path TempFile::Release()
	{
		auto p = std::move(path_);
		path_.clear();
		return p;
	}
}