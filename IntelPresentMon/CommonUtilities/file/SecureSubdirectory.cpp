#include "SecureSubdirectory.h"
#include "../win/WinAPI.h"
#include "../win/Handle.h"
#include "../win/HrError.h"
#include "../win/Security.h"
#include <winioctl.h>
#include <aclapi.h>
#include <vector>
#include <filesystem>
#include "../Exception.h"
#include "../log/Log.h"

namespace fs = std::filesystem;

namespace pmon::util::file
{
    // internal helpers
    namespace
    {
        // open a directory into handle without following reparse at the path leaf
        win::Handle OpenDirNoFollow_(const fs::path& dirPath,
            DWORD desiredAccess = FILE_LIST_DIRECTORY | READ_CONTROL | WRITE_DAC | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
            DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE)
        {
            auto h = (win::Handle)CreateFileW(
                dirPath.c_str(),
                desiredAccess,
                share,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                nullptr);
            return h;
        }

        // Attempt to delete the reparse point attribute on an object handle (directory)
        // Leaves the object in place as a normal directory if supported by the tag.
        static bool TryDeleteReparsePointByHandle_(HANDLE h, DWORD tag)
        {
            // Minimal header for FSCTL_DELETE_REPARSE_POINT (no reparse data on input).
            struct {
                DWORD ReparseTag;
                WORD  ReparseDataLength;
                WORD  Reserved;
            } hdr{ .ReparseTag = tag };
            DWORD bytes = 0;
            return (bool)DeviceIoControl(h, FSCTL_DELETE_REPARSE_POINT, &hdr, sizeof(hdr),
                nullptr, 0, &bytes, nullptr);
        }

        // check if a directory referenced by the handle has a reparse point
        std::optional<DWORD> IsReparseByHandle_(HANDLE h)
        {
            FILE_ATTRIBUTE_TAG_INFO tag{};
            if (!GetFileInformationByHandleEx(h, FileAttributeTagInfo, &tag, sizeof(tag))) {
                throw Except<win::HrError>("GetFileInformationByHandleEx(child) failed");
            }
            if (tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                return tag.ReparseTag;
            }
            else {
                return std::nullopt;
            }
        }

        // Create/open a directory named 'leafName' as a child of 'parent'
        // If running elevated, additionally undertake the following defensive measures:
        // If the entry is a reparse point, attempt to convert it in place by deleting its reparse
        // attribute; if that fails, delete and recreate as a real directory.
        win::Handle CreateOrOpenDirSystemOnlySecure_(const fs::path& parent,
            const std::wstring& leafName,
            bool isElevated)
        {
            if (leafName.find_first_of(L"/\\") != std::wstring::npos) {
                throw Except<Exception>("leafName must not contain path separators");
            }

            const auto fullpath = parent / leafName;

            // setup ACL to apply
            UniqueLocalPtr<void> pSecDesc;
            if (isElevated) {
                pSecDesc = win::MakeSecurityDescriptor("D:P(A;OICI;FA;;;SY)");
            }
            SECURITY_ATTRIBUTES secAttr{
                .nLength = sizeof(SECURITY_ATTRIBUTES),
                .lpSecurityDescriptor = pSecDesc.get(),
            };

            // 1. try and create with ACL
            const auto CreateNew = [&] {
                if (CreateDirectoryW(fullpath.c_str(), &secAttr)) {
                    // successfully created new directory
                    pmlog_dbg("Created new temp subdir").pmwatch(fullpath.string());
                    return true;
                }
                else if (HRESULT hr = GetLastError(); hr != ERROR_ALREADY_EXISTS) {
                    throw Except<win::HrError>(hr, "Failed to create secure subdir");
                }
                return false;
            };
            const auto isFresh = CreateNew();

            // 2. open the existing dir without following any reparse on it
            auto hExistingSubdir = OpenDirNoFollow_(fullpath);
            if (!hExistingSubdir) {
                throw Except<win::HrError>("Failed to open existing subdir by handle");
            }

            // if dir is not fresh and we are elevated, we have defensive work to do
            if (!isFresh && isElevated) {
                // 3. own the existing dir
                {
                    // get the dacl out of the security description
                    PACL pDacl = nullptr;
                    BOOL daclPresent = FALSE, daclDefaulted = FALSE;
                    if (!GetSecurityDescriptorDacl(pSecDesc.get(), &daclPresent, &pDacl, &daclDefaulted)) {
                        throw Except<win::HrError>("GetSecurityDescriptorDacl failed");
                    }
                    // set security on the directory object
                    if (auto rc = SetSecurityInfo(
                        hExistingSubdir,
                        SE_FILE_OBJECT,
                        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                        nullptr, // owner
                        nullptr, // group
                        pDacl,
                        nullptr  // sacl
                    ); rc != ERROR_SUCCESS) {
                        throw Except<win::HrError>(HRESULT(rc), "SetSecurityInfo(DACL) failed");
                    }
                }

                // 3. check for reparse
                if (auto tag = IsReparseByHandle_(hExistingSubdir)) {
                    pmlog_warn("detected reparse point when estabilishing subdir").pmwatch(fullpath.string());
                    // 3a. try and remove reparse point
                    if (TryDeleteReparsePointByHandle_(hExistingSubdir, *tag)) {
                        pmlog_dbg("deleted reparse point from subdir");
                    }
                    else {
                        pmlog_warn("Failed to delete reparse point from subdir");
                        // 3b. try to delete entire directory
                        hExistingSubdir.Clear();
                        fs::remove_all(fullpath);
                        // 3c. try and create anew with ACL
                        if (!CreateNew()) {
                            throw Except<Exception>("Failed to create new directory after deleting existing");
                        }
                        hExistingSubdir = OpenDirNoFollow_(fullpath);
                    }
                    // 3d. final check that directory no longer has reparse
                    if (IsReparseByHandle_(hExistingSubdir)) {
                        throw Except<Exception>("Could not neutralize reparse obstacle");
                    }
                }
            }

            // we are in the clear
            return hExistingSubdir;
        }
    }

