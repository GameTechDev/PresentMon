#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <iterator>
#include <string>
#include <ranges>
#include <memory>
#include <functional>
#include <unordered_map>

namespace pmapi
{
    namespace intro
    {
        // check if a metric type can be used with dynamic queries
        bool MetricTypeIsDynamic(PM_METRIC_TYPE type);
        // check if a metric type can be used with frame queries
        bool MetricTypeIsFrameEvent(PM_METRIC_TYPE type);

        template<class T>
        class ViewIterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using base_type = typename T::BaseType;
            using reference = value_type&;
            using pointer = value_type*;
            using difference_type = ptrdiff_t;

            ViewIterator() = default;
            ViewIterator(const class Root* pRoot_, const PM_INTROSPECTION_OBJARRAY* pObjArray, difference_type offset = 0u) noexcept
                :
                pRoot{ pRoot_ },
                pArray{ (const base_type* const*)pObjArray->pData + offset }
            {}
            ViewIterator(const ViewIterator& rhs) noexcept : pRoot{ rhs.pRoot }, pArray{ rhs.pArray } {}
            ViewIterator& operator=(const ViewIterator& rhs) noexcept
            {
                // Self-assignment check
                if (this != &rhs) {
                    pRoot = rhs.pRoot;
                    pArray = rhs.pArray;
                }
                return *this;
            }
            ViewIterator& operator+=(difference_type rhs) noexcept
            {
                pArray += rhs;
                return *this;
            }
            ViewIterator& operator-=(difference_type rhs) noexcept
            {
                pArray -= rhs;
                return *this;
            }
            value_type operator*() const noexcept
            {
                return value_type{ pRoot, *pArray };
            }
            value_type operator->() const noexcept
            {
                return **this;
            }
            value_type operator[](size_t idx) const noexcept
            {
                return value_type{ pRoot, pArray[idx] };
            }

            ViewIterator& operator++() noexcept
            {
                ++pArray;
                return *this;
            }
            ViewIterator& operator--() noexcept
            {
                --pArray;
                return *this;
            }
            ViewIterator operator++(int) noexcept { ViewIterator tmp(*this); ++(*this); return tmp; }
            ViewIterator operator--(int) noexcept { ViewIterator tmp(*this); --(*this); return tmp; }
            difference_type operator-(const ViewIterator& rhs) const noexcept
            {
                return difference_type(pArray - rhs.pArray);
            }
            ViewIterator operator+(difference_type rhs) const noexcept
            {
                auto dup = *this;
                return dup += rhs;
            }
            ViewIterator operator-(difference_type rhs) const noexcept
            {
                auto dup = *this;
                return dup -= rhs;
            }

            bool operator==(const ViewIterator& rhs) const noexcept { return pArray == rhs.pArray; }
            std::strong_ordering operator<=>(const ViewIterator& rhs) const noexcept
            {
                return pArray <=> rhs.pArray;
            }
        private:
            // data
            const base_type* const* pArray = nullptr;
            const class Root* pRoot = nullptr;
        };

        template<class V>
        using ViewRange = std::ranges::subrange<ViewIterator<V>, ViewIterator<V>>;

        // information about the key values in a specific enumeration
        class EnumKeyView
        {            
            using BaseType = PM_INTROSPECTION_ENUM_KEY;
            using SelfType = EnumKeyView;
            friend class ViewIterator<SelfType>;
            friend class Root;
        public:
            int GetId() const;
            std::string GetSymbol() const;
            std::string GetName() const;
            std::string GetShortName() const;
            std::string GetDescription() const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            EnumKeyView(const class Root* pRoot_, const BaseType* pBase_);
            // data
            const class Root* pRoot = nullptr;
            const BaseType* pBase = nullptr;
        };

