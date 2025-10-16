#pragma once

#include <filesystem>
#include <string>

namespace pmon::util::file
{
    class SecureSubdirectory
    {
    public:
        static SecureSubdirectory Create(const std::filesystem::path& parent,
            const std::wstring& name,
            bool isElevated = false,
            bool deleteOnDestruct = false,
            bool clearOnConstruct = false);
        static SecureSubdirectory CreateInSystemTemp(const std::wstring& name,
            bool isElevated = false,
            bool deleteOnDestruct = false,
            bool clearOnConstruct = false);

        SecureSubdirectory() = default;
        ~SecureSubdirectory();
        SecureSubdirectory(const SecureSubdirectory&) = delete;
        SecureSubdirectory& operator=(const SecureSubdirectory&) = delete;
        SecureSubdirectory(SecureSubdirectory&&) noexcept;
        SecureSubdirectory& operator=(SecureSubdirectory&&) noexcept;

        const std::filesystem::path& Path() const noexcept { return path_; }
        void Clear();
        void Remove();
        bool Empty() const;
        operator bool() const
        {
            return !Empty();
        }

    private:
        // data
        std::filesystem::path path_;
        bool deleteOnDestruct_ = false;
        bool isElevated_ = false;
    };
}
