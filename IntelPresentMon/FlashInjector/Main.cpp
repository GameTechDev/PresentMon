#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/win/Utilities.h"
#include "../CommonUtilities/win/Event.h"
#include "CliOptions.h"

#include <set>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <format>
#include <atomic>

#include "LibraryInject.h"
#include "Logging.h"

namespace stdfs = std::filesystem;
using namespace pmon::util;
using namespace std::literals;

// null logger factory to satisfy linking requirements for CommonUtilities
namespace pmon::util::log
{
    std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
    {
        return {};
    }
}

int main(int argc, char** argv)
{
    // Initial logging
    LOGI << "Injector process started" << std::endl;

    // Initialize arguments
    if (auto res = clio::Options::Init(argc, argv, true)) {
        return *res;
    }
    auto& opts = clio::Options::Get();

    stdfs::path injectorPath;
    {
        std::vector<char> buffer(MAX_PATH);
        auto size = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) {
            LOGE << "Failed to get this executable path." << std::endl;;
        }
        injectorPath = std::string(buffer.begin(), buffer.begin() + size);
        injectorPath = injectorPath.parent_path();
    }

    // DLL to inject
    const stdfs::path libraryPath = injectorPath / std::format("FlashInjectorLibrary-{}.dll", PM_BUILD_PLATFORM);

    const bool weAre32Bit = PM_BUILD_PLATFORM == "Win32"s;

    if (!stdfs::exists(libraryPath)) {
        LOGE << "Cannot find library: " << libraryPath << std::endl;;
        exit(1);
    }

    LOGI << "Waiting for processes that match executable name..." << std::endl;

    std::mutex targetModuleNameMtx;
    std::string targetModuleName;
    // thread whose sole job is to read from stdin without blocking the main thread
    std::thread{ [&] {
        std::string line;
        while (true) {
            std::getline(std::cin, line);
            std::lock_guard lk{ targetModuleNameMtx };
            targetModuleName = str::ToLower(line);
        }
    } }.detach();

    // keep a set of processes already attached so we don't attempt multiple attachments per process
    std::unordered_set<DWORD> processesAttached;
    while (true) {
        // atomic load target name and skip if empty string
        const auto tgt = [&] { std::lock_guard lk{ targetModuleNameMtx }; return targetModuleName; }();
        if (!tgt.empty()) {
            for (auto&& [processId, processName] : LibraryInject::GetProcessNames()) {
                const auto processNameLower = str::ToLower(processName);
                if (processNameLower == tgt && !processesAttached.contains(processId)) {
                    auto hProcTarget = win::OpenProcess(processId, PROCESS_QUERY_LIMITED_INFORMATION);
                    if (win::ProcessIs32Bit(hProcTarget) == weAre32Bit) {
                        LibraryInject::Attach(processId, libraryPath);
                        LOGI << "    Injected DLL to process with PID: " << processId << std::endl;
                        processesAttached.insert(processId);
                        // inform kernel of attachment so it can connect the action client
                        std::cout << processId << std::endl;
                    }
                }
            }
        }
        // check for new processes only every N ms to reduce CPU load
        std::this_thread::sleep_for(40ms);
    }

    return 0;
}