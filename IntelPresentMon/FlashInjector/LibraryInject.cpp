#include "LibraryInject.h"
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/win/Handle.h"
#include "../CommonUtilities/win/Utilities.h"

#include "Logging.h"

#include <Psapi.h>
#include <unordered_set>

using namespace pmon::util;

namespace LibraryInject
{
    static win::Handle OpenProcessAndMaybeElevate_(uint32_t processId)
    {
        auto hProcess = (win::Handle)OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess) {
            LOGI << "OpenProcess failed with [" << win::GetErrorDescription(GetLastError()) << "]; re-attemping with elevated privileges" << std::endl;
            
            // Increase privileges
            win::Handle hToken;
            {
                HANDLE tempHandle;
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tempHandle)) {
                    LOGE << "OpenProcessToken failed for current process. Error: " << win::GetErrorDescription(GetLastError()) << std::endl;
                    exit(1);
                }
                hToken = win::Handle(tempHandle);
            }

            LUID debugPrivilegeId;
            if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &debugPrivilegeId)) {
                LOGE << "LookupPrivilegeValue failed for current process. Error: " << win::GetErrorDescription(GetLastError()) << std::endl;
                exit(1);
            }

            TOKEN_PRIVILEGES tokenPrivileges = {
                .PrivilegeCount = 1,
                .Privileges = {
                    { .Luid = debugPrivilegeId, .Attributes = SE_PRIVILEGE_ENABLED }
                }
            };
            if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(tokenPrivileges), NULL, NULL)) {
                LOGE << "AdjustTokenPrivileges failed for current process. Error: " << win::GetErrorDescription(GetLastError()) << std::endl;
                exit(1);
            }

            if (!(hProcess = (win::Handle)OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId))) {
                LOGE << "Could not open process to attach with elevated privileges. Error: " << win::GetErrorDescription(GetLastError()) << std::endl;
                exit(1);
            }
        }

        return hProcess;
    }

    static void InjectDll(const HANDLE hProcess, const std::filesystem::path& dllPath)
    {
        auto  absDllPath = std::filesystem::absolute(dllPath).string();
        auto  remoteMemSize = absDllPath.size() + 1; // +1 to account for null character 
        auto* pRemoteMem = VirtualAllocEx(hProcess, NULL, remoteMemSize, MEM_COMMIT, PAGE_READWRITE);

        if (pRemoteMem == NULL)
        {
            auto error = GetLastError();
            LOGE << "Could not allocate memory in target process. Error: " << win::GetErrorDescription(error) << std::endl;
            exit(1);
        }

        if (!WriteProcessMemory(hProcess, pRemoteMem, absDllPath.c_str(), remoteMemSize, NULL))
        {
            auto error = GetLastError();
            LOGE << "WriteProcessMemory failed. Error: " << win::GetErrorDescription(error) << std::endl;
            exit(1);
        }
        FlushInstructionCache(hProcess, pRemoteMem, remoteMemSize);

        HMODULE hKernel32 = GetModuleHandle("Kernel32");
        HANDLE hRemoteThread = CreateRemoteThread(
            hProcess, 
            NULL, 
            0, 
            reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(hKernel32, "LoadLibraryA")), 
            pRemoteMem,
            0, 
            NULL
        );

        if (hRemoteThread == NULL)
        {
            auto error = GetLastError();
            LOGE << "Could not create remote thread in target process. Error: " << win::GetErrorDescription(error);
            LOGE << "Tip: Check bit-ness of the application and DXGIOverlay.exe." << std::endl;
            VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
            exit(1);
        }

        WaitForSingleObject(hRemoteThread, INFINITE);
        CloseHandle(hRemoteThread);

        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
    }

    void Attach(uint32_t processId, const std::filesystem::path& dllPath)
    {
        auto hTargetProcess = LibraryInject::OpenProcessAndMaybeElevate_(processId);
        LibraryInject::InjectDll(hTargetProcess, dllPath);
    }

    ProcessMap GetProcessNames()
    {
        ProcessMap processMap;

        DWORD processIds[1024];
        DWORD processIdsSize;
        EnumProcesses(processIds, sizeof(processIds), &processIdsSize);

        auto processIdsFound = processIdsSize / sizeof(DWORD);
        for (unsigned idx = 0; idx < processIdsFound; idx++)
        {
            auto processId = processIds[idx];
            if (processId == 0)
            {
                continue;
            }

            auto hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processId);
            if (!hProcess)
            {
                continue;
            }

            CHAR pProcessName[MAX_PATH] = "NOT_FOUND";
            GetModuleBaseName(hProcess, 0, pProcessName, sizeof(pProcessName) / sizeof(CHAR));

            CloseHandle(hProcess);

            processMap[processId] = pProcessName;
        }

        return processMap;
    }
}