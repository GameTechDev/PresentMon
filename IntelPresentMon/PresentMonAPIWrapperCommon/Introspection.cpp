#include "Introspection.h"
#include <format>
#include <cassert>
#include <ranges>
#include "Exception.h"
#include <IntelPresentMon/CommonUtilities/log/Log.h>

namespace rn = std::ranges;

namespace pmapi::intro
{
    bool MetricTypeIsDynamic(PM_METRIC_TYPE type)
    {
        return type == PM_METRIC_TYPE_DYNAMIC || type == PM_METRIC_TYPE_DYNAMIC_FRAME;
    }

    bool MetricTypeIsFrameEvent(PM_METRIC_TYPE type)
    {
        return type == PM_METRIC_TYPE_FRAME_EVENT || type == PM_METRIC_TYPE_DYNAMIC_FRAME;
    }



    int EnumKeyView::GetId() const
    {
        return pBase->id;
    }

    std::string EnumKeyView::GetSymbol() const
    {
        return pBase->pSymbol->pData;
    }

    std::string EnumKeyView::GetName() const
    {
        return pBase->pName->pData;
    }

    std::string EnumKeyView::GetShortName() const
    {
        return pBase->pShortName->pData;
    }

    std::string EnumKeyView::GetDescription() const
    {
        return pBase->pDescription->pData;
    }

    const EnumKeyView::SelfType* EnumKeyView::operator->() const
    {
        return this;
    }

    const EnumKeyView::BaseType* EnumKeyView::GetBasePtr() const
    {
        return pBase;
    }

