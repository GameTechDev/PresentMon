#pragma once
#include <unordered_map>
#include <functional>
#include <string>
#include <typeindex>
#include <type_traits>
#include <sstream>
#include <format>
#include "../GeneratedReflection.h"
#include "../GeneratedReflectionHelpers.h"

// target includes
#include "../../../../IntelPresentMon/ControlLib/igcl/igcl_api.h"
#include "../../../../IntelPresentMon/ControlLib/nvapi/nvapi.h"
#include "../../../../IntelPresentMon/ControlLib/nvml/nvml.h"
#include "../../../../IntelPresentMon/ControlLib/adl/adl_sdk.h"

namespace pmon::util::ref::gen
{
	using namespace std::literals;

	void RegisterDumpers_(std::unordered_map<std::type_index, std::function<std::string(const void*)>>& dumpers)
	{
		// rooted typedefs
		dumpers[typeid(ctl_mem_handle_t)] = [](const void* pTypedef) {
			const auto& s = *static_cast<const ctl_mem_handle_t*>(pTypedef);
			std::ostringstream oss;
			oss << std::boolalpha << "typedef ctl_mem_handle_t {"
				<< " .value = " << (s ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ctl_pwr_handle_t)] = [](const void* pTypedef) {
			const auto& s = *static_cast<const ctl_pwr_handle_t*>(pTypedef);
			std::ostringstream oss;
			oss << std::boolalpha << "typedef ctl_pwr_handle_t {"
				<< " .value = " << (s ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s)) : "null"s)
				<< " }";
			return oss.str();
		};

		// structs
		dumpers[typeid(_ctl_data_value_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_data_value_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ctl_data_value_t {"
				<< " .data8 = " << (int)s.data8
				<< " .datau8 = " << (int)s.datau8
				<< " .data16 = " << s.data16
				<< " .datau16 = " << s.datau16
				<< " .data32 = " << s.data32
				<< " .datau32 = " << s.datau32
				<< " .data64 = " << s.data64
				<< " .datau64 = " << s.datau64
				<< " .datafloat = " << s.datafloat
				<< " .datadouble = " << s.datadouble
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_application_id_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_application_id_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_application_id_t {"
				<< " .Data1 = " << s.Data1
				<< " .Data2 = " << s.Data2
				<< " .Data3 = " << s.Data3
				<< " .Data4 = " << DumpArray_(s.Data4, [&](const auto& elem_0) { return (int)elem_0; })
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_init_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_init_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_init_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .AppVersion = " << s.AppVersion
				<< " .flags = " << s.flags
				<< " .SupportedVersion = " << s.SupportedVersion
				<< " .ApplicationUID = " << DumpGenerated(s.ApplicationUID)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_firmware_version_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_firmware_version_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_firmware_version_t {"
				<< " .major_version = " << s.major_version
				<< " .minor_version = " << s.minor_version
				<< " .build_number = " << s.build_number
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_adapter_bdf_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_adapter_bdf_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_adapter_bdf_t {"
				<< " .bus = " << (int)s.bus
				<< " .device = " << (int)s.device
				<< " .function = " << (int)s.function
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_device_adapter_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_device_adapter_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_device_adapter_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .pDeviceID = " << (s.pDeviceID ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pDeviceID)) : "null"s)
				<< " .device_id_size = " << s.device_id_size
				<< " .device_type = " << DumpGenerated(s.device_type)
				<< " .supported_subfunction_flags = " << s.supported_subfunction_flags
				<< " .driver_version = " << s.driver_version
				<< " .firmware_version = " << DumpGenerated(s.firmware_version)
				<< " .pci_vendor_id = " << s.pci_vendor_id
				<< " .pci_device_id = " << s.pci_device_id
				<< " .rev_id = " << s.rev_id
				<< " .num_eus_per_sub_slice = " << s.num_eus_per_sub_slice
				<< " .num_sub_slices_per_slice = " << s.num_sub_slices_per_slice
				<< " .num_slices = " << s.num_slices
				<< " .name = " << s.name
				<< " .graphics_adapter_properties = " << s.graphics_adapter_properties
				<< " .Frequency = " << s.Frequency
				<< " .pci_subsys_id = " << s.pci_subsys_id
				<< " .pci_subsys_vendor_id = " << s.pci_subsys_vendor_id
				<< " .adapter_bdf = " << DumpGenerated(s.adapter_bdf)
				<< " .num_xe_cores = " << s.num_xe_cores
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_fan_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_fan_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_fan_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .canControl = " << s.canControl
				<< " .supportedModes = " << s.supportedModes
				<< " .supportedUnits = " << s.supportedUnits
				<< " .maxRPM = " << s.maxRPM
				<< " .maxPoints = " << s.maxPoints
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_mem_state_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_mem_state_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_mem_state_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .free = " << s.free
				<< " .size = " << s.size
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_mem_bandwidth_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_mem_bandwidth_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_mem_bandwidth_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .maxBandwidth = " << s.maxBandwidth
				<< " .timestamp = " << s.timestamp
				<< " .readCounter = " << s.readCounter
				<< " .writeCounter = " << s.writeCounter
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_oc_telemetry_item_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_oc_telemetry_item_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_oc_telemetry_item_t {"
				<< " .bSupported = " << s.bSupported
				<< " .units = " << DumpGenerated(s.units)
				<< " .type = " << DumpGenerated(s.type)
				<< " .value = " << DumpGenerated(s.value)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_psu_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_psu_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_psu_info_t {"
				<< " .bSupported = " << s.bSupported
				<< " .psuType = " << DumpGenerated(s.psuType)
				<< " .energyCounter = " << DumpGenerated(s.energyCounter)
				<< " .voltage = " << DumpGenerated(s.voltage)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_telemetry_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_telemetry_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_telemetry_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .timeStamp = " << DumpGenerated(s.timeStamp)
				<< " .gpuEnergyCounter = " << DumpGenerated(s.gpuEnergyCounter)
				<< " .gpuVoltage = " << DumpGenerated(s.gpuVoltage)
				<< " .gpuCurrentClockFrequency = " << DumpGenerated(s.gpuCurrentClockFrequency)
				<< " .gpuCurrentTemperature = " << DumpGenerated(s.gpuCurrentTemperature)
				<< " .globalActivityCounter = " << DumpGenerated(s.globalActivityCounter)
				<< " .renderComputeActivityCounter = " << DumpGenerated(s.renderComputeActivityCounter)
				<< " .mediaActivityCounter = " << DumpGenerated(s.mediaActivityCounter)
				<< " .gpuPowerLimited = " << s.gpuPowerLimited
				<< " .gpuTemperatureLimited = " << s.gpuTemperatureLimited
				<< " .gpuCurrentLimited = " << s.gpuCurrentLimited
				<< " .gpuVoltageLimited = " << s.gpuVoltageLimited
				<< " .gpuUtilizationLimited = " << s.gpuUtilizationLimited
				<< " .vramEnergyCounter = " << DumpGenerated(s.vramEnergyCounter)
				<< " .vramVoltage = " << DumpGenerated(s.vramVoltage)
				<< " .vramCurrentClockFrequency = " << DumpGenerated(s.vramCurrentClockFrequency)
				<< " .vramCurrentEffectiveFrequency = " << DumpGenerated(s.vramCurrentEffectiveFrequency)
				<< " .vramReadBandwidthCounter = " << DumpGenerated(s.vramReadBandwidthCounter)
				<< " .vramWriteBandwidthCounter = " << DumpGenerated(s.vramWriteBandwidthCounter)
				<< " .vramCurrentTemperature = " << DumpGenerated(s.vramCurrentTemperature)
				<< " .vramPowerLimited = " << s.vramPowerLimited
				<< " .vramTemperatureLimited = " << s.vramTemperatureLimited
				<< " .vramCurrentLimited = " << s.vramCurrentLimited
				<< " .vramVoltageLimited = " << s.vramVoltageLimited
				<< " .vramUtilizationLimited = " << s.vramUtilizationLimited
				<< " .totalCardEnergyCounter = " << DumpGenerated(s.totalCardEnergyCounter)
				<< " .psu = " << DumpArray_(s.psu, [&](const auto& elem_0) { return DumpGenerated(elem_0); })
				<< " .fanSpeed = " << DumpArray_(s.fanSpeed, [&](const auto& elem_0) { return DumpGenerated(elem_0); })
				<< " .gpuVrTemp = " << DumpGenerated(s.gpuVrTemp)
				<< " .vramVrTemp = " << DumpGenerated(s.vramVrTemp)
				<< " .saVrTemp = " << DumpGenerated(s.saVrTemp)
				<< " .gpuEffectiveClock = " << DumpGenerated(s.gpuEffectiveClock)
				<< " .gpuOverVoltagePercent = " << DumpGenerated(s.gpuOverVoltagePercent)
				<< " .gpuPowerPercent = " << DumpGenerated(s.gpuPowerPercent)
				<< " .gpuTemperaturePercent = " << DumpGenerated(s.gpuTemperaturePercent)
				<< " .vramReadBandwidth = " << DumpGenerated(s.vramReadBandwidth)
				<< " .vramWriteBandwidth = " << DumpGenerated(s.vramWriteBandwidth)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_sustained_limit_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_sustained_limit_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_sustained_limit_t {"
				<< " .enabled = " << s.enabled
				<< " .power = " << s.power
				<< " .interval = " << s.interval
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_burst_limit_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_burst_limit_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_burst_limit_t {"
				<< " .enabled = " << s.enabled
				<< " .power = " << s.power
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_peak_limit_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_peak_limit_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_peak_limit_t {"
				<< " .powerAC = " << s.powerAC
				<< " .powerDC = " << s.powerDC
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_limits_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_limits_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_limits_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .sustainedPowerLimit = " << DumpGenerated(s.sustainedPowerLimit)
				<< " .burstPowerLimit = " << DumpGenerated(s.burstPowerLimit)
				<< " .peakPowerLimits = " << DumpGenerated(s.peakPowerLimits)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_DYNAMIC_PSTATES_INFO_EX)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_DYNAMIC_PSTATES_INFO_EX*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_DYNAMIC_PSTATES_INFO_EX {"
				<< " .version = " << s.version
				<< " .flags = " << s.flags
				<< " .utilization = " << DumpArray_(s.utilization, [&](const auto& elem_0) { return ([&]() { std::ostringstream oss_1; oss_1 << std::boolalpha << "struct {"; oss_1 << " .bIsPresent = " << elem_0.bIsPresent; oss_1 << " .percentage = " << elem_0.percentage; oss_1 << " }"; return oss_1.str(); }()); })
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_THERMAL_SETTINGS_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_THERMAL_SETTINGS_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_THERMAL_SETTINGS_V2 {"
				<< " .version = " << s.version
				<< " .count = " << s.count
				<< " .sensor = " << DumpArray_(s.sensor, [&](const auto& elem_0) { return ([&]() { std::ostringstream oss_1; oss_1 << std::boolalpha << "struct {"; oss_1 << " .controller = " << DumpGenerated(elem_0.controller); oss_1 << " .defaultMinTemp = " << elem_0.defaultMinTemp; oss_1 << " .defaultMaxTemp = " << elem_0.defaultMaxTemp; oss_1 << " .currentTemp = " << elem_0.currentTemp; oss_1 << " .target = " << DumpGenerated(elem_0.target); oss_1 << " }"; return oss_1.str(); }()); })
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_CLOCK_FREQUENCIES_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_CLOCK_FREQUENCIES_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_CLOCK_FREQUENCIES_V2 {"
				<< " .version = " << s.version
				<< " .ClockType = " << s.ClockType
				<< " .reserved = " << s.reserved
				<< " .reserved1 = " << s.reserved1
				<< " .domain = " << DumpArray_(s.domain, [&](const auto& elem_0) { return ([&]() { std::ostringstream oss_1; oss_1 << std::boolalpha << "struct {"; oss_1 << " .bIsPresent = " << elem_0.bIsPresent; oss_1 << " .reserved = " << elem_0.reserved; oss_1 << " .frequency = " << elem_0.frequency; oss_1 << " }"; return oss_1.str(); }()); })
				<< " }";
			return oss.str();
		};
		dumpers[typeid(nvmlPciInfo_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const nvmlPciInfo_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct nvmlPciInfo_t {"
				<< " .bus = " << s.bus
				<< " .busId = " << s.busId
				<< " .busIdLegacy = " << s.busIdLegacy
				<< " .device = " << s.device
				<< " .domain = " << s.domain
				<< " .pciDeviceId = " << s.pciDeviceId
				<< " .pciSubSystemId = " << s.pciSubSystemId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(nvmlMemory_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const nvmlMemory_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct nvmlMemory_t {"
				<< " .free = " << s.free
				<< " .total = " << s.total
				<< " .used = " << s.used
				<< " }";
			return oss.str();
		};
		dumpers[typeid(AdapterInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const AdapterInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct AdapterInfo {"
				<< " .iSize = " << s.iSize
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .strUDID = " << s.strUDID
				<< " .iBusNumber = " << s.iBusNumber
				<< " .iDeviceNumber = " << s.iDeviceNumber
				<< " .iFunctionNumber = " << s.iFunctionNumber
				<< " .iVendorID = " << s.iVendorID
				<< " .strAdapterName = " << s.strAdapterName
				<< " .strDisplayName = " << s.strDisplayName
				<< " .iPresent = " << s.iPresent
				<< " .iExist = " << s.iExist
				<< " .strDriverPath = " << s.strDriverPath
				<< " .strDriverPathExt = " << s.strDriverPathExt
				<< " .strPNPString = " << s.strPNPString
				<< " .iOSDisplayIndex = " << s.iOSDisplayIndex
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMemoryInfoX4)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMemoryInfoX4*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMemoryInfoX4 {"
				<< " .iMemorySize = " << s.iMemorySize
				<< " .strMemoryType = " << s.strMemoryType
				<< " .iMemoryBandwidth = " << s.iMemoryBandwidth
				<< " .iHyperMemorySize = " << s.iHyperMemorySize
				<< " .iInvisibleMemorySize = " << s.iInvisibleMemorySize
				<< " .iVisibleMemorySize = " << s.iVisibleMemorySize
				<< " .iVramVendorRevId = " << s.iVramVendorRevId
				<< " .iMemoryBandwidthX2 = " << s.iMemoryBandwidthX2
				<< " .iMemoryBitRateX2 = " << s.iMemoryBitRateX2
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPMActivity)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPMActivity*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPMActivity {"
				<< " .iSize = " << s.iSize
				<< " .iEngineClock = " << s.iEngineClock
				<< " .iMemoryClock = " << s.iMemoryClock
				<< " .iVddc = " << s.iVddc
				<< " .iActivityPercent = " << s.iActivityPercent
				<< " .iCurrentPerformanceLevel = " << s.iCurrentPerformanceLevel
				<< " .iCurrentBusSpeed = " << s.iCurrentBusSpeed
				<< " .iCurrentBusLanes = " << s.iCurrentBusLanes
				<< " .iMaximumBusLanes = " << s.iMaximumBusLanes
				<< " .iReserved = " << s.iReserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLThermalControllerInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLThermalControllerInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLThermalControllerInfo {"
				<< " .iSize = " << s.iSize
				<< " .iThermalDomain = " << s.iThermalDomain
				<< " .iDomainIndex = " << s.iDomainIndex
				<< " .iFlags = " << s.iFlags
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLTemperature)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLTemperature*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLTemperature {"
				<< " .iSize = " << s.iSize
				<< " .iTemperature = " << s.iTemperature
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFanSpeedInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFanSpeedInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFanSpeedInfo {"
				<< " .iSize = " << s.iSize
				<< " .iFlags = " << s.iFlags
				<< " .iMinPercent = " << s.iMinPercent
				<< " .iMaxPercent = " << s.iMaxPercent
				<< " .iMinRPM = " << s.iMinRPM
				<< " .iMaxRPM = " << s.iMaxRPM
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFanSpeedValue)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFanSpeedValue*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFanSpeedValue {"
				<< " .iSize = " << s.iSize
				<< " .iSpeedType = " << s.iSpeedType
				<< " .iFanSpeed = " << s.iFanSpeed
				<< " .iFlags = " << s.iFlags
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6ParameterRange)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6ParameterRange*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6ParameterRange {"
				<< " .iMin = " << s.iMin
				<< " .iMax = " << s.iMax
				<< " .iStep = " << s.iStep
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6Capabilities)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6Capabilities*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6Capabilities {"
				<< " .iCapabilities = " << s.iCapabilities
				<< " .iSupportedStates = " << s.iSupportedStates
				<< " .iNumberOfPerformanceLevels = " << s.iNumberOfPerformanceLevels
				<< " .sEngineClockRange = " << DumpGenerated(s.sEngineClockRange)
				<< " .sMemoryClockRange = " << DumpGenerated(s.sMemoryClockRange)
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6CurrentStatus)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6CurrentStatus*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6CurrentStatus {"
				<< " .iEngineClock = " << s.iEngineClock
				<< " .iMemoryClock = " << s.iMemoryClock
				<< " .iActivityPercent = " << s.iActivityPercent
				<< " .iCurrentPerformanceLevel = " << s.iCurrentPerformanceLevel
				<< " .iCurrentBusSpeed = " << s.iCurrentBusSpeed
				<< " .iCurrentBusLanes = " << s.iCurrentBusLanes
				<< " .iMaximumBusLanes = " << s.iMaximumBusLanes
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6ThermalControllerCaps)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6ThermalControllerCaps*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6ThermalControllerCaps {"
				<< " .iCapabilities = " << s.iCapabilities
				<< " .iFanMinPercent = " << s.iFanMinPercent
				<< " .iFanMaxPercent = " << s.iFanMaxPercent
				<< " .iFanMinRPM = " << s.iFanMinRPM
				<< " .iFanMaxRPM = " << s.iFanMaxRPM
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6FanSpeedInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6FanSpeedInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6FanSpeedInfo {"
				<< " .iSpeedType = " << s.iSpeedType
				<< " .iFanSpeedPercent = " << s.iFanSpeedPercent
				<< " .iFanSpeedRPM = " << s.iFanSpeedRPM
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNParameterRange)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNParameterRange*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNParameterRange {"
				<< " .iMode = " << s.iMode
				<< " .iMin = " << s.iMin
				<< " .iMax = " << s.iMax
				<< " .iStep = " << s.iStep
				<< " .iDefault = " << s.iDefault
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNCapabilitiesX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNCapabilitiesX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNCapabilitiesX2 {"
				<< " .iMaximumNumberOfPerformanceLevels = " << s.iMaximumNumberOfPerformanceLevels
				<< " .iFlags = " << s.iFlags
				<< " .sEngineClockRange = " << DumpGenerated(s.sEngineClockRange)
				<< " .sMemoryClockRange = " << DumpGenerated(s.sMemoryClockRange)
				<< " .svddcRange = " << DumpGenerated(s.svddcRange)
				<< " .power = " << DumpGenerated(s.power)
				<< " .powerTuneTemperature = " << DumpGenerated(s.powerTuneTemperature)
				<< " .fanTemperature = " << DumpGenerated(s.fanTemperature)
				<< " .fanSpeed = " << DumpGenerated(s.fanSpeed)
				<< " .minimumPerformanceClock = " << DumpGenerated(s.minimumPerformanceClock)
				<< " .throttleNotificaion = " << DumpGenerated(s.throttleNotificaion)
				<< " .autoSystemClock = " << DumpGenerated(s.autoSystemClock)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNFanControl)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNFanControl*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNFanControl {"
				<< " .iMode = " << s.iMode
				<< " .iFanControlMode = " << s.iFanControlMode
				<< " .iCurrentFanSpeedMode = " << s.iCurrentFanSpeedMode
				<< " .iCurrentFanSpeed = " << s.iCurrentFanSpeed
				<< " .iTargetFanSpeed = " << s.iTargetFanSpeed
				<< " .iTargetTemperature = " << s.iTargetTemperature
				<< " .iMinPerformanceClock = " << s.iMinPerformanceClock
				<< " .iMinFanLimit = " << s.iMinFanLimit
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNPerformanceStatus)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNPerformanceStatus*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNPerformanceStatus {"
				<< " .iCoreClock = " << s.iCoreClock
				<< " .iMemoryClock = " << s.iMemoryClock
				<< " .iDCEFClock = " << s.iDCEFClock
				<< " .iGFXClock = " << s.iGFXClock
				<< " .iUVDClock = " << s.iUVDClock
				<< " .iVCEClock = " << s.iVCEClock
				<< " .iGPUActivityPercent = " << s.iGPUActivityPercent
				<< " .iCurrentCorePerformanceLevel = " << s.iCurrentCorePerformanceLevel
				<< " .iCurrentMemoryPerformanceLevel = " << s.iCurrentMemoryPerformanceLevel
				<< " .iCurrentDCEFPerformanceLevel = " << s.iCurrentDCEFPerformanceLevel
				<< " .iCurrentGFXPerformanceLevel = " << s.iCurrentGFXPerformanceLevel
				<< " .iUVDPerformanceLevel = " << s.iUVDPerformanceLevel
				<< " .iVCEPerformanceLevel = " << s.iVCEPerformanceLevel
				<< " .iCurrentBusSpeed = " << s.iCurrentBusSpeed
				<< " .iCurrentBusLanes = " << s.iCurrentBusLanes
				<< " .iMaximumBusLanes = " << s.iMaximumBusLanes
				<< " .iVDDC = " << s.iVDDC
				<< " .iVDDCI = " << s.iVDDCI
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSingleSensorData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSingleSensorData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSingleSensorData {"
				<< " .supported = " << s.supported
				<< " .value = " << s.value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPMLogDataOutput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPMLogDataOutput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPMLogDataOutput {"
				<< " .size = " << s.size
				<< " .sensors = " << DumpArray_(s.sensors, [&](const auto& elem_0) { return DumpGenerated(elem_0); })
				<< " }";
			return oss.str();
		};

		// enums
		dumpers[typeid(_ctl_result_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_result_t*>(pEnum);
			switch (e) {
			case _ctl_result_t::CTL_RESULT_SUCCESS: return "CTL_RESULT_SUCCESS"s;
			case _ctl_result_t::CTL_RESULT_SUCCESS_STILL_OPEN_BY_ANOTHER_CALLER: return "CTL_RESULT_SUCCESS_STILL_OPEN_BY_ANOTHER_CALLER"s;
			case _ctl_result_t::CTL_RESULT_ERROR_SUCCESS_END: return "CTL_RESULT_ERROR_SUCCESS_END"s;
			case _ctl_result_t::CTL_RESULT_ERROR_GENERIC_START: return "CTL_RESULT_ERROR_GENERIC_START"s;
			case _ctl_result_t::CTL_RESULT_ERROR_NOT_INITIALIZED: return "CTL_RESULT_ERROR_NOT_INITIALIZED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_ALREADY_INITIALIZED: return "CTL_RESULT_ERROR_ALREADY_INITIALIZED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DEVICE_LOST: return "CTL_RESULT_ERROR_DEVICE_LOST"s;
			case _ctl_result_t::CTL_RESULT_ERROR_OUT_OF_HOST_MEMORY: return "CTL_RESULT_ERROR_OUT_OF_HOST_MEMORY"s;
			case _ctl_result_t::CTL_RESULT_ERROR_OUT_OF_DEVICE_MEMORY: return "CTL_RESULT_ERROR_OUT_OF_DEVICE_MEMORY"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS: return "CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS"s;
			case _ctl_result_t::CTL_RESULT_ERROR_NOT_AVAILABLE: return "CTL_RESULT_ERROR_NOT_AVAILABLE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNINITIALIZED: return "CTL_RESULT_ERROR_UNINITIALIZED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNSUPPORTED_VERSION: return "CTL_RESULT_ERROR_UNSUPPORTED_VERSION"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNSUPPORTED_FEATURE: return "CTL_RESULT_ERROR_UNSUPPORTED_FEATURE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_ARGUMENT: return "CTL_RESULT_ERROR_INVALID_ARGUMENT"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_API_HANDLE: return "CTL_RESULT_ERROR_INVALID_API_HANDLE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_NULL_HANDLE: return "CTL_RESULT_ERROR_INVALID_NULL_HANDLE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_NULL_POINTER: return "CTL_RESULT_ERROR_INVALID_NULL_POINTER"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_SIZE: return "CTL_RESULT_ERROR_INVALID_SIZE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNSUPPORTED_SIZE: return "CTL_RESULT_ERROR_UNSUPPORTED_SIZE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT: return "CTL_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DATA_READ: return "CTL_RESULT_ERROR_DATA_READ"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DATA_WRITE: return "CTL_RESULT_ERROR_DATA_WRITE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DATA_NOT_FOUND: return "CTL_RESULT_ERROR_DATA_NOT_FOUND"s;
			case _ctl_result_t::CTL_RESULT_ERROR_NOT_IMPLEMENTED: return "CTL_RESULT_ERROR_NOT_IMPLEMENTED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_OS_CALL: return "CTL_RESULT_ERROR_OS_CALL"s;
			case _ctl_result_t::CTL_RESULT_ERROR_KMD_CALL: return "CTL_RESULT_ERROR_KMD_CALL"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNLOAD: return "CTL_RESULT_ERROR_UNLOAD"s;
			case _ctl_result_t::CTL_RESULT_ERROR_ZE_LOADER: return "CTL_RESULT_ERROR_ZE_LOADER"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE: return "CTL_RESULT_ERROR_INVALID_OPERATION_TYPE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_NULL_OS_INTERFACE: return "CTL_RESULT_ERROR_NULL_OS_INTERFACE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE: return "CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE: return "CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_WAIT_TIMEOUT: return "CTL_RESULT_ERROR_WAIT_TIMEOUT"s;
			case _ctl_result_t::CTL_RESULT_ERROR_PERSISTANCE_NOT_SUPPORTED: return "CTL_RESULT_ERROR_PERSISTANCE_NOT_SUPPORTED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_PLATFORM_NOT_SUPPORTED: return "CTL_RESULT_ERROR_PLATFORM_NOT_SUPPORTED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNKNOWN_APPLICATION_UID: return "CTL_RESULT_ERROR_UNKNOWN_APPLICATION_UID"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_ENUMERATION: return "CTL_RESULT_ERROR_INVALID_ENUMERATION"s;
			case _ctl_result_t::CTL_RESULT_ERROR_FILE_DELETE: return "CTL_RESULT_ERROR_FILE_DELETE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_RESET_DEVICE_REQUIRED: return "CTL_RESULT_ERROR_RESET_DEVICE_REQUIRED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_FULL_REBOOT_REQUIRED: return "CTL_RESULT_ERROR_FULL_REBOOT_REQUIRED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_LOAD: return "CTL_RESULT_ERROR_LOAD"s;
			case _ctl_result_t::CTL_RESULT_ERROR_UNKNOWN: return "CTL_RESULT_ERROR_UNKNOWN"s;
			case _ctl_result_t::CTL_RESULT_ERROR_RETRY_OPERATION: return "CTL_RESULT_ERROR_RETRY_OPERATION"s;
			case _ctl_result_t::CTL_RESULT_ERROR_IGSC_LOADER: return "CTL_RESULT_ERROR_IGSC_LOADER"s;
			case _ctl_result_t::CTL_RESULT_ERROR_RESTRICTED_APPLICATION: return "CTL_RESULT_ERROR_RESTRICTED_APPLICATION"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_START: return "CTL_RESULT_ERROR_CORE_START"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_NOT_SUPPORTED: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_NOT_SUPPORTED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_VOLTAGE_OUTSIDE_RANGE: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_VOLTAGE_OUTSIDE_RANGE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_FREQUENCY_OUTSIDE_RANGE: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_FREQUENCY_OUTSIDE_RANGE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_POWER_OUTSIDE_RANGE: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_POWER_OUTSIDE_RANGE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_TEMPERATURE_OUTSIDE_RANGE: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_TEMPERATURE_OUTSIDE_RANGE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_IN_VOLTAGE_LOCKED_MODE: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_IN_VOLTAGE_LOCKED_MODE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_RESET_REQUIRED: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_RESET_REQUIRED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_WAIVER_NOT_SET: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_WAIVER_NOT_SET"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_DEPRECATED_API: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_DEPRECATED_API"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_LED_GET_STATE_NOT_SUPPORTED_FOR_I2C_LED: return "CTL_RESULT_ERROR_CORE_LED_GET_STATE_NOT_SUPPORTED_FOR_I2C_LED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_LED_SET_STATE_NOT_SUPPORTED_FOR_I2C_LED: return "CTL_RESULT_ERROR_CORE_LED_SET_STATE_NOT_SUPPORTED_FOR_I2C_LED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_LED_TOO_FREQUENT_SET_REQUESTS: return "CTL_RESULT_ERROR_CORE_LED_TOO_FREQUENT_SET_REQUESTS"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_VRAM_MEMORY_SPEED_OUTSIDE_RANGE: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_VRAM_MEMORY_SPEED_OUTSIDE_RANGE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_OVERCLOCK_INVALID_CUSTOM_VF_CURVE: return "CTL_RESULT_ERROR_CORE_OVERCLOCK_INVALID_CUSTOM_VF_CURVE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CORE_END: return "CTL_RESULT_ERROR_CORE_END"s;
			case _ctl_result_t::CTL_RESULT_ERROR_3D_START: return "CTL_RESULT_ERROR_3D_START"s;
			case _ctl_result_t::CTL_RESULT_ERROR_3D_END: return "CTL_RESULT_ERROR_3D_END"s;
			case _ctl_result_t::CTL_RESULT_ERROR_MEDIA_START: return "CTL_RESULT_ERROR_MEDIA_START"s;
			case _ctl_result_t::CTL_RESULT_ERROR_MEDIA_END: return "CTL_RESULT_ERROR_MEDIA_END"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DISPLAY_START: return "CTL_RESULT_ERROR_DISPLAY_START"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_AUX_ACCESS_FLAG: return "CTL_RESULT_ERROR_INVALID_AUX_ACCESS_FLAG"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_SHARPNESS_FILTER_FLAG: return "CTL_RESULT_ERROR_INVALID_SHARPNESS_FILTER_FLAG"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DISPLAY_NOT_ATTACHED: return "CTL_RESULT_ERROR_DISPLAY_NOT_ATTACHED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DISPLAY_NOT_ACTIVE: return "CTL_RESULT_ERROR_DISPLAY_NOT_ACTIVE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_POWERFEATURE_OPTIMIZATION_FLAG: return "CTL_RESULT_ERROR_INVALID_POWERFEATURE_OPTIMIZATION_FLAG"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_POWERSOURCE_TYPE_FOR_DPST: return "CTL_RESULT_ERROR_INVALID_POWERSOURCE_TYPE_FOR_DPST"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_PIXTX_GET_CONFIG_QUERY_TYPE: return "CTL_RESULT_ERROR_INVALID_PIXTX_GET_CONFIG_QUERY_TYPE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_PIXTX_SET_CONFIG_OPERATION_TYPE: return "CTL_RESULT_ERROR_INVALID_PIXTX_SET_CONFIG_OPERATION_TYPE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_SET_CONFIG_NUMBER_OF_SAMPLES: return "CTL_RESULT_ERROR_INVALID_SET_CONFIG_NUMBER_OF_SAMPLES"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_ID: return "CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_ID"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_TYPE: return "CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_TYPE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_NUMBER: return "CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_NUMBER"s;
			case _ctl_result_t::CTL_RESULT_ERROR_INSUFFICIENT_PIXTX_BLOCK_CONFIG_MEMORY: return "CTL_RESULT_ERROR_INSUFFICIENT_PIXTX_BLOCK_CONFIG_MEMORY"s;
			case _ctl_result_t::CTL_RESULT_ERROR_3DLUT_INVALID_PIPE: return "CTL_RESULT_ERROR_3DLUT_INVALID_PIPE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_3DLUT_INVALID_DATA: return "CTL_RESULT_ERROR_3DLUT_INVALID_DATA"s;
			case _ctl_result_t::CTL_RESULT_ERROR_3DLUT_NOT_SUPPORTED_IN_HDR: return "CTL_RESULT_ERROR_3DLUT_NOT_SUPPORTED_IN_HDR"s;
			case _ctl_result_t::CTL_RESULT_ERROR_3DLUT_INVALID_OPERATION: return "CTL_RESULT_ERROR_3DLUT_INVALID_OPERATION"s;
			case _ctl_result_t::CTL_RESULT_ERROR_3DLUT_UNSUCCESSFUL: return "CTL_RESULT_ERROR_3DLUT_UNSUCCESSFUL"s;
			case _ctl_result_t::CTL_RESULT_ERROR_AUX_DEFER: return "CTL_RESULT_ERROR_AUX_DEFER"s;
			case _ctl_result_t::CTL_RESULT_ERROR_AUX_TIMEOUT: return "CTL_RESULT_ERROR_AUX_TIMEOUT"s;
			case _ctl_result_t::CTL_RESULT_ERROR_AUX_INCOMPLETE_WRITE: return "CTL_RESULT_ERROR_AUX_INCOMPLETE_WRITE"s;
			case _ctl_result_t::CTL_RESULT_ERROR_I2C_AUX_STATUS_UNKNOWN: return "CTL_RESULT_ERROR_I2C_AUX_STATUS_UNKNOWN"s;
			case _ctl_result_t::CTL_RESULT_ERROR_I2C_AUX_UNSUCCESSFUL: return "CTL_RESULT_ERROR_I2C_AUX_UNSUCCESSFUL"s;
			case _ctl_result_t::CTL_RESULT_ERROR_LACE_INVALID_DATA_ARGUMENT_PASSED: return "CTL_RESULT_ERROR_LACE_INVALID_DATA_ARGUMENT_PASSED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_EXTERNAL_DISPLAY_ATTACHED: return "CTL_RESULT_ERROR_EXTERNAL_DISPLAY_ATTACHED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CUSTOM_MODE_STANDARD_CUSTOM_MODE_EXISTS: return "CTL_RESULT_ERROR_CUSTOM_MODE_STANDARD_CUSTOM_MODE_EXISTS"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CUSTOM_MODE_NON_CUSTOM_MATCHING_MODE_EXISTS: return "CTL_RESULT_ERROR_CUSTOM_MODE_NON_CUSTOM_MATCHING_MODE_EXISTS"s;
			case _ctl_result_t::CTL_RESULT_ERROR_CUSTOM_MODE_INSUFFICIENT_MEMORY: return "CTL_RESULT_ERROR_CUSTOM_MODE_INSUFFICIENT_MEMORY"s;
			case _ctl_result_t::CTL_RESULT_ERROR_ADAPTER_ALREADY_LINKED: return "CTL_RESULT_ERROR_ADAPTER_ALREADY_LINKED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_ADAPTER_NOT_IDENTICAL: return "CTL_RESULT_ERROR_ADAPTER_NOT_IDENTICAL"s;
			case _ctl_result_t::CTL_RESULT_ERROR_ADAPTER_NOT_SUPPORTED_ON_LDA_SECONDARY: return "CTL_RESULT_ERROR_ADAPTER_NOT_SUPPORTED_ON_LDA_SECONDARY"s;
			case _ctl_result_t::CTL_RESULT_ERROR_SET_FBC_FEATURE_NOT_SUPPORTED: return "CTL_RESULT_ERROR_SET_FBC_FEATURE_NOT_SUPPORTED"s;
			case _ctl_result_t::CTL_RESULT_ERROR_DISPLAY_END: return "CTL_RESULT_ERROR_DISPLAY_END"s;
			case _ctl_result_t::CTL_RESULT_MAX: return "CTL_RESULT_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_units_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_units_t*>(pEnum);
			switch (e) {
			case _ctl_units_t::CTL_UNITS_FREQUENCY_MHZ: return "CTL_UNITS_FREQUENCY_MHZ"s;
			case _ctl_units_t::CTL_UNITS_OPERATIONS_GTS: return "CTL_UNITS_OPERATIONS_GTS"s;
			case _ctl_units_t::CTL_UNITS_OPERATIONS_MTS: return "CTL_UNITS_OPERATIONS_MTS"s;
			case _ctl_units_t::CTL_UNITS_VOLTAGE_VOLTS: return "CTL_UNITS_VOLTAGE_VOLTS"s;
			case _ctl_units_t::CTL_UNITS_POWER_WATTS: return "CTL_UNITS_POWER_WATTS"s;
			case _ctl_units_t::CTL_UNITS_TEMPERATURE_CELSIUS: return "CTL_UNITS_TEMPERATURE_CELSIUS"s;
			case _ctl_units_t::CTL_UNITS_ENERGY_JOULES: return "CTL_UNITS_ENERGY_JOULES"s;
			case _ctl_units_t::CTL_UNITS_TIME_SECONDS: return "CTL_UNITS_TIME_SECONDS"s;
			case _ctl_units_t::CTL_UNITS_MEMORY_BYTES: return "CTL_UNITS_MEMORY_BYTES"s;
			case _ctl_units_t::CTL_UNITS_ANGULAR_SPEED_RPM: return "CTL_UNITS_ANGULAR_SPEED_RPM"s;
			case _ctl_units_t::CTL_UNITS_POWER_MILLIWATTS: return "CTL_UNITS_POWER_MILLIWATTS"s;
			case _ctl_units_t::CTL_UNITS_PERCENT: return "CTL_UNITS_PERCENT"s;
			case _ctl_units_t::CTL_UNITS_MEM_SPEED_GBPS: return "CTL_UNITS_MEM_SPEED_GBPS"s;
			case _ctl_units_t::CTL_UNITS_VOLTAGE_MILLIVOLTS: return "CTL_UNITS_VOLTAGE_MILLIVOLTS"s;
			case _ctl_units_t::CTL_UNITS_BANDWIDTH_MBPS: return "CTL_UNITS_BANDWIDTH_MBPS"s;
			case _ctl_units_t::CTL_UNITS_UNKNOWN: return "CTL_UNITS_UNKNOWN"s;
			case _ctl_units_t::CTL_UNITS_MAX: return "CTL_UNITS_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_data_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_data_type_t*>(pEnum);
			switch (e) {
			case _ctl_data_type_t::CTL_DATA_TYPE_INT8: return "CTL_DATA_TYPE_INT8"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_UINT8: return "CTL_DATA_TYPE_UINT8"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_INT16: return "CTL_DATA_TYPE_INT16"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_UINT16: return "CTL_DATA_TYPE_UINT16"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_INT32: return "CTL_DATA_TYPE_INT32"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_UINT32: return "CTL_DATA_TYPE_UINT32"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_INT64: return "CTL_DATA_TYPE_INT64"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_UINT64: return "CTL_DATA_TYPE_UINT64"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_FLOAT: return "CTL_DATA_TYPE_FLOAT"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_DOUBLE: return "CTL_DATA_TYPE_DOUBLE"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_STRING_ASCII: return "CTL_DATA_TYPE_STRING_ASCII"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_STRING_UTF16: return "CTL_DATA_TYPE_STRING_UTF16"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_STRING_UTF132: return "CTL_DATA_TYPE_STRING_UTF132"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_UNKNOWN: return "CTL_DATA_TYPE_UNKNOWN"s;
			case _ctl_data_type_t::CTL_DATA_TYPE_MAX: return "CTL_DATA_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_device_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_device_type_t*>(pEnum);
			switch (e) {
			case _ctl_device_type_t::CTL_DEVICE_TYPE_GRAPHICS: return "CTL_DEVICE_TYPE_GRAPHICS"s;
			case _ctl_device_type_t::CTL_DEVICE_TYPE_SYSTEM: return "CTL_DEVICE_TYPE_SYSTEM"s;
			case _ctl_device_type_t::CTL_DEVICE_TYPE_MAX: return "CTL_DEVICE_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_psu_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_psu_type_t*>(pEnum);
			switch (e) {
			case _ctl_psu_type_t::CTL_PSU_TYPE_PSU_NONE: return "CTL_PSU_TYPE_PSU_NONE"s;
			case _ctl_psu_type_t::CTL_PSU_TYPE_PSU_PCIE: return "CTL_PSU_TYPE_PSU_PCIE"s;
			case _ctl_psu_type_t::CTL_PSU_TYPE_PSU_6PIN: return "CTL_PSU_TYPE_PSU_6PIN"s;
			case _ctl_psu_type_t::CTL_PSU_TYPE_PSU_8PIN: return "CTL_PSU_TYPE_PSU_8PIN"s;
			case _ctl_psu_type_t::CTL_PSU_TYPE_MAX: return "CTL_PSU_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NvAPI_Status)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NvAPI_Status*>(pEnum);
			switch (e) {
			case _NvAPI_Status::NVAPI_OK: return "NVAPI_OK"s;
			case _NvAPI_Status::NVAPI_ERROR: return "NVAPI_ERROR"s;
			case _NvAPI_Status::NVAPI_LIBRARY_NOT_FOUND: return "NVAPI_LIBRARY_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_NO_IMPLEMENTATION: return "NVAPI_NO_IMPLEMENTATION"s;
			case _NvAPI_Status::NVAPI_API_NOT_INITIALIZED: return "NVAPI_API_NOT_INITIALIZED"s;
			case _NvAPI_Status::NVAPI_INVALID_ARGUMENT: return "NVAPI_INVALID_ARGUMENT"s;
			case _NvAPI_Status::NVAPI_NVIDIA_DEVICE_NOT_FOUND: return "NVAPI_NVIDIA_DEVICE_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_END_ENUMERATION: return "NVAPI_END_ENUMERATION"s;
			case _NvAPI_Status::NVAPI_INVALID_HANDLE: return "NVAPI_INVALID_HANDLE"s;
			case _NvAPI_Status::NVAPI_INCOMPATIBLE_STRUCT_VERSION: return "NVAPI_INCOMPATIBLE_STRUCT_VERSION"s;
			case _NvAPI_Status::NVAPI_HANDLE_INVALIDATED: return "NVAPI_HANDLE_INVALIDATED"s;
			case _NvAPI_Status::NVAPI_OPENGL_CONTEXT_NOT_CURRENT: return "NVAPI_OPENGL_CONTEXT_NOT_CURRENT"s;
			case _NvAPI_Status::NVAPI_INVALID_POINTER: return "NVAPI_INVALID_POINTER"s;
			case _NvAPI_Status::NVAPI_NO_GL_EXPERT: return "NVAPI_NO_GL_EXPERT"s;
			case _NvAPI_Status::NVAPI_INSTRUMENTATION_DISABLED: return "NVAPI_INSTRUMENTATION_DISABLED"s;
			case _NvAPI_Status::NVAPI_NO_GL_NSIGHT: return "NVAPI_NO_GL_NSIGHT"s;
			case _NvAPI_Status::NVAPI_EXPECTED_LOGICAL_GPU_HANDLE: return "NVAPI_EXPECTED_LOGICAL_GPU_HANDLE"s;
			case _NvAPI_Status::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: return "NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE"s;
			case _NvAPI_Status::NVAPI_EXPECTED_DISPLAY_HANDLE: return "NVAPI_EXPECTED_DISPLAY_HANDLE"s;
			case _NvAPI_Status::NVAPI_INVALID_COMBINATION: return "NVAPI_INVALID_COMBINATION"s;
			case _NvAPI_Status::NVAPI_NOT_SUPPORTED: return "NVAPI_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_PORTID_NOT_FOUND: return "NVAPI_PORTID_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_EXPECTED_UNATTACHED_DISPLAY_HANDLE: return "NVAPI_EXPECTED_UNATTACHED_DISPLAY_HANDLE"s;
			case _NvAPI_Status::NVAPI_INVALID_PERF_LEVEL: return "NVAPI_INVALID_PERF_LEVEL"s;
			case _NvAPI_Status::NVAPI_DEVICE_BUSY: return "NVAPI_DEVICE_BUSY"s;
			case _NvAPI_Status::NVAPI_NV_PERSIST_FILE_NOT_FOUND: return "NVAPI_NV_PERSIST_FILE_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_PERSIST_DATA_NOT_FOUND: return "NVAPI_PERSIST_DATA_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_EXPECTED_TV_DISPLAY: return "NVAPI_EXPECTED_TV_DISPLAY"s;
			case _NvAPI_Status::NVAPI_EXPECTED_TV_DISPLAY_ON_DCONNECTOR: return "NVAPI_EXPECTED_TV_DISPLAY_ON_DCONNECTOR"s;
			case _NvAPI_Status::NVAPI_NO_ACTIVE_SLI_TOPOLOGY: return "NVAPI_NO_ACTIVE_SLI_TOPOLOGY"s;
			case _NvAPI_Status::NVAPI_SLI_RENDERING_MODE_NOTALLOWED: return "NVAPI_SLI_RENDERING_MODE_NOTALLOWED"s;
			case _NvAPI_Status::NVAPI_EXPECTED_DIGITAL_FLAT_PANEL: return "NVAPI_EXPECTED_DIGITAL_FLAT_PANEL"s;
			case _NvAPI_Status::NVAPI_ARGUMENT_EXCEED_MAX_SIZE: return "NVAPI_ARGUMENT_EXCEED_MAX_SIZE"s;
			case _NvAPI_Status::NVAPI_DEVICE_SWITCHING_NOT_ALLOWED: return "NVAPI_DEVICE_SWITCHING_NOT_ALLOWED"s;
			case _NvAPI_Status::NVAPI_TESTING_CLOCKS_NOT_SUPPORTED: return "NVAPI_TESTING_CLOCKS_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_UNKNOWN_UNDERSCAN_CONFIG: return "NVAPI_UNKNOWN_UNDERSCAN_CONFIG"s;
			case _NvAPI_Status::NVAPI_TIMEOUT_RECONFIGURING_GPU_TOPO: return "NVAPI_TIMEOUT_RECONFIGURING_GPU_TOPO"s;
			case _NvAPI_Status::NVAPI_DATA_NOT_FOUND: return "NVAPI_DATA_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_EXPECTED_ANALOG_DISPLAY: return "NVAPI_EXPECTED_ANALOG_DISPLAY"s;
			case _NvAPI_Status::NVAPI_NO_VIDLINK: return "NVAPI_NO_VIDLINK"s;
			case _NvAPI_Status::NVAPI_REQUIRES_REBOOT: return "NVAPI_REQUIRES_REBOOT"s;
			case _NvAPI_Status::NVAPI_INVALID_HYBRID_MODE: return "NVAPI_INVALID_HYBRID_MODE"s;
			case _NvAPI_Status::NVAPI_MIXED_TARGET_TYPES: return "NVAPI_MIXED_TARGET_TYPES"s;
			case _NvAPI_Status::NVAPI_SYSWOW64_NOT_SUPPORTED: return "NVAPI_SYSWOW64_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_IMPLICIT_SET_GPU_TOPOLOGY_CHANGE_NOT_ALLOWED: return "NVAPI_IMPLICIT_SET_GPU_TOPOLOGY_CHANGE_NOT_ALLOWED"s;
			case _NvAPI_Status::NVAPI_REQUEST_USER_TO_CLOSE_NON_MIGRATABLE_APPS: return "NVAPI_REQUEST_USER_TO_CLOSE_NON_MIGRATABLE_APPS"s;
			case _NvAPI_Status::NVAPI_OUT_OF_MEMORY: return "NVAPI_OUT_OF_MEMORY"s;
			case _NvAPI_Status::NVAPI_WAS_STILL_DRAWING: return "NVAPI_WAS_STILL_DRAWING"s;
			case _NvAPI_Status::NVAPI_FILE_NOT_FOUND: return "NVAPI_FILE_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_TOO_MANY_UNIQUE_STATE_OBJECTS: return "NVAPI_TOO_MANY_UNIQUE_STATE_OBJECTS"s;
			case _NvAPI_Status::NVAPI_INVALID_CALL: return "NVAPI_INVALID_CALL"s;
			case _NvAPI_Status::NVAPI_D3D10_1_LIBRARY_NOT_FOUND: return "NVAPI_D3D10_1_LIBRARY_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_FUNCTION_NOT_FOUND: return "NVAPI_FUNCTION_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_INVALID_USER_PRIVILEGE: return "NVAPI_INVALID_USER_PRIVILEGE"s;
			case _NvAPI_Status::NVAPI_EXPECTED_NON_PRIMARY_DISPLAY_HANDLE: return "NVAPI_EXPECTED_NON_PRIMARY_DISPLAY_HANDLE"s;
			case _NvAPI_Status::NVAPI_EXPECTED_COMPUTE_GPU_HANDLE: return "NVAPI_EXPECTED_COMPUTE_GPU_HANDLE"s;
			case _NvAPI_Status::NVAPI_STEREO_NOT_INITIALIZED: return "NVAPI_STEREO_NOT_INITIALIZED"s;
			case _NvAPI_Status::NVAPI_STEREO_REGISTRY_ACCESS_FAILED: return "NVAPI_STEREO_REGISTRY_ACCESS_FAILED"s;
			case _NvAPI_Status::NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED: return "NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED: return "NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_STEREO_NOT_ENABLED: return "NVAPI_STEREO_NOT_ENABLED"s;
			case _NvAPI_Status::NVAPI_STEREO_NOT_TURNED_ON: return "NVAPI_STEREO_NOT_TURNED_ON"s;
			case _NvAPI_Status::NVAPI_STEREO_INVALID_DEVICE_INTERFACE: return "NVAPI_STEREO_INVALID_DEVICE_INTERFACE"s;
			case _NvAPI_Status::NVAPI_STEREO_PARAMETER_OUT_OF_RANGE: return "NVAPI_STEREO_PARAMETER_OUT_OF_RANGE"s;
			case _NvAPI_Status::NVAPI_STEREO_FRUSTUM_ADJUST_MODE_NOT_SUPPORTED: return "NVAPI_STEREO_FRUSTUM_ADJUST_MODE_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_TOPO_NOT_POSSIBLE: return "NVAPI_TOPO_NOT_POSSIBLE"s;
			case _NvAPI_Status::NVAPI_MODE_CHANGE_FAILED: return "NVAPI_MODE_CHANGE_FAILED"s;
			case _NvAPI_Status::NVAPI_D3D11_LIBRARY_NOT_FOUND: return "NVAPI_D3D11_LIBRARY_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_INVALID_ADDRESS: return "NVAPI_INVALID_ADDRESS"s;
			case _NvAPI_Status::NVAPI_STRING_TOO_SMALL: return "NVAPI_STRING_TOO_SMALL"s;
			case _NvAPI_Status::NVAPI_MATCHING_DEVICE_NOT_FOUND: return "NVAPI_MATCHING_DEVICE_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_DRIVER_RUNNING: return "NVAPI_DRIVER_RUNNING"s;
			case _NvAPI_Status::NVAPI_DRIVER_NOTRUNNING: return "NVAPI_DRIVER_NOTRUNNING"s;
			case _NvAPI_Status::NVAPI_ERROR_DRIVER_RELOAD_REQUIRED: return "NVAPI_ERROR_DRIVER_RELOAD_REQUIRED"s;
			case _NvAPI_Status::NVAPI_SET_NOT_ALLOWED: return "NVAPI_SET_NOT_ALLOWED"s;
			case _NvAPI_Status::NVAPI_ADVANCED_DISPLAY_TOPOLOGY_REQUIRED: return "NVAPI_ADVANCED_DISPLAY_TOPOLOGY_REQUIRED"s;
			case _NvAPI_Status::NVAPI_SETTING_NOT_FOUND: return "NVAPI_SETTING_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_SETTING_SIZE_TOO_LARGE: return "NVAPI_SETTING_SIZE_TOO_LARGE"s;
			case _NvAPI_Status::NVAPI_TOO_MANY_SETTINGS_IN_PROFILE: return "NVAPI_TOO_MANY_SETTINGS_IN_PROFILE"s;
			case _NvAPI_Status::NVAPI_PROFILE_NOT_FOUND: return "NVAPI_PROFILE_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_PROFILE_NAME_IN_USE: return "NVAPI_PROFILE_NAME_IN_USE"s;
			case _NvAPI_Status::NVAPI_PROFILE_NAME_EMPTY: return "NVAPI_PROFILE_NAME_EMPTY"s;
			case _NvAPI_Status::NVAPI_EXECUTABLE_NOT_FOUND: return "NVAPI_EXECUTABLE_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_EXECUTABLE_ALREADY_IN_USE: return "NVAPI_EXECUTABLE_ALREADY_IN_USE"s;
			case _NvAPI_Status::NVAPI_DATATYPE_MISMATCH: return "NVAPI_DATATYPE_MISMATCH"s;
			case _NvAPI_Status::NVAPI_PROFILE_REMOVED: return "NVAPI_PROFILE_REMOVED"s;
			case _NvAPI_Status::NVAPI_UNREGISTERED_RESOURCE: return "NVAPI_UNREGISTERED_RESOURCE"s;
			case _NvAPI_Status::NVAPI_ID_OUT_OF_RANGE: return "NVAPI_ID_OUT_OF_RANGE"s;
			case _NvAPI_Status::NVAPI_DISPLAYCONFIG_VALIDATION_FAILED: return "NVAPI_DISPLAYCONFIG_VALIDATION_FAILED"s;
			case _NvAPI_Status::NVAPI_DPMST_CHANGED: return "NVAPI_DPMST_CHANGED"s;
			case _NvAPI_Status::NVAPI_INSUFFICIENT_BUFFER: return "NVAPI_INSUFFICIENT_BUFFER"s;
			case _NvAPI_Status::NVAPI_ACCESS_DENIED: return "NVAPI_ACCESS_DENIED"s;
			case _NvAPI_Status::NVAPI_MOSAIC_NOT_ACTIVE: return "NVAPI_MOSAIC_NOT_ACTIVE"s;
			case _NvAPI_Status::NVAPI_SHARE_RESOURCE_RELOCATED: return "NVAPI_SHARE_RESOURCE_RELOCATED"s;
			case _NvAPI_Status::NVAPI_REQUEST_USER_TO_DISABLE_DWM: return "NVAPI_REQUEST_USER_TO_DISABLE_DWM"s;
			case _NvAPI_Status::NVAPI_D3D_DEVICE_LOST: return "NVAPI_D3D_DEVICE_LOST"s;
			case _NvAPI_Status::NVAPI_INVALID_CONFIGURATION: return "NVAPI_INVALID_CONFIGURATION"s;
			case _NvAPI_Status::NVAPI_STEREO_HANDSHAKE_NOT_DONE: return "NVAPI_STEREO_HANDSHAKE_NOT_DONE"s;
			case _NvAPI_Status::NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS: return "NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS"s;
			case _NvAPI_Status::NVAPI_DEFAULT_STEREO_PROFILE_IS_NOT_DEFINED: return "NVAPI_DEFAULT_STEREO_PROFILE_IS_NOT_DEFINED"s;
			case _NvAPI_Status::NVAPI_DEFAULT_STEREO_PROFILE_DOES_NOT_EXIST: return "NVAPI_DEFAULT_STEREO_PROFILE_DOES_NOT_EXIST"s;
			case _NvAPI_Status::NVAPI_CLUSTER_ALREADY_EXISTS: return "NVAPI_CLUSTER_ALREADY_EXISTS"s;
			case _NvAPI_Status::NVAPI_DPMST_DISPLAY_ID_EXPECTED: return "NVAPI_DPMST_DISPLAY_ID_EXPECTED"s;
			case _NvAPI_Status::NVAPI_INVALID_DISPLAY_ID: return "NVAPI_INVALID_DISPLAY_ID"s;
			case _NvAPI_Status::NVAPI_STREAM_IS_OUT_OF_SYNC: return "NVAPI_STREAM_IS_OUT_OF_SYNC"s;
			case _NvAPI_Status::NVAPI_INCOMPATIBLE_AUDIO_DRIVER: return "NVAPI_INCOMPATIBLE_AUDIO_DRIVER"s;
			case _NvAPI_Status::NVAPI_VALUE_ALREADY_SET: return "NVAPI_VALUE_ALREADY_SET"s;
			case _NvAPI_Status::NVAPI_TIMEOUT: return "NVAPI_TIMEOUT"s;
			case _NvAPI_Status::NVAPI_GPU_WORKSTATION_FEATURE_INCOMPLETE: return "NVAPI_GPU_WORKSTATION_FEATURE_INCOMPLETE"s;
			case _NvAPI_Status::NVAPI_STEREO_INIT_ACTIVATION_NOT_DONE: return "NVAPI_STEREO_INIT_ACTIVATION_NOT_DONE"s;
			case _NvAPI_Status::NVAPI_SYNC_NOT_ACTIVE: return "NVAPI_SYNC_NOT_ACTIVE"s;
			case _NvAPI_Status::NVAPI_SYNC_MASTER_NOT_FOUND: return "NVAPI_SYNC_MASTER_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_INVALID_SYNC_TOPOLOGY: return "NVAPI_INVALID_SYNC_TOPOLOGY"s;
			case _NvAPI_Status::NVAPI_ECID_SIGN_ALGO_UNSUPPORTED: return "NVAPI_ECID_SIGN_ALGO_UNSUPPORTED"s;
			case _NvAPI_Status::NVAPI_ECID_KEY_VERIFICATION_FAILED: return "NVAPI_ECID_KEY_VERIFICATION_FAILED"s;
			case _NvAPI_Status::NVAPI_FIRMWARE_OUT_OF_DATE: return "NVAPI_FIRMWARE_OUT_OF_DATE"s;
			case _NvAPI_Status::NVAPI_FIRMWARE_REVISION_NOT_SUPPORTED: return "NVAPI_FIRMWARE_REVISION_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_LICENSE_CALLER_AUTHENTICATION_FAILED: return "NVAPI_LICENSE_CALLER_AUTHENTICATION_FAILED"s;
			case _NvAPI_Status::NVAPI_D3D_DEVICE_NOT_REGISTERED: return "NVAPI_D3D_DEVICE_NOT_REGISTERED"s;
			case _NvAPI_Status::NVAPI_RESOURCE_NOT_ACQUIRED: return "NVAPI_RESOURCE_NOT_ACQUIRED"s;
			case _NvAPI_Status::NVAPI_TIMING_NOT_SUPPORTED: return "NVAPI_TIMING_NOT_SUPPORTED"s;
			case _NvAPI_Status::NVAPI_HDCP_ENCRYPTION_FAILED: return "NVAPI_HDCP_ENCRYPTION_FAILED"s;
			case _NvAPI_Status::NVAPI_PCLK_LIMITATION_FAILED: return "NVAPI_PCLK_LIMITATION_FAILED"s;
			case _NvAPI_Status::NVAPI_NO_CONNECTOR_FOUND: return "NVAPI_NO_CONNECTOR_FOUND"s;
			case _NvAPI_Status::NVAPI_HDCP_DISABLED: return "NVAPI_HDCP_DISABLED"s;
			case _NvAPI_Status::NVAPI_API_IN_USE: return "NVAPI_API_IN_USE"s;
			case _NvAPI_Status::NVAPI_NVIDIA_DISPLAY_NOT_FOUND: return "NVAPI_NVIDIA_DISPLAY_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_PRIV_SEC_VIOLATION: return "NVAPI_PRIV_SEC_VIOLATION"s;
			case _NvAPI_Status::NVAPI_INCORRECT_VENDOR: return "NVAPI_INCORRECT_VENDOR"s;
			case _NvAPI_Status::NVAPI_DISPLAY_IN_USE: return "NVAPI_DISPLAY_IN_USE"s;
			case _NvAPI_Status::NVAPI_UNSUPPORTED_CONFIG_NON_HDCP_HMD: return "NVAPI_UNSUPPORTED_CONFIG_NON_HDCP_HMD"s;
			case _NvAPI_Status::NVAPI_MAX_DISPLAY_LIMIT_REACHED: return "NVAPI_MAX_DISPLAY_LIMIT_REACHED"s;
			case _NvAPI_Status::NVAPI_INVALID_DIRECT_MODE_DISPLAY: return "NVAPI_INVALID_DIRECT_MODE_DISPLAY"s;
			case _NvAPI_Status::NVAPI_GPU_IN_DEBUG_MODE: return "NVAPI_GPU_IN_DEBUG_MODE"s;
			case _NvAPI_Status::NVAPI_D3D_CONTEXT_NOT_FOUND: return "NVAPI_D3D_CONTEXT_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_STEREO_VERSION_MISMATCH: return "NVAPI_STEREO_VERSION_MISMATCH"s;
			case _NvAPI_Status::NVAPI_GPU_NOT_POWERED: return "NVAPI_GPU_NOT_POWERED"s;
			case _NvAPI_Status::NVAPI_ERROR_DRIVER_RELOAD_IN_PROGRESS: return "NVAPI_ERROR_DRIVER_RELOAD_IN_PROGRESS"s;
			case _NvAPI_Status::NVAPI_WAIT_FOR_HW_RESOURCE: return "NVAPI_WAIT_FOR_HW_RESOURCE"s;
			case _NvAPI_Status::NVAPI_REQUIRE_FURTHER_HDCP_ACTION: return "NVAPI_REQUIRE_FURTHER_HDCP_ACTION"s;
			case _NvAPI_Status::NVAPI_DISPLAY_MUX_TRANSITION_FAILED: return "NVAPI_DISPLAY_MUX_TRANSITION_FAILED"s;
			case _NvAPI_Status::NVAPI_INVALID_DSC_VERSION: return "NVAPI_INVALID_DSC_VERSION"s;
			case _NvAPI_Status::NVAPI_INVALID_DSC_SLICECOUNT: return "NVAPI_INVALID_DSC_SLICECOUNT"s;
			case _NvAPI_Status::NVAPI_INVALID_DSC_OUTPUT_BPP: return "NVAPI_INVALID_DSC_OUTPUT_BPP"s;
			case _NvAPI_Status::NVAPI_FAILED_TO_LOAD_FROM_DRIVER_STORE: return "NVAPI_FAILED_TO_LOAD_FROM_DRIVER_STORE"s;
			case _NvAPI_Status::NVAPI_NO_VULKAN: return "NVAPI_NO_VULKAN"s;
			case _NvAPI_Status::NVAPI_REQUEST_PENDING: return "NVAPI_REQUEST_PENDING"s;
			case _NvAPI_Status::NVAPI_RESOURCE_IN_USE: return "NVAPI_RESOURCE_IN_USE"s;
			case _NvAPI_Status::NVAPI_INVALID_IMAGE: return "NVAPI_INVALID_IMAGE"s;
			case _NvAPI_Status::NVAPI_INVALID_PTX: return "NVAPI_INVALID_PTX"s;
			case _NvAPI_Status::NVAPI_NVLINK_UNCORRECTABLE: return "NVAPI_NVLINK_UNCORRECTABLE"s;
			case _NvAPI_Status::NVAPI_JIT_COMPILER_NOT_FOUND: return "NVAPI_JIT_COMPILER_NOT_FOUND"s;
			case _NvAPI_Status::NVAPI_INVALID_SOURCE: return "NVAPI_INVALID_SOURCE"s;
			case _NvAPI_Status::NVAPI_ILLEGAL_INSTRUCTION: return "NVAPI_ILLEGAL_INSTRUCTION"s;
			case _NvAPI_Status::NVAPI_INVALID_PC: return "NVAPI_INVALID_PC"s;
			case _NvAPI_Status::NVAPI_LAUNCH_FAILED: return "NVAPI_LAUNCH_FAILED"s;
			case _NvAPI_Status::NVAPI_NOT_PERMITTED: return "NVAPI_NOT_PERMITTED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(nvmlReturn_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const nvmlReturn_t*>(pEnum);
			switch (e) {
			case nvmlReturn_t::NVML_SUCCESS: return "NVML_SUCCESS"s;
			case nvmlReturn_t::NVML_ERROR_UNINITIALIZED: return "NVML_ERROR_UNINITIALIZED"s;
			case nvmlReturn_t::NVML_ERROR_INVALID_ARGUMENT: return "NVML_ERROR_INVALID_ARGUMENT"s;
			case nvmlReturn_t::NVML_ERROR_NOT_SUPPORTED: return "NVML_ERROR_NOT_SUPPORTED"s;
			case nvmlReturn_t::NVML_ERROR_NO_PERMISSION: return "NVML_ERROR_NO_PERMISSION"s;
			case nvmlReturn_t::NVML_ERROR_ALREADY_INITIALIZED: return "NVML_ERROR_ALREADY_INITIALIZED"s;
			case nvmlReturn_t::NVML_ERROR_NOT_FOUND: return "NVML_ERROR_NOT_FOUND"s;
			case nvmlReturn_t::NVML_ERROR_INSUFFICIENT_SIZE: return "NVML_ERROR_INSUFFICIENT_SIZE"s;
			case nvmlReturn_t::NVML_ERROR_INSUFFICIENT_POWER: return "NVML_ERROR_INSUFFICIENT_POWER"s;
			case nvmlReturn_t::NVML_ERROR_DRIVER_NOT_LOADED: return "NVML_ERROR_DRIVER_NOT_LOADED"s;
			case nvmlReturn_t::NVML_ERROR_TIMEOUT: return "NVML_ERROR_TIMEOUT"s;
			case nvmlReturn_t::NVML_ERROR_IRQ_ISSUE: return "NVML_ERROR_IRQ_ISSUE"s;
			case nvmlReturn_t::NVML_ERROR_LIBRARY_NOT_FOUND: return "NVML_ERROR_LIBRARY_NOT_FOUND"s;
			case nvmlReturn_t::NVML_ERROR_FUNCTION_NOT_FOUND: return "NVML_ERROR_FUNCTION_NOT_FOUND"s;
			case nvmlReturn_t::NVML_ERROR_CORRUPTED_INFOROM: return "NVML_ERROR_CORRUPTED_INFOROM"s;
			case nvmlReturn_t::NVML_ERROR_GPU_IS_LOST: return "NVML_ERROR_GPU_IS_LOST"s;
			case nvmlReturn_t::NVML_ERROR_RESET_REQUIRED: return "NVML_ERROR_RESET_REQUIRED"s;
			case nvmlReturn_t::NVML_ERROR_OPERATING_SYSTEM: return "NVML_ERROR_OPERATING_SYSTEM"s;
			case nvmlReturn_t::NVML_ERROR_LIB_RM_VERSION_MISMATCH: return "NVML_ERROR_LIB_RM_VERSION_MISMATCH"s;
			case nvmlReturn_t::NVML_ERROR_IN_USE: return "NVML_ERROR_IN_USE"s;
			case nvmlReturn_t::NVML_ERROR_MEMORY: return "NVML_ERROR_MEMORY"s;
			case nvmlReturn_t::NVML_ERROR_NO_DATA: return "NVML_ERROR_NO_DATA"s;
			case nvmlReturn_t::NVML_ERROR_VGPU_ECC_NOT_SUPPORTED: return "NVML_ERROR_VGPU_ECC_NOT_SUPPORTED"s;
			case nvmlReturn_t::NVML_ERROR_INSUFFICIENT_RESOURCES: return "NVML_ERROR_INSUFFICIENT_RESOURCES"s;
			case nvmlReturn_t::NVML_ERROR_FREQ_NOT_SUPPORTED: return "NVML_ERROR_FREQ_NOT_SUPPORTED"s;
			case nvmlReturn_t::NVML_ERROR_UNKNOWN: return "NVML_ERROR_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
	}
}