        // information about an enumeration, such as PM_METRIC or PM_PRESENT_MODE
        class EnumView
        {
            using BaseType = PM_INTROSPECTION_ENUM;
            using SelfType = EnumView;
            friend class ViewIterator<SelfType>;
            friend class Root;
        public:
            PM_ENUM GetId() const;
            std::string GetSymbol() const;
            std::string GetDescription() const;
            ViewRange<EnumKeyView> GetKeys() const;
            EnumKeyView FindKey(int key) const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            EnumView(const class Root* pRoot_, const BaseType* pBase_);
            ViewIterator<EnumKeyView> GetKeysBegin_() const;
            ViewIterator<EnumKeyView> GetKeysEnd_() const;
            // data
            const class Root* pRoot;
            const BaseType* pBase = nullptr;
        };

        // information about a hardware device
        // device 0 is a special device used for metrics not tied to any
        // specific hardware device, and thus for metrics which are (typically)
        // always available
        class DeviceView
        {
            using BaseType = PM_INTROSPECTION_DEVICE;
            using SelfType = DeviceView;
            friend class ViewIterator<SelfType>;
            friend class Root;
            friend class DeviceMetricInfoView;
        public:
            EnumKeyView IntrospectType() const;
            PM_DEVICE_TYPE GetType() const;
            EnumKeyView IntrospectVendor() const;
            PM_DEVICE_VENDOR GetVendor() const;
            uint32_t GetId() const;
            std::string GetName() const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            DeviceView(const class Root* pRoot_, const BaseType* pBase_);
            // data
            const class Root* pRoot = nullptr;
            const BaseType* pBase = nullptr;
        };

        // information about metric availability per-device
        class DeviceMetricInfoView
        {
            using BaseType = PM_INTROSPECTION_DEVICE_METRIC_INFO;
            using SelfType = DeviceMetricInfoView;
            friend class ViewIterator<SelfType>;
            friend class Root;
        public:
            DeviceView GetDevice() const;
            EnumKeyView IntrospectAvailablity() const;
            PM_METRIC_AVAILABILITY GetAvailability() const;
            uint32_t GetArraySize() const;
            bool IsAvailable() const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            DeviceMetricInfoView(const class Root* pRoot_, const BaseType* pBase_);
            // data
            const class Root* pRoot;
            const BaseType* pBase = nullptr;
        };

        // information about the data type used to encode a metric value
        class DataTypeInfoView
        {
            using BaseType = PM_INTROSPECTION_DATA_TYPE_INFO;
            using SelfType = DataTypeInfoView;
            friend class MetricView;
        public:
            EnumKeyView IntrospectPolledType() const;
            PM_DATA_TYPE GetPolledType() const;
            EnumKeyView IntrospectFrameType() const;
            PM_DATA_TYPE GetFrameType() const;
            EnumView IntrospectEnum() const;
            PM_ENUM GetEnumId() const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            DataTypeInfoView(const class Root* pRoot_, const BaseType* pBase_);
            // data
            const class Root* pRoot = nullptr;
            const BaseType* pBase = nullptr;
        };

        // information about stat used to aggregate frame data for a dynamic metric
        class StatInfoView
        {
            using BaseType = PM_INTROSPECTION_STAT_INFO;
            using SelfType = StatInfoView;
            friend class ViewIterator<SelfType>;
            friend class MetricView;
        public:
            EnumKeyView IntrospectStat() const;
            PM_STAT GetStat() const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            StatInfoView(const class Root* pRoot_, const BaseType* pBase_);
            // data
            const class Root* pRoot = nullptr;
            const BaseType* pBase = nullptr;
        };

        // information about a unit of measure used for metrics
        class UnitView
        {
            using BaseType = PM_INTROSPECTION_UNIT;
            using SelfType = UnitView;
            friend class ViewIterator<SelfType>;
            friend class Root;
        public:
            EnumKeyView Introspect() const;
            PM_UNIT GetId() const;
            EnumKeyView IntrospectBaseUnit() const;
            PM_UNIT GetBaseUnit() const;
            double GetScale() const;
            double MakeConversionFactor(PM_UNIT destinationUnitId) const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            UnitView(const class Root* pRoot_, const BaseType* pBase_);
            // data
            const class Root* pRoot;
            const BaseType* pBase = nullptr;
        };

