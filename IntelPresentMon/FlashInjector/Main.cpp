#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/win/Utilities.h"
#include "../CommonUtilities/win/com/WbemConnection.h"
#include "../CommonUtilities/win/com/WbemListener.h"
#include "../CommonUtilities/win/com/ProcessSpawnSink.h"
#include "../CommonUtilities/win/Event.h"
#include "CliOptions.h"

#include <set>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <thread>
#include <format>
#include <atomic>

#include "LibraryInject.h"
#include "Logging.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"

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
    // Initialize arguments
    if (auto res = clio::Options::Init(argc, argv)) {
        return *res;
    }
    auto& opts = clio::Options::Get();

    stdfs::path injectorPath;
    {
        std::vector<char> buffer(MAX_PATH);
        auto size = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) {
            LOGE << "Failed to get this executable path.";
        }
        injectorPath = std::string(buffer.begin(), buffer.begin() + size);
        injectorPath = injectorPath.parent_path();
    }

    // DLL to inject
    const stdfs::path libraryPath = injectorPath / std::format("FlashInjectorLibrary-{}.dll", PM_BUILD_PLATFORM);

    const bool weAre32Bit = PM_BUILD_PLATFORM == "Win32"s;

    if (!stdfs::exists(libraryPath)) {
        LOGE << "Cannot find library: " << libraryPath;
        exit(1);
    }

    LOGI << "Waiting for processes that match executable name...";

    std::mutex targetModuleNameMtx;
    std::wstring targetModuleName;
    // thread whose sole job is to read from stdin without blocking the main thread
    std::thread{ [&] {
        std::string line;
        while (true) {
            std::getline(std::cin, line);
            std::lock_guard lk{ targetModuleNameMtx };
            targetModuleName = str::ToWide(str::ToLower(line));
        }
    } }.detach();

    win::Event procSpawnEvt{ false };
    win::com::WbemConnection wbConn;
    win::com::ProcessSpawnSink::EventQueue procSpawnQueue{ [&] { procSpawnEvt.Set(); } };
    auto pSpawnListener = wbConn.MakeListener<win::com::ProcessSpawnSink>(procSpawnQueue, 0.05f);

    while (true) {
        // wait until a process is spawned (queue has at least one spawn event added)
        win::WaitAnyEvent(procSpawnEvt);
        // atomic load target name and skip if empty string
        const auto tgt = [&] { std::lock_guard lk{ targetModuleNameMtx }; return targetModuleName; }();
        if (!tgt.empty()) {
            // keep popping process spawn events off the queue until it is empty
            for (std::optional<win::Process> proc; proc = procSpawnQueue.Pop();) {
                // case insensitive string compare spawned process name vs target
                const auto processNameLower = str::ToLower(proc->name);
                if (processNameLower == tgt) {
                    // check that bitness matches that of this injector
                    auto hProcTarget = win::OpenProcess(proc->pid, PROCESS_QUERY_LIMITED_INFORMATION);
                    if (win::ProcessIs32Bit(hProcTarget) == weAre32Bit) {
                        LOGI << "    Injecting DLL to process with PID: " << proc->pid;
                        // perform the actual injection
                        LibraryInject::Attach(proc->pid, libraryPath);
                        // inform kernel of attachment so it can connect the action client
                        std::cout << proc->pid << std::endl;
                    }
                }
            }
        }
    }

    return 0;
}