    EnumKeyView::EnumKeyView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}



    PM_ENUM EnumView::GetId() const
    {
        return pBase->id;
    }

    std::string EnumView::GetSymbol() const
    {
        return pBase->pSymbol->pData;
    }

    std::string EnumView::GetDescription() const
    {
        return pBase->pDescription->pData;
    }

    ViewRange<EnumKeyView> EnumView::GetKeys() const
    {
        // trying to deduce the template params for subrange causes intellisense to crash
        // workaround this by providing them explicitly as the return type (normally would use auto)
        return { GetKeysBegin_(), GetKeysEnd_() };
    }

    EnumKeyView EnumView::FindKey(int key) const
    {
        auto keys = GetKeys();
        if (auto i = rn::find(keys, key, &EnumKeyView::GetId); i == keys.end()) {
            throw LookupException{ std::format("unable to find key ID={} of enum {}", key, GetSymbol()) };
        }
        else {
            return *i;
        }
    }

    const EnumView::SelfType* EnumView::operator->() const
    {
        return this;
    }

    const EnumView::BaseType* EnumView::GetBasePtr() const
    {
        return pBase;
    }

    EnumView::EnumView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}

    ViewIterator<EnumKeyView> EnumView::GetKeysBegin_() const
    {
        return { pRoot, pBase->pKeys };
    }

    ViewIterator<EnumKeyView> EnumView::GetKeysEnd_() const
    {
        return { pRoot, pBase->pKeys, (int64_t)pBase->pKeys->size };
    }



    EnumKeyView DeviceView::IntrospectType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_TYPE, pBase->type);
    }

    EnumKeyView DeviceView::IntrospectVendor() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DEVICE_VENDOR, pBase->vendor);
    }

    PM_DEVICE_TYPE DeviceView::GetType() const
    {
        return pBase->type;
    }

    PM_DEVICE_VENDOR DeviceView::GetVendor() const
    {
        return pBase->vendor;
    }

    uint32_t DeviceView::GetId() const
    {
        return pBase->id;
    }

    std::string DeviceView::GetName() const
    {
        return pBase->pName->pData;
    }

    const DeviceView::SelfType* DeviceView::operator->() const
    {
        return this;
    }

    const DeviceView::BaseType* DeviceView::GetBasePtr() const
    {
        return pBase;
    }

    DeviceView::DeviceView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}



    DeviceView DeviceMetricInfoView::GetDevice() const
    {
        return pRoot->FindDevice(pBase->deviceId);
    }

    EnumKeyView DeviceMetricInfoView::IntrospectAvailablity() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC_AVAILABILITY, pBase->availability);
    }

    PM_METRIC_AVAILABILITY DeviceMetricInfoView::GetAvailability() const
    {
        return pBase->availability;
    }

    uint32_t DeviceMetricInfoView::GetArraySize() const
    {
        return pBase->arraySize;
    }

    bool DeviceMetricInfoView::IsAvailable() const
    {
        return pBase->availability == PM_METRIC_AVAILABILITY_AVAILABLE;
    }

    const DeviceMetricInfoView::SelfType* DeviceMetricInfoView::operator->() const
    {
        return this;
    }

    const DeviceMetricInfoView::BaseType* DeviceMetricInfoView::GetBasePtr() const
    {
        return pBase;
    }

    DeviceMetricInfoView::DeviceMetricInfoView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}



    EnumKeyView DataTypeInfoView::IntrospectPolledType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, pBase->polledType);
    }

    EnumKeyView DataTypeInfoView::IntrospectFrameType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_DATA_TYPE, pBase->frameType);
    }

    EnumView DataTypeInfoView::IntrospectEnum() const
    {
        if (pBase->polledType != PM_DATA_TYPE_ENUM && pBase->frameType != PM_DATA_TYPE_ENUM) {
            throw DatatypeException{ "cannot get enum data for non-enum data type" };
        }
        return pRoot->FindEnum(pBase->enumId);
    }

    PM_DATA_TYPE DataTypeInfoView::GetPolledType() const
    {
        return pBase->polledType;
    }

    PM_DATA_TYPE DataTypeInfoView::GetFrameType() const
    {
        return pBase->frameType;
    }

    PM_ENUM DataTypeInfoView::GetEnumId() const
    {
        return pBase->enumId;
    }

    const DataTypeInfoView::SelfType* DataTypeInfoView::operator->() const
    {
        return this;
    }

    const DataTypeInfoView::BaseType* DataTypeInfoView::GetBasePtr() const
    {
        return pBase;
    }

    DataTypeInfoView::DataTypeInfoView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}


    EnumKeyView StatInfoView::IntrospectStat() const
    {
        return pRoot->FindEnumKey(PM_ENUM_STAT, pBase->stat);
    }

    PM_STAT StatInfoView::GetStat() const
    {
        return pBase->stat;
    }

    const StatInfoView::SelfType* StatInfoView::operator->() const
    {
        return this;
    }

    const StatInfoView::BaseType* StatInfoView::GetBasePtr() const
    {
        return pBase;
    }

    StatInfoView::StatInfoView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}



    EnumKeyView UnitView::Introspect() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->id);
    }

    PM_UNIT UnitView::GetId() const
    {
        return pBase->id;
    }

    EnumKeyView UnitView::IntrospectBaseUnit() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->baseUnitId);
    }

    PM_UNIT UnitView::GetBaseUnit() const
    {
        return pBase->baseUnitId;
    }

    double UnitView::GetScale() const
    {
        return pBase->scale;
    }

    double UnitView::MakeConversionFactor(PM_UNIT destinationUnitId) const
    {
        auto destUnit = pRoot->FindUnit(destinationUnitId);
        if (destUnit.GetBaseUnit() != GetBaseUnit()) {
            throw std::runtime_error{ "cannot convert incompatible units" };
        }
        return GetScale() / destUnit.GetScale();
    }

    const UnitView::SelfType* UnitView::operator->() const
    {
        return this;
    }

    const UnitView::BaseType* UnitView::GetBasePtr() const
    {
        return pBase;
    }

    UnitView::UnitView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}



    EnumKeyView MetricView::Introspect() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC, pBase->id);
    }

    PM_METRIC MetricView::GetId() const
    {
        return pBase->id;
    }

    EnumKeyView MetricView::IntrospectUnit() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->unit);
    }

    PM_UNIT MetricView::GetUnit() const
    {
        return pBase->unit;
    }

    EnumKeyView MetricView::IntrospectPreferredUnitHint() const
    {
        return pRoot->FindEnumKey(PM_ENUM_UNIT, pBase->preferredUnitHint);
    }

    PM_UNIT MetricView::GetPreferredUnitHint() const
    {
        return pBase->preferredUnitHint;
    }

    EnumKeyView MetricView::IntrospectType() const
    {
        return pRoot->FindEnumKey(PM_ENUM_METRIC_TYPE, pBase->type);
    }

    PM_METRIC_TYPE MetricView::GetType() const
    {
        return pBase->type;
    }

    DataTypeInfoView MetricView::GetDataTypeInfo() const
    {
        return { pRoot, pBase->pTypeInfo };
    }

    ViewRange<StatInfoView> MetricView::GetStatInfo() const
    {
        return { GetStatInfoBegin_(), GetStatInfoEnd_() };
    }

    ViewRange<DeviceMetricInfoView> MetricView::GetDeviceMetricInfo() const
    {
        // trying to deduce the template params for subrange causes intellisense to crash
        // workaround this by providing them explicitly as the return type (normally would use auto)
        return { GetDeviceMetricInfoBegin_(), GetDeviceMetricInfoEnd_() };
    }

    const MetricView::SelfType* MetricView::operator->() const
    {
        return this;
    }

    const MetricView::BaseType* MetricView::GetBasePtr() const
    {
        return pBase;
    }

    ViewIterator<StatInfoView> MetricView::GetStatInfoBegin_() const
    {
        return { pRoot, pBase->pStatInfo };
    }

    ViewIterator<StatInfoView> MetricView::GetStatInfoEnd_() const
    {
        return { pRoot, pBase->pStatInfo, (int64_t)pBase->pStatInfo->size };
    }

    ViewIterator<DeviceMetricInfoView> MetricView::GetDeviceMetricInfoBegin_() const
    {
        return { pRoot, pBase->pDeviceMetricInfo };
    }

    ViewIterator<DeviceMetricInfoView> MetricView::GetDeviceMetricInfoEnd_() const
    {
        return { pRoot, pBase->pDeviceMetricInfo, (int64_t)pBase->pDeviceMetricInfo->size };
    }

    MetricView::MetricView(const class Root* pRoot_, const BaseType* pBase_)
        :
        pRoot{ pRoot_ },
        pBase{ pBase_ }
    {}



    Root::Root(const PM_INTROSPECTION_ROOT* pRoot_, std::function<void(const PM_INTROSPECTION_ROOT*)> deleter_)
        :
        pRoot{ pRoot_ },
        deleter{ std::move(deleter_) }
    {
        assert(pRoot);
        assert(deleter);
        // building lookup tables for enum/key
        for (auto e : GetEnums()) {
            for (auto k : e.GetKeys()) {
                enumKeyMap[MakeEnumKeyMapKey_(e.GetId(), k.GetId())] = k.GetBasePtr();
            }
            enumMap[e.GetId()] = e.GetBasePtr();
        }
        // building lookup table for devices
        for (auto d : GetDevices()) {
            deviceMap[d.GetId()] = d.GetBasePtr();
        }
        // building lookup table for metrics
        for (auto m : GetMetrics()) {
            metricMap[m.GetId()] = m.GetBasePtr();
        }
        // building lookup table for units
        for (auto u : GetUnits()) {
            unitMap[u.GetId()] = u.GetBasePtr();
        }
    }

    Root::~Root()
    {
        if (pRoot) {
            deleter(pRoot);
        }
    }

    ViewRange<EnumView> Root::GetEnums() const
    {
        // trying to deduce the template params for subrange causes intellisense to crash
        // workaround this by providing them explicitly as the return type (normally would use auto)
        return { GetEnumsBegin_(), GetEnumsEnd_() };
    }

    ViewRange<MetricView> Root::GetMetrics() const
    {
        // trying to deduce the template params for subrange causes intellisense to crash
        // workaround this by providing them explicitly as the return type (normally would use auto)
        return { GetMetricsBegin_(), GetMetricsEnd_() };
    }

    ViewRange<DeviceView> Root::GetDevices() const
    {
        // trying to deduce the template params for subrange causes intellisense to crash
        // workaround this by providing them explicitly as the return type (normally would use auto)
        return { GetDevicesBegin_(), GetDevicesEnd_() };
    }

    ViewRange<UnitView> Root::GetUnits() const
    {
        // trying to deduce the template params for subrange causes intellisense to crash
        // workaround this by providing them explicitly as the return type (normally would use auto)
        return { GetUnitsBegin_(), GetUnitsEnd_() };
    }

    EnumKeyView Root::FindEnumKey(PM_ENUM enumId, int keyValue) const
    {
        if (auto i = enumKeyMap.find(MakeEnumKeyMapKey_(enumId, keyValue)); i == enumKeyMap.end()) {
            throw LookupException{ std::format("unable to find key value={} for enum ID={}", keyValue, (int)enumId) };
        }
        else {
            return { this, i->second };
        }
    }

    EnumView Root::FindEnum(PM_ENUM enumId) const
    {
        if (auto i = enumMap.find(enumId); i == enumMap.end()) {
            throw LookupException{ std::format("unable to find enum ID={}", (int)enumId) };
        }
        else {
            return { this, i->second };
        }
    }

    DeviceView Root::FindDevice(uint32_t deviceId) const
    {
        if (auto i = deviceMap.find(deviceId); i == deviceMap.end()) {
            throw LookupException{ std::format("unable to find device ID={}", deviceId) };
        }
        else {
            return { this, i->second };
        }
    }

    MetricView Root::FindMetric(PM_METRIC metricId) const
    {
        if (auto i = metricMap.find(metricId); i == metricMap.end()) {
            pmlog_error(std::format("Cannot find metric id={} in introspection database", (int)metricId)).diag();
            throw LookupException{ std::format("unable to find metric ID={}", (int)metricId) };
        }
        else {
            return { this, i->second };
        }
    }

    UnitView Root::FindUnit(PM_UNIT unitId) const
    {
        if (auto i = unitMap.find(unitId); i == unitMap.end()) {
            throw LookupException{ std::format("unable to find unit ID={}", (int)unitId) };
        }
        else {
            return { this, i->second };
        }
    }

    uint64_t Root::MakeEnumKeyMapKey_(PM_ENUM enumId, int keyValue)
    {
        // pack 64-bit hash key as upper and lower 32-bit values
        return (uint64_t(enumId) << 32) | uint64_t(keyValue);
    }

    ViewIterator<EnumView> Root::GetEnumsBegin_() const
    {
        return ViewIterator<EnumView>{ this, pRoot->pEnums };
    }

    ViewIterator<EnumView> Root::GetEnumsEnd_() const
    {
        return ViewIterator<EnumView>{ this, pRoot->pEnums, (int64_t)pRoot->pEnums->size };
    }

    ViewIterator<MetricView> Root::GetMetricsBegin_() const
    {
        return ViewIterator<MetricView>{ this, pRoot->pMetrics };
    }

    ViewIterator<MetricView> Root::GetMetricsEnd_() const
    {
        return ViewIterator<MetricView>{ this, pRoot->pMetrics, (int64_t)pRoot->pMetrics->size };
    }

    ViewIterator<DeviceView> Root::GetDevicesBegin_() const
    {
        return ViewIterator<DeviceView>{ this, pRoot->pDevices };
    }

    ViewIterator<DeviceView> Root::GetDevicesEnd_() const
    {
        return ViewIterator<DeviceView>{ this, pRoot->pDevices, (int64_t)pRoot->pDevices->size };
    }

    ViewIterator<UnitView> Root::GetUnitsBegin_() const
    {
        return ViewIterator<UnitView>{ this, pRoot->pUnits };
    }

    ViewIterator<UnitView> Root::GetUnitsEnd_() const
    {
        return ViewIterator<UnitView>{ this, pRoot->pUnits, (int64_t)pRoot->pUnits->size };
    }
}