#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <filesystem>

namespace pmapi
{
    // manages a logger that logs ETW events directly to a .etl file
    class EtlLogger
    {
        friend class Session;
    public:
        // create an empty tracker
        EtlLogger() = default;
        // stops tracking the associated process
        ~EtlLogger();
        // move ctor
        EtlLogger(EtlLogger&& other) noexcept;
        // move assignment
        EtlLogger& operator=(EtlLogger&& rhs) noexcept;
        // finish the logging session and receive the .etl file
        void Finish(const std::filesystem::path& outputEtlFullPath);
        // get the id of process being tracked
        PM_ETL_HANDLE GetHandle() const;
        // empty this logger
        void Reset() noexcept;
        // check if logger is empty
        bool Empty() const;
        // alias for Empty();
        operator bool() const;
    private:
        // functions
        EtlLogger(PM_SESSION_HANDLE hSession);
        // zero out members, useful after emptying via move or reset
        void Clear_() noexcept;
        // data
        PM_ETL_HANDLE hLogger_ = 0u;
        PM_SESSION_HANDLE hSession_ = nullptr;
    };
}
