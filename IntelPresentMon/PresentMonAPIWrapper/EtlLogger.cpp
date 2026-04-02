#include "EtlLogger.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>

namespace pmapi
{
    // stops tracking the associated process
    EtlLogger::~EtlLogger()
    {
        try { Reset(); }
        catch (...) {}
    }
    // move ctor
    EtlLogger::EtlLogger(EtlLogger&& other) noexcept
    {
        *this = std::move(other);
    }
    // move assignment
    EtlLogger& EtlLogger::operator=(EtlLogger&& rhs) noexcept
    {
        if (&rhs != this) {
            hLogger_ = rhs.hLogger_;
            hSession_ = rhs.hSession_;
            rhs.Clear_();
        }
        return *this;
    }
    // finish the logging session and receive the .etl file
    void EtlLogger::Finish(const std::filesystem::path& outputEtlFullPath)
    {
        char buffer[PM_MAX_PATH + 1];
        if (auto sta = pmFinishEtlLogging(hSession_, hLogger_, buffer, (uint32_t)std::size(buffer));
            sta != PM_STATUS_SUCCESS) {
            Clear_();
            throw ApiErrorException{ sta, "Failed to finish etl logging" };
        }
        try {
            std::filesystem::rename(buffer, outputEtlFullPath);
            Clear_();
        }
        catch (...) {
            // attempt to remove if still there
            std::error_code ec;
            std::filesystem::remove(buffer, ec);
            throw ApiErrorException{ PM_STATUS_FAILURE, "Failed to move output .etl file" };
        }
    }
    // get the id of process being tracked
    PM_ETL_HANDLE EtlLogger::GetHandle() const
    {
        return hLogger_;
    }
    // empty this logger
    void EtlLogger::Reset() noexcept
    {
        if (hLogger_ && hSession_) {
            char buffer[PM_MAX_PATH + 1];
            if (auto sta = pmFinishEtlLogging(hSession_, hLogger_, buffer, (uint32_t)std::size(buffer));
                sta == PM_STATUS_SUCCESS) {
                std::error_code ec;
                // TODO: report this error via diagnostic custom
                try { std::filesystem::remove(buffer, ec); }
                catch (...) {}
            }
        }
        Clear_();
    }
    // check if logger is empty
    bool EtlLogger::Empty() const
    {
        return hSession_ == nullptr;
    }
    // alias for Empty();
    EtlLogger::operator bool() const
    {
        return !Empty();
    }
    
    EtlLogger::EtlLogger(PM_SESSION_HANDLE hSession)
        :
        hSession_{ hSession }
    {
        if (auto sta = pmStartEtlLogging(hSession_, &hLogger_, 0, 0);
            sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "Failed to start etl logging session" };
        }
    }
    // zero out members, useful after emptying via move or reset
    void EtlLogger::Clear_() noexcept
    {
        hLogger_ = 0;
        hSession_ = nullptr;
    }
}
