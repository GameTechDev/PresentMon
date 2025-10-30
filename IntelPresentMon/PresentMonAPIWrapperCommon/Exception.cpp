#include "Exception.h"
#include "EnumMap.h"
#include <IntelPresentMon/Interprocess/source/metadata/EnumStatus.h>
#include <format>

namespace pmapi
{
    ApiErrorException::ApiErrorException(PM_STATUS err, const std::string& message)
        :
        Exception{ BuildWhatString_(err, message) },
        errorCode_{ err }
    {}

    std::string ApiErrorException::BuildWhatString_(PM_STATUS err, const std::string& message)
    {
        try {
            auto pStatusMap = EnumMap::GetKeyMap(PM_ENUM_STATUS);
            auto& str = pStatusMap->at(err);
            return std::format("{} | Error: {} ({}) {}: {}", message,
                str.narrowSymbol, int(err), str.narrowName, str.narrowDescription);
        }
        catch (...) {
            // we usually end up here trying to translate a PM_STATUS code before we have introspection access
            // fallback to a static lookup generated based on the known set of codes at wrapper compile time
            bool descriptionAvailable = false;
            const char* frag = nullptr;
            const char* name = nullptr;
            const char* desc = nullptr;
            switch (err) {
#define X_STATUS_(enum_name_fragment, key_frag_, name_, short_name, desc_) \
                case PM_STATUS_##key_frag_: frag = #key_frag_; name = name_; desc = desc_; descriptionAvailable = true; break;
                ENUM_KEY_LIST_STATUS(X_STATUS_)
#undef X_STATUS_
            }
            if (descriptionAvailable) {
                return std::format("{} | Error: {} ({}) {}: {}", message, frag, int(err), name, desc);
            }
            else {
                return std::format("{} | Unknown error code [{}]", message, int(err));
            }
        }
    }
}