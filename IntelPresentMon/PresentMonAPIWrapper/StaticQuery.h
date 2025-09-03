#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Introspection.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/EnumMap.h>
#include <IntelPresentMon/Interprocess/source/IntrospectionDataTypeMapping.h>
#include <IntelPresentMon/CommonUtilities/str/String.h>
#include <IntelPresentMon/CommonUtilities/Meta.h>
#include "Session.h"
#include "ProcessTracker.h"
#include <string>
#include <memory>
#include <cassert>
#include <array>

namespace pmapi
{
    namespace
    {
        // this static functor converts static types when bridged with runtime PM_DATA_TYPE info
        template<PM_DATA_TYPE dt, PM_ENUM staticEnumId, typename DestType, size_t blobSize>
        struct SQReadBridger_
        {
            using SourceType = typename pmon::ipc::intro::DataTypeToStaticType<dt, staticEnumId>::type;
            static_assert(pmon::util::VoidableSizeof<SourceType>() <= blobSize, "Inadequate blob size detected");
            static void Invoke(PM_ENUM enumId, DestType& dest, const uint8_t* pBlobBytes)
            {
                // void types are not handleable and generally should not occur
                if constexpr (dt == PM_DATA_TYPE_VOID) {
                    assert(false && "trying to convert void type");
                }
                // strings (char array) can convert to std::basic_string types
                else if constexpr (dt == PM_DATA_TYPE_STRING) {
                    if constexpr (std::same_as<DestType, std::string>) {
                        dest = reinterpret_cast<const char*>(pBlobBytes);
                    }
                    else if constexpr (std::same_as<DestType, std::wstring>) {
                        dest = pmon::util::str::ToWide(reinterpret_cast<const char*>(pBlobBytes));
                    }
                    else {
                        assert(false && "failure to convert enum type");
                    }
                }
                // enums can convert to numeric types or string types
                else if constexpr (dt == PM_DATA_TYPE_ENUM) {
                    if constexpr (std::same_as<DestType, std::string>) {
                        try {
                            const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
                            dest = EnumMap::GetKeyMap(enumId)->at(keyId).narrowName;
                        }
                        catch (...) { dest = "Invalid"; }
                    }
                    else if constexpr (std::same_as<DestType, std::wstring>) {
                        try {
                            const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
                            dest = EnumMap::GetKeyMap(enumId)->at(keyId).wideName;
                        }
                        catch (...) { dest = L"Invalid"; }
                    }
                    else if constexpr (
                        std::is_integral_v<DestType> ||
                        std::is_floating_point_v<DestType> ||
                        std::same_as<DestType, SourceType>) {
                        dest = DestType(*reinterpret_cast<const int*>(pBlobBytes));
                    }
                    else {
                        assert(false && "failure to convert enum type");
                    }
                }
                // what's left is actual numeric types
                else {
                    // don't output if the dest is string type
                    if constexpr (!std::same_as<DestType, std::string> && !std::same_as<DestType, std::wstring>) {
                        dest = static_cast<DestType>(reinterpret_cast<const SourceType&>(*pBlobBytes));
                    }
                    else {
                        assert(false && "failure to convert numeric type");
                    }
                }
            }
            static void Default(DestType& dest, const uint8_t* pBlobBytes) {
                assert(false && "failure to convert unknown type");
            }
        };

        // adapter to convert template taking 4 arguments to template taking 2
        // (we need this because you cannot define templates within a function
        // and the static info is only available inside the templated function)
        template<typename T, size_t blobSize>
        struct SQReadBridger_Adapter_ {
            template<PM_DATA_TYPE dt, PM_ENUM enumId>
            using Bridger = SQReadBridger_<dt, enumId, T, blobSize>;
        };
    }

    // smart StaticQueryResult object that can automatically interpret
    // query result data blobs and convert to the desired type when conversion is possible
    class StaticQueryResult
    {
        friend StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
            PM_METRIC metric, uint32_t deviceId, uint32_t arrayIndex);
    public:
        // access this result as a specific static data type
        // this will perform the conversion based on the runtime information about the metric's type
        // enum types will use the introspection system to generate human-readable strings
        // wide/narrow string conversion will also be performed
        template<typename T>
        T As() const
        {
            T val;
            // the bridge will execute the bridger with the correct blob/source static type based
            // on the runtime PM_DATA_TYPE value passed in, and the bridger will convert that to
            // the requested type T and store in val
            pmon::ipc::intro::BridgeDataTypeWithEnum
                <typename SQReadBridger_Adapter_<T, blobSize_>::Bridger>
                (dataType_, enumId_, val, blob_.data());
            return val;
        }
        // uses the As<T>() member function for perform implicit conversion
        template<typename T>
        operator T() const
        {
            return As<std::remove_reference_t<std::remove_const_t<T>>>();
        }
    private:
        // functions
        StaticQueryResult(PM_DATA_TYPE dataType, PM_ENUM enumId) : dataType_{ dataType }, enumId_{ enumId } {}
        // although NRVO should prevent this from being called, it's not standard guarantee
        // so we still need the copy ctor
        StaticQueryResult(const StaticQueryResult&) = default;
        // data
        // 260 bytes is the maximum possible size for query element data
        static constexpr size_t blobSize_ = 260;
        std::array<uint8_t, blobSize_> blob_{};
        PM_DATA_TYPE dataType_;
        PM_ENUM enumId_;
    };

    // poll a static metric (metric which will not change for the life of a process)
    // static metrics include: process exe name, gpu name, gpu maximum memory, etc.
    // function returns a smart StaticQueryResult object that can automatically interpret
    // query result data blobs and convert to the desired type when conversion is possible
    StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
        PM_METRIC metric, uint32_t deviceId = 0, uint32_t arrayIndex = 0);
}
