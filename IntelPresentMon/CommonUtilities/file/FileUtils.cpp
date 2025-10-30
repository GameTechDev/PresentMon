#include "FileUtils.h"
#include <regex>

namespace pmon::util::file
{
    namespace fs = std::filesystem;

    std::vector<std::filesystem::path> FindFilesMatchingPattern(
        const std::filesystem::path& dir, const std::string& pattern)
    {
        std::vector<fs::path> results;

        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            throw std::runtime_error("Invalid directory: " + dir.string());
        }

        std::regex rx(pattern, std::regex::ECMAScript);

        for (auto const& entry : fs::recursive_directory_iterator(
            dir, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                const std::string filename = entry.path().filename().string();
                if (std::regex_search(filename, rx)) {
                    results.emplace_back(entry.path());
                }
            }
        }

        return results;
    }
}