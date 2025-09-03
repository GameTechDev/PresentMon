#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <stdexcept>
#include <string>

namespace pmapi
{
    class Exception : public std::runtime_error { using runtime_error::runtime_error; };
    // error arising from error code returned from presentmon api function
    class ApiErrorException : public Exception {
    public:
        ApiErrorException(PM_STATUS err, const std::string& message = "");
        PM_STATUS GetCode() const { return errorCode_; }
    private:
        // functions
        static std::string BuildWhatString_(PM_STATUS err, const std::string& message);
        // data
        PM_STATUS errorCode_;
    };
    // error arising from a mismatch of data type detected in wrapper processing
    class DatatypeException : public Exception { using Exception::Exception; };
    // error arising from a failure to find a value during wrapper processing
    class LookupException : public Exception { using Exception::Exception; };
}