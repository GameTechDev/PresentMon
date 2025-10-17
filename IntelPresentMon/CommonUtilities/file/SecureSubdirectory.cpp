#include "SecureSubdirectory.h"
#include "../win/WinAPI.h"
#include "../win/Handle.h"
#include "../win/HrError.h"
#include <winternl.h>
#include <winioctl.h>
#include <sddl.h>
#include <aclapi.h>
#include <vector>
#include <filesystem>
#include "../Exception.h"
#include "../log/Log.h"

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Ntdll.lib")

namespace fs = std::filesystem;

namespace pmon::util::file
{
    // internal helpers
    namespace
    {
        win::Handle OpenDirNoFollow_(const fs::path& dirPath,
            DWORD desiredAccess = FILE_LIST_DIRECTORY | READ_CONTROL | WRITE_DAC | SYNCHRONIZE,
            DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
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
            BOOL ok = DeviceIoControl(
                h,
                FSCTL_DELETE_REPARSE_POINT,
                &hdr,
                sizeof(hdr),
                nullptr,
                0,
                &bytes,
                nullptr);

            if (!ok) {
                pmlog_warn("Could not delete reparse point by handle").hr();
                return false;
            }
            return true;
        }

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

        // Create/open a directory named 'leafName' as a child of 'parent' without following reparses.
        // If the entry is a reparse point, attempt to convert it in place by deleting its reparse
        // attribute; if that fails, delete and recreate as a real directory.
        // Apply SYSTEM-only DACL by handle if isElevated == true.
        fs::path CreateOrOpenDirSystemOnlySecure_(const fs::path& parent,
            const std::wstring& leafName,
            bool isElevated)
        {
            if (leafName.find_first_of(L"/\\") != std::wstring::npos) {
                throw Except<Exception>("leafName must not contain path separators");
            }

            // 1) Open parent directory by handle (no-follow) and verify/normalize reparse if present.
            DWORD parentAccess = FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL;
            if (isElevated) parentAccess |= WRITE_DAC;

            auto hParent = (win::Handle)OpenDirNoFollow_(parent, parentAccess);
            if (!hParent) {
                throw Except<win::HrError>("Open parent directory failed");
            }

            if (auto reparseTag = IsReparseByHandle_(hParent)) {
                // Try to delete the reparse attribute on the parent (best-effort).
                if (!TryDeleteReparsePointByHandle_(hParent, *reparseTag)) {
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
            NTSTATUS st = NtCreateFile(
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
                throw Except<Exception>(std::format("NtCreateFile(directory) failed; NTSTATUS:{}", st));
            }

            // 3) If the child is a reparse point, attempt to normalize it.
            if (auto reparseTag = IsReparseByHandle_(hChild)) {                
                if (bool normalized = TryDeleteReparsePointByHandle_(hChild, *reparseTag); !normalized) {
                    // Close the handle and attempt delete-and-recreate as a plain directory.
                    hChild.Clear();

                    fs::path full = fs::path(parent) / leafName;
                    if (!RemoveDirectoryW(full.c_str())) {
                        pmlog_warn("Failed remove reparse as dir").hr();
                        // As a last-ditch attempt, if it's a file reparse, try DeleteFileW
                        if (!DeleteFileW(full.c_str())) {
                            pmlog_error("Could not remove reparse point").hr();
                        }
                    }

                    // Recreate with FILE_CREATE to ensure a brand-new real directory
                    st = NtCreateFile(
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
                        throw Except<Exception>(std::format(
                            "Recreate plain directory replacing reparse point failed; NTSTATUS:{}", st));
                    }
                }
                // After dealing with reparse point, confirm it's now plain.
                if (IsReparseByHandle_(hChild)) {
                    throw Except<Exception>("Directory remains a reparse point after cleansing efforts");
                }
            }

            // 4) Apply SYSTEM-only DACL by handle (no path) - only if running elevated.
            if (isElevated) {
                // SDDL: D:P(A;OICI;FA;;;SY) -> protected DACL, SYSTEM full, inherits to children.
                LPCWSTR sddl = L"D:P(A;OICI;FA;;;SY)";
                PSECURITY_DESCRIPTOR psd = nullptr;
                if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &psd, nullptr)) {
                    throw Except<win::HrError>("ConvertStringSecurityDescriptorToSecurityDescriptorW failed");
                }

                PACL dacl = nullptr;
                BOOL daclPresent = FALSE, daclDefaulted = FALSE;
                if (!GetSecurityDescriptorDacl(psd, &daclPresent, &dacl, &daclDefaulted)) {
                    ::LocalFree(psd);
                    throw Except<win::HrError>("GetSecurityDescriptorDacl failed");
                }

                DWORD rc = SetSecurityInfo(
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
                    throw Except<win::HrError>((HRESULT)rc, "SetSecurityInfo(DACL) failed");
                }
            }

            // 5) Return the final absolute path for convenience.
            return parent / leafName;
        }
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

        SecureSubdirectory d;
        d.isElevated_ = isElevated;
        d.deleteOnDestruct_ = deleteOnDestruct;
        d.path_ = CreateOrOpenDirSystemOnlySecure_(parent, name, isElevated);

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
                pmlog_error("failed removing secure subdir");
            }
        }
    }

    SecureSubdirectory::SecureSubdirectory(SecureSubdirectory&& other) noexcept
        : 
        path_(std::move(other.path_)),
        deleteOnDestruct_(other.deleteOnDestruct_),
        isElevated_(other.isElevated_)
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
