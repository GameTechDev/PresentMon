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
		~TempFile();

		static TempFile Create(const std::string& name = {});
		static TempFile CreateAt(const std::filesystem::path& path, const std::string& name = {});
		static TempFile AdoptExisting(const std::filesystem::path& path);
		void MoveTo(const std::filesystem::path& path);
		void Ascend();
		const std::filesystem::path& GetPath() const;
		std::filesystem::path Release();
		static std::string MakeRandomName();
		bool Empty() const
		{
			return path_.empty();
		}
		operator bool() const
		{
			return !Empty();
		}
	private:
		// data
		std::filesystem::path path_;
	};
}