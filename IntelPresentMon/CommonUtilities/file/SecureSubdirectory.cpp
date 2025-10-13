#include "SecureSubdirectory.h"
#include "../win/WinAPI.h"
#include "../win/Handle.h"
#include <winternl.h>
#include <winioctl.h>
#include <sddl.h>
#include <aclapi.h>
#include <vector>
#include <filesystem>
#include "../Exception.h"

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Ntdll.lib")

namespace fs = std::filesystem;

namespace pmon::util::file
{
    // ===== Internals / helpers =====

    static win::Handle OpenDirNoFollow(const fs::path& dirPath,
        DWORD desiredAccess = FILE_LIST_DIRECTORY | READ_CONTROL | WRITE_DAC | SYNCHRONIZE,
        DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
    {
        auto h = (win::Handle)::CreateFileW(
            dirPath.c_str(),
            desiredAccess,
            share,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
            nullptr);
        return h;
    }

    // Minimal header for FSCTL_DELETE_REPARSE_POINT (no reparse data on input).
    struct REPARSE_DATA_BUFFER_HEADER
    {
        DWORD ReparseTag;
        WORD  ReparseDataLength;
        WORD  Reserved;
    };

    // Attempt to delete the reparse point attribute on an object handle (directory)
    // Leaves the object in place as a normal directory if supported by the tag.
    static bool TryDeleteReparsePointByHandle_(HANDLE h, DWORD tag)
    {
        REPARSE_DATA_BUFFER_HEADER hdr{};
        hdr.ReparseTag = tag;
        hdr.ReparseDataLength = 0;
        hdr.Reserved = 0;

        DWORD bytes = 0;
        BOOL ok = ::DeviceIoControl(
            h,
            FSCTL_DELETE_REPARSE_POINT,
            &hdr,
            sizeof(hdr),
            nullptr,
            0,
            &bytes,
            nullptr);

        if (!ok) {
            // TODO: log
            return false;
        }
        return true;
    }

    bool SecureSubdirectory::IsReparseByHandle_(void* h)
    {
        FILE_ATTRIBUTE_TAG_INFO tag{};
        if (!::GetFileInformationByHandleEx(h, FileAttributeTagInfo, &tag, sizeof(tag))) {
            // TODO: log
            return false; // treat unknown as not reparse; caller should handle errors separately
        }
        return (tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    }

    // Create/open a directory named 'leafName' as a child of 'parent' without following reparses.
    // If the entry is a reparse point, attempt to convert it in place by deleting its reparse
    // attribute; if that fails, delete and recreate as a real directory.
    // Apply SYSTEM-only DACL by handle if isElevated == true.
    void SecureSubdirectory::CreateOrOpenDirSystemOnlySecure_(const fs::path& parent,
        const std::wstring& leafName,
        fs::path& outPath,
        bool isElevated)
    {
        if (leafName.find_first_of(L"/\\") != std::wstring::npos) {
            throw Except<Exception>("leafName must not contain path separators");
        }

        // 1) Open parent directory by handle (no-follow) and verify/normalize reparse if present.
        DWORD parentAccess = FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL;
        if (isElevated) parentAccess |= WRITE_DAC;

        auto hParent = (win::Handle)OpenDirNoFollow(parent, parentAccess);
        if (!hParent) {
            throw Except<Exception>("Open parent directory failed");
        }

        FILE_ATTRIBUTE_TAG_INFO pt{};
        if (!::GetFileInformationByHandleEx(hParent, FileAttributeTagInfo, &pt, sizeof(pt))) {
            throw Except<Exception>("GetFileInformationByHandleEx(parent) failed");
        }

        if (pt.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            // Try to delete the reparse attribute on the parent (best-effort).
            if (!TryDeleteReparsePointByHandle_(hParent, pt.ReparseTag)) {
                throw Except<Exception>("Parent directory is a reparse point and could not be normalized");
            }
            // Re-check; if still reparse, bail.
            if (IsReparseByHandle_(hParent)) {
                throw Except<Exception>("Parent directory remains a reparse point after normalization");
            }
        }

        // 2) Create/open the child directory *by name relative to parent handle* via NtCreateFile.
        UNICODE_STRING uName{};
        uName.Buffer = const_cast<wchar_t*>(leafName.c_str());
        uName.Length = static_cast<USHORT>(leafName.size() * sizeof(WCHAR));
        uName.MaximumLength = uName.Length;

        OBJECT_ATTRIBUTES oa{};
        InitializeObjectAttributes(&oa, &uName, OBJ_CASE_INSENSITIVE, hParent, nullptr);

        IO_STATUS_BLOCK ios{};
        win::Handle hChild;

        ACCESS_MASK childAccess = FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
        if (isElevated) childAccess |= WRITE_DAC;

        NTSTATUS st = ::NtCreateFile(
            hChild.ClearAndGetAddressOf(),
            childAccess,
            &oa,
            &ios,
            nullptr, // AllocationSize
            FILE_ATTRIBUTE_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN_IF, // create if missing, open if exists
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, // directory open/create
            nullptr,
            0);

        if (st < 0) {
            throw Except<Exception>("NtCreateFile(directory) failed");
        }

        // 3) If the child is a reparse point, attempt to normalize it.
        FILE_ATTRIBUTE_TAG_INFO ct{};
        if (!::GetFileInformationByHandleEx(hChild, FileAttributeTagInfo, &ct, sizeof(ct))) {
            auto hr = GetLastError();
            throw Except<Exception>("GetFileInformationByHandleEx(child) failed");
        }

        if (ct.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            bool normalized = TryDeleteReparsePointByHandle_(hChild, ct.ReparseTag);

            if (!normalized) {
                // Close the handle and attempt delete-and-recreate as a plain directory.
                hChild.Clear();

                fs::path full = fs::path(parent) / leafName;
                if (!::RemoveDirectoryW(full.c_str())) {
                    // As a last-ditch attempt, if it's a file reparse, try DeleteFileW
                    (void)::DeleteFileW(full.c_str()); // TODO: log result only
                }

                // Recreate with FILE_CREATE to ensure a brand-new real directory
                st = ::NtCreateFile(
                    hChild.ClearAndGetAddressOf(),
                    childAccess,
                    &oa,
                    &ios,
                    nullptr,
                    FILE_ATTRIBUTE_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_CREATE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                    nullptr,
                    0);

                if (st < 0) {
                    throw Except<Exception>("Recreate plain directory failed");
                }

                // Verify not a reparse now
                if (IsReparseByHandle_(hChild)) {
                    throw Except<Exception>("Directory remains a reparse point after recreate");
                }
            }
            else {
                // After deleting the reparse attribute, confirm it's now plain.
                if (IsReparseByHandle_(hChild)) {
                    throw Except<Exception>("Directory remains a reparse point after normalization");
                }
            }
        }

        // 4) Apply SYSTEM-only DACL by handle (no path) — only if running elevated.
        if (isElevated) {
            // SDDL: D:P(A;OICI;FA;;;SY) -> protected DACL, SYSTEM full, inherits to children.
            LPCWSTR sddl = L"D:P(A;OICI;FA;;;SY)";
            PSECURITY_DESCRIPTOR psd = nullptr;
            if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &psd, nullptr)) {
                throw Except<Exception>("ConvertStringSecurityDescriptorToSecurityDescriptorW failed");
            }

            PACL dacl = nullptr;
            BOOL daclPresent = FALSE, daclDefaulted = FALSE;
            if (!::GetSecurityDescriptorDacl(psd, &daclPresent, &dacl, &daclDefaulted)) {
                ::LocalFree(psd);
                throw Except<Exception>("GetSecurityDescriptorDacl failed");
            }

            DWORD rc = ::SetSecurityInfo(
                hChild,
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                nullptr, // owner
                nullptr, // group
                dacl,
                nullptr  // sacl
            );

            ::LocalFree(psd);

            if (rc != ERROR_SUCCESS) {
                throw Except<Exception>("SetSecurityInfo(DACL) failed");
            }
        }

        // 5) Return the final absolute path for convenience.
        outPath = fs::path(parent) / leafName;
    }