        // get information about a metric
        class MetricView
        {
            using BaseType = PM_INTROSPECTION_METRIC;
            using SelfType = MetricView;
            friend class ViewIterator<SelfType>;
            friend class Root;
        public:
            // introspect calls get string metadata
            EnumKeyView Introspect() const;
            PM_METRIC GetId() const;
            EnumKeyView IntrospectUnit() const;
            // unit that the value is natively expressed in
            PM_UNIT GetUnit() const;
            EnumKeyView IntrospectPreferredUnitHint() const;
            // unit recommended for user display
            PM_UNIT GetPreferredUnitHint() const;
            EnumKeyView IntrospectType() const;
            // metric type (static, dynamic, frame)
            PM_METRIC_TYPE GetType() const;
            // the datatype this metric is encoded as
            DataTypeInfoView GetDataTypeInfo() const;
            // list of supported stats (avg, 99%, etc.)
            ViewRange<StatInfoView> GetStatInfo() const;
            // which devices support this metric
            ViewRange<DeviceMetricInfoView> GetDeviceMetricInfo() const;
            const SelfType* operator->() const;
            const BaseType* GetBasePtr() const;
        private:
            // functions
            ViewIterator<StatInfoView> GetStatInfoBegin_() const;
            ViewIterator<StatInfoView> GetStatInfoEnd_() const;
            ViewIterator<DeviceMetricInfoView> GetDeviceMetricInfoBegin_() const;
            ViewIterator<DeviceMetricInfoView> GetDeviceMetricInfoEnd_() const;
            MetricView(const class Root* pRoot_, const BaseType* pBase_);
            // data
            const class Root* pRoot;
            const BaseType* pBase = nullptr;
        };

        // provides access to all introspection exposed by the service
        // including enums, metrics, hardware devices, units of measure
        // can iterate or perform lookup via id
        class Root
        {
        public:
            Root(const PM_INTROSPECTION_ROOT* pRoot_, std::function<void(const PM_INTROSPECTION_ROOT*)> deleter_);
            ~Root();
            Root(Root&& rhs) = delete;
            Root& operator=(Root&& rhs) = delete;
            ViewRange<EnumView> GetEnums() const;
            ViewRange<MetricView> GetMetrics() const;
            ViewRange<DeviceView> GetDevices() const;
            ViewRange<UnitView> GetUnits() const;
            EnumKeyView FindEnumKey(PM_ENUM enumId, int keyValue) const;
            EnumView FindEnum(PM_ENUM enumId) const;
            DeviceView FindDevice(uint32_t deviceId) const;
            MetricView FindMetric(PM_METRIC metricId) const;
            UnitView FindUnit(PM_UNIT unitId) const;
        private:
            // functions
            static uint64_t MakeEnumKeyMapKey_(PM_ENUM enumId, int keyValue);
            ViewIterator<EnumView> GetEnumsBegin_() const;
            ViewIterator<EnumView> GetEnumsEnd_() const;
            ViewIterator<MetricView> GetMetricsBegin_() const;
            ViewIterator<MetricView> GetMetricsEnd_() const;
            ViewIterator<DeviceView> GetDevicesBegin_() const;
            ViewIterator<DeviceView> GetDevicesEnd_() const;
            ViewIterator<UnitView> GetUnitsBegin_() const;
            ViewIterator<UnitView> GetUnitsEnd_() const;
            // data
            const PM_INTROSPECTION_ROOT* pRoot = nullptr;
            std::function<void(const PM_INTROSPECTION_ROOT*)> deleter;
            std::unordered_map<uint64_t, const PM_INTROSPECTION_ENUM_KEY*> enumKeyMap;
            std::unordered_map<PM_ENUM, const PM_INTROSPECTION_ENUM*> enumMap;
            std::unordered_map<uint32_t, const PM_INTROSPECTION_DEVICE*> deviceMap;
            std::unordered_map<PM_METRIC, const PM_INTROSPECTION_METRIC*> metricMap;
            std::unordered_map<PM_UNIT, const PM_INTROSPECTION_UNIT*> unitMap;
        };
    }
}