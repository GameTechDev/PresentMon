#pragma once
#include <filesystem>

namespace pmon::util::file
{
	class TempFile
	{
	public:
		TempFile() = default;
		TempFile(TempFile&&) = default;
		TempFile& operator=(TempFile&&) = default;
		TempFile(const TempFile&) = delete;
		TempFile& operator=(const TempFile&) = delete;
		~TempFile();

		TempFile& MoveTo(const std::filesystem::path& path);
		TempFile& Ascend();
		TempFile& MakePublic();
		const std::filesystem::path& GetPath() const;
		std::filesystem::path Release();
		bool Empty() const;
        operator bool() const;

		static TempFile Create();
		static TempFile CreateAt(const std::filesystem::path& path, const std::string& name = {});
		static TempFile AdoptExisting(const std::filesystem::path& path);
		static std::string MakeRandomName();
	private:
		// data
		std::filesystem::path path_;
	};
}