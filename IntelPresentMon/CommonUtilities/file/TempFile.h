#pragma once
#include <filesystem>

namespace pmon::util::file
{
	class TempFile
	{
	public:
		static TempFile Create(const std::string& name = {});
		static TempFile CreateAt(const std::filesystem::path& path, const std::string& name = {});
		static TempFile AdoptExisting(const std::filesystem::path& path);
		void SecureMoveTo(const std::filesystem::path& path);
		void SecureAscend();
		const std::filesystem::path& GetPath() const;
		std::filesystem::path Release();
		static std::string MakeRandomName();
	private:
		// functions
		TempFile() = default;
		// data
		std::filesystem::path path_;
	};
}