#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <IntelPresentMon/CommonUtilities/str/String.h>
#include "Introspection.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <shared_mutex>
#include <atomic>

namespace pmapi
{
    // contains wide and narrow versions for strings describing a PresentMon API enum
    struct EnumKeyStrings
    {
        std::string narrowSymbol;
        std::wstring wideSymbol;
        std::string narrowName;
        std::wstring wideName;
        std::string narrowShortName;
        std::wstring wideShortName;
        std::string narrowDescription;
        std::wstring wideDescription;
    };

    // singleton used to access information about PresentMon enum keys
    class EnumMap
    {
    public:
        // maps an enum key value to its metadata string information
        using KeyMap = std::unordered_map<int, EnumKeyStrings>;
        // given the id of a presentmon enum, get a map mapping keys of that enum to metadata strings
        static std::shared_ptr<const KeyMap> GetKeyMap(PM_ENUM enumId);
        // generate enum lookup maps using the introspection data passed in
        static void Refresh(const pmapi::intro::Root& introRoot);
        // check if the enum lookup maps have been generated already
        static bool Initialized();
    private:
        // functions
        static EnumMap& Get_();
        // data
        std::shared_mutex mtx_;
        std::unordered_map<PM_ENUM, std::shared_ptr<KeyMap>> enumMap_;
        std::atomic<bool> initialized_ = false;
    };
}