    SecureSubdirectory SecureSubdirectory::Create(const fs::path& parent,
        const std::wstring& name,
        bool isElevated,
        bool deleteOnDestruct,
        bool clearOnConstruct)
    {
        if (name.empty()) {
            throw Except<Exception>("name must not be empty");
        }

        fs::path base = parent.empty() ? fs::temp_directory_path() : parent;

        // Ensure 'base' itself can be opened (no-follow). If it's a reparse, try to normalize it.
        {
            DWORD baseAccess = FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL | WRITE_DAC;
            auto hBase = (win::Handle)OpenDirNoFollow(base, baseAccess);
            if (hBase == INVALID_HANDLE_VALUE) {
                // Retry without WRITE_DAC if access denied.
                hBase = OpenDirNoFollow(base, FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL);
                if (!hBase) {
                    throw Except<Exception>("Open base directory failed");
                }
            }

            FILE_ATTRIBUTE_TAG_INFO bi{};
            if (!::GetFileInformationByHandleEx(hBase, FileAttributeTagInfo, &bi, sizeof(bi))) {
                throw Except<Exception>("GetFileInformationByHandleEx(base) failed");
            }

            if ((bi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
                // Try to delete the reparse attribute on the base (best-effort).
                (void)TryDeleteReparsePointByHandle_(hBase, bi.ReparseTag); // TODO: log success/failure
                if (IsReparseByHandle_(hBase)) {
                    throw Except<Exception>("Base directory remains a reparse point");
                }
            }
        }

        SecureSubdirectory d;
        d.isElevated_ = isElevated;
        d.deleteOnDestruct_ = deleteOnDestruct;

        CreateOrOpenDirSystemOnlySecure_(base, name, d.path_, isElevated);

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
        if (deleteOnDestruct_ && !path_.empty()) {
            try {
                Remove();
            }
            catch (...) {
                // TODO: log
            }
        }
    }

    SecureSubdirectory::SecureSubdirectory(SecureSubdirectory&& other) noexcept
        : path_(std::move(other.path_))
        , deleteOnDestruct_(other.deleteOnDestruct_)
        , isElevated_(other.isElevated_)
    {
        other.deleteOnDestruct_ = false;
        other.path_.clear();
        other.isElevated_ = false;
    }

    SecureSubdirectory& SecureSubdirectory::operator=(SecureSubdirectory&& other) noexcept
    {
        if (this == &other) return *this;
        if (deleteOnDestruct_ && !path_.empty()) {
            try {
                Remove();
            }
            catch (...) {
                // TODO: log
            }
        }
        path_ = std::move(other.path_);
        deleteOnDestruct_ = other.deleteOnDestruct_;
        isElevated_ = other.isElevated_;
        other.deleteOnDestruct_ = false;
        other.path_.clear();
        other.isElevated_ = false;
        return *this;
    }

    // Clear now assumes there are NO reparse points inside the directory.
    // It deletes contents using a recursive directory iterator and fs::remove,
    // throwing on errors.
    void SecureSubdirectory::Clear()
    {
        if (path_.empty()) return;

        std::error_code ec;

        // Collect directories for post-order deletion; remove non-directories immediately.
        std::vector<fs::path> dirs;
        for (fs::recursive_directory_iterator it(path_, ec), end; it != end; it.increment(ec)) {
            if (ec) {
                throw Except<Exception>("recursive_directory_iterator failed");
            }

            const fs::path p = it->path();

            // We assume no reparse points; treat directory vs file normally.
            if (it->is_directory(ec)) {
                if (ec) {
                    throw Except<Exception>("is_directory failed");
                }
                dirs.push_back(p);
            }
            else {
                // file or other non-directory
                if (!fs::remove(p, ec)) {
                    if (ec) {
                        throw Except<Exception>("remove (file) failed");
                    }
                }
            }
        }

        // Delete directories in reverse (deepest first)
        for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
            if (!fs::remove(*it, ec)) {
                if (ec) {
                    throw Except<Exception>("remove (dir) failed");
                }
            }
        }
    }
    void SecureSubdirectory::Remove()
    {
        if (!Empty()) {
            Clear();
            fs::remove(path_);
            path_.clear();
        }
    }
    bool SecureSubdirectory::Empty() const
    {
        return path_.empty();
    }
}