    SecureSubdirectory SecureSubdirectory::Create(const fs::path& parent,
        const std::wstring& name,
        bool isElevated,
        bool deleteOnDestruct,
        bool clearOnConstruct)
    {
        SecureSubdirectory d;
        d.isElevated_ = isElevated;
        d.deleteOnDestruct_ = deleteOnDestruct;
        d.path_ = parent / name;
        d.hDirectory_ = CreateOrOpenDirSystemOnlySecure_(parent, name, isElevated);

        if (clearOnConstruct) {
            d.Clear();
        }

        return d;
    }

    SecureSubdirectory SecureSubdirectory::CreateInSystemTemp(const std::wstring& name,
        bool isElevated,
        bool deleteOnDestruct,
        bool clearOnConstruct)
    {
        return Create(fs::temp_directory_path(), name, isElevated, deleteOnDestruct, clearOnConstruct);
    }

    SecureSubdirectory::~SecureSubdirectory()
    {
        if (deleteOnDestruct_ && !Empty()) {
            try {
                Remove();
            }
            catch (...) {
                pmquell(pmlog_error("failed removing secure subdir").pmwatch(path_.string()));
            }
        }
    }

    SecureSubdirectory::SecureSubdirectory(SecureSubdirectory&& other) noexcept
        : 
        path_(std::move(other.path_)),
        deleteOnDestruct_(other.deleteOnDestruct_),
        isElevated_(other.isElevated_),
        hDirectory_(std::move(other.hDirectory_))
    {
        other.deleteOnDestruct_ = false;
        other.path_.clear();
        other.isElevated_ = false;
    }

    SecureSubdirectory& SecureSubdirectory::operator=(SecureSubdirectory&& other) noexcept
    {
        if (this == &other) return *this;
        if (deleteOnDestruct_ && !Empty()) {
            try {
                Remove();
            }
            catch (...) {
                pmlog_error("failed removing secure subdir");
            }
        }
        path_ = std::move(other.path_);
        deleteOnDestruct_ = other.deleteOnDestruct_;
        isElevated_ = other.isElevated_;
        hDirectory_ = std::move(other.hDirectory_);
        other.deleteOnDestruct_ = false;
        other.path_.clear();
        other.isElevated_ = false;
        return *this;
    }
    const std::filesystem::path& SecureSubdirectory::Path() const noexcept
    {
        return path_;
    }
    void SecureSubdirectory::Clear()
    {
        if (Empty()) return;

        for (const auto& entry : std::filesystem::directory_iterator(path_)) {
            try {
                std::filesystem::remove_all(entry.path());
            }
            catch (const std::filesystem::filesystem_error&) {
                throw Except<Exception>("remove_all failed when clearing out SecureSubdirectory");
            }
        }
    }
    void SecureSubdirectory::Remove()
    {
        if (!Empty()) {
            Clear();
            hDirectory_.Clear();
            fs::remove(path_);
            path_.clear();
        }
    }
    bool SecureSubdirectory::Empty() const
    {
        return path_.empty();
    }
    SecureSubdirectory::operator bool() const
    {
        return !Empty();
    }
}
