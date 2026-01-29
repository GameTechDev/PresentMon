#include "IntrospectionHelpers.h"
#include "IntrospectionMetadata.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionCapsLookup.h"
#include <ranges>
#include <optional>

namespace rn = std::ranges;
namespace vi = rn::views;

namespace pmon::ipc::intro
{
	void PopulateEnums(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root)
	{
		auto charAlloc = pSegmentManager->get_allocator<char>();

#define X_REG_KEYS(enum_frag, key_frag, name, short_name, description) REGISTER_ENUM_KEY(pSegmentManager, pEnum, enum_frag, key_frag, name, short_name, description);
#define X_REG_ENUMS(master_frag, enum_frag, name, short_name, description) { \
		auto pEnum = CREATE_INTROSPECTION_ENUM(pSegmentManager, enum_frag, description); \
		\
		MAKE_LIST_SYMBOL(enum_frag)(X_REG_KEYS) \
		root.AddEnum(std::move(pEnum)); }

		ENUM_KEY_LIST_ENUM(X_REG_ENUMS)

#undef X_REG_ENUMS
#undef X_REG_KEYS
	}

	template<PM_METRIC metricId>
	void RegisterUniversalMetricDeviceInfo_(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root, IntrospectionMetric& metric)
	{
		using Lookup = IntrospectionCapsLookup<metricId>;
		if constexpr (IsUniversalMetric<Lookup>) {
			metric.AddDeviceMetricInfo(IntrospectionDeviceMetricInfo{
				0, PM_METRIC_AVAILABILITY_AVAILABLE, 1 });
		}
	}

	void PopulateMetrics(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root)
	{
		std::unordered_map<PM_METRIC, PM_UNIT> preferredMetricOverrides;
#define X_PREF_UNIT(metric, unit) preferredMetricOverrides[metric] = unit;

		PREFERRED_UNIT_LIST(X_PREF_UNIT)

#undef X_REG_METRIC

#define X_REG_METRIC(metric, metric_type, unit, data_type_polled, data_type_frame, enum_id, device_type, ...) { \
		auto pMetric = ShmMakeUnique<IntrospectionMetric>(pSegmentManager, pSegmentManager, \
			metric, metric_type, unit, IntrospectionDataTypeInfo{ data_type_polled, data_type_frame, (PM_ENUM)enum_id }, std::vector{ __VA_ARGS__ }); \
		RegisterUniversalMetricDeviceInfo_<metric>(pSegmentManager, root, *pMetric); \
		if (preferredMetricOverrides.contains(metric)) pMetric->SetPreferredUnitHint(preferredMetricOverrides[metric]); \
		root.AddMetric(std::move(pMetric)); }

		METRIC_LIST(X_REG_METRIC)

#undef X_REG_METRIC
	}



	void PopulateUnits(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root)
	{
#define X_REG_UNIT(unit, baseUnit, scale) { \
		auto pUnit = ShmMakeUnique<IntrospectionUnit>(pSegmentManager, unit, baseUnit, scale); \
		root.AddUnit(std::move(pUnit)); }

		UNIT_LIST(X_REG_UNIT)

#undef X_REG_UNIT
	}

	template<PM_METRIC metric>
	void RegisterGpuMetricDeviceInfo_(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		uint32_t deviceId, const GpuTelemetryBitset& gpuCaps)
	{
		using Lookup = IntrospectionCapsLookup<metric>;
		std::optional<IntrospectionDeviceMetricInfo> info;
		if constexpr (IsUniversalMetric<Lookup>) {}
		else if constexpr (IsGpuDeviceMetric<Lookup>) {
			const auto availability = gpuCaps[size_t(Lookup::gpuCapBit)] ?
				PM_METRIC_AVAILABILITY_AVAILABLE : PM_METRIC_AVAILABILITY_UNAVAILABLE;
			info.emplace(deviceId, availability, 1);
		}
		else if constexpr (IsGpuDeviceMetricArray<Lookup>) {
			const auto nAvailable = (uint32_t)rn::count_if(Lookup::gpuCapBitArray, [&](auto bit){ return gpuCaps[size_t(bit)]; });
			const auto availability = nAvailable > 0 ?
				PM_METRIC_AVAILABILITY_AVAILABLE : PM_METRIC_AVAILABILITY_UNAVAILABLE;
			info.emplace(deviceId, availability, nAvailable);
		}
		else if constexpr (IsGpuDeviceStaticMetric<Lookup>) {
			info.emplace(deviceId, PM_METRIC_AVAILABILITY_AVAILABLE, 1);
		}
		if (info) {
			if (auto i = rn::find(root.GetMetrics(), metric, [](const ShmUniquePtr<IntrospectionMetric>& pMetric) {
				return pMetric->GetId();
			}); i != root.GetMetrics().end()) {
				(*i)->AddDeviceMetricInfo(std::move(*info));
			}
			// TODO: log metric not found
		}
	}

	void PopulateGpuDevice(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root, uint32_t deviceId,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const GpuTelemetryBitset& gpuCaps, std::span<const uint8_t> luidBytes)
	{
		// add the device
		auto charAlloc = pSegmentManager->get_allocator<char>();
        auto pLuid = ShmMakeUnique<IntrospectionDeviceLuid>(pSegmentManager, luidBytes, pSegmentManager);
		root.AddDevice(ShmMakeUnique<IntrospectionDevice>(pSegmentManager, deviceId,
			PM_DEVICE_TYPE_GRAPHICS_ADAPTER, vendor, ShmString{ deviceName.c_str(), charAlloc }, std::move(pLuid)));

		// populate device-metric info for this device
#define X_WALK_METRIC(metric, metric_type, unit, data_type, enum_id, device_type, ...) \
		RegisterGpuMetricDeviceInfo_<metric>(pSegmentManager, root, deviceId, gpuCaps);

		METRIC_LIST(X_WALK_METRIC)

#undef X_WALK_METRIC
	}

	template<PM_METRIC metric>
	void RegisterCpuMetricDeviceInfo_(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		const CpuTelemetryBitset& cpuCaps)
	{
		using Lookup = IntrospectionCapsLookup<metric>;
		if constexpr (IsCpuMetric<Lookup>) {
			if (auto i = rn::find(root.GetMetrics(), metric, [](const ShmUniquePtr<IntrospectionMetric>& pMetric) {
				return pMetric->GetId();
			}); i != root.GetMetrics().end()) {
				const auto availability = cpuCaps[size_t(Lookup::cpuCapBit)] ?
					PM_METRIC_AVAILABILITY_AVAILABLE : PM_METRIC_AVAILABILITY_UNAVAILABLE;
				(*i)->AddDeviceMetricInfo(IntrospectionDeviceMetricInfo{ 0, availability, 1 });
			}
		}

		// TODO: log metric not found
	}

	void PopulateCpu(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const CpuTelemetryBitset& cpuCaps)
	{
		// populate device-metric info for the cpu
#define X_WALK_METRIC(metric, metric_type, unit, data_type, enum_id, device_type, ...) \
		RegisterCpuMetricDeviceInfo_<metric>(pSegmentManager, root, cpuCaps);

		METRIC_LIST(X_WALK_METRIC)

#undef X_WALK_METRIC
	}
}