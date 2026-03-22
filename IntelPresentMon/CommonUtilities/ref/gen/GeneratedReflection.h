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
		// structs
		dumpers[typeid(_ctl_base_interface_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_base_interface_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_base_interface_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_range_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_range_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_range_info_t {"
				<< " .min_possible_value = " << s.min_possible_value
				<< " .max_possible_value = " << s.max_possible_value
				<< " .step_size = " << s.step_size
				<< " .default_value = " << s.default_value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_range_info_int_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_range_info_int_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_range_info_int_t {"
				<< " .min_possible_value = " << s.min_possible_value
				<< " .max_possible_value = " << s.max_possible_value
				<< " .step_size = " << s.step_size
				<< " .default_value = " << s.default_value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_range_info_uint_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_range_info_uint_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_range_info_uint_t {"
				<< " .min_possible_value = " << s.min_possible_value
				<< " .max_possible_value = " << s.max_possible_value
				<< " .step_size = " << s.step_size
				<< " .default_value = " << s.default_value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_info_boolean_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_info_boolean_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_info_boolean_t {"
				<< " .DefaultState = " << s.DefaultState
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_boolean_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_boolean_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_boolean_t {"
				<< " .Enable = " << s.Enable
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_info_enum_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_info_enum_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_info_enum_t {"
				<< " .SupportedTypes = " << s.SupportedTypes
				<< " .DefaultType = " << s.DefaultType
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_enum_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_enum_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_enum_t {"
				<< " .EnableType = " << s.EnableType
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_info_float_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_info_float_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_info_float_t {"
				<< " .DefaultEnable = " << s.DefaultEnable
				<< " .RangeInfo = " << DumpGenerated(s.RangeInfo)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_float_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_float_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_float_t {"
				<< " .Enable = " << s.Enable
				<< " .Value = " << s.Value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_info_int_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_info_int_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_info_int_t {"
				<< " .DefaultEnable = " << s.DefaultEnable
				<< " .RangeInfo = " << DumpGenerated(s.RangeInfo)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_int_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_int_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_int_t {"
				<< " .Enable = " << s.Enable
				<< " .Value = " << s.Value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_info_uint_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_info_uint_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_info_uint_t {"
				<< " .DefaultEnable = " << s.DefaultEnable
				<< " .RangeInfo = " << DumpGenerated(s.RangeInfo)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_uint_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_uint_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_property_uint_t {"
				<< " .Enable = " << s.Enable
				<< " .Value = " << s.Value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ctl_property_info_t {"
				<< " .BoolType = " << DumpGenerated(s.BoolType)
				<< " .FloatType = " << DumpGenerated(s.FloatType)
				<< " .IntType = " << DumpGenerated(s.IntType)
				<< " .EnumType = " << DumpGenerated(s.EnumType)
				<< " .UIntType = " << DumpGenerated(s.UIntType)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_property_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_property_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ctl_property_t {"
				<< " .BoolType = " << DumpGenerated(s.BoolType)
				<< " .FloatType = " << DumpGenerated(s.FloatType)
				<< " .IntType = " << DumpGenerated(s.IntType)
				<< " .EnumType = " << DumpGenerated(s.EnumType)
				<< " .UIntType = " << DumpGenerated(s.UIntType)
				<< " }";
			return oss.str();
		};
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
		dumpers[typeid(_ctl_base_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_base_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_base_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
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
				<< " .Data4 = " << DumpArray_(s.Data4)
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
		dumpers[typeid(_ctl_reserved_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_reserved_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_reserved_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .pSpecialArg = " << (s.pSpecialArg ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pSpecialArg)) : "null"s)
				<< " .ArgSize = " << s.ArgSize
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_reserved_args_base_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_reserved_args_base_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_reserved_args_base_t {"
				<< " .ReservedFuncID = " << DumpGenerated(s.ReservedFuncID)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_unlock_capability_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_unlock_capability_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_unlock_capability_t {"
				<< " .ReservedFuncID = " << DumpGenerated(s.ReservedFuncID)
				<< " .UnlockCapsID = " << DumpGenerated(s.UnlockCapsID)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_runtime_path_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_runtime_path_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_runtime_path_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .UnlockID = " << DumpGenerated(s.UnlockID)
				<< " .pRuntimePath = " << (s.pRuntimePath ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pRuntimePath)) : "null"s)
				<< " .DeviceID = " << s.DeviceID
				<< " .RevID = " << (int)s.RevID
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
		dumpers[typeid(_ctl_generic_void_datatype_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_generic_void_datatype_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_generic_void_datatype_t {"
				<< " .pData = " << (s.pData ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pData)) : "null"s)
				<< " .size = " << s.size
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_revision_datatype_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_revision_datatype_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_revision_datatype_t {"
				<< " .major_version = " << (int)s.major_version
				<< " .minor_version = " << (int)s.minor_version
				<< " .revision_version = " << (int)s.revision_version
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_wait_property_change_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_wait_property_change_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_wait_property_change_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .PropertyType = " << s.PropertyType
				<< " .TimeOutMilliSec = " << s.TimeOutMilliSec
				<< " .EventMiscFlags = " << s.EventMiscFlags
				<< " .pReserved = " << (s.pReserved ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pReserved)) : "null"s)
				<< " .ReservedOutFlags = " << s.ReservedOutFlags
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_rect_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_rect_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_rect_t {"
				<< " .Left = " << s.Left
				<< " .Top = " << s.Top
				<< " .Right = " << s.Right
				<< " .Bottom = " << s.Bottom
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_endurance_gaming_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_endurance_gaming_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_endurance_gaming_caps_t {"
				<< " .EGControlCaps = " << DumpGenerated(s.EGControlCaps)
				<< " .EGModeCaps = " << DumpGenerated(s.EGModeCaps)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_endurance_gaming_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_endurance_gaming_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_endurance_gaming_t {"
				<< " .EGControl = " << DumpGenerated(s.EGControl)
				<< " .EGMode = " << DumpGenerated(s.EGMode)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_endurance_gaming2_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_endurance_gaming2_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_endurance_gaming2_t {"
				<< " .EGControl = " << DumpGenerated(s.EGControl)
				<< " .EGMode = " << DumpGenerated(s.EGMode)
				<< " .IsFPRequired = " << s.IsFPRequired
				<< " .TargetFPS = " << s.TargetFPS
				<< " .RefreshRate = " << s.RefreshRate
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_adaptivesync_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_adaptivesync_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_adaptivesync_caps_t {"
				<< " .AdaptiveBalanceSupported = " << s.AdaptiveBalanceSupported
				<< " .AdaptiveBalanceStrengthCaps = " << DumpGenerated(s.AdaptiveBalanceStrengthCaps)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_adaptivesync_getset_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_adaptivesync_getset_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_adaptivesync_getset_t {"
				<< " .AdaptiveSync = " << s.AdaptiveSync
				<< " .AdaptiveBalance = " << s.AdaptiveBalance
				<< " .AllowAsyncForHighFPS = " << s.AllowAsyncForHighFPS
				<< " .AdaptiveBalanceStrength = " << s.AdaptiveBalanceStrength
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_3d_app_profiles_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_3d_app_profiles_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_3d_app_profiles_caps_t {"
				<< " .SupportedTierTypes = " << s.SupportedTierTypes
				<< " .Reserved = " << s.Reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_3d_app_profiles_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_3d_app_profiles_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_3d_app_profiles_t {"
				<< " .TierType = " << DumpGenerated(s.TierType)
				<< " .SupportedTierProfiles = " << s.SupportedTierProfiles
				<< " .DefaultEnabledTierProfiles = " << s.DefaultEnabledTierProfiles
				<< " .CustomizationSupportedTierProfiles = " << s.CustomizationSupportedTierProfiles
				<< " .EnabledTierProfiles = " << s.EnabledTierProfiles
				<< " .CustomizationEnabledTierProfiles = " << s.CustomizationEnabledTierProfiles
				<< " .Reserved = " << s.Reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_3d_tier_details_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_3d_tier_details_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_3d_tier_details_t {"
				<< " .TierType = " << DumpGenerated(s.TierType)
				<< " .TierProfile = " << DumpGenerated(s.TierProfile)
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_3d_feature_details_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_3d_feature_details_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_3d_feature_details_t {"
				<< " .FeatureType = " << DumpGenerated(s.FeatureType)
				<< " .ValueType = " << DumpGenerated(s.ValueType)
				<< " .Value = " << DumpGenerated(s.Value)
				<< " .CustomValueSize = " << s.CustomValueSize
				<< " .pCustomValue = " << (s.pCustomValue ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pCustomValue)) : "null"s)
				<< " .PerAppSupport = " << s.PerAppSupport
				<< " .ConflictingFeatures = " << s.ConflictingFeatures
				<< " .FeatureMiscSupport = " << s.FeatureMiscSupport
				<< " .Reserved = " << s.Reserved
				<< " .Reserved1 = " << s.Reserved1
				<< " .Reserved2 = " << s.Reserved2
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_3d_feature_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_3d_feature_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_3d_feature_caps_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .NumSupportedFeatures = " << s.NumSupportedFeatures
				<< " .pFeatureDetails = " << (s.pFeatureDetails ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pFeatureDetails)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_3d_feature_getset_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_3d_feature_getset_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_3d_feature_getset_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .FeatureType = " << DumpGenerated(s.FeatureType)
				<< " .ApplicationName = " << (s.ApplicationName ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.ApplicationName)) : "null"s)
				<< " .ApplicationNameLength = " << (int)s.ApplicationNameLength
				<< " .bSet = " << s.bSet
				<< " .ValueType = " << DumpGenerated(s.ValueType)
				<< " .Value = " << DumpGenerated(s.Value)
				<< " .CustomValueSize = " << s.CustomValueSize
				<< " .pCustomValue = " << (s.pCustomValue ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pCustomValue)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_kmd_load_features_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_kmd_load_features_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_kmd_load_features_t {"
				<< " .ReservedFuncID = " << DumpGenerated(s.ReservedFuncID)
				<< " .bLoad = " << s.bLoad
				<< " .SubsetFeatureMask = " << s.SubsetFeatureMask
				<< " .ApplicationName = " << (s.ApplicationName ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.ApplicationName)) : "null"s)
				<< " .ApplicationNameLength = " << (int)s.ApplicationNameLength
				<< " .CallerComponent = " << (int)s.CallerComponent
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_display_timing_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_display_timing_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_display_timing_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .PixelClock = " << s.PixelClock
				<< " .HActive = " << s.HActive
				<< " .VActive = " << s.VActive
				<< " .HTotal = " << s.HTotal
				<< " .VTotal = " << s.VTotal
				<< " .HBlank = " << s.HBlank
				<< " .VBlank = " << s.VBlank
				<< " .HSync = " << s.HSync
				<< " .VSync = " << s.VSync
				<< " .RefreshRate = " << s.RefreshRate
				<< " .SignalStandard = " << DumpGenerated(s.SignalStandard)
				<< " .VicId = " << (int)s.VicId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_display_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_display_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_display_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Os_display_encoder_handle = " << DumpGenerated(s.Os_display_encoder_handle)
				<< " .Type = " << DumpGenerated(s.Type)
				<< " .AttachedDisplayMuxType = " << DumpGenerated(s.AttachedDisplayMuxType)
				<< " .ProtocolConverterOutput = " << DumpGenerated(s.ProtocolConverterOutput)
				<< " .SupportedSpec = " << DumpGenerated(s.SupportedSpec)
				<< " .SupportedOutputBPCFlags = " << s.SupportedOutputBPCFlags
				<< " .ProtocolConverterType = " << s.ProtocolConverterType
				<< " .DisplayConfigFlags = " << s.DisplayConfigFlags
				<< " .FeatureEnabledFlags = " << s.FeatureEnabledFlags
				<< " .FeatureSupportedFlags = " << s.FeatureSupportedFlags
				<< " .AdvancedFeatureEnabledFlags = " << s.AdvancedFeatureEnabledFlags
				<< " .AdvancedFeatureSupportedFlags = " << s.AdvancedFeatureSupportedFlags
				<< " .Display_Timing_Info = " << DumpGenerated(s.Display_Timing_Info)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_os_display_encoder_identifier_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_os_display_encoder_identifier_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ctl_os_display_encoder_identifier_t {"
				<< " .WindowsDisplayEncoderID = " << s.WindowsDisplayEncoderID
				<< " .DisplayEncoderID = " << DumpGenerated(s.DisplayEncoderID)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_adapter_display_encoder_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_adapter_display_encoder_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_adapter_display_encoder_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Os_display_encoder_handle = " << DumpGenerated(s.Os_display_encoder_handle)
				<< " .Type = " << DumpGenerated(s.Type)
				<< " .IsOnBoardProtocolConverterOutputPresent = " << s.IsOnBoardProtocolConverterOutputPresent
				<< " .SupportedSpec = " << DumpGenerated(s.SupportedSpec)
				<< " .SupportedOutputBPCFlags = " << s.SupportedOutputBPCFlags
				<< " .EncoderConfigFlags = " << s.EncoderConfigFlags
				<< " .FeatureSupportedFlags = " << s.FeatureSupportedFlags
				<< " .AdvancedFeatureSupportedFlags = " << s.AdvancedFeatureSupportedFlags
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_sharpness_filter_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_sharpness_filter_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_sharpness_filter_properties_t {"
				<< " .FilterType = " << s.FilterType
				<< " .FilterDetails = " << DumpGenerated(s.FilterDetails)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_sharpness_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_sharpness_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_sharpness_caps_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .SupportedFilterFlags = " << s.SupportedFilterFlags
				<< " .NumFilterTypes = " << (int)s.NumFilterTypes
				<< " .pFilterProperty = " << (s.pFilterProperty ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pFilterProperty)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_sharpness_settings_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_sharpness_settings_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_sharpness_settings_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Enable = " << s.Enable
				<< " .FilterType = " << s.FilterType
				<< " .Intensity = " << s.Intensity
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_i2c_access_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_i2c_access_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_i2c_access_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .DataSize = " << s.DataSize
				<< " .Address = " << s.Address
				<< " .OpType = " << DumpGenerated(s.OpType)
				<< " .Offset = " << s.Offset
				<< " .Flags = " << s.Flags
				<< " .RAD = " << s.RAD
				<< " .Data = " << DumpArray_(s.Data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_i2c_access_pinpair_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_i2c_access_pinpair_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_i2c_access_pinpair_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .DataSize = " << s.DataSize
				<< " .Address = " << s.Address
				<< " .OpType = " << DumpGenerated(s.OpType)
				<< " .Offset = " << s.Offset
				<< " .Flags = " << s.Flags
				<< " .Data = " << DumpArray_(s.Data)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_aux_access_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_aux_access_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_aux_access_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .OpType = " << DumpGenerated(s.OpType)
				<< " .Flags = " << s.Flags
				<< " .Address = " << s.Address
				<< " .RAD = " << s.RAD
				<< " .PortID = " << s.PortID
				<< " .DataSize = " << s.DataSize
				<< " .Data = " << DumpArray_(s.Data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_optimization_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_optimization_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_optimization_caps_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .SupportedFeatures = " << s.SupportedFeatures
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_optimization_lrr_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_optimization_lrr_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_optimization_lrr_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .SupportedLRRTypes = " << s.SupportedLRRTypes
				<< " .CurrentLRRTypes = " << s.CurrentLRRTypes
				<< " .bRequirePSRDisable = " << s.bRequirePSRDisable
				<< " .LowRR = " << s.LowRR
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_optimization_psr_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_optimization_psr_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_optimization_psr_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .PSRVersion = " << (int)s.PSRVersion
				<< " .FullFetchUpdate = " << s.FullFetchUpdate
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_optimization_dpst_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_optimization_dpst_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_optimization_dpst_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .MinLevel = " << (int)s.MinLevel
				<< " .MaxLevel = " << (int)s.MaxLevel
				<< " .Level = " << (int)s.Level
				<< " .SupportedFeatures = " << s.SupportedFeatures
				<< " .EnabledFeatures = " << s.EnabledFeatures
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_optimization_settings_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_optimization_settings_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_optimization_settings_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .PowerOptimizationPlan = " << DumpGenerated(s.PowerOptimizationPlan)
				<< " .PowerOptimizationFeature = " << s.PowerOptimizationFeature
				<< " .Enable = " << s.Enable
				<< " .FeatureSpecificData = " << DumpGenerated(s.FeatureSpecificData)
				<< " .PowerSource = " << DumpGenerated(s.PowerSource)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_optimization_feature_specific_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_optimization_feature_specific_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ctl_power_optimization_feature_specific_info_t {"
				<< " .LRRInfo = " << DumpGenerated(s.LRRInfo)
				<< " .PSRInfo = " << DumpGenerated(s.PSRInfo)
				<< " .DPSTInfo = " << DumpGenerated(s.DPSTInfo)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_set_brightness_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_set_brightness_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_set_brightness_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .TargetBrightness = " << s.TargetBrightness
				<< " .SmoothTransitionTimeInMs = " << s.SmoothTransitionTimeInMs
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_get_brightness_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_get_brightness_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_get_brightness_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .TargetBrightness = " << s.TargetBrightness
				<< " .CurrentBrightness = " << s.CurrentBrightness
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_color_primaries_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_color_primaries_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_color_primaries_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .xR = " << s.xR
				<< " .yR = " << s.yR
				<< " .xG = " << s.xG
				<< " .yG = " << s.yG
				<< " .xB = " << s.xB
				<< " .yB = " << s.yB
				<< " .xW = " << s.xW
				<< " .yW = " << s.yW
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_pixel_format_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_pixel_format_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_pixel_format_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .BitsPerColor = " << s.BitsPerColor
				<< " .IsFloat = " << s.IsFloat
				<< " .EncodingType = " << DumpGenerated(s.EncodingType)
				<< " .ColorSpace = " << DumpGenerated(s.ColorSpace)
				<< " .ColorModel = " << DumpGenerated(s.ColorModel)
				<< " .ColorPrimaries = " << DumpGenerated(s.ColorPrimaries)
				<< " .MaxBrightness = " << s.MaxBrightness
				<< " .MinBrightness = " << s.MinBrightness
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_1dlut_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_1dlut_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_1dlut_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .SamplingType = " << DumpGenerated(s.SamplingType)
				<< " .NumSamplesPerChannel = " << s.NumSamplesPerChannel
				<< " .NumChannels = " << s.NumChannels
				<< " .pSampleValues = " << (s.pSampleValues ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pSampleValues)) : "null"s)
				<< " .pSamplePositions = " << (s.pSamplePositions ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pSamplePositions)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_matrix_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_matrix_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_matrix_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .PreOffsets = " << DumpArray_(s.PreOffsets)
				<< " .PostOffsets = " << DumpArray_(s.PostOffsets)
				<< " .Matrix = " << DumpArray_(s.Matrix)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_3dlut_sample_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_3dlut_sample_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_3dlut_sample_t {"
				<< " .Red = " << s.Red
				<< " .Green = " << s.Green
				<< " .Blue = " << s.Blue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_3dlut_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_3dlut_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_3dlut_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .NumSamplesPerChannel = " << s.NumSamplesPerChannel
				<< " .pSampleValues = " << (s.pSampleValues ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pSampleValues)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_block_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_block_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_block_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .BlockId = " << s.BlockId
				<< " .BlockType = " << DumpGenerated(s.BlockType)
				<< " .Config = " << DumpGenerated(s.Config)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ctl_pixtx_config_t {"
				<< " .OneDLutConfig = " << DumpGenerated(s.OneDLutConfig)
				<< " .ThreeDLutConfig = " << DumpGenerated(s.ThreeDLutConfig)
				<< " .MatrixConfig = " << DumpGenerated(s.MatrixConfig)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_pipe_get_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_pipe_get_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_pipe_get_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .QueryType = " << DumpGenerated(s.QueryType)
				<< " .InputPixelFormat = " << DumpGenerated(s.InputPixelFormat)
				<< " .OutputPixelFormat = " << DumpGenerated(s.OutputPixelFormat)
				<< " .NumBlocks = " << s.NumBlocks
				<< " .pBlockConfigs = " << (s.pBlockConfigs ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pBlockConfigs)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pixtx_pipe_set_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pixtx_pipe_set_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pixtx_pipe_set_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .OpertaionType = " << DumpGenerated(s.OpertaionType)
				<< " .Flags = " << s.Flags
				<< " .NumBlocks = " << s.NumBlocks
				<< " .pBlockConfigs = " << (s.pBlockConfigs ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pBlockConfigs)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_panel_descriptor_access_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_panel_descriptor_access_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_panel_descriptor_access_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .OpType = " << DumpGenerated(s.OpType)
				<< " .BlockNumber = " << s.BlockNumber
				<< " .DescriptorDataSize = " << s.DescriptorDataSize
				<< " .pDescriptorData = " << (s.pDescriptorData ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pDescriptorData)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_retro_scaling_settings_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_retro_scaling_settings_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_retro_scaling_settings_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Get = " << s.Get
				<< " .Enable = " << s.Enable
				<< " .RetroScalingType = " << s.RetroScalingType
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_retro_scaling_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_retro_scaling_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_retro_scaling_caps_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .SupportedRetroScaling = " << s.SupportedRetroScaling
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_scaling_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_scaling_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_scaling_caps_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .SupportedScaling = " << s.SupportedScaling
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_scaling_settings_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_scaling_settings_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_scaling_settings_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Enable = " << s.Enable
				<< " .ScalingType = " << s.ScalingType
				<< " .CustomScalingX = " << s.CustomScalingX
				<< " .CustomScalingY = " << s.CustomScalingY
				<< " .HardwareModeSet = " << s.HardwareModeSet
				<< " .PreferredScalingType = " << s.PreferredScalingType
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_lace_lux_aggr_map_entry_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_lace_lux_aggr_map_entry_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_lace_lux_aggr_map_entry_t {"
				<< " .Lux = " << s.Lux
				<< " .AggressivenessPercent = " << (int)s.AggressivenessPercent
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_lace_lux_aggr_map_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_lace_lux_aggr_map_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_lace_lux_aggr_map_t {"
				<< " .MaxNumEntries = " << s.MaxNumEntries
				<< " .NumEntries = " << s.NumEntries
				<< " .pLuxToAggrMappingTable = " << (s.pLuxToAggrMappingTable ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pLuxToAggrMappingTable)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_lace_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_lace_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_lace_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Enabled = " << s.Enabled
				<< " .OpTypeGet = " << s.OpTypeGet
				<< " .OpTypeSet = " << DumpGenerated(s.OpTypeSet)
				<< " .Trigger = " << s.Trigger
				<< " .LaceConfig = " << DumpGenerated(s.LaceConfig)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_lace_aggr_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_lace_aggr_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ctl_lace_aggr_config_t {"
				<< " .FixedAggressivenessLevelPercent = " << (int)s.FixedAggressivenessLevelPercent
				<< " .AggrLevelMap = " << DumpGenerated(s.AggrLevelMap)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_sw_psr_settings_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_sw_psr_settings_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_sw_psr_settings_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Set = " << s.Set
				<< " .Supported = " << s.Supported
				<< " .Enable = " << s.Enable
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_intel_arc_sync_monitor_params_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_intel_arc_sync_monitor_params_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_intel_arc_sync_monitor_params_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .IsIntelArcSyncSupported = " << s.IsIntelArcSyncSupported
				<< " .MinimumRefreshRateInHz = " << s.MinimumRefreshRateInHz
				<< " .MaximumRefreshRateInHz = " << s.MaximumRefreshRateInHz
				<< " .MaxFrameTimeIncreaseInUs = " << s.MaxFrameTimeIncreaseInUs
				<< " .MaxFrameTimeDecreaseInUs = " << s.MaxFrameTimeDecreaseInUs
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_intel_arc_sync_profile_params_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_intel_arc_sync_profile_params_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_intel_arc_sync_profile_params_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .IntelArcSyncProfile = " << DumpGenerated(s.IntelArcSyncProfile)
				<< " .MaxRefreshRateInHz = " << s.MaxRefreshRateInHz
				<< " .MinRefreshRateInHz = " << s.MinRefreshRateInHz
				<< " .MaxFrameTimeIncreaseInUs = " << s.MaxFrameTimeIncreaseInUs
				<< " .MaxFrameTimeDecreaseInUs = " << s.MaxFrameTimeDecreaseInUs
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_edid_management_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_edid_management_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_edid_management_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .OpType = " << DumpGenerated(s.OpType)
				<< " .EdidType = " << DumpGenerated(s.EdidType)
				<< " .EdidSize = " << s.EdidSize
				<< " .pEdidBuf = " << (s.pEdidBuf ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pEdidBuf)) : "null"s)
				<< " .OutFlags = " << s.OutFlags
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_get_set_custom_mode_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_get_set_custom_mode_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_get_set_custom_mode_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .CustomModeOpType = " << DumpGenerated(s.CustomModeOpType)
				<< " .NumOfModes = " << s.NumOfModes
				<< " .pCustomSrcModeList = " << (s.pCustomSrcModeList ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pCustomSrcModeList)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_custom_src_mode_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_custom_src_mode_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_custom_src_mode_t {"
				<< " .SourceX = " << s.SourceX
				<< " .SourceY = " << s.SourceY
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_child_display_target_mode_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_child_display_target_mode_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_child_display_target_mode_t {"
				<< " .Width = " << s.Width
				<< " .Height = " << s.Height
				<< " .RefreshRate = " << s.RefreshRate
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_combined_display_child_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_combined_display_child_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_combined_display_child_info_t {"
				<< " .hDisplayOutput = " << (s.hDisplayOutput ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hDisplayOutput)) : "null"s)
				<< " .FbSrc = " << DumpGenerated(s.FbSrc)
				<< " .FbPos = " << DumpGenerated(s.FbPos)
				<< " .DisplayOrientation = " << DumpGenerated(s.DisplayOrientation)
				<< " .TargetMode = " << DumpGenerated(s.TargetMode)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_combined_display_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_combined_display_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_combined_display_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .OpType = " << DumpGenerated(s.OpType)
				<< " .IsSupported = " << s.IsSupported
				<< " .NumOutputs = " << (int)s.NumOutputs
				<< " .CombinedDesktopWidth = " << s.CombinedDesktopWidth
				<< " .CombinedDesktopHeight = " << s.CombinedDesktopHeight
				<< " .pChildInfo = " << (s.pChildInfo ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pChildInfo)) : "null"s)
				<< " .hCombinedDisplayOutput = " << (s.hCombinedDisplayOutput ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hCombinedDisplayOutput)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_genlock_display_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_genlock_display_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_genlock_display_info_t {"
				<< " .hDisplayOutput = " << (s.hDisplayOutput ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hDisplayOutput)) : "null"s)
				<< " .IsPrimary = " << s.IsPrimary
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_genlock_target_mode_list_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_genlock_target_mode_list_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_genlock_target_mode_list_t {"
				<< " .hDisplayOutput = " << (s.hDisplayOutput ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hDisplayOutput)) : "null"s)
				<< " .NumModes = " << s.NumModes
				<< " .pTargetModes = " << (s.pTargetModes ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pTargetModes)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_genlock_topology_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_genlock_topology_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_genlock_topology_t {"
				<< " .NumGenlockDisplays = " << (int)s.NumGenlockDisplays
				<< " .IsPrimaryGenlockSystem = " << s.IsPrimaryGenlockSystem
				<< " .CommonTargetMode = " << DumpGenerated(s.CommonTargetMode)
				<< " .pGenlockDisplayInfo = " << (s.pGenlockDisplayInfo ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pGenlockDisplayInfo)) : "null"s)
				<< " .pGenlockModeList = " << (s.pGenlockModeList ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pGenlockModeList)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_genlock_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_genlock_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_genlock_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Operation = " << DumpGenerated(s.Operation)
				<< " .GenlockTopology = " << DumpGenerated(s.GenlockTopology)
				<< " .IsGenlockEnabled = " << s.IsGenlockEnabled
				<< " .IsGenlockPossible = " << s.IsGenlockPossible
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_vblank_ts_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_vblank_ts_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_vblank_ts_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .NumOfTargets = " << (int)s.NumOfTargets
				<< " .VblankTS = " << DumpArray_(s.VblankTS)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_lda_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_lda_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_lda_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .NumAdapters = " << (int)s.NumAdapters
				<< " .hLinkedAdapters = " << (s.hLinkedAdapters ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hLinkedAdapters)) : "null"s)
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_dce_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_dce_args_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_dce_args_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Set = " << s.Set
				<< " .TargetBrightnessPercent = " << s.TargetBrightnessPercent
				<< " .PhaseinSpeedMultiplier = " << s.PhaseinSpeedMultiplier
				<< " .NumBins = " << s.NumBins
				<< " .Enable = " << s.Enable
				<< " .IsSupported = " << s.IsSupported
				<< " .pHistogram = " << (s.pHistogram ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pHistogram)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_wire_format_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_wire_format_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_wire_format_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .ColorModel = " << DumpGenerated(s.ColorModel)
				<< " .ColorDepth = " << s.ColorDepth
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_get_set_wire_format_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_get_set_wire_format_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_get_set_wire_format_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Operation = " << DumpGenerated(s.Operation)
				<< " .SupportedWireFormat = " << DumpArray_(s.SupportedWireFormat)
				<< " .WireFormat = " << DumpGenerated(s.WireFormat)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_display_settings_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_display_settings_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_display_settings_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Set = " << s.Set
				<< " .SupportedFlags = " << s.SupportedFlags
				<< " .ControllableFlags = " << s.ControllableFlags
				<< " .ValidFlags = " << s.ValidFlags
				<< " .LowLatency = " << DumpGenerated(s.LowLatency)
				<< " .SourceTM = " << DumpGenerated(s.SourceTM)
				<< " .ContentType = " << DumpGenerated(s.ContentType)
				<< " .QuantizationRange = " << DumpGenerated(s.QuantizationRange)
				<< " .SupportedPictureAR = " << s.SupportedPictureAR
				<< " .PictureAR = " << DumpGenerated(s.PictureAR)
				<< " .AudioSettings = " << DumpGenerated(s.AudioSettings)
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_ecc_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_ecc_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_ecc_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .isSupported = " << s.isSupported
				<< " .canControl = " << s.canControl
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_ecc_state_desc_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_ecc_state_desc_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_ecc_state_desc_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .currentEccState = " << DumpGenerated(s.currentEccState)
				<< " .pendingEccState = " << DumpGenerated(s.pendingEccState)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_engine_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_engine_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_engine_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .type = " << DumpGenerated(s.type)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_engine_stats_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_engine_stats_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_engine_stats_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .activeTime = " << s.activeTime
				<< " .timestamp = " << s.timestamp
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_fan_speed_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_fan_speed_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_fan_speed_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .speed = " << s.speed
				<< " .units = " << DumpGenerated(s.units)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_fan_temp_speed_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_fan_temp_speed_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_fan_temp_speed_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .temperature = " << s.temperature
				<< " .speed = " << DumpGenerated(s.speed)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_fan_speed_table_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_fan_speed_table_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_fan_speed_table_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .numPoints = " << s.numPoints
				<< " .table = " << DumpArray_(s.table)
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
		dumpers[typeid(_ctl_fan_config_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_fan_config_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_fan_config_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .mode = " << DumpGenerated(s.mode)
				<< " .speedFixed = " << DumpGenerated(s.speedFixed)
				<< " .speedTable = " << DumpGenerated(s.speedTable)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_firmware_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_firmware_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_firmware_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .name = " << s.name
				<< " .version = " << s.version
				<< " .FirmwareConfig = " << s.FirmwareConfig
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_firmware_component_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_firmware_component_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_firmware_component_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .name = " << s.name
				<< " .version = " << s.version
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_freq_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_freq_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_freq_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .type = " << DumpGenerated(s.type)
				<< " .canControl = " << s.canControl
				<< " .min = " << s.min
				<< " .max = " << s.max
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_freq_range_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_freq_range_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_freq_range_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .min = " << s.min
				<< " .max = " << s.max
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_freq_state_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_freq_state_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_freq_state_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .currentVoltage = " << s.currentVoltage
				<< " .request = " << s.request
				<< " .tdp = " << s.tdp
				<< " .efficient = " << s.efficient
				<< " .actual = " << s.actual
				<< " .throttleReasons = " << s.throttleReasons
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_freq_throttle_time_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_freq_throttle_time_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_freq_throttle_time_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .throttleTime = " << s.throttleTime
				<< " .timestamp = " << s.timestamp
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_led_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_led_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_led_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .canControl = " << s.canControl
				<< " .isI2C = " << s.isI2C
				<< " .isPWM = " << s.isPWM
				<< " .haveRGB = " << s.haveRGB
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_led_color_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_led_color_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_led_color_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .red = " << s.red
				<< " .green = " << s.green
				<< " .blue = " << s.blue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_led_state_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_led_state_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_led_state_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .isOn = " << s.isOn
				<< " .pwm = " << s.pwm
				<< " .color = " << DumpGenerated(s.color)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_super_resolution_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_super_resolution_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_super_resolution_info_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .super_resolution_flag = " << s.super_resolution_flag
				<< " .super_resolution_range_in_width = " << DumpGenerated(s.super_resolution_range_in_width)
				<< " .super_resolution_range_in_height = " << DumpGenerated(s.super_resolution_range_in_height)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_super_resolution_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_super_resolution_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_super_resolution_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .super_resolution_flag = " << s.super_resolution_flag
				<< " .super_resolution_max_in_enabled = " << s.super_resolution_max_in_enabled
				<< " .super_resolution_max_in_width = " << s.super_resolution_max_in_width
				<< " .super_resolution_max_in_height = " << s.super_resolution_max_in_height
				<< " .super_resolution_reboot_reset = " << s.super_resolution_reboot_reset
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " .ReservedBytes = " << s.ReservedBytes
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_noise_reduction_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_noise_reduction_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_noise_reduction_info_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .noise_reduction = " << DumpGenerated(s.noise_reduction)
				<< " .noise_reduction_auto_detect_supported = " << s.noise_reduction_auto_detect_supported
				<< " .noise_reduction_auto_detect = " << DumpGenerated(s.noise_reduction_auto_detect)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_noise_reduction_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_noise_reduction_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_noise_reduction_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .noise_reduction = " << DumpGenerated(s.noise_reduction)
				<< " .noise_reduction_auto_detect = " << DumpGenerated(s.noise_reduction_auto_detect)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_adaptive_contrast_enhancement_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_adaptive_contrast_enhancement_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_adaptive_contrast_enhancement_info_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .adaptive_contrast_enhancement = " << DumpGenerated(s.adaptive_contrast_enhancement)
				<< " .adaptive_contrast_enhancement_coexistence_supported = " << s.adaptive_contrast_enhancement_coexistence_supported
				<< " .adaptive_contrast_enhancement_coexistence = " << DumpGenerated(s.adaptive_contrast_enhancement_coexistence)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_adaptive_contrast_enhancement_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_adaptive_contrast_enhancement_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_adaptive_contrast_enhancement_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .adaptive_contrast_enhancement = " << DumpGenerated(s.adaptive_contrast_enhancement)
				<< " .adaptive_contrast_enhancement_coexistence = " << DumpGenerated(s.adaptive_contrast_enhancement_coexistence)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_standard_color_correction_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_standard_color_correction_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_standard_color_correction_info_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .standard_color_correction_default_enable = " << s.standard_color_correction_default_enable
				<< " .brightness = " << DumpGenerated(s.brightness)
				<< " .contrast = " << DumpGenerated(s.contrast)
				<< " .hue = " << DumpGenerated(s.hue)
				<< " .saturation = " << DumpGenerated(s.saturation)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_standard_color_correction_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_standard_color_correction_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_standard_color_correction_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .standard_color_correction_enable = " << s.standard_color_correction_enable
				<< " .brightness = " << s.brightness
				<< " .contrast = " << s.contrast
				<< " .hue = " << s.hue
				<< " .saturation = " << s.saturation
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_total_color_correction_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_total_color_correction_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_total_color_correction_info_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .total_color_correction_default_enable = " << s.total_color_correction_default_enable
				<< " .red = " << DumpGenerated(s.red)
				<< " .green = " << DumpGenerated(s.green)
				<< " .blue = " << DumpGenerated(s.blue)
				<< " .yellow = " << DumpGenerated(s.yellow)
				<< " .cyan = " << DumpGenerated(s.cyan)
				<< " .magenta = " << DumpGenerated(s.magenta)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_total_color_correction_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_total_color_correction_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_total_color_correction_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .total_color_correction_enable = " << s.total_color_correction_enable
				<< " .red = " << s.red
				<< " .green = " << s.green
				<< " .blue = " << s.blue
				<< " .yellow = " << s.yellow
				<< " .cyan = " << s.cyan
				<< " .magenta = " << s.magenta
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_feature_details_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_feature_details_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_feature_details_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .FeatureType = " << DumpGenerated(s.FeatureType)
				<< " .ValueType = " << DumpGenerated(s.ValueType)
				<< " .Value = " << DumpGenerated(s.Value)
				<< " .CustomValueSize = " << s.CustomValueSize
				<< " .pCustomValue = " << (s.pCustomValue ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pCustomValue)) : "null"s)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_feature_caps_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_feature_caps_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_feature_caps_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .NumSupportedFeatures = " << s.NumSupportedFeatures
				<< " .pFeatureDetails = " << (s.pFeatureDetails ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pFeatureDetails)) : "null"s)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_video_processing_feature_getset_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_video_processing_feature_getset_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_video_processing_feature_getset_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .FeatureType = " << DumpGenerated(s.FeatureType)
				<< " .ApplicationName = " << (s.ApplicationName ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.ApplicationName)) : "null"s)
				<< " .ApplicationNameLength = " << (int)s.ApplicationNameLength
				<< " .bSet = " << s.bSet
				<< " .ValueType = " << DumpGenerated(s.ValueType)
				<< " .Value = " << DumpGenerated(s.Value)
				<< " .CustomValueSize = " << s.CustomValueSize
				<< " .pCustomValue = " << (s.pCustomValue ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pCustomValue)) : "null"s)
				<< " .ReservedFields = " << DumpArray_(s.ReservedFields)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_mem_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_mem_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_mem_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .type = " << DumpGenerated(s.type)
				<< " .location = " << DumpGenerated(s.location)
				<< " .physicalSize = " << s.physicalSize
				<< " .busWidth = " << s.busWidth
				<< " .numChannels = " << s.numChannels
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
		dumpers[typeid(_ctl_oc_control_info_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_oc_control_info_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_oc_control_info_t {"
				<< " .bSupported = " << s.bSupported
				<< " .bRelative = " << s.bRelative
				<< " .bReference = " << s.bReference
				<< " .units = " << DumpGenerated(s.units)
				<< " .min = " << s.min
				<< " .max = " << s.max
				<< " .step = " << s.step
				<< " .Default = " << s.Default
				<< " .reference = " << s.reference
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_oc_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_oc_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_oc_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .bSupported = " << s.bSupported
				<< " .gpuFrequencyOffset = " << DumpGenerated(s.gpuFrequencyOffset)
				<< " .gpuVoltageOffset = " << DumpGenerated(s.gpuVoltageOffset)
				<< " .vramFrequencyOffset = " << DumpGenerated(s.vramFrequencyOffset)
				<< " .vramVoltageOffset = " << DumpGenerated(s.vramVoltageOffset)
				<< " .powerLimit = " << DumpGenerated(s.powerLimit)
				<< " .temperatureLimit = " << DumpGenerated(s.temperatureLimit)
				<< " .vramMemSpeedLimit = " << DumpGenerated(s.vramMemSpeedLimit)
				<< " .gpuVFCurveVoltageLimit = " << DumpGenerated(s.gpuVFCurveVoltageLimit)
				<< " .gpuVFCurveFrequencyLimit = " << DumpGenerated(s.gpuVFCurveFrequencyLimit)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_oc_vf_pair_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_oc_vf_pair_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_oc_vf_pair_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .Voltage = " << s.Voltage
				<< " .Frequency = " << s.Frequency
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
				<< " .psu = " << DumpArray_(s.psu)
				<< " .fanSpeed = " << DumpArray_(s.fanSpeed)
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
		dumpers[typeid(_ctl_voltage_frequency_point_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_voltage_frequency_point_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_voltage_frequency_point_t {"
				<< " .Voltage = " << s.Voltage
				<< " .Frequency = " << s.Frequency
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pci_address_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pci_address_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pci_address_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .domain = " << s.domain
				<< " .bus = " << s.bus
				<< " .device = " << s.device
				<< " .function = " << s.function
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pci_speed_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pci_speed_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pci_speed_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .gen = " << s.gen
				<< " .width = " << s.width
				<< " .maxBandwidth = " << s.maxBandwidth
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pci_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pci_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pci_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .address = " << DumpGenerated(s.address)
				<< " .maxSpeed = " << DumpGenerated(s.maxSpeed)
				<< " .resizable_bar_supported = " << s.resizable_bar_supported
				<< " .resizable_bar_enabled = " << s.resizable_bar_enabled
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_pci_state_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_pci_state_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_pci_state_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .speed = " << DumpGenerated(s.speed)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .canControl = " << s.canControl
				<< " .defaultLimit = " << s.defaultLimit
				<< " .minLimit = " << s.minLimit
				<< " .maxLimit = " << s.maxLimit
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_power_energy_counter_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_energy_counter_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_energy_counter_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .energy = " << s.energy
				<< " .timestamp = " << s.timestamp
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
		dumpers[typeid(_ctl_energy_threshold_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_energy_threshold_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_energy_threshold_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .enable = " << s.enable
				<< " .threshold = " << s.threshold
				<< " .processId = " << s.processId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_temp_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_temp_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_temp_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .type = " << DumpGenerated(s.type)
				<< " .maxTemperature = " << s.maxTemperature
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_RECT)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_RECT*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_RECT {"
				<< " .left = " << s.left
				<< " .top = " << s.top
				<< " .right = " << s.right
				<< " .bottom = " << s.bottom
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvLogicalGpuHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvLogicalGpuHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvLogicalGpuHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvPhysicalGpuHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvPhysicalGpuHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvPhysicalGpuHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvDisplayHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvDisplayHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvDisplayHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvMonitorHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvMonitorHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvMonitorHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvUnAttachedDisplayHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvUnAttachedDisplayHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvUnAttachedDisplayHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvVisualComputingDeviceHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvVisualComputingDeviceHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvVisualComputingDeviceHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvEventHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvEventHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvEventHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvHICHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvHICHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvHICHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvGSyncDeviceHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvGSyncDeviceHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvGSyncDeviceHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvVioHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvVioHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvVioHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvTransitionHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvTransitionHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvTransitionHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvAudioHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvAudioHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvAudioHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(Nv3DVPContextHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const Nv3DVPContextHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct Nv3DVPContextHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(Nv3DVPTransceiverHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const Nv3DVPTransceiverHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct Nv3DVPTransceiverHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(Nv3DVPGlassesHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const Nv3DVPGlassesHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct Nv3DVPGlassesHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvSourceHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvSourceHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvSourceHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvTargetHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvTargetHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvTargetHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NVDX_SwapChainHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NVDX_SwapChainHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NVDX_SwapChainHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvPresentBarrierClientHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvPresentBarrierClientHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvPresentBarrierClientHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvSBox)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvSBox*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvSBox {"
				<< " .sX = " << s.sX
				<< " .sY = " << s.sY
				<< " .sWidth = " << s.sWidth
				<< " .sHeight = " << s.sHeight
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvGUID)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvGUID*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvGUID {"
				<< " .data1 = " << s.data1
				<< " .data2 = " << s.data2
				<< " .data3 = " << s.data3
				<< " .data4 = " << DumpArray_(s.data4)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_EDID_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_EDID_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_EDID_V1 {"
				<< " .version = " << s.version
				<< " .EDID_Data = " << DumpArray_(s.EDID_Data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_EDID_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_EDID_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_EDID_V2 {"
				<< " .version = " << s.version
				<< " .EDID_Data = " << DumpArray_(s.EDID_Data)
				<< " .sizeofEDID = " << s.sizeofEDID
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_EDID_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_EDID_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_EDID_V3 {"
				<< " .version = " << s.version
				<< " .EDID_Data = " << DumpArray_(s.EDID_Data)
				<< " .sizeofEDID = " << s.sizeofEDID
				<< " .edidId = " << s.edidId
				<< " .offset = " << s.offset
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_VIEWPORTF)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_VIEWPORTF*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_VIEWPORTF {"
				<< " .x = " << s.x
				<< " .y = " << s.y
				<< " .w = " << s.w
				<< " .h = " << s.h
				<< " }";
			return oss.str();
		};
		dumpers[typeid(tagNV_TIMINGEXT)] = [](const void* pStruct) {
			const auto& s = *static_cast<const tagNV_TIMINGEXT*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct tagNV_TIMINGEXT {"
				<< " .flag = " << s.flag
				<< " .rr = " << s.rr
				<< " .rrx1k = " << s.rrx1k
				<< " .aspect = " << s.aspect
				<< " .rep = " << s.rep
				<< " .status = " << s.status
				<< " .name = " << DumpArray_(s.name)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_TIMING)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_TIMING*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_TIMING {"
				<< " .HVisible = " << s.HVisible
				<< " .HBorder = " << s.HBorder
				<< " .HFrontPorch = " << s.HFrontPorch
				<< " .HSyncWidth = " << s.HSyncWidth
				<< " .HTotal = " << s.HTotal
				<< " .HSyncPol = " << (int)s.HSyncPol
				<< " .VVisible = " << s.VVisible
				<< " .VBorder = " << s.VBorder
				<< " .VFrontPorch = " << s.VFrontPorch
				<< " .VSyncWidth = " << s.VSyncWidth
				<< " .VTotal = " << s.VTotal
				<< " .VSyncPol = " << (int)s.VSyncPol
				<< " .interlaced = " << s.interlaced
				<< " .pclk = " << s.pclk
				<< " .etc = " << DumpGenerated(s.etc)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_VIEW_TARGET_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_VIEW_TARGET_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_VIEW_TARGET_INFO {"
				<< " .version = " << s.version
				<< " .count = " << s.count
				<< " .target = " << DumpArray_(s.target)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_PATH)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_PATH*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_PATH {"
				<< " .deviceMask = " << s.deviceMask
				<< " .sourceId = " << s.sourceId
				<< " .bPrimary = " << s.bPrimary
				<< " .connector = " << DumpGenerated(s.connector)
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .depth = " << s.depth
				<< " .colorFormat = " << DumpGenerated(s.colorFormat)
				<< " .rotation = " << DumpGenerated(s.rotation)
				<< " .scaling = " << DumpGenerated(s.scaling)
				<< " .refreshRate = " << s.refreshRate
				<< " .interlaced = " << s.interlaced
				<< " .tvFormat = " << DumpGenerated(s.tvFormat)
				<< " .posx = " << s.posx
				<< " .posy = " << s.posy
				<< " .bGDIPrimary = " << s.bGDIPrimary
				<< " .bForceModeSet = " << s.bForceModeSet
				<< " .bFocusDisplay = " << s.bFocusDisplay
				<< " .gpuId = " << s.gpuId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_PATH_INFO_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_PATH_INFO_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_PATH_INFO_V3 {"
				<< " .version = " << s.version
				<< " .count = " << s.count
				<< " .path = " << DumpArray_(s.path)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_PATH_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_PATH_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_PATH_INFO {"
				<< " .version = " << s.version
				<< " .count = " << s.count
				<< " .path = " << DumpArray_(s.path)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_POSITION)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_POSITION*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_POSITION {"
				<< " .x = " << s.x
				<< " .y = " << s.y
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_RESOLUTION)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_RESOLUTION*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_RESOLUTION {"
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .colorDepth = " << s.colorDepth
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_V1 {"
				<< " .version = " << s.version
				<< " .rotation = " << DumpGenerated(s.rotation)
				<< " .scaling = " << DumpGenerated(s.scaling)
				<< " .refreshRate1K = " << s.refreshRate1K
				<< " .interlaced = " << s.interlaced
				<< " .primary = " << s.primary
				<< " .reservedBit1 = " << s.reservedBit1
				<< " .disableVirtualModeSupport = " << s.disableVirtualModeSupport
				<< " .isPreferredUnscaledTarget = " << s.isPreferredUnscaledTarget
				<< " .reserved = " << s.reserved
				<< " .connector = " << DumpGenerated(s.connector)
				<< " .tvFormat = " << DumpGenerated(s.tvFormat)
				<< " .timingOverride = " << DumpGenerated(s.timingOverride)
				<< " .timing = " << DumpGenerated(s.timing)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_PATH_TARGET_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAYCONFIG_PATH_TARGET_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAYCONFIG_PATH_TARGET_INFO_V1 {"
				<< " .displayId = " << s.displayId
				<< " .details = " << (s.details ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.details)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_PATH_TARGET_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAYCONFIG_PATH_TARGET_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAYCONFIG_PATH_TARGET_INFO_V2 {"
				<< " .displayId = " << s.displayId
				<< " .details = " << (s.details ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.details)) : "null"s)
				<< " .targetId = " << s.targetId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V1 {"
				<< " .resolution = " << DumpGenerated(s.resolution)
				<< " .colorFormat = " << DumpGenerated(s.colorFormat)
				<< " .position = " << DumpGenerated(s.position)
				<< " .spanningOrientation = " << DumpGenerated(s.spanningOrientation)
				<< " .bGDIPrimary = " << s.bGDIPrimary
				<< " .bSLIFocus = " << s.bSLIFocus
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_PATH_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAYCONFIG_PATH_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAYCONFIG_PATH_INFO_V1 {"
				<< " .version = " << s.version
				<< " .reserved_sourceId = " << s.reserved_sourceId
				<< " .targetInfoCount = " << s.targetInfoCount
				<< " .targetInfo = " << (s.targetInfo ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.targetInfo)) : "null"s)
				<< " .sourceModeInfo = " << (s.sourceModeInfo ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.sourceModeInfo)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_PATH_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAYCONFIG_PATH_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAYCONFIG_PATH_INFO {"
				<< " .version = " << s.version
				<< " .targetInfoCount = " << s.targetInfoCount
				<< " .targetInfo = " << (s.targetInfo ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.targetInfo)) : "null"s)
				<< " .sourceModeInfo = " << (s.sourceModeInfo ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.sourceModeInfo)) : "null"s)
				<< " .IsNonNVIDIAAdapter = " << s.IsNonNVIDIAAdapter
				<< " .reserved = " << s.reserved
				<< " .pOSAdapterID = " << (s.pOSAdapterID ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pOSAdapterID)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_PERF_PSTATES20_PARAM_DELTA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_PERF_PSTATES20_PARAM_DELTA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_PERF_PSTATES20_PARAM_DELTA {"
				<< " .value = " << s.value
				<< " .valueRange = " << DumpGenerated(s.valueRange)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_PSTATE20_CLOCK_ENTRY_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_PSTATE20_CLOCK_ENTRY_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_PSTATE20_CLOCK_ENTRY_V1 {"
				<< " .domainId = " << DumpGenerated(s.domainId)
				<< " .typeId = " << DumpGenerated(s.typeId)
				<< " .bIsEditable = " << s.bIsEditable
				<< " .reserved = " << s.reserved
				<< " .freqDelta_kHz = " << DumpGenerated(s.freqDelta_kHz)
				<< " .data = " << DumpGenerated(s.data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1 {"
				<< " .domainId = " << DumpGenerated(s.domainId)
				<< " .bIsEditable = " << s.bIsEditable
				<< " .reserved = " << s.reserved
				<< " .volt_uV = " << s.volt_uV
				<< " .voltDelta_uV = " << DumpGenerated(s.voltDelta_uV)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_PERF_PSTATES20_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_PERF_PSTATES20_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_PERF_PSTATES20_INFO_V1 {"
				<< " .version = " << s.version
				<< " .bIsEditable = " << s.bIsEditable
				<< " .reserved = " << s.reserved
				<< " .numPstates = " << s.numPstates
				<< " .numClocks = " << s.numClocks
				<< " .numBaseVoltages = " << s.numBaseVoltages
				<< " .pstates = " << DumpArray_(s.pstates)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_PERF_PSTATES20_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_PERF_PSTATES20_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_PERF_PSTATES20_INFO_V2 {"
				<< " .version = " << s.version
				<< " .bIsEditable = " << s.bIsEditable
				<< " .reserved = " << s.reserved
				<< " .numPstates = " << s.numPstates
				<< " .numClocks = " << s.numClocks
				<< " .numBaseVoltages = " << s.numBaseVoltages
				<< " .pstates = " << DumpArray_(s.pstates)
				<< " .ov = " << DumpGenerated(s.ov)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_DRIVER_VERSION)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_DRIVER_VERSION*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_DRIVER_VERSION {"
				<< " .version = " << s.version
				<< " .drvVersion = " << s.drvVersion
				<< " .bldChangeListNum = " << s.bldChangeListNum
				<< " .szBuildBranchString = " << s.szBuildBranchString
				<< " .szAdapterString = " << s.szAdapterString
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_DRIVER_MEMORY_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_DRIVER_MEMORY_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_DRIVER_MEMORY_INFO_V1 {"
				<< " .version = " << s.version
				<< " .dedicatedVideoMemory = " << s.dedicatedVideoMemory
				<< " .availableDedicatedVideoMemory = " << s.availableDedicatedVideoMemory
				<< " .systemVideoMemory = " << s.systemVideoMemory
				<< " .sharedSystemMemory = " << s.sharedSystemMemory
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_DRIVER_MEMORY_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_DRIVER_MEMORY_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_DRIVER_MEMORY_INFO_V2 {"
				<< " .version = " << s.version
				<< " .dedicatedVideoMemory = " << s.dedicatedVideoMemory
				<< " .availableDedicatedVideoMemory = " << s.availableDedicatedVideoMemory
				<< " .systemVideoMemory = " << s.systemVideoMemory
				<< " .sharedSystemMemory = " << s.sharedSystemMemory
				<< " .curAvailableDedicatedVideoMemory = " << s.curAvailableDedicatedVideoMemory
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_DRIVER_MEMORY_INFO_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_DRIVER_MEMORY_INFO_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_DRIVER_MEMORY_INFO_V3 {"
				<< " .version = " << s.version
				<< " .dedicatedVideoMemory = " << s.dedicatedVideoMemory
				<< " .availableDedicatedVideoMemory = " << s.availableDedicatedVideoMemory
				<< " .systemVideoMemory = " << s.systemVideoMemory
				<< " .sharedSystemMemory = " << s.sharedSystemMemory
				<< " .curAvailableDedicatedVideoMemory = " << s.curAvailableDedicatedVideoMemory
				<< " .dedicatedVideoMemoryEvictionsSize = " << s.dedicatedVideoMemoryEvictionsSize
				<< " .dedicatedVideoMemoryEvictionCount = " << s.dedicatedVideoMemoryEvictionCount
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_DISPLAYIDS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_DISPLAYIDS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_DISPLAYIDS {"
				<< " .version = " << s.version
				<< " .connectorType = " << DumpGenerated(s.connectorType)
				<< " .displayId = " << s.displayId
				<< " .isDynamic = " << s.isDynamic
				<< " .isMultiStreamRootNode = " << s.isMultiStreamRootNode
				<< " .isActive = " << s.isActive
				<< " .isCluster = " << s.isCluster
				<< " .isOSVisible = " << s.isOSVisible
				<< " .isWFD = " << s.isWFD
				<< " .isConnected = " << s.isConnected
				<< " .reservedInternal = " << s.reservedInternal
				<< " .isPhysicallyConnected = " << s.isPhysicallyConnected
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_BOARD_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_BOARD_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_BOARD_INFO {"
				<< " .version = " << s.version
				<< " .BoardNum = " << DumpArray_(s.BoardNum)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_ARCH_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_ARCH_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_ARCH_INFO_V1 {"
				<< " .version = " << s.version
				<< " .architecture = " << s.architecture
				<< " .implementation = " << s.implementation
				<< " .revision = " << s.revision
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_ARCH_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_ARCH_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_ARCH_INFO_V2 {"
				<< " .version = " << s.version
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_I2C_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_I2C_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_I2C_INFO_V1 {"
				<< " .version = " << s.version
				<< " .displayMask = " << s.displayMask
				<< " .bIsDDCPort = " << (int)s.bIsDDCPort
				<< " .i2cDevAddress = " << (int)s.i2cDevAddress
				<< " .pbI2cRegAddress = " << (s.pbI2cRegAddress ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pbI2cRegAddress)) : "null"s)
				<< " .regAddrSize = " << s.regAddrSize
				<< " .pbData = " << (s.pbData ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pbData)) : "null"s)
				<< " .cbSize = " << s.cbSize
				<< " .i2cSpeed = " << s.i2cSpeed
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_I2C_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_I2C_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_I2C_INFO_V2 {"
				<< " .version = " << s.version
				<< " .displayMask = " << s.displayMask
				<< " .bIsDDCPort = " << (int)s.bIsDDCPort
				<< " .i2cDevAddress = " << (int)s.i2cDevAddress
				<< " .pbI2cRegAddress = " << (s.pbI2cRegAddress ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pbI2cRegAddress)) : "null"s)
				<< " .regAddrSize = " << s.regAddrSize
				<< " .pbData = " << (s.pbData ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pbData)) : "null"s)
				<< " .cbSize = " << s.cbSize
				<< " .i2cSpeed = " << s.i2cSpeed
				<< " .i2cSpeedKhz = " << DumpGenerated(s.i2cSpeedKhz)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_I2C_INFO_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_I2C_INFO_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_I2C_INFO_V3 {"
				<< " .version = " << s.version
				<< " .displayMask = " << s.displayMask
				<< " .bIsDDCPort = " << (int)s.bIsDDCPort
				<< " .i2cDevAddress = " << (int)s.i2cDevAddress
				<< " .pbI2cRegAddress = " << (s.pbI2cRegAddress ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pbI2cRegAddress)) : "null"s)
				<< " .regAddrSize = " << s.regAddrSize
				<< " .pbData = " << (s.pbData ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pbData)) : "null"s)
				<< " .cbSize = " << s.cbSize
				<< " .i2cSpeed = " << s.i2cSpeed
				<< " .i2cSpeedKhz = " << DumpGenerated(s.i2cSpeedKhz)
				<< " .portId = " << (int)s.portId
				<< " .bIsPortIdSet = " << s.bIsPortIdSet
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_GET_HDCP_SUPPORT_STATUS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_GET_HDCP_SUPPORT_STATUS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_GET_HDCP_SUPPORT_STATUS {"
				<< " .version = " << s.version
				<< " .hdcpFuseState = " << DumpGenerated(s.hdcpFuseState)
				<< " .hdcpKeySource = " << DumpGenerated(s.hdcpKeySource)
				<< " .hdcpKeySourceState = " << DumpGenerated(s.hdcpKeySourceState)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_COMPUTE_GPU_TOPOLOGY_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_COMPUTE_GPU_TOPOLOGY_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_COMPUTE_GPU_TOPOLOGY_V1 {"
				<< " .version = " << s.version
				<< " .gpuCount = " << s.gpuCount
				<< " .computeGpus = " << DumpArray_(s.computeGpus)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_COMPUTE_GPU)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_COMPUTE_GPU*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_COMPUTE_GPU {"
				<< " .hPhysicalGpu = " << (s.hPhysicalGpu ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hPhysicalGpu)) : "null"s)
				<< " .flags = " << s.flags
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_COMPUTE_GPU_TOPOLOGY_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_COMPUTE_GPU_TOPOLOGY_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_COMPUTE_GPU_TOPOLOGY_V2 {"
				<< " .version = " << s.version
				<< " .gpuCount = " << s.gpuCount
				<< " .computeGpus = " << (s.computeGpus ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.computeGpus)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_ECC_STATUS_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_ECC_STATUS_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_ECC_STATUS_INFO {"
				<< " .version = " << s.version
				<< " .isSupported = " << s.isSupported
				<< " .configurationOptions = " << DumpGenerated(s.configurationOptions)
				<< " .isEnabled = " << s.isEnabled
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_ECC_ERROR_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_ECC_ERROR_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_ECC_ERROR_INFO {"
				<< " .version = " << s.version
				<< " .current = " << DumpGenerated(s.current)
				<< " .aggregate = " << DumpGenerated(s.aggregate)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_ECC_CONFIGURATION_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_ECC_CONFIGURATION_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_ECC_CONFIGURATION_INFO {"
				<< " .version = " << s.version
				<< " .isEnabled = " << s.isEnabled
				<< " .isEnabledByDefault = " << s.isEnabledByDefault
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_QSYNC_EVENT_DATA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_QSYNC_EVENT_DATA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_QSYNC_EVENT_DATA {"
				<< " .qsyncEvent = " << DumpGenerated(s.qsyncEvent)
				<< " .reserved = " << DumpArray_(s.reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_EVENT_REGISTER_CALLBACK)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_EVENT_REGISTER_CALLBACK*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_EVENT_REGISTER_CALLBACK {"
				<< " .version = " << s.version
				<< " .eventId = " << DumpGenerated(s.eventId)
				<< " .callbackParam = " << (s.callbackParam ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.callbackParam)) : "null"s)
				<< " .nvCallBackFunc = " << DumpGenerated(s.nvCallBackFunc)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_SCANOUT_INTENSITY_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_SCANOUT_INTENSITY_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_SCANOUT_INTENSITY_DATA_V1 {"
				<< " .version = " << s.version
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .blendingTexture = " << (s.blendingTexture ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.blendingTexture)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_SCANOUT_INTENSITY_DATA_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_SCANOUT_INTENSITY_DATA_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_SCANOUT_INTENSITY_DATA_V2 {"
				<< " .version = " << s.version
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .blendingTexture = " << (s.blendingTexture ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.blendingTexture)) : "null"s)
				<< " .offsetTexture = " << (s.offsetTexture ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.offsetTexture)) : "null"s)
				<< " .offsetTexChannels = " << s.offsetTexChannels
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_SCANOUT_INTENSITY_STATE_DATA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_SCANOUT_INTENSITY_STATE_DATA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_SCANOUT_INTENSITY_STATE_DATA {"
				<< " .version = " << s.version
				<< " .bEnabled = " << s.bEnabled
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_SCANOUT_WARPING_DATA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_SCANOUT_WARPING_DATA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_SCANOUT_WARPING_DATA {"
				<< " .version = " << s.version
				<< " .vertices = " << (s.vertices ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.vertices)) : "null"s)
				<< " .vertexFormat = " << DumpGenerated(s.vertexFormat)
				<< " .numVertices = " << s.numVertices
				<< " .textureRect = " << (s.textureRect ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.textureRect)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_SCANOUT_WARPING_STATE_DATA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_SCANOUT_WARPING_STATE_DATA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_SCANOUT_WARPING_STATE_DATA {"
				<< " .version = " << s.version
				<< " .bEnabled = " << s.bEnabled
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_SCANOUT_INFORMATION)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_SCANOUT_INFORMATION*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_SCANOUT_INFORMATION {"
				<< " .version = " << s.version
				<< " .sourceDesktopRect = " << DumpGenerated(s.sourceDesktopRect)
				<< " .sourceViewportRect = " << DumpGenerated(s.sourceViewportRect)
				<< " .targetViewportRect = " << DumpGenerated(s.targetViewportRect)
				<< " .targetDisplayWidth = " << s.targetDisplayWidth
				<< " .targetDisplayHeight = " << s.targetDisplayHeight
				<< " .cloneImportance = " << s.cloneImportance
				<< " .sourceToTargetRotation = " << DumpGenerated(s.sourceToTargetRotation)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_VIRTUALIZATION_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_VIRTUALIZATION_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_VIRTUALIZATION_INFO {"
				<< " .version = " << s.version
				<< " .virtualizationMode = " << DumpGenerated(s.virtualizationMode)
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LOGICAL_GPU_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LOGICAL_GPU_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LOGICAL_GPU_DATA_V1 {"
				<< " .version = " << s.version
				<< " .pOSAdapterId = " << (s.pOSAdapterId ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pOSAdapterId)) : "null"s)
				<< " .physicalGpuCount = " << s.physicalGpuCount
				<< " .physicalGpuHandles = " << DumpArray_(s.physicalGpuHandles)
				<< " .reserved = " << DumpArray_(s.reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSE_EXPIRY_DETAILS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSE_EXPIRY_DETAILS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSE_EXPIRY_DETAILS {"
				<< " .year = " << s.year
				<< " .month = " << s.month
				<< " .day = " << s.day
				<< " .hour = " << s.hour
				<< " .min = " << s.min
				<< " .sec = " << s.sec
				<< " .status = " << (int)s.status
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSE_FEATURE_DETAILS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSE_FEATURE_DETAILS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSE_FEATURE_DETAILS_V1 {"
				<< " .version = " << s.version
				<< " .isEnabled = " << s.isEnabled
				<< " .reserved = " << s.reserved
				<< " .featureCode = " << DumpGenerated(s.featureCode)
				<< " .licenseInfo = " << s.licenseInfo
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSE_FEATURE_DETAILS_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSE_FEATURE_DETAILS_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSE_FEATURE_DETAILS_V2 {"
				<< " .version = " << s.version
				<< " .isEnabled = " << s.isEnabled
				<< " .reserved = " << s.reserved
				<< " .featureCode = " << DumpGenerated(s.featureCode)
				<< " .licenseInfo = " << s.licenseInfo
				<< " .productName = " << s.productName
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSE_FEATURE_DETAILS_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSE_FEATURE_DETAILS_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSE_FEATURE_DETAILS_V3 {"
				<< " .version = " << s.version
				<< " .isEnabled = " << s.isEnabled
				<< " .isFeatureEnabled = " << s.isFeatureEnabled
				<< " .reserved = " << s.reserved
				<< " .featureCode = " << DumpGenerated(s.featureCode)
				<< " .licenseInfo = " << s.licenseInfo
				<< " .productName = " << s.productName
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSE_FEATURE_DETAILS_V4)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSE_FEATURE_DETAILS_V4*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSE_FEATURE_DETAILS_V4 {"
				<< " .version = " << s.version
				<< " .isEnabled = " << s.isEnabled
				<< " .isFeatureEnabled = " << s.isFeatureEnabled
				<< " .reserved = " << s.reserved
				<< " .featureCode = " << DumpGenerated(s.featureCode)
				<< " .licenseInfo = " << s.licenseInfo
				<< " .productName = " << s.productName
				<< " .licenseExpiry = " << DumpGenerated(s.licenseExpiry)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSABLE_FEATURES_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSABLE_FEATURES_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSABLE_FEATURES_V1 {"
				<< " .version = " << s.version
				<< " .isLicenseSupported = " << s.isLicenseSupported
				<< " .reserved = " << s.reserved
				<< " .licensableFeatureCount = " << s.licensableFeatureCount
				<< " .signature = " << DumpArray_(s.signature)
				<< " .licenseDetails = " << DumpArray_(s.licenseDetails)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSABLE_FEATURES_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSABLE_FEATURES_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSABLE_FEATURES_V2 {"
				<< " .version = " << s.version
				<< " .isLicenseSupported = " << s.isLicenseSupported
				<< " .reserved = " << s.reserved
				<< " .licensableFeatureCount = " << s.licensableFeatureCount
				<< " .signature = " << DumpArray_(s.signature)
				<< " .licenseDetails = " << DumpArray_(s.licenseDetails)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSABLE_FEATURES_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSABLE_FEATURES_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSABLE_FEATURES_V3 {"
				<< " .version = " << s.version
				<< " .isLicenseSupported = " << s.isLicenseSupported
				<< " .reserved = " << s.reserved
				<< " .licensableFeatureCount = " << s.licensableFeatureCount
				<< " .signature = " << DumpArray_(s.signature)
				<< " .licenseDetails = " << DumpArray_(s.licenseDetails)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LICENSABLE_FEATURES_V4)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LICENSABLE_FEATURES_V4*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LICENSABLE_FEATURES_V4 {"
				<< " .version = " << s.version
				<< " .isLicenseSupported = " << s.isLicenseSupported
				<< " .reserved = " << s.reserved
				<< " .licensableFeatureCount = " << s.licensableFeatureCount
				<< " .signature = " << DumpArray_(s.signature)
				<< " .licenseDetails = " << DumpArray_(s.licenseDetails)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_VR_READY_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_VR_READY_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_VR_READY_V1 {"
				<< " .version = " << s.version
				<< " .isVRReady = " << s.isVRReady
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_PERF_PSTATES_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_PERF_PSTATES_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_PERF_PSTATES_INFO_V1 {"
				<< " .version = " << s.version
				<< " .flags = " << s.flags
				<< " .numPstates = " << s.numPstates
				<< " .numClocks = " << s.numClocks
				<< " .pstates = " << DumpArray_(s.pstates)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_PERF_PSTATES_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_PERF_PSTATES_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_PERF_PSTATES_INFO_V2 {"
				<< " .version = " << s.version
				<< " .flags = " << s.flags
				<< " .numPstates = " << s.numPstates
				<< " .numClocks = " << s.numClocks
				<< " .numVoltages = " << s.numVoltages
				<< " .pstates = " << DumpArray_(s.pstates)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_DYNAMIC_PSTATES_INFO_EX)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_DYNAMIC_PSTATES_INFO_EX*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_DYNAMIC_PSTATES_INFO_EX {"
				<< " .version = " << s.version
				<< " .flags = " << s.flags
				<< " .utilization = " << DumpArray_(s.utilization)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_THERMAL_SETTINGS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_THERMAL_SETTINGS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_THERMAL_SETTINGS_V1 {"
				<< " .version = " << s.version
				<< " .count = " << s.count
				<< " .sensor = " << DumpArray_(s.sensor)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_THERMAL_SETTINGS_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_THERMAL_SETTINGS_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_THERMAL_SETTINGS_V2 {"
				<< " .version = " << s.version
				<< " .count = " << s.count
				<< " .sensor = " << DumpArray_(s.sensor)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_CLOCK_FREQUENCIES_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_CLOCK_FREQUENCIES_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_CLOCK_FREQUENCIES_V1 {"
				<< " .version = " << s.version
				<< " .reserved = " << s.reserved
				<< " .domain = " << DumpArray_(s.domain)
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
				<< " .domain = " << DumpArray_(s.domain)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_V1 {"
				<< " .version = " << s.version
				<< " .hPhysicalGpu = " << (s.hPhysicalGpu ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hPhysicalGpu)) : "null"s)
				<< " .Attribute = " << DumpGenerated(s.Attribute)
				<< " .bSupported = " << s.bSupported
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_GET_ILLUMINATION_PARM_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_GET_ILLUMINATION_PARM_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_GET_ILLUMINATION_PARM_V1 {"
				<< " .version = " << s.version
				<< " .hPhysicalGpu = " << (s.hPhysicalGpu ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hPhysicalGpu)) : "null"s)
				<< " .Attribute = " << DumpGenerated(s.Attribute)
				<< " .Value = " << s.Value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_SET_ILLUMINATION_PARM_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_SET_ILLUMINATION_PARM_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_SET_ILLUMINATION_PARM_V1 {"
				<< " .version = " << s.version
				<< " .hPhysicalGpu = " << (s.hPhysicalGpu ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hPhysicalGpu)) : "null"s)
				<< " .Attribute = " << DumpGenerated(s.Attribute)
				<< " .Value = " << s.Value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_MCUV10)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_MCUV10*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_MCUV10 {"
				<< " .i2cDevIdx = " << (int)s.i2cDevIdx
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_GPIO_PWM_RGBW)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_GPIO_PWM_RGBW*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_GPIO_PWM_RGBW {"
				<< " .gpioPinRed = " << (int)s.gpioPinRed
				<< " .gpioPinGreen = " << (int)s.gpioPinGreen
				<< " .gpioPinBlue = " << (int)s.gpioPinBlue
				<< " .gpioPinWhite = " << (int)s.gpioPinWhite
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_GPIO_PWM_SINGLE_COLOR)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_GPIO_PWM_SINGLE_COLOR*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_DATA_GPIO_PWM_SINGLE_COLOR {"
				<< " .gpioPinSingleColor = " << (int)s.gpioPinSingleColor
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_DEVICE_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_V1 {"
				<< " .type = " << DumpGenerated(s.type)
				<< " .ctrlModeMask = " << s.ctrlModeMask
				<< " .data = " << DumpGenerated(s.data)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .numIllumDevices = " << s.numIllumDevices
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " .devices = " << DumpArray_(s.devices)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_DEVICE_SYNC_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_CLIENT_ILLUM_DEVICE_SYNC_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_CLIENT_ILLUM_DEVICE_SYNC_V1 {"
				<< " .bSync = " << (int)s.bSync
				<< " .timeStampms = " << s.timeStampms
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_DEVICE_CONTROL_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_CLIENT_ILLUM_DEVICE_CONTROL_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_CLIENT_ILLUM_DEVICE_CONTROL_V1 {"
				<< " .type = " << DumpGenerated(s.type)
				<< " .syncData = " << DumpGenerated(s.syncData)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_DEVICE_CONTROL_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GPU_CLIENT_ILLUM_DEVICE_CONTROL_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GPU_CLIENT_ILLUM_DEVICE_CONTROL_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .numIllumDevices = " << s.numIllumDevices
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " .devices = " << DumpArray_(s.devices)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_RGB)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_RGB*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_RGB {"
				<< " .rsvd = " << (int)s.rsvd
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_RGBW)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_RGBW*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_RGBW {"
				<< " .rsvd = " << (int)s.rsvd
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_SINGLE_COLOR)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_SINGLE_COLOR*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_INFO_DATA_SINGLE_COLOR {"
				<< " .rsvd = " << (int)s.rsvd
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_INFO_V1 {"
				<< " .type = " << DumpGenerated(s.type)
				<< " .illumDeviceIdx = " << (int)s.illumDeviceIdx
				<< " .provIdx = " << (int)s.provIdx
				<< " .zoneLocation = " << DumpGenerated(s.zoneLocation)
				<< " .data = " << DumpGenerated(s.data)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .numIllumZones = " << s.numIllumZones
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " .zones = " << DumpArray_(s.zones)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS {"
				<< " .colorR = " << (int)s.colorR
				<< " .colorG = " << (int)s.colorG
				<< " .colorB = " << (int)s.colorB
				<< " .brightnessPct = " << (int)s.brightnessPct
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB {"
				<< " .rgbParams = " << DumpGenerated(s.rgbParams)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR {"
				<< " .cycleType = " << DumpGenerated(s.cycleType)
				<< " .grpCount = " << (int)s.grpCount
				<< " .riseTimems = " << s.riseTimems
				<< " .fallTimems = " << s.fallTimems
				<< " .ATimems = " << s.ATimems
				<< " .BTimems = " << s.BTimems
				<< " .grpIdleTimems = " << s.grpIdleTimems
				<< " .phaseOffsetms = " << s.phaseOffsetms
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGB)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGB*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGB {"
				<< " .rgbParams = " << DumpArray_(s.rgbParams)
				<< " .piecewiseLinearData = " << DumpGenerated(s.piecewiseLinearData)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGB)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGB*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGB {"
				<< " .data = " << DumpGenerated(s.data)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED_PARAMS {"
				<< " .brightnessPct = " << (int)s.brightnessPct
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED {"
				<< " .colorFixedParams = " << DumpGenerated(s.colorFixedParams)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_COLOR_FIXED)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_COLOR_FIXED*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_COLOR_FIXED {"
				<< " .colorFixedParams = " << DumpArray_(s.colorFixedParams)
				<< " .piecewiseLinearData = " << DumpGenerated(s.piecewiseLinearData)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_COLOR_FIXED)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_COLOR_FIXED*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_COLOR_FIXED {"
				<< " .data = " << DumpGenerated(s.data)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS {"
				<< " .colorR = " << (int)s.colorR
				<< " .colorG = " << (int)s.colorG
				<< " .colorB = " << (int)s.colorB
				<< " .colorW = " << (int)s.colorW
				<< " .brightnessPct = " << (int)s.brightnessPct
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW {"
				<< " .rgbwParams = " << DumpGenerated(s.rgbwParams)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGBW)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGBW*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGBW {"
				<< " .rgbwParams = " << DumpArray_(s.rgbwParams)
				<< " .piecewiseLinearData = " << DumpGenerated(s.piecewiseLinearData)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGBW)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGBW*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGBW {"
				<< " .data = " << DumpGenerated(s.data)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS {"
				<< " .brightnessPct = " << (int)s.brightnessPct
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR {"
				<< " .singleColorParams = " << DumpGenerated(s.singleColorParams)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_SINGLE_COLOR)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_SINGLE_COLOR*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_SINGLE_COLOR {"
				<< " .singleColorParams = " << DumpArray_(s.singleColorParams)
				<< " .piecewiseLinearData = " << DumpGenerated(s.piecewiseLinearData)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_SINGLE_COLOR)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_SINGLE_COLOR*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_SINGLE_COLOR {"
				<< " .data = " << DumpGenerated(s.data)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 {"
				<< " .type = " << DumpGenerated(s.type)
				<< " .ctrlMode = " << DumpGenerated(s.ctrlMode)
				<< " .data = " << DumpGenerated(s.data)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .bDefault = " << s.bDefault
				<< " .rsvdField = " << s.rsvdField
				<< " .numIllumZonesControl = " << s.numIllumZonesControl
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " .zones = " << DumpArray_(s.zones)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAY_PORT_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAY_PORT_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAY_PORT_INFO_V1 {"
				<< " .version = " << s.version
				<< " .dpcd_ver = " << s.dpcd_ver
				<< " .maxLinkRate = " << DumpGenerated(s.maxLinkRate)
				<< " .maxLaneCount = " << DumpGenerated(s.maxLaneCount)
				<< " .curLinkRate = " << DumpGenerated(s.curLinkRate)
				<< " .curLaneCount = " << DumpGenerated(s.curLaneCount)
				<< " .colorFormat = " << DumpGenerated(s.colorFormat)
				<< " .dynamicRange = " << DumpGenerated(s.dynamicRange)
				<< " .colorimetry = " << DumpGenerated(s.colorimetry)
				<< " .bpc = " << DumpGenerated(s.bpc)
				<< " .isDp = " << s.isDp
				<< " .isInternalDp = " << s.isInternalDp
				<< " .isColorCtrlSupported = " << s.isColorCtrlSupported
				<< " .is6BPCSupported = " << s.is6BPCSupported
				<< " .is8BPCSupported = " << s.is8BPCSupported
				<< " .is10BPCSupported = " << s.is10BPCSupported
				<< " .is12BPCSupported = " << s.is12BPCSupported
				<< " .is16BPCSupported = " << s.is16BPCSupported
				<< " .isYCrCb420Supported = " << s.isYCrCb420Supported
				<< " .isYCrCb422Supported = " << s.isYCrCb422Supported
				<< " .isYCrCb444Supported = " << s.isYCrCb444Supported
				<< " .isRgb444SupportedOnCurrentMode = " << s.isRgb444SupportedOnCurrentMode
				<< " .isYCbCr444SupportedOnCurrentMode = " << s.isYCbCr444SupportedOnCurrentMode
				<< " .isYCbCr422SupportedOnCurrentMode = " << s.isYCbCr422SupportedOnCurrentMode
				<< " .isYCbCr420SupportedOnCurrentMode = " << s.isYCbCr420SupportedOnCurrentMode
				<< " .is6BPCSupportedOnCurrentMode = " << s.is6BPCSupportedOnCurrentMode
				<< " .is8BPCSupportedOnCurrentMode = " << s.is8BPCSupportedOnCurrentMode
				<< " .is10BPCSupportedOnCurrentMode = " << s.is10BPCSupportedOnCurrentMode
				<< " .is12BPCSupportedOnCurrentMode = " << s.is12BPCSupportedOnCurrentMode
				<< " .is16BPCSupportedOnCurrentMode = " << s.is16BPCSupportedOnCurrentMode
				<< " .isMonxvYCC601Capable = " << s.isMonxvYCC601Capable
				<< " .isMonxvYCC709Capable = " << s.isMonxvYCC709Capable
				<< " .isMonsYCC601Capable = " << s.isMonsYCC601Capable
				<< " .isMonAdobeYCC601Capable = " << s.isMonAdobeYCC601Capable
				<< " .isMonAdobeRGBCapable = " << s.isMonAdobeRGBCapable
				<< " .isMonBT2020RGBCapable = " << s.isMonBT2020RGBCapable
				<< " .isMonBT2020YCCCapable = " << s.isMonBT2020YCCCapable
				<< " .isMonBT2020cYCCCapable = " << s.isMonBT2020cYCCCapable
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_DISPLAY_PORT_CONFIG)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_DISPLAY_PORT_CONFIG*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_DISPLAY_PORT_CONFIG {"
				<< " .version = " << s.version
				<< " .linkRate = " << DumpGenerated(s.linkRate)
				<< " .laneCount = " << DumpGenerated(s.laneCount)
				<< " .colorFormat = " << DumpGenerated(s.colorFormat)
				<< " .dynamicRange = " << DumpGenerated(s.dynamicRange)
				<< " .colorimetry = " << DumpGenerated(s.colorimetry)
				<< " .bpc = " << DumpGenerated(s.bpc)
				<< " .isHPD = " << s.isHPD
				<< " .isSetDeferred = " << s.isSetDeferred
				<< " .isChromaLpfOff = " << s.isChromaLpfOff
				<< " .isDitherOff = " << s.isDitherOff
				<< " .testLinkTrain = " << s.testLinkTrain
				<< " .testColorChange = " << s.testColorChange
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_HDMI_SUPPORT_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_HDMI_SUPPORT_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_HDMI_SUPPORT_INFO_V1 {"
				<< " .version = " << s.version
				<< " .isGpuHDMICapable = " << s.isGpuHDMICapable
				<< " .isMonUnderscanCapable = " << s.isMonUnderscanCapable
				<< " .isMonBasicAudioCapable = " << s.isMonBasicAudioCapable
				<< " .isMonYCbCr444Capable = " << s.isMonYCbCr444Capable
				<< " .isMonYCbCr422Capable = " << s.isMonYCbCr422Capable
				<< " .isMonxvYCC601Capable = " << s.isMonxvYCC601Capable
				<< " .isMonxvYCC709Capable = " << s.isMonxvYCC709Capable
				<< " .isMonHDMI = " << s.isMonHDMI
				<< " .reserved = " << s.reserved
				<< " .EDID861ExtRev = " << s.EDID861ExtRev
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_HDMI_SUPPORT_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_HDMI_SUPPORT_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_HDMI_SUPPORT_INFO_V2 {"
				<< " .version = " << s.version
				<< " .isGpuHDMICapable = " << s.isGpuHDMICapable
				<< " .isMonUnderscanCapable = " << s.isMonUnderscanCapable
				<< " .isMonBasicAudioCapable = " << s.isMonBasicAudioCapable
				<< " .isMonYCbCr444Capable = " << s.isMonYCbCr444Capable
				<< " .isMonYCbCr422Capable = " << s.isMonYCbCr422Capable
				<< " .isMonxvYCC601Capable = " << s.isMonxvYCC601Capable
				<< " .isMonxvYCC709Capable = " << s.isMonxvYCC709Capable
				<< " .isMonHDMI = " << s.isMonHDMI
				<< " .isMonsYCC601Capable = " << s.isMonsYCC601Capable
				<< " .isMonAdobeYCC601Capable = " << s.isMonAdobeYCC601Capable
				<< " .isMonAdobeRGBCapable = " << s.isMonAdobeRGBCapable
				<< " .reserved = " << s.reserved
				<< " .EDID861ExtRev = " << s.EDID861ExtRev
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_INFOFRAME_PROPERTY)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_INFOFRAME_PROPERTY*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_INFOFRAME_PROPERTY {"
				<< " .mode = " << s.mode
				<< " .blackList = " << s.blackList
				<< " .reserved = " << s.reserved
				<< " .version = " << s.version
				<< " .length = " << s.length
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_INFOFRAME_VIDEO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_INFOFRAME_VIDEO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_INFOFRAME_VIDEO {"
				<< " .vic = " << s.vic
				<< " .pixelRepeat = " << s.pixelRepeat
				<< " .colorSpace = " << s.colorSpace
				<< " .colorimetry = " << s.colorimetry
				<< " .extendedColorimetry = " << s.extendedColorimetry
				<< " .rgbQuantizationRange = " << s.rgbQuantizationRange
				<< " .yccQuantizationRange = " << s.yccQuantizationRange
				<< " .itContent = " << s.itContent
				<< " .contentTypes = " << s.contentTypes
				<< " .scanInfo = " << s.scanInfo
				<< " .activeFormatInfoPresent = " << s.activeFormatInfoPresent
				<< " .activeFormatAspectRatio = " << s.activeFormatAspectRatio
				<< " .picAspectRatio = " << s.picAspectRatio
				<< " .nonuniformScaling = " << s.nonuniformScaling
				<< " .barInfo = " << s.barInfo
				<< " .top_bar = " << s.top_bar
				<< " .bottom_bar = " << s.bottom_bar
				<< " .left_bar = " << s.left_bar
				<< " .right_bar = " << s.right_bar
				<< " .Future17 = " << s.Future17
				<< " .Future47 = " << s.Future47
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_INFOFRAME_AUDIO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_INFOFRAME_AUDIO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_INFOFRAME_AUDIO {"
				<< " .codingType = " << s.codingType
				<< " .codingExtensionType = " << s.codingExtensionType
				<< " .sampleSize = " << s.sampleSize
				<< " .sampleRate = " << s.sampleRate
				<< " .channelCount = " << s.channelCount
				<< " .speakerPlacement = " << s.speakerPlacement
				<< " .downmixInhibit = " << s.downmixInhibit
				<< " .lfePlaybackLevel = " << s.lfePlaybackLevel
				<< " .levelShift = " << s.levelShift
				<< " .Future12 = " << s.Future12
				<< " .Future2x = " << s.Future2x
				<< " .Future3x = " << s.Future3x
				<< " .Future52 = " << s.Future52
				<< " .Future6 = " << s.Future6
				<< " .Future7 = " << s.Future7
				<< " .Future8 = " << s.Future8
				<< " .Future9 = " << s.Future9
				<< " .Future10 = " << s.Future10
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_INFOFRAME_DATA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_INFOFRAME_DATA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_INFOFRAME_DATA {"
				<< " .version = " << s.version
				<< " .size = " << s.size
				<< " .cmd = " << (int)s.cmd
				<< " .type = " << (int)s.type
				<< " .infoframe = " << DumpGenerated(s.infoframe)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_COLOR_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_COLOR_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_COLOR_DATA_V1 {"
				<< " .version = " << s.version
				<< " .size = " << s.size
				<< " .cmd = " << (int)s.cmd
				<< " .data = " << DumpGenerated(s.data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_COLOR_DATA_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_COLOR_DATA_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_COLOR_DATA_V2 {"
				<< " .version = " << s.version
				<< " .size = " << s.size
				<< " .cmd = " << (int)s.cmd
				<< " .data = " << DumpGenerated(s.data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_COLOR_DATA_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_COLOR_DATA_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_COLOR_DATA_V3 {"
				<< " .version = " << s.version
				<< " .size = " << s.size
				<< " .cmd = " << (int)s.cmd
				<< " .data = " << DumpGenerated(s.data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_COLOR_DATA_V4)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_COLOR_DATA_V4*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_COLOR_DATA_V4 {"
				<< " .version = " << s.version
				<< " .size = " << s.size
				<< " .cmd = " << (int)s.cmd
				<< " .data = " << DumpGenerated(s.data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_COLOR_DATA_V5)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_COLOR_DATA_V5*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_COLOR_DATA_V5 {"
				<< " .version = " << s.version
				<< " .size = " << s.size
				<< " .cmd = " << (int)s.cmd
				<< " .data = " << DumpGenerated(s.data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_HDR_CAPABILITIES_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_HDR_CAPABILITIES_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_HDR_CAPABILITIES_V1 {"
				<< " .version = " << s.version
				<< " .isST2084EotfSupported = " << s.isST2084EotfSupported
				<< " .isTraditionalHdrGammaSupported = " << s.isTraditionalHdrGammaSupported
				<< " .isEdrSupported = " << s.isEdrSupported
				<< " .driverExpandDefaultHdrParameters = " << s.driverExpandDefaultHdrParameters
				<< " .isTraditionalSdrGammaSupported = " << s.isTraditionalSdrGammaSupported
				<< " .reserved = " << s.reserved
				<< " .static_metadata_descriptor_id = " << DumpGenerated(s.static_metadata_descriptor_id)
				<< " .display_data = " << DumpGenerated(s.display_data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_HDR_CAPABILITIES_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_HDR_CAPABILITIES_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_HDR_CAPABILITIES_V2 {"
				<< " .version = " << s.version
				<< " .isST2084EotfSupported = " << s.isST2084EotfSupported
				<< " .isTraditionalHdrGammaSupported = " << s.isTraditionalHdrGammaSupported
				<< " .isEdrSupported = " << s.isEdrSupported
				<< " .driverExpandDefaultHdrParameters = " << s.driverExpandDefaultHdrParameters
				<< " .isTraditionalSdrGammaSupported = " << s.isTraditionalSdrGammaSupported
				<< " .isDolbyVisionSupported = " << s.isDolbyVisionSupported
				<< " .reserved = " << s.reserved
				<< " .static_metadata_descriptor_id = " << DumpGenerated(s.static_metadata_descriptor_id)
				<< " .display_data = " << DumpGenerated(s.display_data)
				<< " .dv_static_metadata = " << DumpGenerated(s.dv_static_metadata)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_HDR_COLOR_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_HDR_COLOR_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_HDR_COLOR_DATA_V1 {"
				<< " .version = " << s.version
				<< " .cmd = " << DumpGenerated(s.cmd)
				<< " .hdrMode = " << DumpGenerated(s.hdrMode)
				<< " .static_metadata_descriptor_id = " << DumpGenerated(s.static_metadata_descriptor_id)
				<< " .mastering_display_data = " << DumpGenerated(s.mastering_display_data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_HDR_COLOR_DATA_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_HDR_COLOR_DATA_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_HDR_COLOR_DATA_V2 {"
				<< " .version = " << s.version
				<< " .cmd = " << DumpGenerated(s.cmd)
				<< " .hdrMode = " << DumpGenerated(s.hdrMode)
				<< " .static_metadata_descriptor_id = " << DumpGenerated(s.static_metadata_descriptor_id)
				<< " .mastering_display_data = " << DumpGenerated(s.mastering_display_data)
				<< " .hdrColorFormat = " << DumpGenerated(s.hdrColorFormat)
				<< " .hdrDynamicRange = " << DumpGenerated(s.hdrDynamicRange)
				<< " .hdrBpc = " << DumpGenerated(s.hdrBpc)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_TIMING_FLAG)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_TIMING_FLAG*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_TIMING_FLAG {"
				<< " .isInterlaced = " << s.isInterlaced
				<< " .reserved0 = " << s.reserved0
				<< " .scaling = " << s.scaling
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_TIMING_INPUT)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_TIMING_INPUT*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_TIMING_INPUT {"
				<< " .version = " << s.version
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .rr = " << s.rr
				<< " .flag = " << DumpGenerated(s.flag)
				<< " .type = " << DumpGenerated(s.type)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MONITOR_CAPS_VCDB)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MONITOR_CAPS_VCDB*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MONITOR_CAPS_VCDB {"
				<< " .quantizationRangeYcc = " << (int)s.quantizationRangeYcc
				<< " .quantizationRangeRgb = " << (int)s.quantizationRangeRgb
				<< " .scanInfoPreferredVideoFormat = " << (int)s.scanInfoPreferredVideoFormat
				<< " .scanInfoITVideoFormats = " << (int)s.scanInfoITVideoFormats
				<< " .scanInfoCEVideoFormats = " << (int)s.scanInfoCEVideoFormats
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MONITOR_CAPS_VSDB)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MONITOR_CAPS_VSDB*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MONITOR_CAPS_VSDB {"
				<< " .sourcePhysicalAddressB = " << (int)s.sourcePhysicalAddressB
				<< " .sourcePhysicalAddressA = " << (int)s.sourcePhysicalAddressA
				<< " .sourcePhysicalAddressD = " << (int)s.sourcePhysicalAddressD
				<< " .sourcePhysicalAddressC = " << (int)s.sourcePhysicalAddressC
				<< " .supportDualDviOperation = " << (int)s.supportDualDviOperation
				<< " .reserved6 = " << (int)s.reserved6
				<< " .supportDeepColorYCbCr444 = " << (int)s.supportDeepColorYCbCr444
				<< " .supportDeepColor30bits = " << (int)s.supportDeepColor30bits
				<< " .supportDeepColor36bits = " << (int)s.supportDeepColor36bits
				<< " .supportDeepColor48bits = " << (int)s.supportDeepColor48bits
				<< " .supportAI = " << (int)s.supportAI
				<< " .maxTmdsClock = " << (int)s.maxTmdsClock
				<< " .cnc0SupportGraphicsTextContent = " << (int)s.cnc0SupportGraphicsTextContent
				<< " .cnc1SupportPhotoContent = " << (int)s.cnc1SupportPhotoContent
				<< " .cnc2SupportCinemaContent = " << (int)s.cnc2SupportCinemaContent
				<< " .cnc3SupportGameContent = " << (int)s.cnc3SupportGameContent
				<< " .reserved8 = " << (int)s.reserved8
				<< " .hasVicEntries = " << (int)s.hasVicEntries
				<< " .hasInterlacedLatencyField = " << (int)s.hasInterlacedLatencyField
				<< " .hasLatencyField = " << (int)s.hasLatencyField
				<< " .videoLatency = " << (int)s.videoLatency
				<< " .audioLatency = " << (int)s.audioLatency
				<< " .interlacedVideoLatency = " << (int)s.interlacedVideoLatency
				<< " .interlacedAudioLatency = " << (int)s.interlacedAudioLatency
				<< " .reserved13 = " << (int)s.reserved13
				<< " .has3dEntries = " << (int)s.has3dEntries
				<< " .hdmi3dLength = " << (int)s.hdmi3dLength
				<< " .hdmiVicLength = " << (int)s.hdmiVicLength
				<< " .hdmi_vic = " << DumpArray_(s.hdmi_vic)
				<< " .hdmi_3d = " << DumpArray_(s.hdmi_3d)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MONITOR_CAPS_GENERIC)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MONITOR_CAPS_GENERIC*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MONITOR_CAPS_GENERIC {"
				<< " .supportVRR = " << (int)s.supportVRR
				<< " .supportULMB = " << (int)s.supportULMB
				<< " .isTrueGsync = " << (int)s.isTrueGsync
				<< " .isRLACapable = " << (int)s.isRLACapable
				<< " .reserved = " << (int)s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MONITOR_CAPABILITIES_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MONITOR_CAPABILITIES_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MONITOR_CAPABILITIES_V1 {"
				<< " .version = " << s.version
				<< " .size = " << s.size
				<< " .infoType = " << s.infoType
				<< " .connectorType = " << s.connectorType
				<< " .bIsValidInfo = " << (int)s.bIsValidInfo
				<< " .data = " << DumpGenerated(s.data)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MONITOR_COLOR_DATA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MONITOR_COLOR_DATA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MONITOR_COLOR_DATA {"
				<< " .version = " << s.version
				<< " .colorFormat = " << DumpGenerated(s.colorFormat)
				<< " .backendBitDepths = " << DumpGenerated(s.backendBitDepths)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_CUSTOM_DISPLAY)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_CUSTOM_DISPLAY*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_CUSTOM_DISPLAY {"
				<< " .version = " << s.version
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .depth = " << s.depth
				<< " .colorFormat = " << DumpGenerated(s.colorFormat)
				<< " .srcPartition = " << DumpGenerated(s.srcPartition)
				<< " .xRatio = " << s.xRatio
				<< " .yRatio = " << s.yRatio
				<< " .timing = " << DumpGenerated(s.timing)
				<< " .hwModeSetOnly = " << s.hwModeSetOnly
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GET_ADAPTIVE_SYNC_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GET_ADAPTIVE_SYNC_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GET_ADAPTIVE_SYNC_DATA_V1 {"
				<< " .version = " << s.version
				<< " .maxFrameInterval = " << s.maxFrameInterval
				<< " .bDisableAdaptiveSync = " << s.bDisableAdaptiveSync
				<< " .bDisableFrameSplitting = " << s.bDisableFrameSplitting
				<< " .reserved = " << s.reserved
				<< " .lastFlipRefreshCount = " << s.lastFlipRefreshCount
				<< " .lastFlipTimeStamp = " << s.lastFlipTimeStamp
				<< " .reservedEx = " << DumpArray_(s.reservedEx)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_SET_ADAPTIVE_SYNC_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_SET_ADAPTIVE_SYNC_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_SET_ADAPTIVE_SYNC_DATA_V1 {"
				<< " .version = " << s.version
				<< " .maxFrameInterval = " << s.maxFrameInterval
				<< " .bDisableAdaptiveSync = " << s.bDisableAdaptiveSync
				<< " .bDisableFrameSplitting = " << s.bDisableFrameSplitting
				<< " .reserved = " << s.reserved
				<< " .reservedEx = " << DumpArray_(s.reservedEx)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GET_VIRTUAL_REFRESH_RATE_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GET_VIRTUAL_REFRESH_RATE_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GET_VIRTUAL_REFRESH_RATE_DATA_V1 {"
				<< " .version = " << s.version
				<< " .frameIntervalUs = " << s.frameIntervalUs
				<< " .reservedEx = " << DumpArray_(s.reservedEx)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_SET_VIRTUAL_REFRESH_RATE_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_SET_VIRTUAL_REFRESH_RATE_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_SET_VIRTUAL_REFRESH_RATE_DATA_V1 {"
				<< " .version = " << s.version
				<< " .frameIntervalUs = " << s.frameIntervalUs
				<< " .reservedEx = " << DumpArray_(s.reservedEx)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_SET_PREFERRED_STEREO_DISPLAY_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_SET_PREFERRED_STEREO_DISPLAY_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_SET_PREFERRED_STEREO_DISPLAY_V1 {"
				<< " .version = " << s.version
				<< " .displayId = " << s.displayId
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GET_PREFERRED_STEREO_DISPLAY_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GET_PREFERRED_STEREO_DISPLAY_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GET_PREFERRED_STEREO_DISPLAY_V1 {"
				<< " .version = " << s.version
				<< " .displayId = " << s.displayId
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MANAGED_DEDICATED_DISPLAY_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MANAGED_DEDICATED_DISPLAY_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MANAGED_DEDICATED_DISPLAY_INFO {"
				<< " .version = " << s.version
				<< " .displayId = " << s.displayId
				<< " .isAcquired = " << s.isAcquired
				<< " .isMosaic = " << s.isMosaic
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_MOSAIC_TOPO_DETAILS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_MOSAIC_TOPO_DETAILS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_MOSAIC_TOPO_DETAILS {"
				<< " .version = " << s.version
				<< " .hLogicalGPU = " << (s.hLogicalGPU ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hLogicalGPU)) : "null"s)
				<< " .validityMask = " << s.validityMask
				<< " .rowCount = " << s.rowCount
				<< " .colCount = " << s.colCount
				<< " .gpuLayout = " << DumpArray_(s.gpuLayout)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_MOSAIC_TOPO_BRIEF)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_MOSAIC_TOPO_BRIEF*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_MOSAIC_TOPO_BRIEF {"
				<< " .version = " << s.version
				<< " .topo = " << DumpGenerated(s.topo)
				<< " .enabled = " << s.enabled
				<< " .isPossible = " << s.isPossible
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MOSAIC_DISPLAY_SETTING_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MOSAIC_DISPLAY_SETTING_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MOSAIC_DISPLAY_SETTING_V1 {"
				<< " .version = " << s.version
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .bpp = " << s.bpp
				<< " .freq = " << s.freq
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_MOSAIC_DISPLAY_SETTING_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_MOSAIC_DISPLAY_SETTING_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_MOSAIC_DISPLAY_SETTING_V2 {"
				<< " .version = " << s.version
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " .bpp = " << s.bpp
				<< " .freq = " << s.freq
				<< " .rrx1k = " << s.rrx1k
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MOSAIC_SUPPORTED_TOPO_INFO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MOSAIC_SUPPORTED_TOPO_INFO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MOSAIC_SUPPORTED_TOPO_INFO_V1 {"
				<< " .version = " << s.version
				<< " .topoBriefsCount = " << s.topoBriefsCount
				<< " .topoBriefs = " << DumpArray_(s.topoBriefs)
				<< " .displaySettingsCount = " << s.displaySettingsCount
				<< " .displaySettings = " << DumpArray_(s.displaySettings)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MOSAIC_SUPPORTED_TOPO_INFO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MOSAIC_SUPPORTED_TOPO_INFO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MOSAIC_SUPPORTED_TOPO_INFO_V2 {"
				<< " .version = " << s.version
				<< " .topoBriefsCount = " << s.topoBriefsCount
				<< " .topoBriefs = " << DumpArray_(s.topoBriefs)
				<< " .displaySettingsCount = " << s.displaySettingsCount
				<< " .displaySettings = " << DumpArray_(s.displaySettings)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_MOSAIC_TOPO_GROUP)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_MOSAIC_TOPO_GROUP*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_MOSAIC_TOPO_GROUP {"
				<< " .version = " << s.version
				<< " .brief = " << DumpGenerated(s.brief)
				<< " .count = " << s.count
				<< " .topos = " << DumpArray_(s.topos)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MOSAIC_GRID_TOPO_DISPLAY_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MOSAIC_GRID_TOPO_DISPLAY_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MOSAIC_GRID_TOPO_DISPLAY_V1 {"
				<< " .displayId = " << s.displayId
				<< " .overlapX = " << s.overlapX
				<< " .overlapY = " << s.overlapY
				<< " .rotation = " << DumpGenerated(s.rotation)
				<< " .cloneGroup = " << s.cloneGroup
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MOSAIC_GRID_TOPO_DISPLAY_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MOSAIC_GRID_TOPO_DISPLAY_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MOSAIC_GRID_TOPO_DISPLAY_V2 {"
				<< " .version = " << s.version
				<< " .displayId = " << s.displayId
				<< " .overlapX = " << s.overlapX
				<< " .overlapY = " << s.overlapY
				<< " .rotation = " << DumpGenerated(s.rotation)
				<< " .cloneGroup = " << s.cloneGroup
				<< " .pixelShiftType = " << DumpGenerated(s.pixelShiftType)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MOSAIC_GRID_TOPO_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MOSAIC_GRID_TOPO_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MOSAIC_GRID_TOPO_V1 {"
				<< " .version = " << s.version
				<< " .rows = " << s.rows
				<< " .columns = " << s.columns
				<< " .displayCount = " << s.displayCount
				<< " .applyWithBezelCorrect = " << s.applyWithBezelCorrect
				<< " .immersiveGaming = " << s.immersiveGaming
				<< " .baseMosaic = " << s.baseMosaic
				<< " .driverReloadAllowed = " << s.driverReloadAllowed
				<< " .acceleratePrimaryDisplay = " << s.acceleratePrimaryDisplay
				<< " .reserved = " << s.reserved
				<< " .displays = " << DumpArray_(s.displays)
				<< " .displaySettings = " << DumpGenerated(s.displaySettings)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MOSAIC_GRID_TOPO_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MOSAIC_GRID_TOPO_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MOSAIC_GRID_TOPO_V2 {"
				<< " .version = " << s.version
				<< " .rows = " << s.rows
				<< " .columns = " << s.columns
				<< " .displayCount = " << s.displayCount
				<< " .applyWithBezelCorrect = " << s.applyWithBezelCorrect
				<< " .immersiveGaming = " << s.immersiveGaming
				<< " .baseMosaic = " << s.baseMosaic
				<< " .driverReloadAllowed = " << s.driverReloadAllowed
				<< " .acceleratePrimaryDisplay = " << s.acceleratePrimaryDisplay
				<< " .pixelShift = " << s.pixelShift
				<< " .reserved = " << s.reserved
				<< " .displays = " << DumpArray_(s.displays)
				<< " .displaySettings = " << DumpGenerated(s.displaySettings)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_MOSAIC_DISPLAY_TOPO_STATUS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_MOSAIC_DISPLAY_TOPO_STATUS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_MOSAIC_DISPLAY_TOPO_STATUS {"
				<< " .version = " << s.version
				<< " .errorFlags = " << s.errorFlags
				<< " .warningFlags = " << s.warningFlags
				<< " .displayCount = " << s.displayCount
				<< " .displays = " << DumpArray_(s.displays)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_MOSAIC_TOPOLOGY)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_MOSAIC_TOPOLOGY*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_MOSAIC_TOPOLOGY {"
				<< " .version = " << s.version
				<< " .rowCount = " << s.rowCount
				<< " .colCount = " << s.colCount
				<< " .gpuLayout = " << DumpArray_(s.gpuLayout)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_MOSAIC_SUPPORTED_TOPOLOGIES)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_MOSAIC_SUPPORTED_TOPOLOGIES*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_MOSAIC_SUPPORTED_TOPOLOGIES {"
				<< " .version = " << s.version
				<< " .totalCount = " << s.totalCount
				<< " .topos = " << DumpArray_(s.topos)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_CAPABILITIES_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_CAPABILITIES_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_CAPABILITIES_V1 {"
				<< " .version = " << s.version
				<< " .boardId = " << s.boardId
				<< " .revision = " << s.revision
				<< " .capFlags = " << s.capFlags
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_CAPABILITIES_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_CAPABILITIES_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_CAPABILITIES_V2 {"
				<< " .version = " << s.version
				<< " .boardId = " << s.boardId
				<< " .revision = " << s.revision
				<< " .capFlags = " << s.capFlags
				<< " .extendedRevision = " << s.extendedRevision
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_GPU)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_GPU*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_GPU {"
				<< " .version = " << s.version
				<< " .hPhysicalGpu = " << (s.hPhysicalGpu ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hPhysicalGpu)) : "null"s)
				<< " .connector = " << DumpGenerated(s.connector)
				<< " .hProxyPhysicalGpu = " << (s.hProxyPhysicalGpu ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hProxyPhysicalGpu)) : "null"s)
				<< " .isSynced = " << s.isSynced
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_DISPLAY)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_DISPLAY*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_DISPLAY {"
				<< " .version = " << s.version
				<< " .displayId = " << s.displayId
				<< " .isMasterable = " << s.isMasterable
				<< " .reserved = " << s.reserved
				<< " .syncState = " << DumpGenerated(s.syncState)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_DELAY)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_DELAY*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_DELAY {"
				<< " .version = " << s.version
				<< " .numLines = " << s.numLines
				<< " .numPixels = " << s.numPixels
				<< " .maxLines = " << s.maxLines
				<< " .minPixels = " << s.minPixels
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_CONTROL_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_CONTROL_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_CONTROL_PARAMS {"
				<< " .version = " << s.version
				<< " .polarity = " << DumpGenerated(s.polarity)
				<< " .vmode = " << DumpGenerated(s.vmode)
				<< " .interval = " << s.interval
				<< " .source = " << DumpGenerated(s.source)
				<< " .interlaceMode = " << s.interlaceMode
				<< " .syncSourceIsOutput = " << s.syncSourceIsOutput
				<< " .reserved = " << s.reserved
				<< " .syncSkew = " << DumpGenerated(s.syncSkew)
				<< " .startupDelay = " << DumpGenerated(s.startupDelay)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_STATUS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_STATUS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_STATUS {"
				<< " .version = " << s.version
				<< " .bIsSynced = " << s.bIsSynced
				<< " .bIsStereoSynced = " << s.bIsStereoSynced
				<< " .bIsSyncSignalAvailable = " << s.bIsSyncSignalAvailable
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_STATUS_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_STATUS_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_STATUS_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .refreshRate = " << s.refreshRate
				<< " .RJ45_IO = " << DumpArray_(s.RJ45_IO)
				<< " .RJ45_Ethernet = " << DumpArray_(s.RJ45_Ethernet)
				<< " .houseSyncIncoming = " << s.houseSyncIncoming
				<< " .bHouseSync = " << s.bHouseSync
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GSYNC_STATUS_PARAMS_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GSYNC_STATUS_PARAMS_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GSYNC_STATUS_PARAMS_V2 {"
				<< " .version = " << s.version
				<< " .refreshRate = " << s.refreshRate
				<< " .RJ45_IO = " << DumpArray_(s.RJ45_IO)
				<< " .RJ45_Ethernet = " << DumpArray_(s.RJ45_Ethernet)
				<< " .houseSyncIncoming = " << s.houseSyncIncoming
				<< " .bHouseSync = " << s.bHouseSync
				<< " .bInternalSlave = " << s.bInternalSlave
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GET_CURRENT_SLI_STATE_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GET_CURRENT_SLI_STATE_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GET_CURRENT_SLI_STATE_V1 {"
				<< " .version = " << s.version
				<< " .maxNumAFRGroups = " << s.maxNumAFRGroups
				<< " .numAFRGroups = " << s.numAFRGroups
				<< " .currentAFRIndex = " << s.currentAFRIndex
				<< " .nextFrameAFRIndex = " << s.nextFrameAFRIndex
				<< " .previousFrameAFRIndex = " << s.previousFrameAFRIndex
				<< " .bIsCurAFRGroupNew = " << s.bIsCurAFRGroupNew
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_GET_CURRENT_SLI_STATE_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_GET_CURRENT_SLI_STATE_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_GET_CURRENT_SLI_STATE_V2 {"
				<< " .version = " << s.version
				<< " .maxNumAFRGroups = " << s.maxNumAFRGroups
				<< " .numAFRGroups = " << s.numAFRGroups
				<< " .currentAFRIndex = " << s.currentAFRIndex
				<< " .nextFrameAFRIndex = " << s.nextFrameAFRIndex
				<< " .previousFrameAFRIndex = " << s.previousFrameAFRIndex
				<< " .bIsCurAFRGroupNew = " << s.bIsCurAFRGroupNew
				<< " .numVRSLIGpus = " << s.numVRSLIGpus
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_JOIN_PRESENT_BARRIER_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_JOIN_PRESENT_BARRIER_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_JOIN_PRESENT_BARRIER_PARAMS {"
				<< " .dwVersion = " << s.dwVersion
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_PRESENT_BARRIER_FRAME_STATISTICS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_PRESENT_BARRIER_FRAME_STATISTICS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_PRESENT_BARRIER_FRAME_STATISTICS {"
				<< " .dwVersion = " << s.dwVersion
				<< " .SyncMode = " << DumpGenerated(s.SyncMode)
				<< " .PresentCount = " << s.PresentCount
				<< " .PresentInSyncCount = " << s.PresentInSyncCount
				<< " .FlipInSyncCount = " << s.FlipInSyncCount
				<< " .RefreshCount = " << s.RefreshCount
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .bSinglePassStereoSupported = " << s.bSinglePassStereoSupported
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_QUERY_SINGLE_PASS_STEREO_SUPPORT_PARAMS_V2 {"
				<< " .version = " << s.version
				<< " .bSinglePassStereoSupported = " << s.bSinglePassStereoSupported
				<< " .bSinglePassStereoXYZWSupported = " << s.bSinglePassStereoXYZWSupported
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_QUERY_MULTIVIEW_SUPPORT_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_QUERY_MULTIVIEW_SUPPORT_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_QUERY_MULTIVIEW_SUPPORT_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .bMultiViewSupported = " << s.bMultiViewSupported
				<< " .bSinglePassStereoSupported = " << s.bSinglePassStereoSupported
				<< " .bSinglePassStereoXYZWSupported = " << s.bSinglePassStereoXYZWSupported
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MULTIVIEW_PARAMS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MULTIVIEW_PARAMS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MULTIVIEW_PARAMS_V1 {"
				<< " .version = " << s.version
				<< " .numViews = " << s.numViews
				<< " .renderTargetIndexOffset = " << DumpArray_(s.renderTargetIndexOffset)
				<< " .independentViewportMaskEnable = " << (int)s.independentViewportMaskEnable
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_QUERY_MODIFIED_W_SUPPORT_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_QUERY_MODIFIED_W_SUPPORT_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_QUERY_MODIFIED_W_SUPPORT_PARAMS {"
				<< " .version = " << s.version
				<< " .bModifiedWSupported = " << s.bModifiedWSupported
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MODIFIED_W_COEFFICIENTS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MODIFIED_W_COEFFICIENTS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MODIFIED_W_COEFFICIENTS {"
				<< " .fA = " << s.fA
				<< " .fB = " << s.fB
				<< " .fAReserved = " << s.fAReserved
				<< " .fBReserved = " << s.fBReserved
				<< " .fReserved = " << DumpArray_(s.fReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_MODIFIED_W_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_MODIFIED_W_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_MODIFIED_W_PARAMS {"
				<< " .version = " << s.version
				<< " .numEntries = " << s.numEntries
				<< " .modifiedWCoefficients = " << DumpArray_(s.modifiedWCoefficients)
				<< " .id = " << s.id
				<< " .reserved = " << DumpArray_(s.reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GET_SLEEP_STATUS_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GET_SLEEP_STATUS_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GET_SLEEP_STATUS_PARAMS {"
				<< " .version = " << s.version
				<< " .bLowLatencyMode = " << (int)s.bLowLatencyMode
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_SET_SLEEP_MODE_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_SET_SLEEP_MODE_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_SET_SLEEP_MODE_PARAMS {"
				<< " .version = " << s.version
				<< " .bLowLatencyMode = " << (int)s.bLowLatencyMode
				<< " .bLowLatencyBoost = " << (int)s.bLowLatencyBoost
				<< " .minimumIntervalUs = " << s.minimumIntervalUs
				<< " .bUseMarkersToOptimize = " << (int)s.bUseMarkersToOptimize
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LATENCY_RESULT_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LATENCY_RESULT_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LATENCY_RESULT_PARAMS {"
				<< " .version = " << s.version
				<< " .frameReport = " << DumpArray_(s.frameReport)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_LATENCY_MARKER_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_LATENCY_MARKER_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_LATENCY_MARKER_PARAMS {"
				<< " .version = " << s.version
				<< " .frameID = " << s.frameID
				<< " .markerType = " << DumpGenerated(s.markerType)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOCAPS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOCAPS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOCAPS {"
				<< " .version = " << s.version
				<< " .adapterName = " << s.adapterName
				<< " .adapterClass = " << s.adapterClass
				<< " .adapterCaps = " << s.adapterCaps
				<< " .dipSwitch = " << s.dipSwitch
				<< " .dipSwitchReserved = " << s.dipSwitchReserved
				<< " .boardID = " << s.boardID
				<< " .driver = " << DumpGenerated(s.driver)
				<< " .firmWare = " << DumpGenerated(s.firmWare)
				<< " .ownerId = " << s.ownerId
				<< " .ownerType = " << DumpGenerated(s.ownerType)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOCHANNELSTATUS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOCHANNELSTATUS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOCHANNELSTATUS {"
				<< " .smpte352 = " << s.smpte352
				<< " .signalFormat = " << DumpGenerated(s.signalFormat)
				<< " .bitsPerComponent = " << DumpGenerated(s.bitsPerComponent)
				<< " .samplingFormat = " << DumpGenerated(s.samplingFormat)
				<< " .colorSpace = " << DumpGenerated(s.colorSpace)
				<< " .linkID = " << DumpGenerated(s.linkID)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOINPUTSTATUS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOINPUTSTATUS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOINPUTSTATUS {"
				<< " .vidIn = " << DumpArray_(s.vidIn)
				<< " .captureStatus = " << DumpGenerated(s.captureStatus)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOOUTPUTSTATUS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOOUTPUTSTATUS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOOUTPUTSTATUS {"
				<< " .vid1Out = " << DumpGenerated(s.vid1Out)
				<< " .vid2Out = " << DumpGenerated(s.vid2Out)
				<< " .sdiSyncIn = " << DumpGenerated(s.sdiSyncIn)
				<< " .compSyncIn = " << DumpGenerated(s.compSyncIn)
				<< " .syncEnable = " << s.syncEnable
				<< " .syncSource = " << DumpGenerated(s.syncSource)
				<< " .syncFormat = " << DumpGenerated(s.syncFormat)
				<< " .frameLockEnable = " << s.frameLockEnable
				<< " .outputVideoLocked = " << s.outputVideoLocked
				<< " .dataIntegrityCheckErrorCount = " << s.dataIntegrityCheckErrorCount
				<< " .dataIntegrityCheckEnabled = " << s.dataIntegrityCheckEnabled
				<< " .dataIntegrityCheckFailed = " << s.dataIntegrityCheckFailed
				<< " .uSyncSourceLocked = " << s.uSyncSourceLocked
				<< " .uPowerOn = " << s.uPowerOn
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOSTATUS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOSTATUS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOSTATUS {"
				<< " .version = " << s.version
				<< " .nvvioStatusType = " << DumpGenerated(s.nvvioStatusType)
				<< " .vioStatus = " << DumpGenerated(s.vioStatus)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOOUTPUTREGION)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOOUTPUTREGION*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOOUTPUTREGION {"
				<< " .x = " << s.x
				<< " .y = " << s.y
				<< " .width = " << s.width
				<< " .height = " << s.height
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOGAMMARAMP8)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOGAMMARAMP8*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOGAMMARAMP8 {"
				<< " .uRed = " << DumpArray_(s.uRed)
				<< " .uGreen = " << DumpArray_(s.uGreen)
				<< " .uBlue = " << DumpArray_(s.uBlue)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOGAMMARAMP10)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOGAMMARAMP10*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOGAMMARAMP10 {"
				<< " .uRed = " << DumpArray_(s.uRed)
				<< " .uGreen = " << DumpArray_(s.uGreen)
				<< " .uBlue = " << DumpArray_(s.uBlue)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOSYNCDELAY)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOSYNCDELAY*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOSYNCDELAY {"
				<< " .version = " << s.version
				<< " .horizontalDelay = " << s.horizontalDelay
				<< " .verticalDelay = " << s.verticalDelay
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOVIDEOMODE)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOVIDEOMODE*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOVIDEOMODE {"
				<< " .horizontalPixels = " << s.horizontalPixels
				<< " .verticalLines = " << s.verticalLines
				<< " .fFrameRate = " << s.fFrameRate
				<< " .interlaceMode = " << DumpGenerated(s.interlaceMode)
				<< " .videoStandard = " << DumpGenerated(s.videoStandard)
				<< " .videoType = " << DumpGenerated(s.videoType)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOSIGNALFORMATDETAIL)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOSIGNALFORMATDETAIL*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOSIGNALFORMATDETAIL {"
				<< " .signalFormat = " << DumpGenerated(s.signalFormat)
				<< " .videoMode = " << DumpGenerated(s.videoMode)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIODATAFORMATDETAIL)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIODATAFORMATDETAIL*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIODATAFORMATDETAIL {"
				<< " .dataFormat = " << DumpGenerated(s.dataFormat)
				<< " .vioCaps = " << s.vioCaps
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOCOLORCONVERSION)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOCOLORCONVERSION*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOCOLORCONVERSION {"
				<< " .version = " << s.version
				<< " .colorMatrix = " << DumpArray_(s.colorMatrix)
				<< " .colorOffset = " << DumpArray_(s.colorOffset)
				<< " .colorScale = " << DumpArray_(s.colorScale)
				<< " .compositeSafe = " << s.compositeSafe
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOGAMMACORRECTION)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOGAMMACORRECTION*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOGAMMACORRECTION {"
				<< " .version = " << s.version
				<< " .vioGammaCorrectionType = " << s.vioGammaCorrectionType
				<< " .gammaRamp = " << DumpGenerated(s.gammaRamp)
				<< " .fGammaValueR = " << s.fGammaValueR
				<< " .fGammaValueG = " << s.fGammaValueG
				<< " .fGammaValueB = " << s.fGammaValueB
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOCOMPOSITERANGE)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOCOMPOSITERANGE*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOCOMPOSITERANGE {"
				<< " .uRange = " << s.uRange
				<< " .uEnabled = " << s.uEnabled
				<< " .uMin = " << s.uMin
				<< " .uMax = " << s.uMax
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOOUTPUTCONFIG_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOOUTPUTCONFIG_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOOUTPUTCONFIG_V1 {"
				<< " .signalFormat = " << DumpGenerated(s.signalFormat)
				<< " .dataFormat = " << DumpGenerated(s.dataFormat)
				<< " .outputRegion = " << DumpGenerated(s.outputRegion)
				<< " .outputArea = " << DumpGenerated(s.outputArea)
				<< " .colorConversion = " << DumpGenerated(s.colorConversion)
				<< " .gammaCorrection = " << DumpGenerated(s.gammaCorrection)
				<< " .syncEnable = " << s.syncEnable
				<< " .syncSource = " << DumpGenerated(s.syncSource)
				<< " .syncDelay = " << DumpGenerated(s.syncDelay)
				<< " .compositeSyncType = " << DumpGenerated(s.compositeSyncType)
				<< " .frameLockEnable = " << s.frameLockEnable
				<< " .psfSignalFormat = " << s.psfSignalFormat
				<< " .enable422Filter = " << s.enable422Filter
				<< " .compositeTerminate = " << s.compositeTerminate
				<< " .enableDataIntegrityCheck = " << s.enableDataIntegrityCheck
				<< " .cscOverride = " << s.cscOverride
				<< " .flipQueueLength = " << s.flipQueueLength
				<< " .enableANCTimeCodeGeneration = " << s.enableANCTimeCodeGeneration
				<< " .enableComposite = " << s.enableComposite
				<< " .enableAlphaKeyComposite = " << s.enableAlphaKeyComposite
				<< " .compRange = " << DumpGenerated(s.compRange)
				<< " .reservedData = " << DumpArray_(s.reservedData)
				<< " .enableFullColorRange = " << s.enableFullColorRange
				<< " .enableRGBData = " << s.enableRGBData
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOOUTPUTCONFIG_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOOUTPUTCONFIG_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOOUTPUTCONFIG_V2 {"
				<< " .signalFormat = " << DumpGenerated(s.signalFormat)
				<< " .dataFormat = " << DumpGenerated(s.dataFormat)
				<< " .outputRegion = " << DumpGenerated(s.outputRegion)
				<< " .outputArea = " << DumpGenerated(s.outputArea)
				<< " .colorConversion = " << DumpGenerated(s.colorConversion)
				<< " .gammaCorrection = " << DumpGenerated(s.gammaCorrection)
				<< " .syncEnable = " << s.syncEnable
				<< " .syncSource = " << DumpGenerated(s.syncSource)
				<< " .syncDelay = " << DumpGenerated(s.syncDelay)
				<< " .compositeSyncType = " << DumpGenerated(s.compositeSyncType)
				<< " .frameLockEnable = " << s.frameLockEnable
				<< " .psfSignalFormat = " << s.psfSignalFormat
				<< " .enable422Filter = " << s.enable422Filter
				<< " .compositeTerminate = " << s.compositeTerminate
				<< " .enableDataIntegrityCheck = " << s.enableDataIntegrityCheck
				<< " .cscOverride = " << s.cscOverride
				<< " .flipQueueLength = " << s.flipQueueLength
				<< " .enableANCTimeCodeGeneration = " << s.enableANCTimeCodeGeneration
				<< " .enableComposite = " << s.enableComposite
				<< " .enableAlphaKeyComposite = " << s.enableAlphaKeyComposite
				<< " .compRange = " << DumpGenerated(s.compRange)
				<< " .reservedData = " << DumpArray_(s.reservedData)
				<< " .enableFullColorRange = " << s.enableFullColorRange
				<< " .enableRGBData = " << s.enableRGBData
				<< " .ancParityComputation = " << DumpGenerated(s.ancParityComputation)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOOUTPUTCONFIG_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOOUTPUTCONFIG_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOOUTPUTCONFIG_V3 {"
				<< " .signalFormat = " << DumpGenerated(s.signalFormat)
				<< " .dataFormat = " << DumpGenerated(s.dataFormat)
				<< " .outputRegion = " << DumpGenerated(s.outputRegion)
				<< " .outputArea = " << DumpGenerated(s.outputArea)
				<< " .colorConversion = " << DumpGenerated(s.colorConversion)
				<< " .gammaCorrection = " << DumpGenerated(s.gammaCorrection)
				<< " .syncEnable = " << s.syncEnable
				<< " .syncSource = " << DumpGenerated(s.syncSource)
				<< " .syncDelay = " << DumpGenerated(s.syncDelay)
				<< " .compositeSyncType = " << DumpGenerated(s.compositeSyncType)
				<< " .frameLockEnable = " << s.frameLockEnable
				<< " .psfSignalFormat = " << s.psfSignalFormat
				<< " .enable422Filter = " << s.enable422Filter
				<< " .compositeTerminate = " << s.compositeTerminate
				<< " .enableDataIntegrityCheck = " << s.enableDataIntegrityCheck
				<< " .cscOverride = " << s.cscOverride
				<< " .flipQueueLength = " << s.flipQueueLength
				<< " .enableANCTimeCodeGeneration = " << s.enableANCTimeCodeGeneration
				<< " .enableComposite = " << s.enableComposite
				<< " .enableAlphaKeyComposite = " << s.enableAlphaKeyComposite
				<< " .compRange = " << DumpGenerated(s.compRange)
				<< " .reservedData = " << DumpArray_(s.reservedData)
				<< " .enableFullColorRange = " << s.enableFullColorRange
				<< " .enableRGBData = " << s.enableRGBData
				<< " .ancParityComputation = " << DumpGenerated(s.ancParityComputation)
				<< " .enableAudioBlanking = " << s.enableAudioBlanking
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOSTREAM)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOSTREAM*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOSTREAM {"
				<< " .bitsPerComponent = " << s.bitsPerComponent
				<< " .sampling = " << DumpGenerated(s.sampling)
				<< " .expansionEnable = " << s.expansionEnable
				<< " .numLinks = " << s.numLinks
				<< " .links = " << DumpArray_(s.links)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOINPUTCONFIG)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOINPUTCONFIG*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOINPUTCONFIG {"
				<< " .numRawCaptureImages = " << s.numRawCaptureImages
				<< " .signalFormat = " << DumpGenerated(s.signalFormat)
				<< " .numStreams = " << s.numStreams
				<< " .streams = " << DumpArray_(s.streams)
				<< " .bTestMode = " << s.bTestMode
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOCONFIG_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOCONFIG_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOCONFIG_V1 {"
				<< " .version = " << s.version
				<< " .fields = " << s.fields
				<< " .nvvioConfigType = " << DumpGenerated(s.nvvioConfigType)
				<< " .vioConfig = " << DumpGenerated(s.vioConfig)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOCONFIG_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOCONFIG_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOCONFIG_V2 {"
				<< " .version = " << s.version
				<< " .fields = " << s.fields
				<< " .nvvioConfigType = " << DumpGenerated(s.nvvioConfigType)
				<< " .vioConfig = " << DumpGenerated(s.vioConfig)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOCONFIG_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOCONFIG_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOCONFIG_V3 {"
				<< " .version = " << s.version
				<< " .fields = " << s.fields
				<< " .nvvioConfigType = " << DumpGenerated(s.nvvioConfigType)
				<< " .vioConfig = " << DumpGenerated(s.vioConfig)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NVVIOTOPOLOGYTARGET)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NVVIOTOPOLOGYTARGET*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NVVIOTOPOLOGYTARGET {"
				<< " .hPhysicalGpu = " << (s.hPhysicalGpu ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hPhysicalGpu)) : "null"s)
				<< " .hVioHandle = " << (s.hVioHandle ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.hVioHandle)) : "null"s)
				<< " .vioId = " << s.vioId
				<< " .outputId = " << s.outputId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_VIO_TOPOLOGY)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_VIO_TOPOLOGY*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_VIO_TOPOLOGY {"
				<< " .version = " << s.version
				<< " .vioTotalDeviceCount = " << s.vioTotalDeviceCount
				<< " .vioTarget = " << DumpArray_(s.vioTarget)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVVIOPCIINFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVVIOPCIINFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVVIOPCIINFO {"
				<< " .version = " << s.version
				<< " .pciDeviceId = " << s.pciDeviceId
				<< " .pciSubSystemId = " << s.pciSubSystemId
				<< " .pciRevisionId = " << s.pciRevisionId
				<< " .pciDomain = " << s.pciDomain
				<< " .pciBus = " << s.pciBus
				<< " .pciSlot = " << s.pciSlot
				<< " .pciLinkWidth = " << DumpGenerated(s.pciLinkWidth)
				<< " .pciLinkRate = " << DumpGenerated(s.pciLinkRate)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVAPI_STEREO_CAPS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVAPI_STEREO_CAPS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVAPI_STEREO_CAPS {"
				<< " .version = " << s.version
				<< " .supportsWindowedModeOff = " << s.supportsWindowedModeOff
				<< " .supportsWindowedModeAutomatic = " << s.supportsWindowedModeAutomatic
				<< " .supportsWindowedModePersistent = " << s.supportsWindowedModePersistent
				<< " .reserved = " << s.reserved
				<< " .reserved2 = " << DumpArray_(s.reserved2)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvDRSSessionHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvDRSSessionHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvDRSSessionHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NvDRSProfileHandle__)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NvDRSProfileHandle__*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NvDRSProfileHandle__ {"
				<< " .unused = " << s.unused
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_GPU_SUPPORT)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_GPU_SUPPORT*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_GPU_SUPPORT {"
				<< " .geforce = " << s.geforce
				<< " .quadro = " << s.quadro
				<< " .nvs = " << s.nvs
				<< " .reserved4 = " << s.reserved4
				<< " .reserved5 = " << s.reserved5
				<< " .reserved6 = " << s.reserved6
				<< " .reserved7 = " << s.reserved7
				<< " .reserved8 = " << s.reserved8
				<< " .reserved9 = " << s.reserved9
				<< " .reserved10 = " << s.reserved10
				<< " .reserved11 = " << s.reserved11
				<< " .reserved12 = " << s.reserved12
				<< " .reserved13 = " << s.reserved13
				<< " .reserved14 = " << s.reserved14
				<< " .reserved15 = " << s.reserved15
				<< " .reserved16 = " << s.reserved16
				<< " .reserved17 = " << s.reserved17
				<< " .reserved18 = " << s.reserved18
				<< " .reserved19 = " << s.reserved19
				<< " .reserved20 = " << s.reserved20
				<< " .reserved21 = " << s.reserved21
				<< " .reserved22 = " << s.reserved22
				<< " .reserved23 = " << s.reserved23
				<< " .reserved24 = " << s.reserved24
				<< " .reserved25 = " << s.reserved25
				<< " .reserved26 = " << s.reserved26
				<< " .reserved27 = " << s.reserved27
				<< " .reserved28 = " << s.reserved28
				<< " .reserved29 = " << s.reserved29
				<< " .reserved30 = " << s.reserved30
				<< " .reserved31 = " << s.reserved31
				<< " .reserved32 = " << s.reserved32
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_BINARY_SETTING)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_BINARY_SETTING*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_BINARY_SETTING {"
				<< " .valueLength = " << s.valueLength
				<< " .valueData = " << DumpArray_(s.valueData)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_SETTING_VALUES)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_SETTING_VALUES*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_SETTING_VALUES {"
				<< " .version = " << s.version
				<< " .numSettingValues = " << s.numSettingValues
				<< " .settingType = " << DumpGenerated(s.settingType)
				<< " .settingValues = " << DumpArray_(s.settingValues)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_SETTING_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_SETTING_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_SETTING_V1 {"
				<< " .version = " << s.version
				<< " .settingName = " << DumpArray_(s.settingName)
				<< " .settingId = " << s.settingId
				<< " .settingType = " << DumpGenerated(s.settingType)
				<< " .settingLocation = " << DumpGenerated(s.settingLocation)
				<< " .isCurrentPredefined = " << s.isCurrentPredefined
				<< " .isPredefinedValid = " << s.isPredefinedValid
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_APPLICATION_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_APPLICATION_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_APPLICATION_V1 {"
				<< " .version = " << s.version
				<< " .isPredefined = " << s.isPredefined
				<< " .appName = " << DumpArray_(s.appName)
				<< " .userFriendlyName = " << DumpArray_(s.userFriendlyName)
				<< " .launcher = " << DumpArray_(s.launcher)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_APPLICATION_V2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_APPLICATION_V2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_APPLICATION_V2 {"
				<< " .version = " << s.version
				<< " .isPredefined = " << s.isPredefined
				<< " .appName = " << DumpArray_(s.appName)
				<< " .userFriendlyName = " << DumpArray_(s.userFriendlyName)
				<< " .launcher = " << DumpArray_(s.launcher)
				<< " .fileInFolder = " << DumpArray_(s.fileInFolder)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_APPLICATION_V3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_APPLICATION_V3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_APPLICATION_V3 {"
				<< " .version = " << s.version
				<< " .isPredefined = " << s.isPredefined
				<< " .appName = " << DumpArray_(s.appName)
				<< " .userFriendlyName = " << DumpArray_(s.userFriendlyName)
				<< " .launcher = " << DumpArray_(s.launcher)
				<< " .fileInFolder = " << DumpArray_(s.fileInFolder)
				<< " .isMetro = " << s.isMetro
				<< " .isCommandLine = " << s.isCommandLine
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_APPLICATION_V4)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_APPLICATION_V4*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_APPLICATION_V4 {"
				<< " .version = " << s.version
				<< " .isPredefined = " << s.isPredefined
				<< " .appName = " << DumpArray_(s.appName)
				<< " .userFriendlyName = " << DumpArray_(s.userFriendlyName)
				<< " .launcher = " << DumpArray_(s.launcher)
				<< " .fileInFolder = " << DumpArray_(s.fileInFolder)
				<< " .isMetro = " << s.isMetro
				<< " .isCommandLine = " << s.isCommandLine
				<< " .reserved = " << s.reserved
				<< " .commandLine = " << DumpArray_(s.commandLine)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NVDRS_PROFILE_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NVDRS_PROFILE_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NVDRS_PROFILE_V1 {"
				<< " .version = " << s.version
				<< " .profileName = " << DumpArray_(s.profileName)
				<< " .gpuSupport = " << DumpGenerated(s.gpuSupport)
				<< " .isPredefined = " << s.isPredefined
				<< " .numOfApps = " << s.numOfApps
				<< " .numOfSettings = " << s.numOfSettings
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_CHIPSET_INFO_v4)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_CHIPSET_INFO_v4*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_CHIPSET_INFO_v4 {"
				<< " .version = " << s.version
				<< " .vendorId = " << s.vendorId
				<< " .deviceId = " << s.deviceId
				<< " .szVendorName = " << s.szVendorName
				<< " .szChipsetName = " << s.szChipsetName
				<< " .flags = " << s.flags
				<< " .subSysVendorId = " << s.subSysVendorId
				<< " .subSysDeviceId = " << s.subSysDeviceId
				<< " .szSubSysVendorName = " << s.szSubSysVendorName
				<< " .HBvendorId = " << s.HBvendorId
				<< " .HBdeviceId = " << s.HBdeviceId
				<< " .HBsubSysVendorId = " << s.HBsubSysVendorId
				<< " .HBsubSysDeviceId = " << s.HBsubSysDeviceId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_CHIPSET_INFO_v3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_CHIPSET_INFO_v3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_CHIPSET_INFO_v3 {"
				<< " .version = " << s.version
				<< " .vendorId = " << s.vendorId
				<< " .deviceId = " << s.deviceId
				<< " .szVendorName = " << s.szVendorName
				<< " .szChipsetName = " << s.szChipsetName
				<< " .flags = " << s.flags
				<< " .subSysVendorId = " << s.subSysVendorId
				<< " .subSysDeviceId = " << s.subSysDeviceId
				<< " .szSubSysVendorName = " << s.szSubSysVendorName
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_CHIPSET_INFO_v2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_CHIPSET_INFO_v2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_CHIPSET_INFO_v2 {"
				<< " .version = " << s.version
				<< " .vendorId = " << s.vendorId
				<< " .deviceId = " << s.deviceId
				<< " .szVendorName = " << s.szVendorName
				<< " .szChipsetName = " << s.szChipsetName
				<< " .flags = " << s.flags
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_CHIPSET_INFO_v1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_CHIPSET_INFO_v1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_CHIPSET_INFO_v1 {"
				<< " .version = " << s.version
				<< " .vendorId = " << s.vendorId
				<< " .deviceId = " << s.deviceId
				<< " .szVendorName = " << s.szVendorName
				<< " .szChipsetName = " << s.szChipsetName
				<< " }";
			return oss.str();
		};
		dumpers[typeid(NV_LID_DOCK_PARAMS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const NV_LID_DOCK_PARAMS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct NV_LID_DOCK_PARAMS {"
				<< " .version = " << s.version
				<< " .currentLidState = " << s.currentLidState
				<< " .currentDockState = " << s.currentDockState
				<< " .currentLidPolicy = " << s.currentLidPolicy
				<< " .currentDockPolicy = " << s.currentDockPolicy
				<< " .forcedLidMechanismPresent = " << s.forcedLidMechanismPresent
				<< " .forcedDockMechanismPresent = " << s.forcedDockMechanismPresent
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_DISPLAY_DRIVER_INFO)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_DISPLAY_DRIVER_INFO*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_DISPLAY_DRIVER_INFO {"
				<< " .version = " << s.version
				<< " .driverVersion = " << s.driverVersion
				<< " .szBuildBranch = " << s.szBuildBranch
				<< " .bIsDCHDriver = " << s.bIsDCHDriver
				<< " .bIsNVIDIAStudioPackage = " << s.bIsNVIDIAStudioPackage
				<< " .bIsNVIDIAGameReadyPackage = " << s.bIsNVIDIAGameReadyPackage
				<< " .bIsNVIDIARTXProductionBranchPackage = " << s.bIsNVIDIARTXProductionBranchPackage
				<< " .bIsNVIDIARTXNewFeatureBranchPackage = " << s.bIsNVIDIARTXNewFeatureBranchPackage
				<< " .reserved = " << s.reserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_CLIENT_CALLBACK_SETTINGS_SUPER_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_CLIENT_CALLBACK_SETTINGS_SUPER_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_CLIENT_CALLBACK_SETTINGS_SUPER_V1 {"
				<< " .pCallbackParam = " << (s.pCallbackParam ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pCallbackParam)) : "null"s)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_PERIODIC_CALLBACK_SETTINGS_SUPER_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_PERIODIC_CALLBACK_SETTINGS_SUPER_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_PERIODIC_CALLBACK_SETTINGS_SUPER_V1 {"
				<< " .super = " << DumpGenerated(s.super)
				<< " .callbackPeriodms = " << s.callbackPeriodms
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_CALLBACK_DATA_SUPER_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_CALLBACK_DATA_SUPER_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_CALLBACK_DATA_SUPER_V1 {"
				<< " .pCallbackParam = " << (s.pCallbackParam ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pCallbackParam)) : "null"s)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_UTILIZATION_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_UTILIZATION_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_UTILIZATION_DATA_V1 {"
				<< " .utilId = " << DumpGenerated(s.utilId)
				<< " .utilizationPercent = " << s.utilizationPercent
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_CALLBACK_UTILIZATION_DATA_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_CALLBACK_UTILIZATION_DATA_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_CALLBACK_UTILIZATION_DATA_V1 {"
				<< " .super = " << DumpGenerated(s.super)
				<< " .numUtils = " << s.numUtils
				<< " .timestamp = " << s.timestamp
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " .utils = " << DumpArray_(s.utils)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_NV_GPU_CLIENT_UTILIZATION_PERIODIC_CALLBACK_SETTINGS_V1)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _NV_GPU_CLIENT_UTILIZATION_PERIODIC_CALLBACK_SETTINGS_V1*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _NV_GPU_CLIENT_UTILIZATION_PERIODIC_CALLBACK_SETTINGS_V1 {"
				<< " .version = " << s.version
				<< " .super = " << DumpGenerated(s.super)
				<< " .callback = " << (s.callback ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.callback)) : "null"s)
				<< " .rsvd = " << DumpArray_(s.rsvd)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(nvmlValue_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const nvmlValue_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union nvmlValue_t {"
				<< " .dVal = " << s.dVal
				<< " .sllVal = " << s.sllVal
				<< " .uiVal = " << s.uiVal
				<< " .ulVal = " << s.ulVal
				<< " .ullVal = " << s.ullVal
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
		dumpers[typeid(nvmlUtilization_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const nvmlUtilization_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct nvmlUtilization_t {"
				<< " .gpu = " << s.gpu
				<< " .mem = " << s.mem
				<< " }";
			return oss.str();
		};
		dumpers[typeid(nvmlSample_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const nvmlSample_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct nvmlSample_t {"
				<< " .sampleValue = " << DumpGenerated(s.sampleValue)
				<< " .timeStamp = " << s.timeStamp
				<< " }";
			return oss.str();
		};
		dumpers[typeid(nvmlViolationTime_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const nvmlViolationTime_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct nvmlViolationTime_t {"
				<< " .referenceTime = " << s.referenceTime
				<< " .violationTime = " << s.violationTime
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
		dumpers[typeid(ADLAdapterCaps)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAdapterCaps*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAdapterCaps {"
				<< " .iAdapterID = " << s.iAdapterID
				<< " .iNumControllers = " << s.iNumControllers
				<< " .iNumDisplays = " << s.iNumDisplays
				<< " .iNumOverlays = " << s.iNumOverlays
				<< " .iNumOfGLSyncConnectors = " << s.iNumOfGLSyncConnectors
				<< " .iCapsMask = " << s.iCapsMask
				<< " .iCapsValue = " << s.iCapsValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMemoryInfo2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMemoryInfo2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMemoryInfo2 {"
				<< " .iMemorySize = " << s.iMemorySize
				<< " .strMemoryType = " << s.strMemoryType
				<< " .iMemoryBandwidth = " << s.iMemoryBandwidth
				<< " .iHyperMemorySize = " << s.iHyperMemorySize
				<< " .iInvisibleMemorySize = " << s.iInvisibleMemorySize
				<< " .iVisibleMemorySize = " << s.iVisibleMemorySize
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMemoryInfo3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMemoryInfo3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMemoryInfo3 {"
				<< " .iMemorySize = " << s.iMemorySize
				<< " .strMemoryType = " << s.strMemoryType
				<< " .iMemoryBandwidth = " << s.iMemoryBandwidth
				<< " .iHyperMemorySize = " << s.iHyperMemorySize
				<< " .iInvisibleMemorySize = " << s.iInvisibleMemorySize
				<< " .iVisibleMemorySize = " << s.iVisibleMemorySize
				<< " .iVramVendorRevId = " << s.iVramVendorRevId
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
		dumpers[typeid(ADLGcnInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGcnInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGcnInfo {"
				<< " .CuCount = " << s.CuCount
				<< " .TexCount = " << s.TexCount
				<< " .RopCount = " << s.RopCount
				<< " .ASICFamilyId = " << s.ASICFamilyId
				<< " .ASICRevisionId = " << s.ASICRevisionId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLVirtualSegmentSettingsOutput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLVirtualSegmentSettingsOutput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLVirtualSegmentSettingsOutput {"
				<< " .virtualSegmentSupported = " << s.virtualSegmentSupported
				<< " .virtualSegmentDefault = " << s.virtualSegmentDefault
				<< " .virtualSegmentCurrent = " << s.virtualSegmentCurrent
				<< " .iMinSizeInMB = " << s.iMinSizeInMB
				<< " .iMaxSizeInMB = " << s.iMaxSizeInMB
				<< " .icurrentSizeInMB = " << s.icurrentSizeInMB
				<< " .idefaultSizeInMB = " << s.idefaultSizeInMB
				<< " .iMask = " << s.iMask
				<< " .iValue = " << s.iValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLChipSetInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLChipSetInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLChipSetInfo {"
				<< " .iBusType = " << s.iBusType
				<< " .iBusSpeedType = " << s.iBusSpeedType
				<< " .iMaxPCIELaneWidth = " << s.iMaxPCIELaneWidth
				<< " .iCurrentPCIELaneWidth = " << s.iCurrentPCIELaneWidth
				<< " .iSupportedAGPSpeeds = " << s.iSupportedAGPSpeeds
				<< " .iCurrentAGPSpeed = " << s.iCurrentAGPSpeed
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMemoryInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMemoryInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMemoryInfo {"
				<< " .iMemorySize = " << s.iMemorySize
				<< " .strMemoryType = " << s.strMemoryType
				<< " .iMemoryBandwidth = " << s.iMemoryBandwidth
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMemoryRequired)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMemoryRequired*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMemoryRequired {"
				<< " .iMemoryReq = " << s.iMemoryReq
				<< " .iType = " << s.iType
				<< " .iDisplayFeatureValue = " << s.iDisplayFeatureValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMemoryDisplayFeatures)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMemoryDisplayFeatures*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMemoryDisplayFeatures {"
				<< " .iDisplayIndex = " << s.iDisplayIndex
				<< " .iDisplayFeatureValue = " << s.iDisplayFeatureValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDDCInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDDCInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDDCInfo {"
				<< " .ulSize = " << s.ulSize
				<< " .ulSupportsDDC = " << s.ulSupportsDDC
				<< " .ulManufacturerID = " << s.ulManufacturerID
				<< " .ulProductID = " << s.ulProductID
				<< " .cDisplayName = " << s.cDisplayName
				<< " .ulMaxHResolution = " << s.ulMaxHResolution
				<< " .ulMaxVResolution = " << s.ulMaxVResolution
				<< " .ulMaxRefresh = " << s.ulMaxRefresh
				<< " .ulPTMCx = " << s.ulPTMCx
				<< " .ulPTMCy = " << s.ulPTMCy
				<< " .ulPTMRefreshRate = " << s.ulPTMRefreshRate
				<< " .ulDDCInfoFlag = " << s.ulDDCInfoFlag
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDDCInfo2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDDCInfo2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDDCInfo2 {"
				<< " .ulSize = " << s.ulSize
				<< " .ulSupportsDDC = " << s.ulSupportsDDC
				<< " .ulManufacturerID = " << s.ulManufacturerID
				<< " .ulProductID = " << s.ulProductID
				<< " .cDisplayName = " << s.cDisplayName
				<< " .ulMaxHResolution = " << s.ulMaxHResolution
				<< " .ulMaxVResolution = " << s.ulMaxVResolution
				<< " .ulMaxRefresh = " << s.ulMaxRefresh
				<< " .ulPTMCx = " << s.ulPTMCx
				<< " .ulPTMCy = " << s.ulPTMCy
				<< " .ulPTMRefreshRate = " << s.ulPTMRefreshRate
				<< " .ulDDCInfoFlag = " << s.ulDDCInfoFlag
				<< " .bPackedPixelSupported = " << s.bPackedPixelSupported
				<< " .iPanelPixelFormat = " << s.iPanelPixelFormat
				<< " .ulSerialID = " << s.ulSerialID
				<< " .ulMinLuminanceData = " << s.ulMinLuminanceData
				<< " .ulAvgLuminanceData = " << s.ulAvgLuminanceData
				<< " .ulMaxLuminanceData = " << s.ulMaxLuminanceData
				<< " .iSupportedTransferFunction = " << s.iSupportedTransferFunction
				<< " .iSupportedColorSpace = " << s.iSupportedColorSpace
				<< " .iNativeDisplayChromaticityRedX = " << s.iNativeDisplayChromaticityRedX
				<< " .iNativeDisplayChromaticityRedY = " << s.iNativeDisplayChromaticityRedY
				<< " .iNativeDisplayChromaticityGreenX = " << s.iNativeDisplayChromaticityGreenX
				<< " .iNativeDisplayChromaticityGreenY = " << s.iNativeDisplayChromaticityGreenY
				<< " .iNativeDisplayChromaticityBlueX = " << s.iNativeDisplayChromaticityBlueX
				<< " .iNativeDisplayChromaticityBlueY = " << s.iNativeDisplayChromaticityBlueY
				<< " .iNativeDisplayChromaticityWhitePointX = " << s.iNativeDisplayChromaticityWhitePointX
				<< " .iNativeDisplayChromaticityWhitePointY = " << s.iNativeDisplayChromaticityWhitePointY
				<< " .iDiffuseScreenReflectance = " << s.iDiffuseScreenReflectance
				<< " .iSpecularScreenReflectance = " << s.iSpecularScreenReflectance
				<< " .iSupportedHDR = " << s.iSupportedHDR
				<< " .iFreesyncFlags = " << s.iFreesyncFlags
				<< " .ulMinLuminanceNoDimmingData = " << s.ulMinLuminanceNoDimmingData
				<< " .ulMaxBacklightMaxLuminanceData = " << s.ulMaxBacklightMaxLuminanceData
				<< " .ulMinBacklightMaxLuminanceData = " << s.ulMinBacklightMaxLuminanceData
				<< " .ulMaxBacklightMinLuminanceData = " << s.ulMaxBacklightMinLuminanceData
				<< " .ulMinBacklightMinLuminanceData = " << s.ulMinBacklightMinLuminanceData
				<< " .iReserved = " << DumpArray_(s.iReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGamma)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGamma*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGamma {"
				<< " .fRed = " << s.fRed
				<< " .fGreen = " << s.fGreen
				<< " .fBlue = " << s.fBlue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLCustomMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLCustomMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLCustomMode {"
				<< " .iFlags = " << s.iFlags
				<< " .iModeWidth = " << s.iModeWidth
				<< " .iModeHeight = " << s.iModeHeight
				<< " .iBaseModeWidth = " << s.iBaseModeWidth
				<< " .iBaseModeHeight = " << s.iBaseModeHeight
				<< " .iRefreshRate = " << s.iRefreshRate
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGetClocksOUT)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGetClocksOUT*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGetClocksOUT {"
				<< " .ulHighCoreClock = " << s.ulHighCoreClock
				<< " .ulHighMemoryClock = " << s.ulHighMemoryClock
				<< " .ulHighVddc = " << s.ulHighVddc
				<< " .ulCoreMin = " << s.ulCoreMin
				<< " .ulCoreMax = " << s.ulCoreMax
				<< " .ulMemoryMin = " << s.ulMemoryMin
				<< " .ulMemoryMax = " << s.ulMemoryMax
				<< " .ulActivityPercent = " << s.ulActivityPercent
				<< " .ulCurrentCoreClock = " << s.ulCurrentCoreClock
				<< " .ulCurrentMemoryClock = " << s.ulCurrentMemoryClock
				<< " .ulReserved = " << s.ulReserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayConfig)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayConfig*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayConfig {"
				<< " .ulSize = " << s.ulSize
				<< " .ulConnectorType = " << s.ulConnectorType
				<< " .ulDeviceData = " << s.ulDeviceData
				<< " .ulOverridedDeviceData = " << s.ulOverridedDeviceData
				<< " .ulReserved = " << s.ulReserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayID)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayID*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayID {"
				<< " .iDisplayLogicalIndex = " << s.iDisplayLogicalIndex
				<< " .iDisplayPhysicalIndex = " << s.iDisplayPhysicalIndex
				<< " .iDisplayLogicalAdapterIndex = " << s.iDisplayLogicalAdapterIndex
				<< " .iDisplayPhysicalAdapterIndex = " << s.iDisplayPhysicalAdapterIndex
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayInfo {"
				<< " .displayID = " << DumpGenerated(s.displayID)
				<< " .iDisplayControllerIndex = " << s.iDisplayControllerIndex
				<< " .strDisplayName = " << s.strDisplayName
				<< " .strDisplayManufacturerName = " << s.strDisplayManufacturerName
				<< " .iDisplayType = " << s.iDisplayType
				<< " .iDisplayOutputType = " << s.iDisplayOutputType
				<< " .iDisplayConnector = " << s.iDisplayConnector
				<< " .iDisplayInfoMask = " << s.iDisplayInfoMask
				<< " .iDisplayInfoValue = " << s.iDisplayInfoValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayDPMSTInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayDPMSTInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayDPMSTInfo {"
				<< " .displayID = " << DumpGenerated(s.displayID)
				<< " .iTotalAvailableBandwidthInMpbs = " << s.iTotalAvailableBandwidthInMpbs
				<< " .iAllocatedBandwidthInMbps = " << s.iAllocatedBandwidthInMbps
				<< " .strGlobalUniqueIdentifier = " << s.strGlobalUniqueIdentifier
				<< " .radLinkCount = " << s.radLinkCount
				<< " .iPhysicalConnectorID = " << s.iPhysicalConnectorID
				<< " .rad = " << s.rad
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayMode {"
				<< " .iPelsHeight = " << s.iPelsHeight
				<< " .iPelsWidth = " << s.iPelsWidth
				<< " .iBitsPerPel = " << s.iBitsPerPel
				<< " .iDisplayFrequency = " << s.iDisplayFrequency
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDetailedTiming)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDetailedTiming*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDetailedTiming {"
				<< " .iSize = " << s.iSize
				<< " .sTimingFlags = " << s.sTimingFlags
				<< " .sHTotal = " << s.sHTotal
				<< " .sHDisplay = " << s.sHDisplay
				<< " .sHSyncStart = " << s.sHSyncStart
				<< " .sHSyncWidth = " << s.sHSyncWidth
				<< " .sVTotal = " << s.sVTotal
				<< " .sVDisplay = " << s.sVDisplay
				<< " .sVSyncStart = " << s.sVSyncStart
				<< " .sVSyncWidth = " << s.sVSyncWidth
				<< " .sPixelClock = " << s.sPixelClock
				<< " .sHOverscanRight = " << s.sHOverscanRight
				<< " .sHOverscanLeft = " << s.sHOverscanLeft
				<< " .sVOverscanBottom = " << s.sVOverscanBottom
				<< " .sVOverscanTop = " << s.sVOverscanTop
				<< " .sOverscan8B = " << s.sOverscan8B
				<< " .sOverscanGR = " << s.sOverscanGR
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayModeInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayModeInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayModeInfo {"
				<< " .iTimingStandard = " << s.iTimingStandard
				<< " .iPossibleStandard = " << s.iPossibleStandard
				<< " .iRefreshRate = " << s.iRefreshRate
				<< " .iPelsWidth = " << s.iPelsWidth
				<< " .iPelsHeight = " << s.iPelsHeight
				<< " .sDetailedTiming = " << DumpGenerated(s.sDetailedTiming)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayProperty)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayProperty*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayProperty {"
				<< " .iSize = " << s.iSize
				<< " .iPropertyType = " << s.iPropertyType
				<< " .iExpansionMode = " << s.iExpansionMode
				<< " .iSupport = " << s.iSupport
				<< " .iCurrent = " << s.iCurrent
				<< " .iDefault = " << s.iDefault
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLClockInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLClockInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLClockInfo {"
				<< " .iCoreClock = " << s.iCoreClock
				<< " .iMemoryClock = " << s.iMemoryClock
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLI2C)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLI2C*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLI2C {"
				<< " .iSize = " << s.iSize
				<< " .iLine = " << s.iLine
				<< " .iAddress = " << s.iAddress
				<< " .iOffset = " << s.iOffset
				<< " .iAction = " << s.iAction
				<< " .iSpeed = " << s.iSpeed
				<< " .iDataSize = " << s.iDataSize
				<< " .pcData = " << (s.pcData ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pcData)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayEDIDData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayEDIDData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayEDIDData {"
				<< " .iSize = " << s.iSize
				<< " .iFlag = " << s.iFlag
				<< " .iEDIDSize = " << s.iEDIDSize
				<< " .iBlockIndex = " << s.iBlockIndex
				<< " .cEDIDData = " << s.cEDIDData
				<< " .iReserved = " << DumpArray_(s.iReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLControllerOverlayInput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLControllerOverlayInput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLControllerOverlayInput {"
				<< " .iSize = " << s.iSize
				<< " .iOverlayAdjust = " << s.iOverlayAdjust
				<< " .iValue = " << s.iValue
				<< " .iReserved = " << s.iReserved
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAdjustmentinfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAdjustmentinfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAdjustmentinfo {"
				<< " .iDefault = " << s.iDefault
				<< " .iMin = " << s.iMin
				<< " .iMax = " << s.iMax
				<< " .iStep = " << s.iStep
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLControllerOverlayInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLControllerOverlayInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLControllerOverlayInfo {"
				<< " .iSize = " << s.iSize
				<< " .sOverlayInfo = " << DumpGenerated(s.sOverlayInfo)
				<< " .iReserved = " << DumpArray_(s.iReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGLSyncModuleID)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGLSyncModuleID*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGLSyncModuleID {"
				<< " .iModuleID = " << s.iModuleID
				<< " .iGlSyncGPUPort = " << s.iGlSyncGPUPort
				<< " .iFWBootSectorVersion = " << s.iFWBootSectorVersion
				<< " .iFWUserSectorVersion = " << s.iFWUserSectorVersion
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGLSyncPortCaps)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGLSyncPortCaps*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGLSyncPortCaps {"
				<< " .iPortType = " << s.iPortType
				<< " .iNumOfLEDs = " << s.iNumOfLEDs
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGLSyncGenlockConfig)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGLSyncGenlockConfig*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGLSyncGenlockConfig {"
				<< " .iValidMask = " << s.iValidMask
				<< " .iSyncDelay = " << s.iSyncDelay
				<< " .iFramelockCntlVector = " << s.iFramelockCntlVector
				<< " .iSignalSource = " << s.iSignalSource
				<< " .iSampleRate = " << s.iSampleRate
				<< " .iSyncField = " << s.iSyncField
				<< " .iTriggerEdge = " << s.iTriggerEdge
				<< " .iScanRateCoeff = " << s.iScanRateCoeff
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGlSyncPortInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGlSyncPortInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGlSyncPortInfo {"
				<< " .iPortType = " << s.iPortType
				<< " .iNumOfLEDs = " << s.iNumOfLEDs
				<< " .iPortState = " << s.iPortState
				<< " .iFrequency = " << s.iFrequency
				<< " .iSignalType = " << s.iSignalType
				<< " .iSignalSource = " << s.iSignalSource
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGlSyncPortControl)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGlSyncPortControl*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGlSyncPortControl {"
				<< " .iPortType = " << s.iPortType
				<< " .iControlVector = " << s.iControlVector
				<< " .iSignalSource = " << s.iSignalSource
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGlSyncMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGlSyncMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGlSyncMode {"
				<< " .iControlVector = " << s.iControlVector
				<< " .iStatusVector = " << s.iStatusVector
				<< " .iGLSyncConnectorIndex = " << s.iGLSyncConnectorIndex
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGlSyncMode2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGlSyncMode2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGlSyncMode2 {"
				<< " .iControlVector = " << s.iControlVector
				<< " .iStatusVector = " << s.iStatusVector
				<< " .iGLSyncConnectorIndex = " << s.iGLSyncConnectorIndex
				<< " .iDisplayIndex = " << s.iDisplayIndex
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLInfoPacket)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLInfoPacket*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLInfoPacket {"
				<< " .hb0 = " << (int)s.hb0
				<< " .hb1 = " << (int)s.hb1
				<< " .hb2 = " << (int)s.hb2
				<< " .sb = " << s.sb
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAVIInfoPacket)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAVIInfoPacket*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAVIInfoPacket {"
				<< " .bPB3_ITC = " << (int)s.bPB3_ITC
				<< " .bPB5 = " << (int)s.bPB5
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODClockSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODClockSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODClockSetting {"
				<< " .iDefaultClock = " << s.iDefaultClock
				<< " .iCurrentClock = " << s.iCurrentClock
				<< " .iMaxClock = " << s.iMaxClock
				<< " .iMinClock = " << s.iMinClock
				<< " .iRequestedClock = " << s.iRequestedClock
				<< " .iStepClock = " << s.iStepClock
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAdapterODClockInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAdapterODClockInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAdapterODClockInfo {"
				<< " .iSize = " << s.iSize
				<< " .iFlags = " << s.iFlags
				<< " .sMemoryClock = " << DumpGenerated(s.sMemoryClock)
				<< " .sEngineClock = " << DumpGenerated(s.sEngineClock)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAdapterODClockConfig)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAdapterODClockConfig*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAdapterODClockConfig {"
				<< " .iSize = " << s.iSize
				<< " .iFlags = " << s.iFlags
				<< " .iMemoryClock = " << s.iMemoryClock
				<< " .iEngineClock = " << s.iEngineClock
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
		dumpers[typeid(ADLODParameterRange)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODParameterRange*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODParameterRange {"
				<< " .iMin = " << s.iMin
				<< " .iMax = " << s.iMax
				<< " .iStep = " << s.iStep
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODParameters)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODParameters*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODParameters {"
				<< " .iSize = " << s.iSize
				<< " .iNumberOfPerformanceLevels = " << s.iNumberOfPerformanceLevels
				<< " .iActivityReportingSupported = " << s.iActivityReportingSupported
				<< " .iDiscretePerformanceLevels = " << s.iDiscretePerformanceLevels
				<< " .iReserved = " << s.iReserved
				<< " .sEngineClock = " << DumpGenerated(s.sEngineClock)
				<< " .sMemoryClock = " << DumpGenerated(s.sMemoryClock)
				<< " .sVddc = " << DumpGenerated(s.sVddc)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODPerformanceLevel)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODPerformanceLevel*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODPerformanceLevel {"
				<< " .iEngineClock = " << s.iEngineClock
				<< " .iMemoryClock = " << s.iMemoryClock
				<< " .iVddc = " << s.iVddc
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODPerformanceLevels)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODPerformanceLevels*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODPerformanceLevels {"
				<< " .iSize = " << s.iSize
				<< " .iReserved = " << s.iReserved
				<< " .aLevels = " << DumpArray_(s.aLevels)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLCrossfireComb)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLCrossfireComb*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLCrossfireComb {"
				<< " .iNumLinkAdapter = " << s.iNumLinkAdapter
				<< " .iAdaptLink = " << DumpArray_(s.iAdaptLink)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLCrossfireInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLCrossfireInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLCrossfireInfo {"
				<< " .iErrorCode = " << s.iErrorCode
				<< " .iState = " << s.iState
				<< " .iSupported = " << s.iSupported
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLBiosInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLBiosInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLBiosInfo {"
				<< " .strPartNumber = " << s.strPartNumber
				<< " .strVersion = " << s.strVersion
				<< " .strDate = " << s.strDate
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAdapterLocation)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAdapterLocation*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAdapterLocation {"
				<< " .iBus = " << s.iBus
				<< " .iDevice = " << s.iDevice
				<< " .iFunction = " << s.iFunction
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLVersionsInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLVersionsInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLVersionsInfo {"
				<< " .strDriverVer = " << s.strDriverVer
				<< " .strCatalystVersion = " << s.strCatalystVersion
				<< " .strCatalystWebLink = " << s.strCatalystWebLink
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLVersionsInfoX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLVersionsInfoX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLVersionsInfoX2 {"
				<< " .strDriverVer = " << s.strDriverVer
				<< " .strCatalystVersion = " << s.strCatalystVersion
				<< " .strCrimsonVersion = " << s.strCrimsonVersion
				<< " .strCatalystWebLink = " << s.strCatalystWebLink
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMVPUCaps)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMVPUCaps*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMVPUCaps {"
				<< " .iSize = " << s.iSize
				<< " .iAdapterCount = " << s.iAdapterCount
				<< " .iPossibleMVPUMasters = " << s.iPossibleMVPUMasters
				<< " .iPossibleMVPUSlaves = " << s.iPossibleMVPUSlaves
				<< " .cAdapterPath = " << DumpArray_(s.cAdapterPath)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMVPUStatus)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMVPUStatus*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMVPUStatus {"
				<< " .iSize = " << s.iSize
				<< " .iActiveAdapterCount = " << s.iActiveAdapterCount
				<< " .iStatus = " << s.iStatus
				<< " .aAdapterLocation = " << DumpArray_(s.aAdapterLocation)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLActivatableSource)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLActivatableSource*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLActivatableSource {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iNumActivatableSources = " << s.iNumActivatableSources
				<< " .iActivatableSourceMask = " << s.iActivatableSourceMask
				<< " .iActivatableSourceValue = " << s.iActivatableSourceValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMode {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .displayID = " << DumpGenerated(s.displayID)
				<< " .iXPos = " << s.iXPos
				<< " .iYPos = " << s.iYPos
				<< " .iXRes = " << s.iXRes
				<< " .iYRes = " << s.iYRes
				<< " .iColourDepth = " << s.iColourDepth
				<< " .fRefreshRate = " << s.fRefreshRate
				<< " .iOrientation = " << s.iOrientation
				<< " .iModeFlag = " << s.iModeFlag
				<< " .iModeMask = " << s.iModeMask
				<< " .iModeValue = " << s.iModeValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayTarget)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayTarget*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayTarget {"
				<< " .displayID = " << DumpGenerated(s.displayID)
				<< " .iDisplayMapIndex = " << s.iDisplayMapIndex
				<< " .iDisplayTargetMask = " << s.iDisplayTargetMask
				<< " .iDisplayTargetValue = " << s.iDisplayTargetValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(tagADLBezelTransientMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const tagADLBezelTransientMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct tagADLBezelTransientMode {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iSLSMapIndex = " << s.iSLSMapIndex
				<< " .iSLSModeIndex = " << s.iSLSModeIndex
				<< " .displayMode = " << DumpGenerated(s.displayMode)
				<< " .iNumBezelOffset = " << s.iNumBezelOffset
				<< " .iFirstBezelOffsetArrayIndex = " << s.iFirstBezelOffsetArrayIndex
				<< " .iSLSBezelTransientModeMask = " << s.iSLSBezelTransientModeMask
				<< " .iSLSBezelTransientModeValue = " << s.iSLSBezelTransientModeValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAdapterDisplayCap)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAdapterDisplayCap*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAdapterDisplayCap {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iAdapterDisplayCapMask = " << s.iAdapterDisplayCapMask
				<< " .iAdapterDisplayCapValue = " << s.iAdapterDisplayCapValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayMap)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayMap*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayMap {"
				<< " .iDisplayMapIndex = " << s.iDisplayMapIndex
				<< " .displayMode = " << DumpGenerated(s.displayMode)
				<< " .iNumDisplayTarget = " << s.iNumDisplayTarget
				<< " .iFirstDisplayTargetArrayIndex = " << s.iFirstDisplayTargetArrayIndex
				<< " .iDisplayMapMask = " << s.iDisplayMapMask
				<< " .iDisplayMapValue = " << s.iDisplayMapValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPossibleMap)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPossibleMap*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPossibleMap {"
				<< " .iIndex = " << s.iIndex
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iNumDisplayMap = " << s.iNumDisplayMap
				<< " .displayMap = " << (s.displayMap ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.displayMap)) : "null"s)
				<< " .iNumDisplayTarget = " << s.iNumDisplayTarget
				<< " .displayTarget = " << (s.displayTarget ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.displayTarget)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPossibleMapping)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPossibleMapping*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPossibleMapping {"
				<< " .iDisplayIndex = " << s.iDisplayIndex
				<< " .iDisplayControllerIndex = " << s.iDisplayControllerIndex
				<< " .iDisplayMannerSupported = " << s.iDisplayMannerSupported
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPossibleMapResult)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPossibleMapResult*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPossibleMapResult {"
				<< " .iIndex = " << s.iIndex
				<< " .iPossibleMapResultMask = " << s.iPossibleMapResultMask
				<< " .iPossibleMapResultValue = " << s.iPossibleMapResultValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSLSGrid)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSLSGrid*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSLSGrid {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iSLSGridIndex = " << s.iSLSGridIndex
				<< " .iSLSGridRow = " << s.iSLSGridRow
				<< " .iSLSGridColumn = " << s.iSLSGridColumn
				<< " .iSLSGridMask = " << s.iSLSGridMask
				<< " .iSLSGridValue = " << s.iSLSGridValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSLSMap)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSLSMap*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSLSMap {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iSLSMapIndex = " << s.iSLSMapIndex
				<< " .grid = " << DumpGenerated(s.grid)
				<< " .iSurfaceMapIndex = " << s.iSurfaceMapIndex
				<< " .iOrientation = " << s.iOrientation
				<< " .iNumSLSTarget = " << s.iNumSLSTarget
				<< " .iFirstSLSTargetArrayIndex = " << s.iFirstSLSTargetArrayIndex
				<< " .iNumNativeMode = " << s.iNumNativeMode
				<< " .iFirstNativeModeArrayIndex = " << s.iFirstNativeModeArrayIndex
				<< " .iNumBezelMode = " << s.iNumBezelMode
				<< " .iFirstBezelModeArrayIndex = " << s.iFirstBezelModeArrayIndex
				<< " .iNumBezelOffset = " << s.iNumBezelOffset
				<< " .iFirstBezelOffsetArrayIndex = " << s.iFirstBezelOffsetArrayIndex
				<< " .iSLSMapMask = " << s.iSLSMapMask
				<< " .iSLSMapValue = " << s.iSLSMapValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSLSOffset)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSLSOffset*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSLSOffset {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iSLSMapIndex = " << s.iSLSMapIndex
				<< " .displayID = " << DumpGenerated(s.displayID)
				<< " .iBezelModeIndex = " << s.iBezelModeIndex
				<< " .iBezelOffsetX = " << s.iBezelOffsetX
				<< " .iBezelOffsetY = " << s.iBezelOffsetY
				<< " .iDisplayWidth = " << s.iDisplayWidth
				<< " .iDisplayHeight = " << s.iDisplayHeight
				<< " .iBezelOffsetMask = " << s.iBezelOffsetMask
				<< " .iBezelffsetValue = " << s.iBezelffsetValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSLSMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSLSMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSLSMode {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iSLSMapIndex = " << s.iSLSMapIndex
				<< " .iSLSModeIndex = " << s.iSLSModeIndex
				<< " .displayMode = " << DumpGenerated(s.displayMode)
				<< " .iSLSNativeModeMask = " << s.iSLSNativeModeMask
				<< " .iSLSNativeModeValue = " << s.iSLSNativeModeValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPossibleSLSMap)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPossibleSLSMap*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPossibleSLSMap {"
				<< " .iSLSMapIndex = " << s.iSLSMapIndex
				<< " .iNumSLSMap = " << s.iNumSLSMap
				<< " .lpSLSMap = " << (s.lpSLSMap ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.lpSLSMap)) : "null"s)
				<< " .iNumSLSTarget = " << s.iNumSLSTarget
				<< " .lpDisplayTarget = " << (s.lpDisplayTarget ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.lpDisplayTarget)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSLSTarget)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSLSTarget*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSLSTarget {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iSLSMapIndex = " << s.iSLSMapIndex
				<< " .displayTarget = " << DumpGenerated(s.displayTarget)
				<< " .iSLSGridPositionX = " << s.iSLSGridPositionX
				<< " .iSLSGridPositionY = " << s.iSLSGridPositionY
				<< " .viewSize = " << DumpGenerated(s.viewSize)
				<< " .iSLSTargetMask = " << s.iSLSTargetMask
				<< " .iSLSTargetValue = " << s.iSLSTargetValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLBezelOffsetSteppingSize)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLBezelOffsetSteppingSize*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLBezelOffsetSteppingSize {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iSLSMapIndex = " << s.iSLSMapIndex
				<< " .iBezelOffsetSteppingSizeX = " << s.iBezelOffsetSteppingSizeX
				<< " .iBezelOffsetSteppingSizeY = " << s.iBezelOffsetSteppingSizeY
				<< " .iBezelOffsetSteppingSizeMask = " << s.iBezelOffsetSteppingSizeMask
				<< " .iBezelOffsetSteppingSizeValue = " << s.iBezelOffsetSteppingSizeValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSLSOverlappedMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSLSOverlappedMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSLSOverlappedMode {"
				<< " .SLSMode = " << DumpGenerated(s.SLSMode)
				<< " .iNumSLSTarget = " << s.iNumSLSTarget
				<< " .iFirstTargetArrayIndex = " << s.iFirstTargetArrayIndex
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPXConfigCaps)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPXConfigCaps*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPXConfigCaps {"
				<< " .iAdapterIndex = " << s.iAdapterIndex
				<< " .iPXConfigCapMask = " << s.iPXConfigCapMask
				<< " .iPXConfigCapValue = " << s.iPXConfigCapValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLApplicationData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLApplicationData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLApplicationData {"
				<< " .strPathName = " << s.strPathName
				<< " .strFileName = " << s.strFileName
				<< " .strTimeStamp = " << s.strTimeStamp
				<< " .strVersion = " << s.strVersion
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLApplicationDataX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLApplicationDataX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLApplicationDataX2 {"
				<< " .strPathName = " << DumpArray_(s.strPathName)
				<< " .strFileName = " << DumpArray_(s.strFileName)
				<< " .strTimeStamp = " << DumpArray_(s.strTimeStamp)
				<< " .strVersion = " << DumpArray_(s.strVersion)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLApplicationDataX3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLApplicationDataX3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLApplicationDataX3 {"
				<< " .strPathName = " << DumpArray_(s.strPathName)
				<< " .strFileName = " << DumpArray_(s.strFileName)
				<< " .strTimeStamp = " << DumpArray_(s.strTimeStamp)
				<< " .strVersion = " << DumpArray_(s.strVersion)
				<< " .iProcessId = " << s.iProcessId
				<< " }";
			return oss.str();
		};
		dumpers[typeid(PropertyRecord)] = [](const void* pStruct) {
			const auto& s = *static_cast<const PropertyRecord*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct PropertyRecord {"
				<< " .strName = " << s.strName
				<< " .eType = " << DumpGenerated(s.eType)
				<< " .iDataSize = " << s.iDataSize
				<< " .uData = " << DumpArray_(s.uData)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLApplicationProfile)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLApplicationProfile*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLApplicationProfile {"
				<< " .iCount = " << s.iCount
				<< " .record = " << DumpArray_(s.record)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPowerControlInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPowerControlInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPowerControlInfo {"
				<< " .iMinValue = " << s.iMinValue
				<< " .iMaxValue = " << s.iMaxValue
				<< " .iStepValue = " << s.iStepValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLControllerMode)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLControllerMode*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLControllerMode {"
				<< " .iModifiers = " << s.iModifiers
				<< " .iViewPositionCx = " << s.iViewPositionCx
				<< " .iViewPositionCy = " << s.iViewPositionCy
				<< " .iViewPanLockLeft = " << s.iViewPanLockLeft
				<< " .iViewPanLockRight = " << s.iViewPanLockRight
				<< " .iViewPanLockTop = " << s.iViewPanLockTop
				<< " .iViewPanLockBottom = " << s.iViewPanLockBottom
				<< " .iViewResolutionCx = " << s.iViewResolutionCx
				<< " .iViewResolutionCy = " << s.iViewResolutionCy
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayIdentifier)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayIdentifier*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayIdentifier {"
				<< " .ulDisplayIndex = " << s.ulDisplayIndex
				<< " .ulManufacturerId = " << s.ulManufacturerId
				<< " .ulProductId = " << s.ulProductId
				<< " .ulSerialNo = " << s.ulSerialNo
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
		dumpers[typeid(ADLOD6PerformanceLevel)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6PerformanceLevel*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6PerformanceLevel {"
				<< " .iEngineClock = " << s.iEngineClock
				<< " .iMemoryClock = " << s.iMemoryClock
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6StateInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6StateInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6StateInfo {"
				<< " .iNumberOfPerformanceLevels = " << s.iNumberOfPerformanceLevels
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " .aLevels = " << DumpArray_(s.aLevels)
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
		dumpers[typeid(ADLOD6FanSpeedValue)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6FanSpeedValue*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6FanSpeedValue {"
				<< " .iSpeedType = " << s.iSpeedType
				<< " .iFanSpeed = " << s.iFanSpeed
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6PowerControlInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6PowerControlInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6PowerControlInfo {"
				<< " .iMinValue = " << s.iMinValue
				<< " .iMaxValue = " << s.iMaxValue
				<< " .iStepValue = " << s.iStepValue
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6VoltageControlInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6VoltageControlInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6VoltageControlInfo {"
				<< " .iMinValue = " << s.iMinValue
				<< " .iMaxValue = " << s.iMaxValue
				<< " .iStepValue = " << s.iStepValue
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLECCData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLECCData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLECCData {"
				<< " .iSec = " << s.iSec
				<< " .iDed = " << s.iDed
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayModeX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayModeX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayModeX2 {"
				<< " .iWidth = " << s.iWidth
				<< " .iHeight = " << s.iHeight
				<< " .iScanType = " << s.iScanType
				<< " .iRefreshRate = " << s.iRefreshRate
				<< " .iTimingStandard = " << s.iTimingStandard
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAppInterceptionInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAppInterceptionInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAppInterceptionInfo {"
				<< " .AppName = " << DumpArray_(s.AppName)
				<< " .ProcessId = " << s.ProcessId
				<< " .AppFormat = " << DumpGenerated(s.AppFormat)
				<< " .AppState = " << DumpGenerated(s.AppState)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAppInterceptionInfoX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAppInterceptionInfoX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAppInterceptionInfoX2 {"
				<< " .AppName = " << DumpArray_(s.AppName)
				<< " .ProcessId = " << s.ProcessId
				<< " .WaitForResumeNeeded = " << s.WaitForResumeNeeded
				<< " .CommandLine = " << DumpArray_(s.CommandLine)
				<< " .AppFormat = " << DumpGenerated(s.AppFormat)
				<< " .AppState = " << DumpGenerated(s.AppState)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAppInterceptionInfoX3)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAppInterceptionInfoX3*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAppInterceptionInfoX3 {"
				<< " .AppName = " << DumpArray_(s.AppName)
				<< " .ProcessId = " << s.ProcessId
				<< " .WaitForResumeNeeded = " << s.WaitForResumeNeeded
				<< " .RayTracingStatus = " << s.RayTracingStatus
				<< " .CommandLine = " << DumpArray_(s.CommandLine)
				<< " .AppFormat = " << DumpGenerated(s.AppFormat)
				<< " .AppState = " << DumpGenerated(s.AppState)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPropertyRecordCreate)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPropertyRecordCreate*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPropertyRecordCreate {"
				<< " .strPropertyName = " << (s.strPropertyName ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strPropertyName)) : "null"s)
				<< " .eType = " << DumpGenerated(s.eType)
				<< " .strPropertyValue = " << (s.strPropertyValue ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strPropertyValue)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLApplicationRecord)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLApplicationRecord*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLApplicationRecord {"
				<< " .strTitle = " << (s.strTitle ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strTitle)) : "null"s)
				<< " .strPathName = " << (s.strPathName ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strPathName)) : "null"s)
				<< " .strFileName = " << (s.strFileName ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strFileName)) : "null"s)
				<< " .strVersion = " << (s.strVersion ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strVersion)) : "null"s)
				<< " .strNotes = " << (s.strNotes ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strNotes)) : "null"s)
				<< " .strArea = " << (s.strArea ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strArea)) : "null"s)
				<< " .strProfileName = " << (s.strProfileName ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.strProfileName)) : "null"s)
				<< " .recordSource = " << DumpGenerated(s.recordSource)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6CapabilitiesEx)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6CapabilitiesEx*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6CapabilitiesEx {"
				<< " .iCapabilities = " << s.iCapabilities
				<< " .iSupportedStates = " << s.iSupportedStates
				<< " .sEngineClockPercent = " << DumpGenerated(s.sEngineClockPercent)
				<< " .sMemoryClockPercent = " << DumpGenerated(s.sMemoryClockPercent)
				<< " .sPowerControlPercent = " << DumpGenerated(s.sPowerControlPercent)
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6StateEx)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6StateEx*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6StateEx {"
				<< " .iEngineClockPercent = " << s.iEngineClockPercent
				<< " .iMemoryClockPercent = " << s.iMemoryClockPercent
				<< " .iPowerControlPercent = " << s.iPowerControlPercent
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD6MaxClockAdjust)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD6MaxClockAdjust*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD6MaxClockAdjust {"
				<< " .iEngineClockMax = " << s.iEngineClockMax
				<< " .iMemoryClockMax = " << s.iMemoryClockMax
				<< " .iExtValue = " << s.iExtValue
				<< " .iExtMask = " << s.iExtMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLConnectorInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLConnectorInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLConnectorInfo {"
				<< " .iConnectorIndex = " << s.iConnectorIndex
				<< " .iConnectorId = " << s.iConnectorId
				<< " .iSlotIndex = " << s.iSlotIndex
				<< " .iType = " << s.iType
				<< " .iOffset = " << s.iOffset
				<< " .iLength = " << s.iLength
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLBracketSlotInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLBracketSlotInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLBracketSlotInfo {"
				<< " .iSlotIndex = " << s.iSlotIndex
				<< " .iLength = " << s.iLength
				<< " .iWidth = " << s.iWidth
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMSTRad)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMSTRad*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMSTRad {"
				<< " .iLinkNumber = " << s.iLinkNumber
				<< " .rad = " << s.rad
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDevicePort)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDevicePort*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDevicePort {"
				<< " .iConnectorIndex = " << s.iConnectorIndex
				<< " .aMSTRad = " << DumpGenerated(s.aMSTRad)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSupportedConnections)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSupportedConnections*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSupportedConnections {"
				<< " .iSupportedConnections = " << s.iSupportedConnections
				<< " .iSupportedProperties = " << DumpArray_(s.iSupportedProperties)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLConnectionState)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLConnectionState*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLConnectionState {"
				<< " .iEmulationStatus = " << s.iEmulationStatus
				<< " .iEmulationMode = " << s.iEmulationMode
				<< " .iDisplayIndex = " << s.iDisplayIndex
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLConnectionProperties)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLConnectionProperties*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLConnectionProperties {"
				<< " .iValidProperties = " << s.iValidProperties
				<< " .iBitrate = " << s.iBitrate
				<< " .iNumberOfLanes = " << s.iNumberOfLanes
				<< " .iColorDepth = " << s.iColorDepth
				<< " .iStereo3DCaps = " << s.iStereo3DCaps
				<< " .iOutputBandwidth = " << s.iOutputBandwidth
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLConnectionData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLConnectionData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLConnectionData {"
				<< " .iConnectionType = " << s.iConnectionType
				<< " .aConnectionProperties = " << DumpGenerated(s.aConnectionProperties)
				<< " .iNumberofPorts = " << s.iNumberofPorts
				<< " .iActiveConnections = " << s.iActiveConnections
				<< " .iDataSize = " << s.iDataSize
				<< " .EdidData = " << s.EdidData
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLAdapterCapsX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLAdapterCapsX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLAdapterCapsX2 {"
				<< " .iAdapterID = " << s.iAdapterID
				<< " .iNumControllers = " << s.iNumControllers
				<< " .iNumDisplays = " << s.iNumDisplays
				<< " .iNumOverlays = " << s.iNumOverlays
				<< " .iNumOfGLSyncConnectors = " << s.iNumOfGLSyncConnectors
				<< " .iCapsMask = " << s.iCapsMask
				<< " .iCapsValue = " << s.iCapsValue
				<< " .iNumConnectors = " << s.iNumConnectors
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ADL_ECC_EDC_FLAG)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ADL_ECC_EDC_FLAG*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ADL_ECC_EDC_FLAG {"
				<< " .bits = " << DumpGenerated(s.bits)
				<< " .u32All = " << s.u32All
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLErrorRecord)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLErrorRecord*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLErrorRecord {"
				<< " .Severity = " << DumpGenerated(s.Severity)
				<< " .countValid = " << s.countValid
				<< " .count = " << s.count
				<< " .locationValid = " << s.locationValid
				<< " .CU = " << s.CU
				<< " .StructureName = " << s.StructureName
				<< " .tiestamp = " << s.tiestamp
				<< " .padding = " << DumpArray_(s.padding)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ADL_ERROR_PATTERN)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ADL_ERROR_PATTERN*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ADL_ERROR_PATTERN {"
				<< " .bits = " << DumpGenerated(s.bits)
				<< " .u64Value = " << s.u64Value
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_ERROR_INJECTION_DATA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_ERROR_INJECTION_DATA*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_ERROR_INJECTION_DATA {"
				<< " .errorAddress = " << s.errorAddress
				<< " .errorPattern = " << DumpGenerated(s.errorPattern)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLErrorInjection)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLErrorInjection*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLErrorInjection {"
				<< " .blockId = " << DumpGenerated(s.blockId)
				<< " .errorInjectionMode = " << DumpGenerated(s.errorInjectionMode)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLErrorInjectionX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLErrorInjectionX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLErrorInjectionX2 {"
				<< " .blockId = " << DumpGenerated(s.blockId)
				<< " .errorInjectionMode = " << DumpGenerated(s.errorInjectionMode)
				<< " .errorInjectionData = " << DumpGenerated(s.errorInjectionData)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFreeSyncCap)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFreeSyncCap*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFreeSyncCap {"
				<< " .iCaps = " << s.iCaps
				<< " .iMinRefreshRateInMicroHz = " << s.iMinRefreshRateInMicroHz
				<< " .iMaxRefreshRateInMicroHz = " << s.iMaxRefreshRateInMicroHz
				<< " .ucLabelIndex = " << (int)s.ucLabelIndex
				<< " .cReserved = " << s.cReserved
				<< " .iReserved = " << DumpArray_(s.iReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDceSettings)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDceSettings*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDceSettings {"
				<< " .type = " << DumpGenerated(s.type)
				<< " .Settings = " << DumpGenerated(s.Settings)
				<< " .iReserved = " << DumpArray_(s.iReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGraphicCoreInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGraphicCoreInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGraphicCoreInfo {"
				<< " .iGCGen = " << s.iGCGen
				<< " .iNumSIMDs = " << s.iNumSIMDs
				<< " .iNumROPs = " << s.iNumROPs
				<< " .iReserved = " << DumpArray_(s.iReserved)
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
		dumpers[typeid(ADLODNCapabilities)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNCapabilities*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNCapabilities {"
				<< " .iMaximumNumberOfPerformanceLevels = " << s.iMaximumNumberOfPerformanceLevels
				<< " .sEngineClockRange = " << DumpGenerated(s.sEngineClockRange)
				<< " .sMemoryClockRange = " << DumpGenerated(s.sMemoryClockRange)
				<< " .svddcRange = " << DumpGenerated(s.svddcRange)
				<< " .power = " << DumpGenerated(s.power)
				<< " .powerTuneTemperature = " << DumpGenerated(s.powerTuneTemperature)
				<< " .fanTemperature = " << DumpGenerated(s.fanTemperature)
				<< " .fanSpeed = " << DumpGenerated(s.fanSpeed)
				<< " .minimumPerformanceClock = " << DumpGenerated(s.minimumPerformanceClock)
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
		dumpers[typeid(ADLODNPerformanceLevel)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNPerformanceLevel*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNPerformanceLevel {"
				<< " .iClock = " << s.iClock
				<< " .iVddc = " << s.iVddc
				<< " .iEnabled = " << s.iEnabled
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNPerformanceLevels)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNPerformanceLevels*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNPerformanceLevels {"
				<< " .iSize = " << s.iSize
				<< " .iMode = " << s.iMode
				<< " .iNumberOfPerformanceLevels = " << s.iNumberOfPerformanceLevels
				<< " .aLevels = " << DumpArray_(s.aLevels)
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
		dumpers[typeid(ADLODNPowerLimitSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNPowerLimitSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNPowerLimitSetting {"
				<< " .iMode = " << s.iMode
				<< " .iTDPLimit = " << s.iTDPLimit
				<< " .iMaxOperatingTemperature = " << s.iMaxOperatingTemperature
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
		dumpers[typeid(ADLODNPerformanceLevelX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNPerformanceLevelX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNPerformanceLevelX2 {"
				<< " .iClock = " << s.iClock
				<< " .iVddc = " << s.iVddc
				<< " .iEnabled = " << s.iEnabled
				<< " .iControl = " << s.iControl
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNPerformanceLevelsX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNPerformanceLevelsX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNPerformanceLevelsX2 {"
				<< " .iSize = " << s.iSize
				<< " .iMode = " << s.iMode
				<< " .iNumberOfPerformanceLevels = " << s.iNumberOfPerformanceLevels
				<< " .aLevels = " << DumpArray_(s.aLevels)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNCurrentPowerParameters)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNCurrentPowerParameters*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNCurrentPowerParameters {"
				<< " .size = " << s.size
				<< " .powerType = " << DumpGenerated(s.powerType)
				<< " .currentPower = " << s.currentPower
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLODNExtSingleInitSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLODNExtSingleInitSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLODNExtSingleInitSetting {"
				<< " .mode = " << s.mode
				<< " .minValue = " << s.minValue
				<< " .maxValue = " << s.maxValue
				<< " .step = " << s.step
				<< " .defaultValue = " << s.defaultValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD8SingleInitSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD8SingleInitSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD8SingleInitSetting {"
				<< " .featureID = " << s.featureID
				<< " .minValue = " << s.minValue
				<< " .maxValue = " << s.maxValue
				<< " .defaultValue = " << s.defaultValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD8InitSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD8InitSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD8InitSetting {"
				<< " .count = " << s.count
				<< " .overdrive8Capabilities = " << s.overdrive8Capabilities
				<< " .od8SettingTable = " << DumpArray_(s.od8SettingTable)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD8CurrentSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD8CurrentSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD8CurrentSetting {"
				<< " .count = " << s.count
				<< " .Od8SettingTable = " << DumpArray_(s.Od8SettingTable)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD8SingleSetSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD8SingleSetSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD8SingleSetSetting {"
				<< " .value = " << s.value
				<< " .requested = " << s.requested
				<< " .reset = " << s.reset
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLOD8SetSetting)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLOD8SetSetting*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLOD8SetSetting {"
				<< " .count = " << s.count
				<< " .od8SettingTable = " << DumpArray_(s.od8SettingTable)
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
				<< " .sensors = " << DumpArray_(s.sensors)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPPLogSettings)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPPLogSettings*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPPLogSettings {"
				<< " .BreakOnAssert = " << s.BreakOnAssert
				<< " .BreakOnWarn = " << s.BreakOnWarn
				<< " .LogEnabled = " << s.LogEnabled
				<< " .LogFieldMask = " << s.LogFieldMask
				<< " .LogDestinations = " << s.LogDestinations
				<< " .LogSeverityEnabled = " << s.LogSeverityEnabled
				<< " .LogSourceMask = " << s.LogSourceMask
				<< " .PowerProfilingEnabled = " << s.PowerProfilingEnabled
				<< " .PowerProfilingTimeInterval = " << s.PowerProfilingTimeInterval
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFPSSettingsOutput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFPSSettingsOutput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFPSSettingsOutput {"
				<< " .ulSize = " << s.ulSize
				<< " .bACFPSEnabled = " << s.bACFPSEnabled
				<< " .bDCFPSEnabled = " << s.bDCFPSEnabled
				<< " .ulACFPSCurrent = " << s.ulACFPSCurrent
				<< " .ulDCFPSCurrent = " << s.ulDCFPSCurrent
				<< " .ulACFPSMaximum = " << s.ulACFPSMaximum
				<< " .ulACFPSMinimum = " << s.ulACFPSMinimum
				<< " .ulDCFPSMaximum = " << s.ulDCFPSMaximum
				<< " .ulDCFPSMinimum = " << s.ulDCFPSMinimum
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFPSSettingsInput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFPSSettingsInput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFPSSettingsInput {"
				<< " .ulSize = " << s.ulSize
				<< " .bGlobalSettings = " << s.bGlobalSettings
				<< " .ulACFPSCurrent = " << s.ulACFPSCurrent
				<< " .ulDCFPSCurrent = " << s.ulDCFPSCurrent
				<< " .ulReserved = " << DumpArray_(s.ulReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPMLogSupportInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPMLogSupportInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPMLogSupportInfo {"
				<< " .usSensors = " << DumpArray_(s.usSensors)
				<< " .ulReserved = " << DumpArray_(s.ulReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPMLogStartInput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPMLogStartInput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPMLogStartInput {"
				<< " .usSensors = " << DumpArray_(s.usSensors)
				<< " .ulSampleRate = " << s.ulSampleRate
				<< " .ulReserved = " << DumpArray_(s.ulReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPMLogData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPMLogData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPMLogData {"
				<< " .ulVersion = " << s.ulVersion
				<< " .ulActiveSampleRate = " << s.ulActiveSampleRate
				<< " .ulLastUpdated = " << s.ulLastUpdated
				<< " .ulValues = " << DumpArray_(s.ulValues)
				<< " .ulReserved = " << DumpArray_(s.ulReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPMLogStartOutput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPMLogStartOutput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPMLogStartOutput {"
				<< " .ulReserved = " << DumpArray_(s.ulReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPMLogSensorLimits)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPMLogSensorLimits*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPMLogSensorLimits {"
				<< " .SensorLimits = " << DumpArray_(s.SensorLimits)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASGetErrorCountsInput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASGetErrorCountsInput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASGetErrorCountsInput {"
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASGetErrorCountsOutput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASGetErrorCountsOutput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASGetErrorCountsOutput {"
				<< " .CorrectedErrors = " << s.CorrectedErrors
				<< " .UnCorrectedErrors = " << s.UnCorrectedErrors
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASGetErrorCounts)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASGetErrorCounts*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASGetErrorCounts {"
				<< " .InputSize = " << s.InputSize
				<< " .Input = " << DumpGenerated(s.Input)
				<< " .OutputSize = " << s.OutputSize
				<< " .Output = " << DumpGenerated(s.Output)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASResetErrorCountsInput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASResetErrorCountsInput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASResetErrorCountsInput {"
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASResetErrorCountsOutput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASResetErrorCountsOutput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASResetErrorCountsOutput {"
				<< " .Reserved = " << DumpArray_(s.Reserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASResetErrorCounts)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASResetErrorCounts*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASResetErrorCounts {"
				<< " .InputSize = " << s.InputSize
				<< " .Input = " << DumpGenerated(s.Input)
				<< " .OutputSize = " << s.OutputSize
				<< " .Output = " << DumpGenerated(s.Output)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASErrorInjectonInput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASErrorInjectonInput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASErrorInjectonInput {"
				<< " .Address = " << s.Address
				<< " .Value = " << DumpGenerated(s.Value)
				<< " .BlockId = " << DumpGenerated(s.BlockId)
				<< " .InjectErrorType = " << DumpGenerated(s.InjectErrorType)
				<< " .SubBlockIndex = " << DumpGenerated(s.SubBlockIndex)
				<< " .padding = " << DumpArray_(s.padding)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASErrorInjectionOutput)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASErrorInjectionOutput*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASErrorInjectionOutput {"
				<< " .ErrorInjectionStatus = " << s.ErrorInjectionStatus
				<< " .padding = " << DumpArray_(s.padding)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLRASErrorInjection)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLRASErrorInjection*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLRASErrorInjection {"
				<< " .InputSize = " << s.InputSize
				<< " .Input = " << DumpGenerated(s.Input)
				<< " .OutputSize = " << s.OutputSize
				<< " .Output = " << DumpGenerated(s.Output)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSGApplicationInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSGApplicationInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSGApplicationInfo {"
				<< " .strFileName = " << DumpArray_(s.strFileName)
				<< " .strFilePath = " << DumpArray_(s.strFilePath)
				<< " .strVersion = " << DumpArray_(s.strVersion)
				<< " .timeStamp = " << s.timeStamp
				<< " .iProfileExists = " << s.iProfileExists
				<< " .iGPUAffinity = " << s.iGPUAffinity
				<< " .GPUBdf = " << DumpGenerated(s.GPUBdf)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPreFlipPostProcessingInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPreFlipPostProcessingInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPreFlipPostProcessingInfo {"
				<< " .ulSize = " << s.ulSize
				<< " .bEnabled = " << s.bEnabled
				<< " .ulSelectedLUTIndex = " << s.ulSelectedLUTIndex
				<< " .ulSelectedLUTAlgorithm = " << s.ulSelectedLUTAlgorithm
				<< " .ulReserved = " << DumpArray_(s.ulReserved)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_ERROR_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_ERROR_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_ERROR_REASON {"
				<< " .boost = " << s.boost
				<< " .delag = " << s.delag
				<< " .chill = " << s.chill
				<< " .proVsr = " << s.proVsr
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_DELAG_NOTFICATION_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_DELAG_NOTFICATION_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_DELAG_NOTFICATION_REASON {"
				<< " .HotkeyChanged = " << s.HotkeyChanged
				<< " .GlobalEnableChanged = " << s.GlobalEnableChanged
				<< " .GlobalLimitFPSChanged = " << s.GlobalLimitFPSChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_DELAG_SETTINGS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_DELAG_SETTINGS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_DELAG_SETTINGS {"
				<< " .Hotkey = " << s.Hotkey
				<< " .GlobalEnable = " << s.GlobalEnable
				<< " .GlobalLimitFPS = " << s.GlobalLimitFPS
				<< " .GlobalLimitFPS_MinLimit = " << s.GlobalLimitFPS_MinLimit
				<< " .GlobalLimitFPS_MaxLimit = " << s.GlobalLimitFPS_MaxLimit
				<< " .GlobalLimitFPS_Step = " << s.GlobalLimitFPS_Step
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_BOOST_NOTFICATION_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_BOOST_NOTFICATION_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_BOOST_NOTFICATION_REASON {"
				<< " .HotkeyChanged = " << s.HotkeyChanged
				<< " .GlobalEnableChanged = " << s.GlobalEnableChanged
				<< " .GlobalMinResChanged = " << s.GlobalMinResChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_BOOST_SETTINGS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_BOOST_SETTINGS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_BOOST_SETTINGS {"
				<< " .Hotkey = " << s.Hotkey
				<< " .GlobalEnable = " << s.GlobalEnable
				<< " .GlobalMinRes = " << s.GlobalMinRes
				<< " .GlobalMinRes_MinLimit = " << s.GlobalMinRes_MinLimit
				<< " .GlobalMinRes_MaxLimit = " << s.GlobalMinRes_MaxLimit
				<< " .GlobalMinRes_Step = " << s.GlobalMinRes_Step
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_PROVSR_NOTFICATION_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_PROVSR_NOTFICATION_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_PROVSR_NOTFICATION_REASON {"
				<< " .HotkeyChanged = " << s.HotkeyChanged
				<< " .GlobalEnableChanged = " << s.GlobalEnableChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_PROVSR_SETTINGS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_PROVSR_SETTINGS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_PROVSR_SETTINGS {"
				<< " .Hotkey = " << s.Hotkey
				<< " .GlobalEnable = " << s.GlobalEnable
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_IMAGE_BOOST_NOTFICATION_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_IMAGE_BOOST_NOTFICATION_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_IMAGE_BOOST_NOTFICATION_REASON {"
				<< " .HotkeyChanged = " << s.HotkeyChanged
				<< " .GlobalEnableChanged = " << s.GlobalEnableChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_IMAGE_BOOST_SETTINGS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_IMAGE_BOOST_SETTINGS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_IMAGE_BOOST_SETTINGS {"
				<< " .Hotkey = " << s.Hotkey
				<< " .GlobalEnable = " << s.GlobalEnable
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_RIS_NOTFICATION_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_RIS_NOTFICATION_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_RIS_NOTFICATION_REASON {"
				<< " .GlobalEnableChanged = " << s.GlobalEnableChanged
				<< " .GlobalSharpeningDegreeChanged = " << s.GlobalSharpeningDegreeChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_RIS_SETTINGS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_RIS_SETTINGS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_RIS_SETTINGS {"
				<< " .GlobalEnable = " << s.GlobalEnable
				<< " .GlobalSharpeningDegree = " << s.GlobalSharpeningDegree
				<< " .GlobalSharpeningDegree_MinLimit = " << s.GlobalSharpeningDegree_MinLimit
				<< " .GlobalSharpeningDegree_MaxLimit = " << s.GlobalSharpeningDegree_MaxLimit
				<< " .GlobalSharpeningDegree_Step = " << s.GlobalSharpeningDegree_Step
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_CHILL_NOTFICATION_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_CHILL_NOTFICATION_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_CHILL_NOTFICATION_REASON {"
				<< " .HotkeyChanged = " << s.HotkeyChanged
				<< " .GlobalEnableChanged = " << s.GlobalEnableChanged
				<< " .GlobalMinFPSChanged = " << s.GlobalMinFPSChanged
				<< " .GlobalMaxFPSChanged = " << s.GlobalMaxFPSChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_CHILL_SETTINGS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_CHILL_SETTINGS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_CHILL_SETTINGS {"
				<< " .Hotkey = " << s.Hotkey
				<< " .GlobalEnable = " << s.GlobalEnable
				<< " .GlobalMinFPS = " << s.GlobalMinFPS
				<< " .GlobalMaxFPS = " << s.GlobalMaxFPS
				<< " .GlobalFPS_MinLimit = " << s.GlobalFPS_MinLimit
				<< " .GlobalFPS_MaxLimit = " << s.GlobalFPS_MaxLimit
				<< " .GlobalFPS_Step = " << s.GlobalFPS_Step
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_DRIVERUPSCALE_NOTFICATION_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_DRIVERUPSCALE_NOTFICATION_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_DRIVERUPSCALE_NOTFICATION_REASON {"
				<< " .ModeOverrideEnabledChanged = " << s.ModeOverrideEnabledChanged
				<< " .GlobalEnabledChanged = " << s.GlobalEnabledChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_DRIVERUPSCALE_SETTINGS)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_DRIVERUPSCALE_SETTINGS*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_DRIVERUPSCALE_SETTINGS {"
				<< " .ModeOverrideEnabled = " << s.ModeOverrideEnabled
				<< " .GlobalEnabled = " << s.GlobalEnabled
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_RADEON_LED_COLOR_CONFIG)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_RADEON_LED_COLOR_CONFIG*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_RADEON_LED_COLOR_CONFIG {"
				<< " .R = " << s.R
				<< " .G = " << s.G
				<< " .B = " << s.B
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_RADEON_LED_PATTERN_CONFIG_GENERIC)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_RADEON_LED_PATTERN_CONFIG_GENERIC*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_RADEON_LED_PATTERN_CONFIG_GENERIC {"
				<< " .brightness = " << s.brightness
				<< " .speed = " << s.speed
				<< " .directionCounterClockWise = " << s.directionCounterClockWise
				<< " .colorConfig = " << DumpGenerated(s.colorConfig)
				<< " .morseCodeText = " << s.morseCodeText
				<< " .morseCodeTextOutPut = " << s.morseCodeTextOutPut
				<< " .morseCodeTextOutPutLen = " << s.morseCodeTextOutPutLen
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_RADEON_LED_CUSTOM_LED_CONFIG)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_RADEON_LED_CUSTOM_LED_CONFIG*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_RADEON_LED_CUSTOM_LED_CONFIG {"
				<< " .brightness = " << s.brightness
				<< " .colorConfig = " << DumpArray_(s.colorConfig)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_RADEON_LED_PATTERN_CONFIG)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_RADEON_LED_PATTERN_CONFIG*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_RADEON_LED_PATTERN_CONFIG {"
				<< " .control = " << DumpGenerated(s.control)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(AdapterInfoX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const AdapterInfoX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct AdapterInfoX2 {"
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
				<< " .iInfoMask = " << s.iInfoMask
				<< " .iInfoValue = " << s.iInfoValue
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGamutReference)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGamutReference*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGamutReference {"
				<< " .iGamutRef = " << s.iGamutRef
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGamutInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGamutInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGamutInfo {"
				<< " .SupportedGamutSpace = " << s.SupportedGamutSpace
				<< " .SupportedWhitePoint = " << s.SupportedWhitePoint
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLPoint)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLPoint*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLPoint {"
				<< " .iX = " << s.iX
				<< " .iY = " << s.iY
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGamutCoordinates)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGamutCoordinates*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGamutCoordinates {"
				<< " .Red = " << DumpGenerated(s.Red)
				<< " .Green = " << DumpGenerated(s.Green)
				<< " .Blue = " << DumpGenerated(s.Blue)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLGamutData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLGamutData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLGamutData {"
				<< " .iFeature = " << s.iFeature
				<< " .iPredefinedGamut = " << s.iPredefinedGamut
				<< " .iPredefinedWhitePoint = " << s.iPredefinedWhitePoint
				<< " .CustomWhitePoint = " << DumpGenerated(s.CustomWhitePoint)
				<< " .CustomGamut = " << DumpGenerated(s.CustomGamut)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDetailedTimingX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDetailedTimingX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDetailedTimingX2 {"
				<< " .iSize = " << s.iSize
				<< " .sTimingFlags = " << s.sTimingFlags
				<< " .sHTotal = " << s.sHTotal
				<< " .sHDisplay = " << s.sHDisplay
				<< " .sHSyncStart = " << s.sHSyncStart
				<< " .sHSyncWidth = " << s.sHSyncWidth
				<< " .sVTotal = " << s.sVTotal
				<< " .sVDisplay = " << s.sVDisplay
				<< " .sVSyncStart = " << s.sVSyncStart
				<< " .sVSyncWidth = " << s.sVSyncWidth
				<< " .sPixelClock = " << s.sPixelClock
				<< " .sHOverscanRight = " << s.sHOverscanRight
				<< " .sHOverscanLeft = " << s.sHOverscanLeft
				<< " .sVOverscanBottom = " << s.sVOverscanBottom
				<< " .sVOverscanTop = " << s.sVOverscanTop
				<< " .sOverscan8B = " << s.sOverscan8B
				<< " .sOverscanGR = " << s.sOverscanGR
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLDisplayModeInfoX2)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLDisplayModeInfoX2*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLDisplayModeInfoX2 {"
				<< " .iTimingStandard = " << s.iTimingStandard
				<< " .iPossibleStandard = " << s.iPossibleStandard
				<< " .iRefreshRate = " << s.iRefreshRate
				<< " .iPelsWidth = " << s.iPelsWidth
				<< " .iPelsHeight = " << s.iPelsHeight
				<< " .sDetailedTiming = " << DumpGenerated(s.sDetailedTiming)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLI2CLargePayload)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLI2CLargePayload*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLI2CLargePayload {"
				<< " .iSize = " << s.iSize
				<< " .iLine = " << s.iLine
				<< " .iAddress = " << s.iAddress
				<< " .iOffset = " << s.iOffset
				<< " .iAction = " << s.iAction
				<< " .iSpeed = " << s.iSpeed
				<< " .iFlags = " << s.iFlags
				<< " .iDataSize = " << s.iDataSize
				<< " .pcData = " << (s.pcData ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.pcData)) : "null"s)
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFeatureName)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFeatureName*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFeatureName {"
				<< " .FeatureName = " << s.FeatureName
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFeatureCaps)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFeatureCaps*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFeatureCaps {"
				<< " .Name = " << DumpGenerated(s.Name)
				<< " .iGroupID = " << s.iGroupID
				<< " .iVisualID = " << s.iVisualID
				<< " .iPageID = " << s.iPageID
				<< " .iFeatureMask = " << s.iFeatureMask
				<< " .iFeatureProperties = " << s.iFeatureProperties
				<< " .iControlType = " << s.iControlType
				<< " .iControlStyle = " << s.iControlStyle
				<< " .iAdjustmentType = " << s.iAdjustmentType
				<< " .iAdjustmentStyle = " << s.iAdjustmentStyle
				<< " .bDefault = " << s.bDefault
				<< " .iMin = " << s.iMin
				<< " .iMax = " << s.iMax
				<< " .iStep = " << s.iStep
				<< " .iDefault = " << s.iDefault
				<< " .fMin = " << s.fMin
				<< " .fMax = " << s.fMax
				<< " .fStep = " << s.fStep
				<< " .fDefault = " << s.fDefault
				<< " .EnumMask = " << s.EnumMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLFeatureValues)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLFeatureValues*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLFeatureValues {"
				<< " .Name = " << DumpGenerated(s.Name)
				<< " .bCurrent = " << s.bCurrent
				<< " .iCurrent = " << s.iCurrent
				<< " .fCurrent = " << s.fCurrent
				<< " .EnumStates = " << s.EnumStates
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLHDCPSettings)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLHDCPSettings*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLHDCPSettings {"
				<< " .iHDCPProtectionVersion = " << s.iHDCPProtectionVersion
				<< " .iHDCPCaps = " << s.iHDCPCaps
				<< " .iAllowAll = " << s.iAllowAll
				<< " .iHDCPVale = " << s.iHDCPVale
				<< " .iHDCPMask = " << s.iHDCPMask
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLMantleAppInfo)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLMantleAppInfo*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLMantleAppInfo {"
				<< " .apiVersion = " << s.apiVersion
				<< " .driverVersion = " << s.driverVersion
				<< " .vendorId = " << s.vendorId
				<< " .deviceId = " << s.deviceId
				<< " .gpuType = " << s.gpuType
				<< " .gpuName = " << s.gpuName
				<< " .maxMemRefsPerSubmission = " << s.maxMemRefsPerSubmission
				<< " .virtualMemPageSize = " << s.virtualMemPageSize
				<< " .maxInlineMemoryUpdateSize = " << s.maxInlineMemoryUpdateSize
				<< " .maxBoundDescriptorSets = " << s.maxBoundDescriptorSets
				<< " .maxThreadGroupSize = " << s.maxThreadGroupSize
				<< " .timestampFrequency = " << s.timestampFrequency
				<< " .multiColorTargetClears = " << s.multiColorTargetClears
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSDIData)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSDIData*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSDIData {"
				<< " .iSDIState = " << s.iSDIState
				<< " .iSizeofSDISegment = " << s.iSizeofSDISegment
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_FRTCPRO_Settings)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_FRTCPRO_Settings*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_FRTCPRO_Settings {"
				<< " .DefaultState = " << s.DefaultState
				<< " .CurrentState = " << s.CurrentState
				<< " .DefaultValue = " << s.DefaultValue
				<< " .CurrentValue = " << s.CurrentValue
				<< " .maxSupportedFps = " << s.maxSupportedFps
				<< " .minSupportedFps = " << s.minSupportedFps
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_FRTCPRO_CHANGED_REASON)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_FRTCPRO_CHANGED_REASON*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_FRTCPRO_CHANGED_REASON {"
				<< " .StateChanged = " << s.StateChanged
				<< " .ValueChanged = " << s.ValueChanged
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADL_DL_DISPLAY_MODE)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADL_DL_DISPLAY_MODE*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADL_DL_DISPLAY_MODE {"
				<< " .iPelsHeight = " << s.iPelsHeight
				<< " .iPelsWidth = " << s.iPelsWidth
				<< " .iBitsPerPel = " << s.iBitsPerPel
				<< " .iDisplayFrequency = " << s.iDisplayFrequency
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ADLDCESupport)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ADLDCESupport*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "union _ADLDCESupport {"
				<< " .bits = " << DumpGenerated(s.bits)
				<< " .u32All = " << s.u32All
				<< " }";
			return oss.str();
		};
		dumpers[typeid(ADLSmartShiftSettings)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ADLSmartShiftSettings*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct ADLSmartShiftSettings {"
				<< " .iMinRange = " << s.iMinRange
				<< " .iMaxRange = " << s.iMaxRange
				<< " .iDefaultMode = " << s.iDefaultMode
				<< " .iDefaultValue = " << s.iDefaultValue
				<< " .iCurrentMode = " << s.iCurrentMode
				<< " .iCurrentValue = " << s.iCurrentValue
				<< " .iFlags = " << s.iFlags
				<< " }";
			return oss.str();
		};

		// enums
		dumpers[typeid(_ctl_init_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_init_flag_t*>(pEnum);
			switch (e) {
			case _ctl_init_flag_t::CTL_INIT_FLAG_USE_LEVEL_ZERO: return "CTL_INIT_FLAG_USE_LEVEL_ZERO"s;
			case _ctl_init_flag_t::CTL_INIT_FLAG_IGSC_FUL: return "CTL_INIT_FLAG_IGSC_FUL"s;
			case _ctl_init_flag_t::CTL_INIT_FLAG_MAX: return "CTL_INIT_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_property_value_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_property_value_type_t*>(pEnum);
			switch (e) {
			case _ctl_property_value_type_t::CTL_PROPERTY_VALUE_TYPE_BOOL: return "CTL_PROPERTY_VALUE_TYPE_BOOL"s;
			case _ctl_property_value_type_t::CTL_PROPERTY_VALUE_TYPE_FLOAT: return "CTL_PROPERTY_VALUE_TYPE_FLOAT"s;
			case _ctl_property_value_type_t::CTL_PROPERTY_VALUE_TYPE_INT32: return "CTL_PROPERTY_VALUE_TYPE_INT32"s;
			case _ctl_property_value_type_t::CTL_PROPERTY_VALUE_TYPE_UINT32: return "CTL_PROPERTY_VALUE_TYPE_UINT32"s;
			case _ctl_property_value_type_t::CTL_PROPERTY_VALUE_TYPE_ENUM: return "CTL_PROPERTY_VALUE_TYPE_ENUM"s;
			case _ctl_property_value_type_t::CTL_PROPERTY_VALUE_TYPE_CUSTOM: return "CTL_PROPERTY_VALUE_TYPE_CUSTOM"s;
			case _ctl_property_value_type_t::CTL_PROPERTY_VALUE_TYPE_MAX: return "CTL_PROPERTY_VALUE_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
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
		dumpers[typeid(_ctl_supported_functions_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_supported_functions_flag_t*>(pEnum);
			switch (e) {
			case _ctl_supported_functions_flag_t::CTL_SUPPORTED_FUNCTIONS_FLAG_DISPLAY: return "CTL_SUPPORTED_FUNCTIONS_FLAG_DISPLAY"s;
			case _ctl_supported_functions_flag_t::CTL_SUPPORTED_FUNCTIONS_FLAG_3D: return "CTL_SUPPORTED_FUNCTIONS_FLAG_3D"s;
			case _ctl_supported_functions_flag_t::CTL_SUPPORTED_FUNCTIONS_FLAG_MEDIA: return "CTL_SUPPORTED_FUNCTIONS_FLAG_MEDIA"s;
			case _ctl_supported_functions_flag_t::CTL_SUPPORTED_FUNCTIONS_FLAG_MAX: return "CTL_SUPPORTED_FUNCTIONS_FLAG_MAX"s;
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
		dumpers[typeid(_ctl_adapter_properties_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_adapter_properties_flag_t*>(pEnum);
			switch (e) {
			case _ctl_adapter_properties_flag_t::CTL_ADAPTER_PROPERTIES_FLAG_INTEGRATED: return "CTL_ADAPTER_PROPERTIES_FLAG_INTEGRATED"s;
			case _ctl_adapter_properties_flag_t::CTL_ADAPTER_PROPERTIES_FLAG_LDA_PRIMARY: return "CTL_ADAPTER_PROPERTIES_FLAG_LDA_PRIMARY"s;
			case _ctl_adapter_properties_flag_t::CTL_ADAPTER_PROPERTIES_FLAG_LDA_SECONDARY: return "CTL_ADAPTER_PROPERTIES_FLAG_LDA_SECONDARY"s;
			case _ctl_adapter_properties_flag_t::CTL_ADAPTER_PROPERTIES_FLAG_MAX: return "CTL_ADAPTER_PROPERTIES_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_operation_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_operation_type_t*>(pEnum);
			switch (e) {
			case _ctl_operation_type_t::CTL_OPERATION_TYPE_READ: return "CTL_OPERATION_TYPE_READ"s;
			case _ctl_operation_type_t::CTL_OPERATION_TYPE_WRITE: return "CTL_OPERATION_TYPE_WRITE"s;
			case _ctl_operation_type_t::CTL_OPERATION_TYPE_MAX: return "CTL_OPERATION_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_property_type_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_property_type_flag_t*>(pEnum);
			switch (e) {
			case _ctl_property_type_flag_t::CTL_PROPERTY_TYPE_FLAG_DISPLAY: return "CTL_PROPERTY_TYPE_FLAG_DISPLAY"s;
			case _ctl_property_type_flag_t::CTL_PROPERTY_TYPE_FLAG_3D: return "CTL_PROPERTY_TYPE_FLAG_3D"s;
			case _ctl_property_type_flag_t::CTL_PROPERTY_TYPE_FLAG_MEDIA: return "CTL_PROPERTY_TYPE_FLAG_MEDIA"s;
			case _ctl_property_type_flag_t::CTL_PROPERTY_TYPE_FLAG_CORE: return "CTL_PROPERTY_TYPE_FLAG_CORE"s;
			case _ctl_property_type_flag_t::CTL_PROPERTY_TYPE_FLAG_MAX: return "CTL_PROPERTY_TYPE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_orientation_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_orientation_t*>(pEnum);
			switch (e) {
			case _ctl_display_orientation_t::CTL_DISPLAY_ORIENTATION_0: return "CTL_DISPLAY_ORIENTATION_0"s;
			case _ctl_display_orientation_t::CTL_DISPLAY_ORIENTATION_90: return "CTL_DISPLAY_ORIENTATION_90"s;
			case _ctl_display_orientation_t::CTL_DISPLAY_ORIENTATION_180: return "CTL_DISPLAY_ORIENTATION_180"s;
			case _ctl_display_orientation_t::CTL_DISPLAY_ORIENTATION_270: return "CTL_DISPLAY_ORIENTATION_270"s;
			case _ctl_display_orientation_t::CTL_DISPLAY_ORIENTATION_MAX: return "CTL_DISPLAY_ORIENTATION_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_endurance_gaming_control_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_endurance_gaming_control_t*>(pEnum);
			switch (e) {
			case _ctl_3d_endurance_gaming_control_t::CTL_3D_ENDURANCE_GAMING_CONTROL_TURN_OFF: return "CTL_3D_ENDURANCE_GAMING_CONTROL_TURN_OFF"s;
			case _ctl_3d_endurance_gaming_control_t::CTL_3D_ENDURANCE_GAMING_CONTROL_TURN_ON: return "CTL_3D_ENDURANCE_GAMING_CONTROL_TURN_ON"s;
			case _ctl_3d_endurance_gaming_control_t::CTL_3D_ENDURANCE_GAMING_CONTROL_AUTO: return "CTL_3D_ENDURANCE_GAMING_CONTROL_AUTO"s;
			case _ctl_3d_endurance_gaming_control_t::CTL_3D_ENDURANCE_GAMING_CONTROL_MAX: return "CTL_3D_ENDURANCE_GAMING_CONTROL_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_endurance_gaming_mode_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_endurance_gaming_mode_t*>(pEnum);
			switch (e) {
			case _ctl_3d_endurance_gaming_mode_t::CTL_3D_ENDURANCE_GAMING_MODE_BETTER_PERFORMANCE: return "CTL_3D_ENDURANCE_GAMING_MODE_BETTER_PERFORMANCE"s;
			case _ctl_3d_endurance_gaming_mode_t::CTL_3D_ENDURANCE_GAMING_MODE_BALANCED: return "CTL_3D_ENDURANCE_GAMING_MODE_BALANCED"s;
			case _ctl_3d_endurance_gaming_mode_t::CTL_3D_ENDURANCE_GAMING_MODE_MAXIMUM_BATTERY: return "CTL_3D_ENDURANCE_GAMING_MODE_MAXIMUM_BATTERY"s;
			case _ctl_3d_endurance_gaming_mode_t::CTL_3D_ENDURANCE_GAMING_MODE_MAX: return "CTL_3D_ENDURANCE_GAMING_MODE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_tier_type_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_tier_type_flag_t*>(pEnum);
			switch (e) {
			case _ctl_3d_tier_type_flag_t::CTL_3D_TIER_TYPE_FLAG_COMPATIBILITY: return "CTL_3D_TIER_TYPE_FLAG_COMPATIBILITY"s;
			case _ctl_3d_tier_type_flag_t::CTL_3D_TIER_TYPE_FLAG_PERFORMANCE: return "CTL_3D_TIER_TYPE_FLAG_PERFORMANCE"s;
			case _ctl_3d_tier_type_flag_t::CTL_3D_TIER_TYPE_FLAG_MAX: return "CTL_3D_TIER_TYPE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_tier_profile_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_tier_profile_flag_t*>(pEnum);
			switch (e) {
			case _ctl_3d_tier_profile_flag_t::CTL_3D_TIER_PROFILE_FLAG_TIER_1: return "CTL_3D_TIER_PROFILE_FLAG_TIER_1"s;
			case _ctl_3d_tier_profile_flag_t::CTL_3D_TIER_PROFILE_FLAG_TIER_2: return "CTL_3D_TIER_PROFILE_FLAG_TIER_2"s;
			case _ctl_3d_tier_profile_flag_t::CTL_3D_TIER_PROFILE_FLAG_TIER_RECOMMENDED: return "CTL_3D_TIER_PROFILE_FLAG_TIER_RECOMMENDED"s;
			case _ctl_3d_tier_profile_flag_t::CTL_3D_TIER_PROFILE_FLAG_MAX: return "CTL_3D_TIER_PROFILE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_feature_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_feature_t*>(pEnum);
			switch (e) {
			case _ctl_3d_feature_t::CTL_3D_FEATURE_FRAME_PACING: return "CTL_3D_FEATURE_FRAME_PACING"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_ENDURANCE_GAMING: return "CTL_3D_FEATURE_ENDURANCE_GAMING"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_FRAME_LIMIT: return "CTL_3D_FEATURE_FRAME_LIMIT"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_ANISOTROPIC: return "CTL_3D_FEATURE_ANISOTROPIC"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_CMAA: return "CTL_3D_FEATURE_CMAA"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_TEXTURE_FILTERING_QUALITY: return "CTL_3D_FEATURE_TEXTURE_FILTERING_QUALITY"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_ADAPTIVE_TESSELLATION: return "CTL_3D_FEATURE_ADAPTIVE_TESSELLATION"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_SHARPENING_FILTER: return "CTL_3D_FEATURE_SHARPENING_FILTER"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_MSAA: return "CTL_3D_FEATURE_MSAA"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_GAMING_FLIP_MODES: return "CTL_3D_FEATURE_GAMING_FLIP_MODES"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_ADAPTIVE_SYNC_PLUS: return "CTL_3D_FEATURE_ADAPTIVE_SYNC_PLUS"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_APP_PROFILES: return "CTL_3D_FEATURE_APP_PROFILES"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_APP_PROFILE_DETAILS: return "CTL_3D_FEATURE_APP_PROFILE_DETAILS"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_EMULATED_TYPED_64BIT_ATOMICS: return "CTL_3D_FEATURE_EMULATED_TYPED_64BIT_ATOMICS"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_VRR_WINDOWED_BLT: return "CTL_3D_FEATURE_VRR_WINDOWED_BLT"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_GLOBAL_OR_PER_APP: return "CTL_3D_FEATURE_GLOBAL_OR_PER_APP"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_LOW_LATENCY: return "CTL_3D_FEATURE_LOW_LATENCY"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_FRAME_GENERATION: return "CTL_3D_FEATURE_FRAME_GENERATION"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_PREBUILT_SHADER_DOWNLOAD: return "CTL_3D_FEATURE_PREBUILT_SHADER_DOWNLOAD"s;
			case _ctl_3d_feature_t::CTL_3D_FEATURE_MAX: return "CTL_3D_FEATURE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_signal_standard_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_signal_standard_type_t*>(pEnum);
			switch (e) {
			case _ctl_signal_standard_type_t::CTL_SIGNAL_STANDARD_TYPE_UNKNOWN: return "CTL_SIGNAL_STANDARD_TYPE_UNKNOWN"s;
			case _ctl_signal_standard_type_t::CTL_SIGNAL_STANDARD_TYPE_CUSTOM: return "CTL_SIGNAL_STANDARD_TYPE_CUSTOM"s;
			case _ctl_signal_standard_type_t::CTL_SIGNAL_STANDARD_TYPE_DMT: return "CTL_SIGNAL_STANDARD_TYPE_DMT"s;
			case _ctl_signal_standard_type_t::CTL_SIGNAL_STANDARD_TYPE_GTF: return "CTL_SIGNAL_STANDARD_TYPE_GTF"s;
			case _ctl_signal_standard_type_t::CTL_SIGNAL_STANDARD_TYPE_CVT: return "CTL_SIGNAL_STANDARD_TYPE_CVT"s;
			case _ctl_signal_standard_type_t::CTL_SIGNAL_STANDARD_TYPE_CTA: return "CTL_SIGNAL_STANDARD_TYPE_CTA"s;
			case _ctl_signal_standard_type_t::CTL_SIGNAL_STANDARD_TYPE_MAX: return "CTL_SIGNAL_STANDARD_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_output_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_output_types_t*>(pEnum);
			switch (e) {
			case _ctl_display_output_types_t::CTL_DISPLAY_OUTPUT_TYPES_INVALID: return "CTL_DISPLAY_OUTPUT_TYPES_INVALID"s;
			case _ctl_display_output_types_t::CTL_DISPLAY_OUTPUT_TYPES_DISPLAYPORT: return "CTL_DISPLAY_OUTPUT_TYPES_DISPLAYPORT"s;
			case _ctl_display_output_types_t::CTL_DISPLAY_OUTPUT_TYPES_HDMI: return "CTL_DISPLAY_OUTPUT_TYPES_HDMI"s;
			case _ctl_display_output_types_t::CTL_DISPLAY_OUTPUT_TYPES_DVI: return "CTL_DISPLAY_OUTPUT_TYPES_DVI"s;
			case _ctl_display_output_types_t::CTL_DISPLAY_OUTPUT_TYPES_MIPI: return "CTL_DISPLAY_OUTPUT_TYPES_MIPI"s;
			case _ctl_display_output_types_t::CTL_DISPLAY_OUTPUT_TYPES_CRT: return "CTL_DISPLAY_OUTPUT_TYPES_CRT"s;
			case _ctl_display_output_types_t::CTL_DISPLAY_OUTPUT_TYPES_MAX: return "CTL_DISPLAY_OUTPUT_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_attached_display_mux_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_attached_display_mux_type_t*>(pEnum);
			switch (e) {
			case _ctl_attached_display_mux_type_t::CTL_ATTACHED_DISPLAY_MUX_TYPE_NATIVE: return "CTL_ATTACHED_DISPLAY_MUX_TYPE_NATIVE"s;
			case _ctl_attached_display_mux_type_t::CTL_ATTACHED_DISPLAY_MUX_TYPE_THUNDERBOLT: return "CTL_ATTACHED_DISPLAY_MUX_TYPE_THUNDERBOLT"s;
			case _ctl_attached_display_mux_type_t::CTL_ATTACHED_DISPLAY_MUX_TYPE_TYPE_C: return "CTL_ATTACHED_DISPLAY_MUX_TYPE_TYPE_C"s;
			case _ctl_attached_display_mux_type_t::CTL_ATTACHED_DISPLAY_MUX_TYPE_USB4: return "CTL_ATTACHED_DISPLAY_MUX_TYPE_USB4"s;
			case _ctl_attached_display_mux_type_t::CTL_ATTACHED_DISPLAY_MUX_TYPE_MAX: return "CTL_ATTACHED_DISPLAY_MUX_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_power_optimization_plan_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_power_optimization_plan_t*>(pEnum);
			switch (e) {
			case _ctl_power_optimization_plan_t::CTL_POWER_OPTIMIZATION_PLAN_BALANCED: return "CTL_POWER_OPTIMIZATION_PLAN_BALANCED"s;
			case _ctl_power_optimization_plan_t::CTL_POWER_OPTIMIZATION_PLAN_HIGH_PERFORMANCE: return "CTL_POWER_OPTIMIZATION_PLAN_HIGH_PERFORMANCE"s;
			case _ctl_power_optimization_plan_t::CTL_POWER_OPTIMIZATION_PLAN_POWER_SAVER: return "CTL_POWER_OPTIMIZATION_PLAN_POWER_SAVER"s;
			case _ctl_power_optimization_plan_t::CTL_POWER_OPTIMIZATION_PLAN_MAX: return "CTL_POWER_OPTIMIZATION_PLAN_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_power_source_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_power_source_t*>(pEnum);
			switch (e) {
			case _ctl_power_source_t::CTL_POWER_SOURCE_AC: return "CTL_POWER_SOURCE_AC"s;
			case _ctl_power_source_t::CTL_POWER_SOURCE_DC: return "CTL_POWER_SOURCE_DC"s;
			case _ctl_power_source_t::CTL_POWER_SOURCE_MAX: return "CTL_POWER_SOURCE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_gamma_encoding_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_gamma_encoding_type_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_gamma_encoding_type_t::CTL_PIXTX_GAMMA_ENCODING_TYPE_SRGB: return "CTL_PIXTX_GAMMA_ENCODING_TYPE_SRGB"s;
			case _ctl_pixtx_gamma_encoding_type_t::CTL_PIXTX_GAMMA_ENCODING_TYPE_REC709: return "CTL_PIXTX_GAMMA_ENCODING_TYPE_REC709"s;
			case _ctl_pixtx_gamma_encoding_type_t::CTL_PIXTX_GAMMA_ENCODING_TYPE_ST2084: return "CTL_PIXTX_GAMMA_ENCODING_TYPE_ST2084"s;
			case _ctl_pixtx_gamma_encoding_type_t::CTL_PIXTX_GAMMA_ENCODING_TYPE_HLG: return "CTL_PIXTX_GAMMA_ENCODING_TYPE_HLG"s;
			case _ctl_pixtx_gamma_encoding_type_t::CTL_PIXTX_GAMMA_ENCODING_TYPE_LINEAR: return "CTL_PIXTX_GAMMA_ENCODING_TYPE_LINEAR"s;
			case _ctl_pixtx_gamma_encoding_type_t::CTL_PIXTX_GAMMA_ENCODING_TYPE_MAX: return "CTL_PIXTX_GAMMA_ENCODING_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_color_space_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_color_space_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_REC709: return "CTL_PIXTX_COLOR_SPACE_REC709"s;
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_REC2020: return "CTL_PIXTX_COLOR_SPACE_REC2020"s;
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_ADOBE_RGB: return "CTL_PIXTX_COLOR_SPACE_ADOBE_RGB"s;
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_P3_D65: return "CTL_PIXTX_COLOR_SPACE_P3_D65"s;
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_P3_DCI: return "CTL_PIXTX_COLOR_SPACE_P3_DCI"s;
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_P3_D60: return "CTL_PIXTX_COLOR_SPACE_P3_D60"s;
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_CUSTOM: return "CTL_PIXTX_COLOR_SPACE_CUSTOM"s;
			case _ctl_pixtx_color_space_t::CTL_PIXTX_COLOR_SPACE_MAX: return "CTL_PIXTX_COLOR_SPACE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_color_model_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_color_model_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_RGB_FR: return "CTL_PIXTX_COLOR_MODEL_RGB_FR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_RGB_LR: return "CTL_PIXTX_COLOR_MODEL_RGB_LR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_YCBCR_422_FR: return "CTL_PIXTX_COLOR_MODEL_YCBCR_422_FR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_YCBCR_422_LR: return "CTL_PIXTX_COLOR_MODEL_YCBCR_422_LR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_YCBCR_420_FR: return "CTL_PIXTX_COLOR_MODEL_YCBCR_420_FR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_YCBCR_420_LR: return "CTL_PIXTX_COLOR_MODEL_YCBCR_420_LR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_YCBCR_444_FR: return "CTL_PIXTX_COLOR_MODEL_YCBCR_444_FR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_YCBCR_444_LR: return "CTL_PIXTX_COLOR_MODEL_YCBCR_444_LR"s;
			case _ctl_pixtx_color_model_t::CTL_PIXTX_COLOR_MODEL_MAX: return "CTL_PIXTX_COLOR_MODEL_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_lut_sampling_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_lut_sampling_type_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_lut_sampling_type_t::CTL_PIXTX_LUT_SAMPLING_TYPE_UNIFORM: return "CTL_PIXTX_LUT_SAMPLING_TYPE_UNIFORM"s;
			case _ctl_pixtx_lut_sampling_type_t::CTL_PIXTX_LUT_SAMPLING_TYPE_NONUNIFORM: return "CTL_PIXTX_LUT_SAMPLING_TYPE_NONUNIFORM"s;
			case _ctl_pixtx_lut_sampling_type_t::CTL_PIXTX_LUT_SAMPLING_TYPE_MAX: return "CTL_PIXTX_LUT_SAMPLING_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_block_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_block_type_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_block_type_t::CTL_PIXTX_BLOCK_TYPE_1D_LUT: return "CTL_PIXTX_BLOCK_TYPE_1D_LUT"s;
			case _ctl_pixtx_block_type_t::CTL_PIXTX_BLOCK_TYPE_3D_LUT: return "CTL_PIXTX_BLOCK_TYPE_3D_LUT"s;
			case _ctl_pixtx_block_type_t::CTL_PIXTX_BLOCK_TYPE_3X3_MATRIX: return "CTL_PIXTX_BLOCK_TYPE_3X3_MATRIX"s;
			case _ctl_pixtx_block_type_t::CTL_PIXTX_BLOCK_TYPE_3X3_MATRIX_AND_OFFSETS: return "CTL_PIXTX_BLOCK_TYPE_3X3_MATRIX_AND_OFFSETS"s;
			case _ctl_pixtx_block_type_t::CTL_PIXTX_BLOCK_TYPE_MAX: return "CTL_PIXTX_BLOCK_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_config_query_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_config_query_type_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_config_query_type_t::CTL_PIXTX_CONFIG_QUERY_TYPE_CAPABILITY: return "CTL_PIXTX_CONFIG_QUERY_TYPE_CAPABILITY"s;
			case _ctl_pixtx_config_query_type_t::CTL_PIXTX_CONFIG_QUERY_TYPE_CURRENT: return "CTL_PIXTX_CONFIG_QUERY_TYPE_CURRENT"s;
			case _ctl_pixtx_config_query_type_t::CTL_PIXTX_CONFIG_QUERY_TYPE_MAX: return "CTL_PIXTX_CONFIG_QUERY_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_config_opertaion_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_config_opertaion_type_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_config_opertaion_type_t::CTL_PIXTX_CONFIG_OPERTAION_TYPE_RESTORE_DEFAULT: return "CTL_PIXTX_CONFIG_OPERTAION_TYPE_RESTORE_DEFAULT"s;
			case _ctl_pixtx_config_opertaion_type_t::CTL_PIXTX_CONFIG_OPERTAION_TYPE_SET_CUSTOM: return "CTL_PIXTX_CONFIG_OPERTAION_TYPE_SET_CUSTOM"s;
			case _ctl_pixtx_config_opertaion_type_t::CTL_PIXTX_CONFIG_OPERTAION_TYPE_MAX: return "CTL_PIXTX_CONFIG_OPERTAION_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_set_operation_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_set_operation_t*>(pEnum);
			switch (e) {
			case _ctl_set_operation_t::CTL_SET_OPERATION_RESTORE_DEFAULT: return "CTL_SET_OPERATION_RESTORE_DEFAULT"s;
			case _ctl_set_operation_t::CTL_SET_OPERATION_CUSTOM: return "CTL_SET_OPERATION_CUSTOM"s;
			case _ctl_set_operation_t::CTL_SET_OPERATION_MAX: return "CTL_SET_OPERATION_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_intel_arc_sync_profile_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_intel_arc_sync_profile_t*>(pEnum);
			switch (e) {
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_INVALID: return "CTL_INTEL_ARC_SYNC_PROFILE_INVALID"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_RECOMMENDED: return "CTL_INTEL_ARC_SYNC_PROFILE_RECOMMENDED"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_EXCELLENT: return "CTL_INTEL_ARC_SYNC_PROFILE_EXCELLENT"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_GOOD: return "CTL_INTEL_ARC_SYNC_PROFILE_GOOD"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_COMPATIBLE: return "CTL_INTEL_ARC_SYNC_PROFILE_COMPATIBLE"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_OFF: return "CTL_INTEL_ARC_SYNC_PROFILE_OFF"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_VESA: return "CTL_INTEL_ARC_SYNC_PROFILE_VESA"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_CUSTOM: return "CTL_INTEL_ARC_SYNC_PROFILE_CUSTOM"s;
			case _ctl_intel_arc_sync_profile_t::CTL_INTEL_ARC_SYNC_PROFILE_MAX: return "CTL_INTEL_ARC_SYNC_PROFILE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_edid_management_optype_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_edid_management_optype_t*>(pEnum);
			switch (e) {
			case _ctl_edid_management_optype_t::CTL_EDID_MANAGEMENT_OPTYPE_READ_EDID: return "CTL_EDID_MANAGEMENT_OPTYPE_READ_EDID"s;
			case _ctl_edid_management_optype_t::CTL_EDID_MANAGEMENT_OPTYPE_LOCK_EDID: return "CTL_EDID_MANAGEMENT_OPTYPE_LOCK_EDID"s;
			case _ctl_edid_management_optype_t::CTL_EDID_MANAGEMENT_OPTYPE_UNLOCK_EDID: return "CTL_EDID_MANAGEMENT_OPTYPE_UNLOCK_EDID"s;
			case _ctl_edid_management_optype_t::CTL_EDID_MANAGEMENT_OPTYPE_OVERRIDE_EDID: return "CTL_EDID_MANAGEMENT_OPTYPE_OVERRIDE_EDID"s;
			case _ctl_edid_management_optype_t::CTL_EDID_MANAGEMENT_OPTYPE_UNDO_OVERRIDE_EDID: return "CTL_EDID_MANAGEMENT_OPTYPE_UNDO_OVERRIDE_EDID"s;
			case _ctl_edid_management_optype_t::CTL_EDID_MANAGEMENT_OPTYPE_MAX: return "CTL_EDID_MANAGEMENT_OPTYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_edid_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_edid_type_t*>(pEnum);
			switch (e) {
			case _ctl_edid_type_t::CTL_EDID_TYPE_CURRENT: return "CTL_EDID_TYPE_CURRENT"s;
			case _ctl_edid_type_t::CTL_EDID_TYPE_OVERRIDE: return "CTL_EDID_TYPE_OVERRIDE"s;
			case _ctl_edid_type_t::CTL_EDID_TYPE_MONITOR: return "CTL_EDID_TYPE_MONITOR"s;
			case _ctl_edid_type_t::CTL_EDID_TYPE_MAX: return "CTL_EDID_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_custom_mode_operation_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_custom_mode_operation_types_t*>(pEnum);
			switch (e) {
			case _ctl_custom_mode_operation_types_t::CTL_CUSTOM_MODE_OPERATION_TYPES_GET_CUSTOM_SOURCE_MODES: return "CTL_CUSTOM_MODE_OPERATION_TYPES_GET_CUSTOM_SOURCE_MODES"s;
			case _ctl_custom_mode_operation_types_t::CTL_CUSTOM_MODE_OPERATION_TYPES_ADD_CUSTOM_SOURCE_MODE: return "CTL_CUSTOM_MODE_OPERATION_TYPES_ADD_CUSTOM_SOURCE_MODE"s;
			case _ctl_custom_mode_operation_types_t::CTL_CUSTOM_MODE_OPERATION_TYPES_REMOVE_CUSTOM_SOURCE_MODES: return "CTL_CUSTOM_MODE_OPERATION_TYPES_REMOVE_CUSTOM_SOURCE_MODES"s;
			case _ctl_custom_mode_operation_types_t::CTL_CUSTOM_MODE_OPERATION_TYPES_MAX: return "CTL_CUSTOM_MODE_OPERATION_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_combined_display_optype_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_combined_display_optype_t*>(pEnum);
			switch (e) {
			case _ctl_combined_display_optype_t::CTL_COMBINED_DISPLAY_OPTYPE_IS_SUPPORTED_CONFIG: return "CTL_COMBINED_DISPLAY_OPTYPE_IS_SUPPORTED_CONFIG"s;
			case _ctl_combined_display_optype_t::CTL_COMBINED_DISPLAY_OPTYPE_ENABLE: return "CTL_COMBINED_DISPLAY_OPTYPE_ENABLE"s;
			case _ctl_combined_display_optype_t::CTL_COMBINED_DISPLAY_OPTYPE_DISABLE: return "CTL_COMBINED_DISPLAY_OPTYPE_DISABLE"s;
			case _ctl_combined_display_optype_t::CTL_COMBINED_DISPLAY_OPTYPE_QUERY_CONFIG: return "CTL_COMBINED_DISPLAY_OPTYPE_QUERY_CONFIG"s;
			case _ctl_combined_display_optype_t::CTL_COMBINED_DISPLAY_OPTYPE_MAX: return "CTL_COMBINED_DISPLAY_OPTYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_genlock_operation_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_genlock_operation_t*>(pEnum);
			switch (e) {
			case _ctl_genlock_operation_t::CTL_GENLOCK_OPERATION_GET_TIMING_DETAILS: return "CTL_GENLOCK_OPERATION_GET_TIMING_DETAILS"s;
			case _ctl_genlock_operation_t::CTL_GENLOCK_OPERATION_VALIDATE: return "CTL_GENLOCK_OPERATION_VALIDATE"s;
			case _ctl_genlock_operation_t::CTL_GENLOCK_OPERATION_ENABLE: return "CTL_GENLOCK_OPERATION_ENABLE"s;
			case _ctl_genlock_operation_t::CTL_GENLOCK_OPERATION_DISABLE: return "CTL_GENLOCK_OPERATION_DISABLE"s;
			case _ctl_genlock_operation_t::CTL_GENLOCK_OPERATION_GET_TOPOLOGY: return "CTL_GENLOCK_OPERATION_GET_TOPOLOGY"s;
			case _ctl_genlock_operation_t::CTL_GENLOCK_OPERATION_MAX: return "CTL_GENLOCK_OPERATION_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_wire_format_color_model_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_wire_format_color_model_t*>(pEnum);
			switch (e) {
			case _ctl_wire_format_color_model_t::CTL_WIRE_FORMAT_COLOR_MODEL_RGB: return "CTL_WIRE_FORMAT_COLOR_MODEL_RGB"s;
			case _ctl_wire_format_color_model_t::CTL_WIRE_FORMAT_COLOR_MODEL_YCBCR_420: return "CTL_WIRE_FORMAT_COLOR_MODEL_YCBCR_420"s;
			case _ctl_wire_format_color_model_t::CTL_WIRE_FORMAT_COLOR_MODEL_YCBCR_422: return "CTL_WIRE_FORMAT_COLOR_MODEL_YCBCR_422"s;
			case _ctl_wire_format_color_model_t::CTL_WIRE_FORMAT_COLOR_MODEL_YCBCR_444: return "CTL_WIRE_FORMAT_COLOR_MODEL_YCBCR_444"s;
			case _ctl_wire_format_color_model_t::CTL_WIRE_FORMAT_COLOR_MODEL_MAX: return "CTL_WIRE_FORMAT_COLOR_MODEL_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_wire_format_operation_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_wire_format_operation_type_t*>(pEnum);
			switch (e) {
			case _ctl_wire_format_operation_type_t::CTL_WIRE_FORMAT_OPERATION_TYPE_GET: return "CTL_WIRE_FORMAT_OPERATION_TYPE_GET"s;
			case _ctl_wire_format_operation_type_t::CTL_WIRE_FORMAT_OPERATION_TYPE_SET: return "CTL_WIRE_FORMAT_OPERATION_TYPE_SET"s;
			case _ctl_wire_format_operation_type_t::CTL_WIRE_FORMAT_OPERATION_TYPE_RESTORE_DEFAULT: return "CTL_WIRE_FORMAT_OPERATION_TYPE_RESTORE_DEFAULT"s;
			case _ctl_wire_format_operation_type_t::CTL_WIRE_FORMAT_OPERATION_TYPE_MAX: return "CTL_WIRE_FORMAT_OPERATION_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_setting_low_latency_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_setting_low_latency_t*>(pEnum);
			switch (e) {
			case _ctl_display_setting_low_latency_t::CTL_DISPLAY_SETTING_LOW_LATENCY_DEFAULT: return "CTL_DISPLAY_SETTING_LOW_LATENCY_DEFAULT"s;
			case _ctl_display_setting_low_latency_t::CTL_DISPLAY_SETTING_LOW_LATENCY_DISABLED: return "CTL_DISPLAY_SETTING_LOW_LATENCY_DISABLED"s;
			case _ctl_display_setting_low_latency_t::CTL_DISPLAY_SETTING_LOW_LATENCY_ENABLED: return "CTL_DISPLAY_SETTING_LOW_LATENCY_ENABLED"s;
			case _ctl_display_setting_low_latency_t::CTL_DISPLAY_SETTING_LOW_LATENCY_MAX: return "CTL_DISPLAY_SETTING_LOW_LATENCY_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_setting_sourcetm_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_setting_sourcetm_t*>(pEnum);
			switch (e) {
			case _ctl_display_setting_sourcetm_t::CTL_DISPLAY_SETTING_SOURCETM_DEFAULT: return "CTL_DISPLAY_SETTING_SOURCETM_DEFAULT"s;
			case _ctl_display_setting_sourcetm_t::CTL_DISPLAY_SETTING_SOURCETM_DISABLED: return "CTL_DISPLAY_SETTING_SOURCETM_DISABLED"s;
			case _ctl_display_setting_sourcetm_t::CTL_DISPLAY_SETTING_SOURCETM_ENABLED: return "CTL_DISPLAY_SETTING_SOURCETM_ENABLED"s;
			case _ctl_display_setting_sourcetm_t::CTL_DISPLAY_SETTING_SOURCETM_MAX: return "CTL_DISPLAY_SETTING_SOURCETM_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_setting_content_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_setting_content_type_t*>(pEnum);
			switch (e) {
			case _ctl_display_setting_content_type_t::CTL_DISPLAY_SETTING_CONTENT_TYPE_DEFAULT: return "CTL_DISPLAY_SETTING_CONTENT_TYPE_DEFAULT"s;
			case _ctl_display_setting_content_type_t::CTL_DISPLAY_SETTING_CONTENT_TYPE_DISABLED: return "CTL_DISPLAY_SETTING_CONTENT_TYPE_DISABLED"s;
			case _ctl_display_setting_content_type_t::CTL_DISPLAY_SETTING_CONTENT_TYPE_DESKTOP: return "CTL_DISPLAY_SETTING_CONTENT_TYPE_DESKTOP"s;
			case _ctl_display_setting_content_type_t::CTL_DISPLAY_SETTING_CONTENT_TYPE_MEDIA: return "CTL_DISPLAY_SETTING_CONTENT_TYPE_MEDIA"s;
			case _ctl_display_setting_content_type_t::CTL_DISPLAY_SETTING_CONTENT_TYPE_GAMING: return "CTL_DISPLAY_SETTING_CONTENT_TYPE_GAMING"s;
			case _ctl_display_setting_content_type_t::CTL_DISPLAY_SETTING_CONTENT_TYPE_MAX: return "CTL_DISPLAY_SETTING_CONTENT_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_setting_quantization_range_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_setting_quantization_range_t*>(pEnum);
			switch (e) {
			case _ctl_display_setting_quantization_range_t::CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_DEFAULT: return "CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_DEFAULT"s;
			case _ctl_display_setting_quantization_range_t::CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_LIMITED_RANGE: return "CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_LIMITED_RANGE"s;
			case _ctl_display_setting_quantization_range_t::CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_FULL_RANGE: return "CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_FULL_RANGE"s;
			case _ctl_display_setting_quantization_range_t::CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_MAX: return "CTL_DISPLAY_SETTING_QUANTIZATION_RANGE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_setting_picture_ar_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_setting_picture_ar_flag_t*>(pEnum);
			switch (e) {
			case _ctl_display_setting_picture_ar_flag_t::CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_DEFAULT: return "CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_DEFAULT"s;
			case _ctl_display_setting_picture_ar_flag_t::CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_DISABLED: return "CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_DISABLED"s;
			case _ctl_display_setting_picture_ar_flag_t::CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_4_3: return "CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_4_3"s;
			case _ctl_display_setting_picture_ar_flag_t::CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_16_9: return "CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_16_9"s;
			case _ctl_display_setting_picture_ar_flag_t::CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_64_27: return "CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_64_27"s;
			case _ctl_display_setting_picture_ar_flag_t::CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_256_135: return "CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_AR_256_135"s;
			case _ctl_display_setting_picture_ar_flag_t::CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_MAX: return "CTL_DISPLAY_SETTING_PICTURE_AR_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_setting_audio_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_setting_audio_t*>(pEnum);
			switch (e) {
			case _ctl_display_setting_audio_t::CTL_DISPLAY_SETTING_AUDIO_DEFAULT: return "CTL_DISPLAY_SETTING_AUDIO_DEFAULT"s;
			case _ctl_display_setting_audio_t::CTL_DISPLAY_SETTING_AUDIO_DISABLED: return "CTL_DISPLAY_SETTING_AUDIO_DISABLED"s;
			case _ctl_display_setting_audio_t::CTL_DISPLAY_SETTING_AUDIO_MAX: return "CTL_DISPLAY_SETTING_AUDIO_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_ecc_state_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_ecc_state_t*>(pEnum);
			switch (e) {
			case _ctl_ecc_state_t::CTL_ECC_STATE_ECC_DEFAULT_STATE: return "CTL_ECC_STATE_ECC_DEFAULT_STATE"s;
			case _ctl_ecc_state_t::CTL_ECC_STATE_ECC_ENABLED_STATE: return "CTL_ECC_STATE_ECC_ENABLED_STATE"s;
			case _ctl_ecc_state_t::CTL_ECC_STATE_ECC_DISABLED_STATE: return "CTL_ECC_STATE_ECC_DISABLED_STATE"s;
			case _ctl_ecc_state_t::CTL_ECC_STATE_MAX: return "CTL_ECC_STATE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_engine_group_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_engine_group_t*>(pEnum);
			switch (e) {
			case _ctl_engine_group_t::CTL_ENGINE_GROUP_GT: return "CTL_ENGINE_GROUP_GT"s;
			case _ctl_engine_group_t::CTL_ENGINE_GROUP_RENDER: return "CTL_ENGINE_GROUP_RENDER"s;
			case _ctl_engine_group_t::CTL_ENGINE_GROUP_MEDIA: return "CTL_ENGINE_GROUP_MEDIA"s;
			case _ctl_engine_group_t::CTL_ENGINE_GROUP_MAX: return "CTL_ENGINE_GROUP_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_fan_speed_units_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_fan_speed_units_t*>(pEnum);
			switch (e) {
			case _ctl_fan_speed_units_t::CTL_FAN_SPEED_UNITS_RPM: return "CTL_FAN_SPEED_UNITS_RPM"s;
			case _ctl_fan_speed_units_t::CTL_FAN_SPEED_UNITS_PERCENT: return "CTL_FAN_SPEED_UNITS_PERCENT"s;
			case _ctl_fan_speed_units_t::CTL_FAN_SPEED_UNITS_MAX: return "CTL_FAN_SPEED_UNITS_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_fan_speed_mode_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_fan_speed_mode_t*>(pEnum);
			switch (e) {
			case _ctl_fan_speed_mode_t::CTL_FAN_SPEED_MODE_DEFAULT: return "CTL_FAN_SPEED_MODE_DEFAULT"s;
			case _ctl_fan_speed_mode_t::CTL_FAN_SPEED_MODE_FIXED: return "CTL_FAN_SPEED_MODE_FIXED"s;
			case _ctl_fan_speed_mode_t::CTL_FAN_SPEED_MODE_TABLE: return "CTL_FAN_SPEED_MODE_TABLE"s;
			case _ctl_fan_speed_mode_t::CTL_FAN_SPEED_MODE_MAX: return "CTL_FAN_SPEED_MODE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_freq_domain_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_freq_domain_t*>(pEnum);
			switch (e) {
			case _ctl_freq_domain_t::CTL_FREQ_DOMAIN_GPU: return "CTL_FREQ_DOMAIN_GPU"s;
			case _ctl_freq_domain_t::CTL_FREQ_DOMAIN_MEMORY: return "CTL_FREQ_DOMAIN_MEMORY"s;
			case _ctl_freq_domain_t::CTL_FREQ_DOMAIN_MEDIA: return "CTL_FREQ_DOMAIN_MEDIA"s;
			case _ctl_freq_domain_t::CTL_FREQ_DOMAIN_MAX: return "CTL_FREQ_DOMAIN_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_video_processing_feature_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_video_processing_feature_t*>(pEnum);
			switch (e) {
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_FILM_MODE_DETECTION: return "CTL_VIDEO_PROCESSING_FEATURE_FILM_MODE_DETECTION"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_NOISE_REDUCTION: return "CTL_VIDEO_PROCESSING_FEATURE_NOISE_REDUCTION"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_SHARPNESS: return "CTL_VIDEO_PROCESSING_FEATURE_SHARPNESS"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_ADAPTIVE_CONTRAST_ENHANCEMENT: return "CTL_VIDEO_PROCESSING_FEATURE_ADAPTIVE_CONTRAST_ENHANCEMENT"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_SUPER_RESOLUTION: return "CTL_VIDEO_PROCESSING_FEATURE_SUPER_RESOLUTION"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_STANDARD_COLOR_CORRECTION: return "CTL_VIDEO_PROCESSING_FEATURE_STANDARD_COLOR_CORRECTION"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_TOTAL_COLOR_CORRECTION: return "CTL_VIDEO_PROCESSING_FEATURE_TOTAL_COLOR_CORRECTION"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_SKIN_TONE_ENHANCEMENT: return "CTL_VIDEO_PROCESSING_FEATURE_SKIN_TONE_ENHANCEMENT"s;
			case _ctl_video_processing_feature_t::CTL_VIDEO_PROCESSING_FEATURE_MAX: return "CTL_VIDEO_PROCESSING_FEATURE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_mem_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_mem_type_t*>(pEnum);
			switch (e) {
			case _ctl_mem_type_t::CTL_MEM_TYPE_HBM: return "CTL_MEM_TYPE_HBM"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_DDR: return "CTL_MEM_TYPE_DDR"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_DDR3: return "CTL_MEM_TYPE_DDR3"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_DDR4: return "CTL_MEM_TYPE_DDR4"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_DDR5: return "CTL_MEM_TYPE_DDR5"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_LPDDR: return "CTL_MEM_TYPE_LPDDR"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_LPDDR3: return "CTL_MEM_TYPE_LPDDR3"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_LPDDR4: return "CTL_MEM_TYPE_LPDDR4"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_LPDDR5: return "CTL_MEM_TYPE_LPDDR5"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_GDDR4: return "CTL_MEM_TYPE_GDDR4"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_GDDR5: return "CTL_MEM_TYPE_GDDR5"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_GDDR5X: return "CTL_MEM_TYPE_GDDR5X"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_GDDR6: return "CTL_MEM_TYPE_GDDR6"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_GDDR6X: return "CTL_MEM_TYPE_GDDR6X"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_GDDR7: return "CTL_MEM_TYPE_GDDR7"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_UNKNOWN: return "CTL_MEM_TYPE_UNKNOWN"s;
			case _ctl_mem_type_t::CTL_MEM_TYPE_MAX: return "CTL_MEM_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_mem_loc_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_mem_loc_t*>(pEnum);
			switch (e) {
			case _ctl_mem_loc_t::CTL_MEM_LOC_SYSTEM: return "CTL_MEM_LOC_SYSTEM"s;
			case _ctl_mem_loc_t::CTL_MEM_LOC_DEVICE: return "CTL_MEM_LOC_DEVICE"s;
			case _ctl_mem_loc_t::CTL_MEM_LOC_MAX: return "CTL_MEM_LOC_MAX"s;
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
		dumpers[typeid(_ctl_temp_sensors_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_temp_sensors_t*>(pEnum);
			switch (e) {
			case _ctl_temp_sensors_t::CTL_TEMP_SENSORS_GLOBAL: return "CTL_TEMP_SENSORS_GLOBAL"s;
			case _ctl_temp_sensors_t::CTL_TEMP_SENSORS_GPU: return "CTL_TEMP_SENSORS_GPU"s;
			case _ctl_temp_sensors_t::CTL_TEMP_SENSORS_MEMORY: return "CTL_TEMP_SENSORS_MEMORY"s;
			case _ctl_temp_sensors_t::CTL_TEMP_SENSORS_GLOBAL_MIN: return "CTL_TEMP_SENSORS_GLOBAL_MIN"s;
			case _ctl_temp_sensors_t::CTL_TEMP_SENSORS_GPU_MIN: return "CTL_TEMP_SENSORS_GPU_MIN"s;
			case _ctl_temp_sensors_t::CTL_TEMP_SENSORS_MEMORY_MIN: return "CTL_TEMP_SENSORS_MEMORY_MIN"s;
			case _ctl_temp_sensors_t::CTL_TEMP_SENSORS_MAX: return "CTL_TEMP_SENSORS_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_feature_misc_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_feature_misc_flag_t*>(pEnum);
			switch (e) {
			case _ctl_3d_feature_misc_flag_t::CTL_3D_FEATURE_MISC_FLAG_DX9: return "CTL_3D_FEATURE_MISC_FLAG_DX9"s;
			case _ctl_3d_feature_misc_flag_t::CTL_3D_FEATURE_MISC_FLAG_DX11: return "CTL_3D_FEATURE_MISC_FLAG_DX11"s;
			case _ctl_3d_feature_misc_flag_t::CTL_3D_FEATURE_MISC_FLAG_DX12: return "CTL_3D_FEATURE_MISC_FLAG_DX12"s;
			case _ctl_3d_feature_misc_flag_t::CTL_3D_FEATURE_MISC_FLAG_VULKAN: return "CTL_3D_FEATURE_MISC_FLAG_VULKAN"s;
			case _ctl_3d_feature_misc_flag_t::CTL_3D_FEATURE_MISC_FLAG_LIVE_CHANGE: return "CTL_3D_FEATURE_MISC_FLAG_LIVE_CHANGE"s;
			case _ctl_3d_feature_misc_flag_t::CTL_3D_FEATURE_MISC_FLAG_MAX: return "CTL_3D_FEATURE_MISC_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_anisotropic_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_anisotropic_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_anisotropic_types_t::CTL_3D_ANISOTROPIC_TYPES_APP_CHOICE: return "CTL_3D_ANISOTROPIC_TYPES_APP_CHOICE"s;
			case _ctl_3d_anisotropic_types_t::CTL_3D_ANISOTROPIC_TYPES_2X: return "CTL_3D_ANISOTROPIC_TYPES_2X"s;
			case _ctl_3d_anisotropic_types_t::CTL_3D_ANISOTROPIC_TYPES_4X: return "CTL_3D_ANISOTROPIC_TYPES_4X"s;
			case _ctl_3d_anisotropic_types_t::CTL_3D_ANISOTROPIC_TYPES_8X: return "CTL_3D_ANISOTROPIC_TYPES_8X"s;
			case _ctl_3d_anisotropic_types_t::CTL_3D_ANISOTROPIC_TYPES_16X: return "CTL_3D_ANISOTROPIC_TYPES_16X"s;
			case _ctl_3d_anisotropic_types_t::CTL_3D_ANISOTROPIC_TYPES_MAX: return "CTL_3D_ANISOTROPIC_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_texture_filtering_quality_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_texture_filtering_quality_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_texture_filtering_quality_types_t::CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_PERFORMANCE: return "CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_PERFORMANCE"s;
			case _ctl_3d_texture_filtering_quality_types_t::CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_BALANCED: return "CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_BALANCED"s;
			case _ctl_3d_texture_filtering_quality_types_t::CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_QUALITY: return "CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_QUALITY"s;
			case _ctl_3d_texture_filtering_quality_types_t::CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_MAX: return "CTL_3D_TEXTURE_FILTERING_QUALITY_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_frame_pacing_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_frame_pacing_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_frame_pacing_types_t::CTL_3D_FRAME_PACING_TYPES_DISABLE: return "CTL_3D_FRAME_PACING_TYPES_DISABLE"s;
			case _ctl_3d_frame_pacing_types_t::CTL_3D_FRAME_PACING_TYPES_ENABLE_MODE_FRAME_NO_SMOOTHENING: return "CTL_3D_FRAME_PACING_TYPES_ENABLE_MODE_FRAME_NO_SMOOTHENING"s;
			case _ctl_3d_frame_pacing_types_t::CTL_3D_FRAME_PACING_TYPES_ENABLE_MODE_FRAME_MAX_SMOOTHENING: return "CTL_3D_FRAME_PACING_TYPES_ENABLE_MODE_FRAME_MAX_SMOOTHENING"s;
			case _ctl_3d_frame_pacing_types_t::CTL_3D_FRAME_PACING_TYPES_ENABLE_MODE_COMPETITIVE_GAMING: return "CTL_3D_FRAME_PACING_TYPES_ENABLE_MODE_COMPETITIVE_GAMING"s;
			case _ctl_3d_frame_pacing_types_t::CTL_3D_FRAME_PACING_TYPES_MAX: return "CTL_3D_FRAME_PACING_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_low_latency_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_low_latency_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_low_latency_types_t::CTL_3D_LOW_LATENCY_TYPES_TURN_OFF: return "CTL_3D_LOW_LATENCY_TYPES_TURN_OFF"s;
			case _ctl_3d_low_latency_types_t::CTL_3D_LOW_LATENCY_TYPES_TURN_ON: return "CTL_3D_LOW_LATENCY_TYPES_TURN_ON"s;
			case _ctl_3d_low_latency_types_t::CTL_3D_LOW_LATENCY_TYPES_TURN_ON_BOOST_MODE_ON: return "CTL_3D_LOW_LATENCY_TYPES_TURN_ON_BOOST_MODE_ON"s;
			case _ctl_3d_low_latency_types_t::CTL_3D_LOW_LATENCY_TYPES_MAX: return "CTL_3D_LOW_LATENCY_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_cmaa_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_cmaa_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_cmaa_types_t::CTL_3D_CMAA_TYPES_TURN_OFF: return "CTL_3D_CMAA_TYPES_TURN_OFF"s;
			case _ctl_3d_cmaa_types_t::CTL_3D_CMAA_TYPES_OVERRIDE_MSAA: return "CTL_3D_CMAA_TYPES_OVERRIDE_MSAA"s;
			case _ctl_3d_cmaa_types_t::CTL_3D_CMAA_TYPES_ENHANCE_APPLICATION: return "CTL_3D_CMAA_TYPES_ENHANCE_APPLICATION"s;
			case _ctl_3d_cmaa_types_t::CTL_3D_CMAA_TYPES_MAX: return "CTL_3D_CMAA_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_adaptive_tessellation_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_adaptive_tessellation_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_adaptive_tessellation_types_t::CTL_3D_ADAPTIVE_TESSELLATION_TYPES_TURN_OFF: return "CTL_3D_ADAPTIVE_TESSELLATION_TYPES_TURN_OFF"s;
			case _ctl_3d_adaptive_tessellation_types_t::CTL_3D_ADAPTIVE_TESSELLATION_TYPES_TURN_ON: return "CTL_3D_ADAPTIVE_TESSELLATION_TYPES_TURN_ON"s;
			case _ctl_3d_adaptive_tessellation_types_t::CTL_3D_ADAPTIVE_TESSELLATION_TYPES_MAX: return "CTL_3D_ADAPTIVE_TESSELLATION_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_sharpening_filter_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_sharpening_filter_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_sharpening_filter_types_t::CTL_3D_SHARPENING_FILTER_TYPES_TURN_OFF: return "CTL_3D_SHARPENING_FILTER_TYPES_TURN_OFF"s;
			case _ctl_3d_sharpening_filter_types_t::CTL_3D_SHARPENING_FILTER_TYPES_TURN_ON: return "CTL_3D_SHARPENING_FILTER_TYPES_TURN_ON"s;
			case _ctl_3d_sharpening_filter_types_t::CTL_3D_SHARPENING_FILTER_TYPES_MAX: return "CTL_3D_SHARPENING_FILTER_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_msaa_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_msaa_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_msaa_types_t::CTL_3D_MSAA_TYPES_APP_CHOICE: return "CTL_3D_MSAA_TYPES_APP_CHOICE"s;
			case _ctl_3d_msaa_types_t::CTL_3D_MSAA_TYPES_DISABLED: return "CTL_3D_MSAA_TYPES_DISABLED"s;
			case _ctl_3d_msaa_types_t::CTL_3D_MSAA_TYPES_2X: return "CTL_3D_MSAA_TYPES_2X"s;
			case _ctl_3d_msaa_types_t::CTL_3D_MSAA_TYPES_4X: return "CTL_3D_MSAA_TYPES_4X"s;
			case _ctl_3d_msaa_types_t::CTL_3D_MSAA_TYPES_8X: return "CTL_3D_MSAA_TYPES_8X"s;
			case _ctl_3d_msaa_types_t::CTL_3D_MSAA_TYPES_16X: return "CTL_3D_MSAA_TYPES_16X"s;
			case _ctl_3d_msaa_types_t::CTL_3D_MSAA_TYPES_MAX: return "CTL_3D_MSAA_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_gaming_flip_mode_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_gaming_flip_mode_flag_t*>(pEnum);
			switch (e) {
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_APPLICATION_DEFAULT: return "CTL_GAMING_FLIP_MODE_FLAG_APPLICATION_DEFAULT"s;
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_VSYNC_OFF: return "CTL_GAMING_FLIP_MODE_FLAG_VSYNC_OFF"s;
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_VSYNC_ON: return "CTL_GAMING_FLIP_MODE_FLAG_VSYNC_ON"s;
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_SMOOTH_SYNC: return "CTL_GAMING_FLIP_MODE_FLAG_SMOOTH_SYNC"s;
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_SPEED_FRAME: return "CTL_GAMING_FLIP_MODE_FLAG_SPEED_FRAME"s;
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_CAPPED_FPS: return "CTL_GAMING_FLIP_MODE_FLAG_CAPPED_FPS"s;
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_VSYNC_OFF_IGNORE_ALLOW_LIST: return "CTL_GAMING_FLIP_MODE_FLAG_VSYNC_OFF_IGNORE_ALLOW_LIST"s;
			case _ctl_gaming_flip_mode_flag_t::CTL_GAMING_FLIP_MODE_FLAG_MAX: return "CTL_GAMING_FLIP_MODE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_emulated_typed_64bit_atomics_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_emulated_typed_64bit_atomics_types_t*>(pEnum);
			switch (e) {
			case _ctl_emulated_typed_64bit_atomics_types_t::CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_DEFAULT: return "CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_DEFAULT"s;
			case _ctl_emulated_typed_64bit_atomics_types_t::CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_TURN_ON: return "CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_TURN_ON"s;
			case _ctl_emulated_typed_64bit_atomics_types_t::CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_TURN_OFF: return "CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_TURN_OFF"s;
			case _ctl_emulated_typed_64bit_atomics_types_t::CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_MAX: return "CTL_EMULATED_TYPED_64BIT_ATOMICS_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_vrr_windowed_blt_reserved_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_vrr_windowed_blt_reserved_t*>(pEnum);
			switch (e) {
			case _ctl_3d_vrr_windowed_blt_reserved_t::CTL_3D_VRR_WINDOWED_BLT_RESERVED_AUTO: return "CTL_3D_VRR_WINDOWED_BLT_RESERVED_AUTO"s;
			case _ctl_3d_vrr_windowed_blt_reserved_t::CTL_3D_VRR_WINDOWED_BLT_RESERVED_TURN_ON: return "CTL_3D_VRR_WINDOWED_BLT_RESERVED_TURN_ON"s;
			case _ctl_3d_vrr_windowed_blt_reserved_t::CTL_3D_VRR_WINDOWED_BLT_RESERVED_TURN_OFF: return "CTL_3D_VRR_WINDOWED_BLT_RESERVED_TURN_OFF"s;
			case _ctl_3d_vrr_windowed_blt_reserved_t::CTL_3D_VRR_WINDOWED_BLT_RESERVED_MAX: return "CTL_3D_VRR_WINDOWED_BLT_RESERVED_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_3d_global_or_per_app_types_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_3d_global_or_per_app_types_t*>(pEnum);
			switch (e) {
			case _ctl_3d_global_or_per_app_types_t::CTL_3D_GLOBAL_OR_PER_APP_TYPES_NONE: return "CTL_3D_GLOBAL_OR_PER_APP_TYPES_NONE"s;
			case _ctl_3d_global_or_per_app_types_t::CTL_3D_GLOBAL_OR_PER_APP_TYPES_PER_APP: return "CTL_3D_GLOBAL_OR_PER_APP_TYPES_PER_APP"s;
			case _ctl_3d_global_or_per_app_types_t::CTL_3D_GLOBAL_OR_PER_APP_TYPES_GLOBAL: return "CTL_3D_GLOBAL_OR_PER_APP_TYPES_GLOBAL"s;
			case _ctl_3d_global_or_per_app_types_t::CTL_3D_GLOBAL_OR_PER_APP_TYPES_MAX: return "CTL_3D_GLOBAL_OR_PER_APP_TYPES_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_output_bpc_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_output_bpc_flag_t*>(pEnum);
			switch (e) {
			case _ctl_output_bpc_flag_t::CTL_OUTPUT_BPC_FLAG_6BPC: return "CTL_OUTPUT_BPC_FLAG_6BPC"s;
			case _ctl_output_bpc_flag_t::CTL_OUTPUT_BPC_FLAG_8BPC: return "CTL_OUTPUT_BPC_FLAG_8BPC"s;
			case _ctl_output_bpc_flag_t::CTL_OUTPUT_BPC_FLAG_10BPC: return "CTL_OUTPUT_BPC_FLAG_10BPC"s;
			case _ctl_output_bpc_flag_t::CTL_OUTPUT_BPC_FLAG_12BPC: return "CTL_OUTPUT_BPC_FLAG_12BPC"s;
			case _ctl_output_bpc_flag_t::CTL_OUTPUT_BPC_FLAG_MAX: return "CTL_OUTPUT_BPC_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_std_display_feature_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_std_display_feature_flag_t*>(pEnum);
			switch (e) {
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_HDCP: return "CTL_STD_DISPLAY_FEATURE_FLAG_HDCP"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_HD_AUDIO: return "CTL_STD_DISPLAY_FEATURE_FLAG_HD_AUDIO"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_PSR: return "CTL_STD_DISPLAY_FEATURE_FLAG_PSR"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_ADAPTIVESYNC_VRR: return "CTL_STD_DISPLAY_FEATURE_FLAG_ADAPTIVESYNC_VRR"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_VESA_COMPRESSION: return "CTL_STD_DISPLAY_FEATURE_FLAG_VESA_COMPRESSION"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_HDR: return "CTL_STD_DISPLAY_FEATURE_FLAG_HDR"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_HDMI_QMS: return "CTL_STD_DISPLAY_FEATURE_FLAG_HDMI_QMS"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_HDR10_PLUS_CERTIFIED: return "CTL_STD_DISPLAY_FEATURE_FLAG_HDR10_PLUS_CERTIFIED"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_VESA_HDR_CERTIFIED: return "CTL_STD_DISPLAY_FEATURE_FLAG_VESA_HDR_CERTIFIED"s;
			case _ctl_std_display_feature_flag_t::CTL_STD_DISPLAY_FEATURE_FLAG_MAX: return "CTL_STD_DISPLAY_FEATURE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_intel_display_feature_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_intel_display_feature_flag_t*>(pEnum);
			switch (e) {
			case _ctl_intel_display_feature_flag_t::CTL_INTEL_DISPLAY_FEATURE_FLAG_DPST: return "CTL_INTEL_DISPLAY_FEATURE_FLAG_DPST"s;
			case _ctl_intel_display_feature_flag_t::CTL_INTEL_DISPLAY_FEATURE_FLAG_LACE: return "CTL_INTEL_DISPLAY_FEATURE_FLAG_LACE"s;
			case _ctl_intel_display_feature_flag_t::CTL_INTEL_DISPLAY_FEATURE_FLAG_DRRS: return "CTL_INTEL_DISPLAY_FEATURE_FLAG_DRRS"s;
			case _ctl_intel_display_feature_flag_t::CTL_INTEL_DISPLAY_FEATURE_FLAG_ARC_ADAPTIVE_SYNC_CERTIFIED: return "CTL_INTEL_DISPLAY_FEATURE_FLAG_ARC_ADAPTIVE_SYNC_CERTIFIED"s;
			case _ctl_intel_display_feature_flag_t::CTL_INTEL_DISPLAY_FEATURE_FLAG_MAX: return "CTL_INTEL_DISPLAY_FEATURE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_protocol_converter_location_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_protocol_converter_location_flag_t*>(pEnum);
			switch (e) {
			case _ctl_protocol_converter_location_flag_t::CTL_PROTOCOL_CONVERTER_LOCATION_FLAG_ONBOARD: return "CTL_PROTOCOL_CONVERTER_LOCATION_FLAG_ONBOARD"s;
			case _ctl_protocol_converter_location_flag_t::CTL_PROTOCOL_CONVERTER_LOCATION_FLAG_EXTERNAL: return "CTL_PROTOCOL_CONVERTER_LOCATION_FLAG_EXTERNAL"s;
			case _ctl_protocol_converter_location_flag_t::CTL_PROTOCOL_CONVERTER_LOCATION_FLAG_MAX: return "CTL_PROTOCOL_CONVERTER_LOCATION_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_config_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_config_flag_t*>(pEnum);
			switch (e) {
			case _ctl_display_config_flag_t::CTL_DISPLAY_CONFIG_FLAG_DISPLAY_ACTIVE: return "CTL_DISPLAY_CONFIG_FLAG_DISPLAY_ACTIVE"s;
			case _ctl_display_config_flag_t::CTL_DISPLAY_CONFIG_FLAG_DISPLAY_ATTACHED: return "CTL_DISPLAY_CONFIG_FLAG_DISPLAY_ATTACHED"s;
			case _ctl_display_config_flag_t::CTL_DISPLAY_CONFIG_FLAG_IS_DONGLE_CONNECTED_TO_ENCODER: return "CTL_DISPLAY_CONFIG_FLAG_IS_DONGLE_CONNECTED_TO_ENCODER"s;
			case _ctl_display_config_flag_t::CTL_DISPLAY_CONFIG_FLAG_DITHERING_ENABLED: return "CTL_DISPLAY_CONFIG_FLAG_DITHERING_ENABLED"s;
			case _ctl_display_config_flag_t::CTL_DISPLAY_CONFIG_FLAG_MAX: return "CTL_DISPLAY_CONFIG_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_encoder_config_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_encoder_config_flag_t*>(pEnum);
			switch (e) {
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_INTERNAL_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_INTERNAL_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_VESA_TILED_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_VESA_TILED_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_TYPEC_CAPABLE: return "CTL_ENCODER_CONFIG_FLAG_TYPEC_CAPABLE"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_TBT_CAPABLE: return "CTL_ENCODER_CONFIG_FLAG_TBT_CAPABLE"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_DITHERING_SUPPORTED: return "CTL_ENCODER_CONFIG_FLAG_DITHERING_SUPPORTED"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_VIRTUAL_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_VIRTUAL_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_HIDDEN_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_HIDDEN_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_COLLAGE_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_COLLAGE_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_SPLIT_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_SPLIT_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_COMPANION_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_COMPANION_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_MGPU_COLLAGE_DISPLAY: return "CTL_ENCODER_CONFIG_FLAG_MGPU_COLLAGE_DISPLAY"s;
			case _ctl_encoder_config_flag_t::CTL_ENCODER_CONFIG_FLAG_MAX: return "CTL_ENCODER_CONFIG_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_sharpness_filter_type_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_sharpness_filter_type_flag_t*>(pEnum);
			switch (e) {
			case _ctl_sharpness_filter_type_flag_t::CTL_SHARPNESS_FILTER_TYPE_FLAG_NON_ADAPTIVE: return "CTL_SHARPNESS_FILTER_TYPE_FLAG_NON_ADAPTIVE"s;
			case _ctl_sharpness_filter_type_flag_t::CTL_SHARPNESS_FILTER_TYPE_FLAG_ADAPTIVE: return "CTL_SHARPNESS_FILTER_TYPE_FLAG_ADAPTIVE"s;
			case _ctl_sharpness_filter_type_flag_t::CTL_SHARPNESS_FILTER_TYPE_FLAG_MAX: return "CTL_SHARPNESS_FILTER_TYPE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_i2c_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_i2c_flag_t*>(pEnum);
			switch (e) {
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_ATOMICI2C: return "CTL_I2C_FLAG_ATOMICI2C"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_1BYTE_INDEX: return "CTL_I2C_FLAG_1BYTE_INDEX"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_2BYTE_INDEX: return "CTL_I2C_FLAG_2BYTE_INDEX"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_4BYTE_INDEX: return "CTL_I2C_FLAG_4BYTE_INDEX"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_SPEED_SLOW: return "CTL_I2C_FLAG_SPEED_SLOW"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_SPEED_FAST: return "CTL_I2C_FLAG_SPEED_FAST"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_SPEED_BIT_BASH: return "CTL_I2C_FLAG_SPEED_BIT_BASH"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_DRIVER_OVERRIDE: return "CTL_I2C_FLAG_DRIVER_OVERRIDE"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_START: return "CTL_I2C_FLAG_START"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_STOP: return "CTL_I2C_FLAG_STOP"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_RESTART: return "CTL_I2C_FLAG_RESTART"s;
			case _ctl_i2c_flag_t::CTL_I2C_FLAG_MAX: return "CTL_I2C_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_i2c_pinpair_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_i2c_pinpair_flag_t*>(pEnum);
			switch (e) {
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_ATOMICI2C: return "CTL_I2C_PINPAIR_FLAG_ATOMICI2C"s;
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_1BYTE_INDEX: return "CTL_I2C_PINPAIR_FLAG_1BYTE_INDEX"s;
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_2BYTE_INDEX: return "CTL_I2C_PINPAIR_FLAG_2BYTE_INDEX"s;
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_4BYTE_INDEX: return "CTL_I2C_PINPAIR_FLAG_4BYTE_INDEX"s;
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_SPEED_SLOW: return "CTL_I2C_PINPAIR_FLAG_SPEED_SLOW"s;
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_SPEED_FAST: return "CTL_I2C_PINPAIR_FLAG_SPEED_FAST"s;
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_SPEED_BIT_BASH: return "CTL_I2C_PINPAIR_FLAG_SPEED_BIT_BASH"s;
			case _ctl_i2c_pinpair_flag_t::CTL_I2C_PINPAIR_FLAG_MAX: return "CTL_I2C_PINPAIR_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_aux_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_aux_flag_t*>(pEnum);
			switch (e) {
			case _ctl_aux_flag_t::CTL_AUX_FLAG_NATIVE_AUX: return "CTL_AUX_FLAG_NATIVE_AUX"s;
			case _ctl_aux_flag_t::CTL_AUX_FLAG_I2C_AUX: return "CTL_AUX_FLAG_I2C_AUX"s;
			case _ctl_aux_flag_t::CTL_AUX_FLAG_I2C_AUX_MOT: return "CTL_AUX_FLAG_I2C_AUX_MOT"s;
			case _ctl_aux_flag_t::CTL_AUX_FLAG_MAX: return "CTL_AUX_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_power_optimization_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_power_optimization_flag_t*>(pEnum);
			switch (e) {
			case _ctl_power_optimization_flag_t::CTL_POWER_OPTIMIZATION_FLAG_FBC: return "CTL_POWER_OPTIMIZATION_FLAG_FBC"s;
			case _ctl_power_optimization_flag_t::CTL_POWER_OPTIMIZATION_FLAG_PSR: return "CTL_POWER_OPTIMIZATION_FLAG_PSR"s;
			case _ctl_power_optimization_flag_t::CTL_POWER_OPTIMIZATION_FLAG_DPST: return "CTL_POWER_OPTIMIZATION_FLAG_DPST"s;
			case _ctl_power_optimization_flag_t::CTL_POWER_OPTIMIZATION_FLAG_LRR: return "CTL_POWER_OPTIMIZATION_FLAG_LRR"s;
			case _ctl_power_optimization_flag_t::CTL_POWER_OPTIMIZATION_FLAG_LACE: return "CTL_POWER_OPTIMIZATION_FLAG_LACE"s;
			case _ctl_power_optimization_flag_t::CTL_POWER_OPTIMIZATION_FLAG_MAX: return "CTL_POWER_OPTIMIZATION_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_power_optimization_dpst_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_power_optimization_dpst_flag_t*>(pEnum);
			switch (e) {
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_BKLT: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_BKLT"s;
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_PANEL_CABC: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_PANEL_CABC"s;
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_OPST: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_OPST"s;
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_ELP: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_ELP"s;
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_EPSM: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_EPSM"s;
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_APD: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_APD"s;
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_PIXOPTIX: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_PIXOPTIX"s;
			case _ctl_power_optimization_dpst_flag_t::CTL_POWER_OPTIMIZATION_DPST_FLAG_MAX: return "CTL_POWER_OPTIMIZATION_DPST_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_power_optimization_lrr_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_power_optimization_lrr_flag_t*>(pEnum);
			switch (e) {
			case _ctl_power_optimization_lrr_flag_t::CTL_POWER_OPTIMIZATION_LRR_FLAG_LRR10: return "CTL_POWER_OPTIMIZATION_LRR_FLAG_LRR10"s;
			case _ctl_power_optimization_lrr_flag_t::CTL_POWER_OPTIMIZATION_LRR_FLAG_LRR20: return "CTL_POWER_OPTIMIZATION_LRR_FLAG_LRR20"s;
			case _ctl_power_optimization_lrr_flag_t::CTL_POWER_OPTIMIZATION_LRR_FLAG_LRR25: return "CTL_POWER_OPTIMIZATION_LRR_FLAG_LRR25"s;
			case _ctl_power_optimization_lrr_flag_t::CTL_POWER_OPTIMIZATION_LRR_FLAG_ALRR: return "CTL_POWER_OPTIMIZATION_LRR_FLAG_ALRR"s;
			case _ctl_power_optimization_lrr_flag_t::CTL_POWER_OPTIMIZATION_LRR_FLAG_UBLRR: return "CTL_POWER_OPTIMIZATION_LRR_FLAG_UBLRR"s;
			case _ctl_power_optimization_lrr_flag_t::CTL_POWER_OPTIMIZATION_LRR_FLAG_UBZRR: return "CTL_POWER_OPTIMIZATION_LRR_FLAG_UBZRR"s;
			case _ctl_power_optimization_lrr_flag_t::CTL_POWER_OPTIMIZATION_LRR_FLAG_MAX: return "CTL_POWER_OPTIMIZATION_LRR_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_pixtx_pipe_set_config_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_pixtx_pipe_set_config_flag_t*>(pEnum);
			switch (e) {
			case _ctl_pixtx_pipe_set_config_flag_t::CTL_PIXTX_PIPE_SET_CONFIG_FLAG_PERSIST_ACROSS_POWER_EVENTS: return "CTL_PIXTX_PIPE_SET_CONFIG_FLAG_PERSIST_ACROSS_POWER_EVENTS"s;
			case _ctl_pixtx_pipe_set_config_flag_t::CTL_PIXTX_PIPE_SET_CONFIG_FLAG_MAX: return "CTL_PIXTX_PIPE_SET_CONFIG_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_retro_scaling_type_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_retro_scaling_type_flag_t*>(pEnum);
			switch (e) {
			case _ctl_retro_scaling_type_flag_t::CTL_RETRO_SCALING_TYPE_FLAG_INTEGER: return "CTL_RETRO_SCALING_TYPE_FLAG_INTEGER"s;
			case _ctl_retro_scaling_type_flag_t::CTL_RETRO_SCALING_TYPE_FLAG_NEAREST_NEIGHBOUR: return "CTL_RETRO_SCALING_TYPE_FLAG_NEAREST_NEIGHBOUR"s;
			case _ctl_retro_scaling_type_flag_t::CTL_RETRO_SCALING_TYPE_FLAG_MAX: return "CTL_RETRO_SCALING_TYPE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_scaling_type_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_scaling_type_flag_t*>(pEnum);
			switch (e) {
			case _ctl_scaling_type_flag_t::CTL_SCALING_TYPE_FLAG_IDENTITY: return "CTL_SCALING_TYPE_FLAG_IDENTITY"s;
			case _ctl_scaling_type_flag_t::CTL_SCALING_TYPE_FLAG_CENTERED: return "CTL_SCALING_TYPE_FLAG_CENTERED"s;
			case _ctl_scaling_type_flag_t::CTL_SCALING_TYPE_FLAG_STRETCHED: return "CTL_SCALING_TYPE_FLAG_STRETCHED"s;
			case _ctl_scaling_type_flag_t::CTL_SCALING_TYPE_FLAG_ASPECT_RATIO_CENTERED_MAX: return "CTL_SCALING_TYPE_FLAG_ASPECT_RATIO_CENTERED_MAX"s;
			case _ctl_scaling_type_flag_t::CTL_SCALING_TYPE_FLAG_CUSTOM: return "CTL_SCALING_TYPE_FLAG_CUSTOM"s;
			case _ctl_scaling_type_flag_t::CTL_SCALING_TYPE_FLAG_MAX: return "CTL_SCALING_TYPE_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_get_operation_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_get_operation_flag_t*>(pEnum);
			switch (e) {
			case _ctl_get_operation_flag_t::CTL_GET_OPERATION_FLAG_CURRENT: return "CTL_GET_OPERATION_FLAG_CURRENT"s;
			case _ctl_get_operation_flag_t::CTL_GET_OPERATION_FLAG_DEFAULT: return "CTL_GET_OPERATION_FLAG_DEFAULT"s;
			case _ctl_get_operation_flag_t::CTL_GET_OPERATION_FLAG_CAPABILITY: return "CTL_GET_OPERATION_FLAG_CAPABILITY"s;
			case _ctl_get_operation_flag_t::CTL_GET_OPERATION_FLAG_MAX: return "CTL_GET_OPERATION_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_lace_trigger_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_lace_trigger_flag_t*>(pEnum);
			switch (e) {
			case _ctl_lace_trigger_flag_t::CTL_LACE_TRIGGER_FLAG_AMBIENT_LIGHT: return "CTL_LACE_TRIGGER_FLAG_AMBIENT_LIGHT"s;
			case _ctl_lace_trigger_flag_t::CTL_LACE_TRIGGER_FLAG_FIXED_AGGRESSIVENESS: return "CTL_LACE_TRIGGER_FLAG_FIXED_AGGRESSIVENESS"s;
			case _ctl_lace_trigger_flag_t::CTL_LACE_TRIGGER_FLAG_MAX: return "CTL_LACE_TRIGGER_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_edid_management_out_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_edid_management_out_flag_t*>(pEnum);
			switch (e) {
			case _ctl_edid_management_out_flag_t::CTL_EDID_MANAGEMENT_OUT_FLAG_OS_CONN_NOTIFICATION: return "CTL_EDID_MANAGEMENT_OUT_FLAG_OS_CONN_NOTIFICATION"s;
			case _ctl_edid_management_out_flag_t::CTL_EDID_MANAGEMENT_OUT_FLAG_SUPPLIED_EDID: return "CTL_EDID_MANAGEMENT_OUT_FLAG_SUPPLIED_EDID"s;
			case _ctl_edid_management_out_flag_t::CTL_EDID_MANAGEMENT_OUT_FLAG_MONITOR_EDID: return "CTL_EDID_MANAGEMENT_OUT_FLAG_MONITOR_EDID"s;
			case _ctl_edid_management_out_flag_t::CTL_EDID_MANAGEMENT_OUT_FLAG_DISPLAY_CONNECTED: return "CTL_EDID_MANAGEMENT_OUT_FLAG_DISPLAY_CONNECTED"s;
			case _ctl_edid_management_out_flag_t::CTL_EDID_MANAGEMENT_OUT_FLAG_MAX: return "CTL_EDID_MANAGEMENT_OUT_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_display_setting_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_display_setting_flag_t*>(pEnum);
			switch (e) {
			case _ctl_display_setting_flag_t::CTL_DISPLAY_SETTING_FLAG_LOW_LATENCY: return "CTL_DISPLAY_SETTING_FLAG_LOW_LATENCY"s;
			case _ctl_display_setting_flag_t::CTL_DISPLAY_SETTING_FLAG_SOURCE_TM: return "CTL_DISPLAY_SETTING_FLAG_SOURCE_TM"s;
			case _ctl_display_setting_flag_t::CTL_DISPLAY_SETTING_FLAG_CONTENT_TYPE: return "CTL_DISPLAY_SETTING_FLAG_CONTENT_TYPE"s;
			case _ctl_display_setting_flag_t::CTL_DISPLAY_SETTING_FLAG_QUANTIZATION_RANGE: return "CTL_DISPLAY_SETTING_FLAG_QUANTIZATION_RANGE"s;
			case _ctl_display_setting_flag_t::CTL_DISPLAY_SETTING_FLAG_PICTURE_AR: return "CTL_DISPLAY_SETTING_FLAG_PICTURE_AR"s;
			case _ctl_display_setting_flag_t::CTL_DISPLAY_SETTING_FLAG_AUDIO: return "CTL_DISPLAY_SETTING_FLAG_AUDIO"s;
			case _ctl_display_setting_flag_t::CTL_DISPLAY_SETTING_FLAG_MAX: return "CTL_DISPLAY_SETTING_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_firmware_config_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_firmware_config_flag_t*>(pEnum);
			switch (e) {
			case _ctl_firmware_config_flag_t::CTL_FIRMWARE_CONFIG_FLAG_IS_DEVICE_LINK_SPEED_DOWNGRADE_CAPABLE: return "CTL_FIRMWARE_CONFIG_FLAG_IS_DEVICE_LINK_SPEED_DOWNGRADE_CAPABLE"s;
			case _ctl_firmware_config_flag_t::CTL_FIRMWARE_CONFIG_FLAG_IS_DEVICE_LINK_SPEED_DOWNGRADE_ACTIVE: return "CTL_FIRMWARE_CONFIG_FLAG_IS_DEVICE_LINK_SPEED_DOWNGRADE_ACTIVE"s;
			case _ctl_firmware_config_flag_t::CTL_FIRMWARE_CONFIG_FLAG_MAX: return "CTL_FIRMWARE_CONFIG_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_freq_throttle_reason_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_freq_throttle_reason_flag_t*>(pEnum);
			switch (e) {
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP: return "CTL_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP"s;
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_BURST_PWR_CAP: return "CTL_FREQ_THROTTLE_REASON_FLAG_BURST_PWR_CAP"s;
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_CURRENT_LIMIT: return "CTL_FREQ_THROTTLE_REASON_FLAG_CURRENT_LIMIT"s;
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_THERMAL_LIMIT: return "CTL_FREQ_THROTTLE_REASON_FLAG_THERMAL_LIMIT"s;
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_PSU_ALERT: return "CTL_FREQ_THROTTLE_REASON_FLAG_PSU_ALERT"s;
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_SW_RANGE: return "CTL_FREQ_THROTTLE_REASON_FLAG_SW_RANGE"s;
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_HW_RANGE: return "CTL_FREQ_THROTTLE_REASON_FLAG_HW_RANGE"s;
			case _ctl_freq_throttle_reason_flag_t::CTL_FREQ_THROTTLE_REASON_FLAG_MAX: return "CTL_FREQ_THROTTLE_REASON_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_video_processing_super_resolution_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_video_processing_super_resolution_flag_t*>(pEnum);
			switch (e) {
			case _ctl_video_processing_super_resolution_flag_t::CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_DISABLE: return "CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_DISABLE"s;
			case _ctl_video_processing_super_resolution_flag_t::CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_ENABLE_DEFAULT_SCENARIO_MODE: return "CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_ENABLE_DEFAULT_SCENARIO_MODE"s;
			case _ctl_video_processing_super_resolution_flag_t::CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_ENABLE_CONFERENCE_SCENARIO_MODE: return "CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_ENABLE_CONFERENCE_SCENARIO_MODE"s;
			case _ctl_video_processing_super_resolution_flag_t::CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_ENABLE_CAMERA_SCENARIO_MODE: return "CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_ENABLE_CAMERA_SCENARIO_MODE"s;
			case _ctl_video_processing_super_resolution_flag_t::CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_MAX: return "CTL_VIDEO_PROCESSING_SUPER_RESOLUTION_FLAG_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_vf_curve_details_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_vf_curve_details_t*>(pEnum);
			switch (e) {
			case _ctl_vf_curve_details_t::CTL_VF_CURVE_DETAILS_SIMPLIFIED: return "CTL_VF_CURVE_DETAILS_SIMPLIFIED"s;
			case _ctl_vf_curve_details_t::CTL_VF_CURVE_DETAILS_MEDIUM: return "CTL_VF_CURVE_DETAILS_MEDIUM"s;
			case _ctl_vf_curve_details_t::CTL_VF_CURVE_DETAILS_ELABORATE: return "CTL_VF_CURVE_DETAILS_ELABORATE"s;
			case _ctl_vf_curve_details_t::CTL_VF_CURVE_DETAILS_MAX: return "CTL_VF_CURVE_DETAILS_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ctl_vf_curve_type_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_vf_curve_type_t*>(pEnum);
			switch (e) {
			case _ctl_vf_curve_type_t::CTL_VF_CURVE_TYPE_STOCK: return "CTL_VF_CURVE_TYPE_STOCK"s;
			case _ctl_vf_curve_type_t::CTL_VF_CURVE_TYPE_LIVE: return "CTL_VF_CURVE_TYPE_LIVE"s;
			case _ctl_vf_curve_type_t::CTL_VF_CURVE_TYPE_MAX: return "CTL_VF_CURVE_TYPE_MAX"s;
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
		dumpers[typeid(_NV_DP_LINK_RATE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DP_LINK_RATE*>(pEnum);
			switch (e) {
			case _NV_DP_LINK_RATE::NV_DP_1_62GBPS: return "NV_DP_1_62GBPS"s;
			case _NV_DP_LINK_RATE::NV_DP_2_70GBPS: return "NV_DP_2_70GBPS"s;
			case _NV_DP_LINK_RATE::NV_DP_5_40GBPS: return "NV_DP_5_40GBPS"s;
			case _NV_DP_LINK_RATE::NV_DP_8_10GBPS: return "NV_DP_8_10GBPS"s;
			case _NV_DP_LINK_RATE::NV_EDP_2_16GBPS: return "NV_EDP_2_16GBPS"s;
			case _NV_DP_LINK_RATE::NV_EDP_2_43GBPS: return "NV_EDP_2_43GBPS"s;
			case _NV_DP_LINK_RATE::NV_EDP_3_24GBPS: return "NV_EDP_3_24GBPS"s;
			case _NV_DP_LINK_RATE::NV_EDP_4_32GBPS: return "NV_EDP_4_32GBPS"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DP_LANE_COUNT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DP_LANE_COUNT*>(pEnum);
			switch (e) {
			case _NV_DP_LANE_COUNT::NV_DP_1_LANE: return "NV_DP_1_LANE"s;
			case _NV_DP_LANE_COUNT::NV_DP_2_LANE: return "NV_DP_2_LANE"s;
			case _NV_DP_LANE_COUNT::NV_DP_4_LANE: return "NV_DP_4_LANE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DP_COLOR_FORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DP_COLOR_FORMAT*>(pEnum);
			switch (e) {
			case _NV_DP_COLOR_FORMAT::NV_DP_COLOR_FORMAT_RGB: return "NV_DP_COLOR_FORMAT_RGB"s;
			case _NV_DP_COLOR_FORMAT::NV_DP_COLOR_FORMAT_YCbCr422: return "NV_DP_COLOR_FORMAT_YCbCr422"s;
			case _NV_DP_COLOR_FORMAT::NV_DP_COLOR_FORMAT_YCbCr444: return "NV_DP_COLOR_FORMAT_YCbCr444"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DP_COLORIMETRY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DP_COLORIMETRY*>(pEnum);
			switch (e) {
			case _NV_DP_COLORIMETRY::NV_DP_COLORIMETRY_RGB: return "NV_DP_COLORIMETRY_RGB"s;
			case _NV_DP_COLORIMETRY::NV_DP_COLORIMETRY_YCbCr_ITU601: return "NV_DP_COLORIMETRY_YCbCr_ITU601"s;
			case _NV_DP_COLORIMETRY::NV_DP_COLORIMETRY_YCbCr_ITU709: return "NV_DP_COLORIMETRY_YCbCr_ITU709"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DP_DYNAMIC_RANGE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DP_DYNAMIC_RANGE*>(pEnum);
			switch (e) {
			case _NV_DP_DYNAMIC_RANGE::NV_DP_DYNAMIC_RANGE_VESA: return "NV_DP_DYNAMIC_RANGE_VESA"s;
			case _NV_DP_DYNAMIC_RANGE::NV_DP_DYNAMIC_RANGE_CEA: return "NV_DP_DYNAMIC_RANGE_CEA"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DP_BPC)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DP_BPC*>(pEnum);
			switch (e) {
			case _NV_DP_BPC::NV_DP_BPC_DEFAULT: return "NV_DP_BPC_DEFAULT"s;
			case _NV_DP_BPC::NV_DP_BPC_6: return "NV_DP_BPC_6"s;
			case _NV_DP_BPC::NV_DP_BPC_8: return "NV_DP_BPC_8"s;
			case _NV_DP_BPC::NV_DP_BPC_10: return "NV_DP_BPC_10"s;
			case _NV_DP_BPC::NV_DP_BPC_12: return "NV_DP_BPC_12"s;
			case _NV_DP_BPC::NV_DP_BPC_16: return "NV_DP_BPC_16"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_CONNECTOR_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_CONNECTOR_TYPE*>(pEnum);
			switch (e) {
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_VGA_15_PIN: return "NVAPI_GPU_CONNECTOR_VGA_15_PIN"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_TV_COMPOSITE: return "NVAPI_GPU_CONNECTOR_TV_COMPOSITE"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_TV_SVIDEO: return "NVAPI_GPU_CONNECTOR_TV_SVIDEO"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_TV_HDTV_COMPONENT: return "NVAPI_GPU_CONNECTOR_TV_HDTV_COMPONENT"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_TV_SCART: return "NVAPI_GPU_CONNECTOR_TV_SCART"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_TV_COMPOSITE_SCART_ON_EIAJ4120: return "NVAPI_GPU_CONNECTOR_TV_COMPOSITE_SCART_ON_EIAJ4120"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_TV_HDTV_EIAJ4120: return "NVAPI_GPU_CONNECTOR_TV_HDTV_EIAJ4120"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_PC_POD_HDTV_YPRPB: return "NVAPI_GPU_CONNECTOR_PC_POD_HDTV_YPRPB"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_PC_POD_SVIDEO: return "NVAPI_GPU_CONNECTOR_PC_POD_SVIDEO"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_PC_POD_COMPOSITE: return "NVAPI_GPU_CONNECTOR_PC_POD_COMPOSITE"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_DVI_I_TV_SVIDEO: return "NVAPI_GPU_CONNECTOR_DVI_I_TV_SVIDEO"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_DVI_I_TV_COMPOSITE: return "NVAPI_GPU_CONNECTOR_DVI_I_TV_COMPOSITE"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_DVI_I: return "NVAPI_GPU_CONNECTOR_DVI_I"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_DVI_D: return "NVAPI_GPU_CONNECTOR_DVI_D"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_ADC: return "NVAPI_GPU_CONNECTOR_ADC"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_LFH_DVI_I_1: return "NVAPI_GPU_CONNECTOR_LFH_DVI_I_1"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_LFH_DVI_I_2: return "NVAPI_GPU_CONNECTOR_LFH_DVI_I_2"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_SPWG: return "NVAPI_GPU_CONNECTOR_SPWG"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_OEM: return "NVAPI_GPU_CONNECTOR_OEM"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_DISPLAYPORT_EXTERNAL: return "NVAPI_GPU_CONNECTOR_DISPLAYPORT_EXTERNAL"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_DISPLAYPORT_INTERNAL: return "NVAPI_GPU_CONNECTOR_DISPLAYPORT_INTERNAL"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_DISPLAYPORT_MINI_EXT: return "NVAPI_GPU_CONNECTOR_DISPLAYPORT_MINI_EXT"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_HDMI_A: return "NVAPI_GPU_CONNECTOR_HDMI_A"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_HDMI_C_MINI: return "NVAPI_GPU_CONNECTOR_HDMI_C_MINI"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_1: return "NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_1"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_2: return "NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_2"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_VIRTUAL_WFD: return "NVAPI_GPU_CONNECTOR_VIRTUAL_WFD"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_USB_C: return "NVAPI_GPU_CONNECTOR_USB_C"s;
			case _NV_GPU_CONNECTOR_TYPE::NVAPI_GPU_CONNECTOR_UNKNOWN: return "NVAPI_GPU_CONNECTOR_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DISPLAY_TV_FORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DISPLAY_TV_FORMAT*>(pEnum);
			switch (e) {
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_NONE: return "NV_DISPLAY_TV_FORMAT_NONE"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_NTSCM: return "NV_DISPLAY_TV_FORMAT_SD_NTSCM"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_NTSCJ: return "NV_DISPLAY_TV_FORMAT_SD_NTSCJ"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_PALM: return "NV_DISPLAY_TV_FORMAT_SD_PALM"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_PALBDGH: return "NV_DISPLAY_TV_FORMAT_SD_PALBDGH"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_PALN: return "NV_DISPLAY_TV_FORMAT_SD_PALN"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_PALNC: return "NV_DISPLAY_TV_FORMAT_SD_PALNC"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_576i: return "NV_DISPLAY_TV_FORMAT_SD_576i"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_480i: return "NV_DISPLAY_TV_FORMAT_SD_480i"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_ED_480p: return "NV_DISPLAY_TV_FORMAT_ED_480p"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_ED_576p: return "NV_DISPLAY_TV_FORMAT_ED_576p"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_720p: return "NV_DISPLAY_TV_FORMAT_HD_720p"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_1080i: return "NV_DISPLAY_TV_FORMAT_HD_1080i"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_1080p: return "NV_DISPLAY_TV_FORMAT_HD_1080p"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_720p50: return "NV_DISPLAY_TV_FORMAT_HD_720p50"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_1080p24: return "NV_DISPLAY_TV_FORMAT_HD_1080p24"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_1080i50: return "NV_DISPLAY_TV_FORMAT_HD_1080i50"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_1080p50: return "NV_DISPLAY_TV_FORMAT_HD_1080p50"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp30: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp30"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp25: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp25"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp24: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp24"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp24_SMPTE: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp24_SMPTE"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp50_3840: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp50_3840"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp60_3840: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp60_3840"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp30_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp30_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp25_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp25_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp24_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp24_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp50_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp50_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp60_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp60_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp24_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp24_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp25_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp25_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp30_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp30_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp48_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp48_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp50_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp50_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp60_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp60_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp100_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp100_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_8Kp120_7680: return "NV_DISPLAY_TV_FORMAT_UHD_8Kp120_7680"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp48_3840: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp48_3840"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp48_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp48_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp100_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp100_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp100_3840: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp100_3840"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp120_4096: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp120_4096"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp120_3840: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp120_3840"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp100_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp100_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp120_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp120_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp24_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp24_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp25_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp25_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp30_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp30_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp48_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp48_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp50_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp50_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_4Kp60_5120: return "NV_DISPLAY_TV_FORMAT_UHD_4Kp60_5120"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp24_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp24_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp25_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp25_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp30_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp30_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp48_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp48_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp50_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp50_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp60_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp60_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp100_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp100_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_UHD_10Kp120_10240: return "NV_DISPLAY_TV_FORMAT_UHD_10Kp120_10240"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_SD_OTHER: return "NV_DISPLAY_TV_FORMAT_SD_OTHER"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_ED_OTHER: return "NV_DISPLAY_TV_FORMAT_ED_OTHER"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_HD_OTHER: return "NV_DISPLAY_TV_FORMAT_HD_OTHER"s;
			case _NV_DISPLAY_TV_FORMAT::NV_DISPLAY_TV_FORMAT_ANY: return "NV_DISPLAY_TV_FORMAT_ANY"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_TARGET_VIEW_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_TARGET_VIEW_MODE*>(pEnum);
			switch (e) {
			case _NV_TARGET_VIEW_MODE::NV_VIEW_MODE_STANDARD: return "NV_VIEW_MODE_STANDARD"s;
			case _NV_TARGET_VIEW_MODE::NV_VIEW_MODE_CLONE: return "NV_VIEW_MODE_CLONE"s;
			case _NV_TARGET_VIEW_MODE::NV_VIEW_MODE_HSPAN: return "NV_VIEW_MODE_HSPAN"s;
			case _NV_TARGET_VIEW_MODE::NV_VIEW_MODE_VSPAN: return "NV_VIEW_MODE_VSPAN"s;
			case _NV_TARGET_VIEW_MODE::NV_VIEW_MODE_DUALVIEW: return "NV_VIEW_MODE_DUALVIEW"s;
			case _NV_TARGET_VIEW_MODE::NV_VIEW_MODE_MULTIVIEW: return "NV_VIEW_MODE_MULTIVIEW"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_SCALING)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_SCALING*>(pEnum);
			switch (e) {
			case _NV_SCALING::NV_SCALING_DEFAULT: return "NV_SCALING_DEFAULT"s;
			case _NV_SCALING::NV_SCALING_GPU_SCALING_TO_CLOSEST: return "NV_SCALING_GPU_SCALING_TO_CLOSEST"s;
			case _NV_SCALING::NV_SCALING_GPU_SCALING_TO_NATIVE: return "NV_SCALING_GPU_SCALING_TO_NATIVE"s;
			case _NV_SCALING::NV_SCALING_GPU_SCANOUT_TO_NATIVE: return "NV_SCALING_GPU_SCANOUT_TO_NATIVE"s;
			case _NV_SCALING::NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_NATIVE: return "NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_NATIVE"s;
			case _NV_SCALING::NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_CLOSEST: return "NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_CLOSEST"s;
			case _NV_SCALING::NV_SCALING_GPU_SCANOUT_TO_CLOSEST: return "NV_SCALING_GPU_SCANOUT_TO_CLOSEST"s;
			case _NV_SCALING::NV_SCALING_GPU_INTEGER_ASPECT_SCALING: return "NV_SCALING_GPU_INTEGER_ASPECT_SCALING"s;
			case _NV_SCALING::NV_SCALING_CUSTOMIZED: return "NV_SCALING_CUSTOMIZED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_ROTATE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_ROTATE*>(pEnum);
			switch (e) {
			case _NV_ROTATE::NV_ROTATE_0: return "NV_ROTATE_0"s;
			case _NV_ROTATE::NV_ROTATE_90: return "NV_ROTATE_90"s;
			case _NV_ROTATE::NV_ROTATE_180: return "NV_ROTATE_180"s;
			case _NV_ROTATE::NV_ROTATE_270: return "NV_ROTATE_270"s;
			case _NV_ROTATE::NV_ROTATE_IGNORED: return "NV_ROTATE_IGNORED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_FORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_FORMAT*>(pEnum);
			switch (e) {
			case _NV_FORMAT::NV_FORMAT_UNKNOWN: return "NV_FORMAT_UNKNOWN"s;
			case _NV_FORMAT::NV_FORMAT_P8: return "NV_FORMAT_P8"s;
			case _NV_FORMAT::NV_FORMAT_R5G6B5: return "NV_FORMAT_R5G6B5"s;
			case _NV_FORMAT::NV_FORMAT_A8R8G8B8: return "NV_FORMAT_A8R8G8B8"s;
			case _NV_FORMAT::NV_FORMAT_A16B16G16R16F: return "NV_FORMAT_A16B16G16R16F"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_TIMING_OVERRIDE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_TIMING_OVERRIDE*>(pEnum);
			switch (e) {
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_CURRENT: return "NV_TIMING_OVERRIDE_CURRENT"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_AUTO: return "NV_TIMING_OVERRIDE_AUTO"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_EDID: return "NV_TIMING_OVERRIDE_EDID"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_DMT: return "NV_TIMING_OVERRIDE_DMT"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_DMT_RB: return "NV_TIMING_OVERRIDE_DMT_RB"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_CVT: return "NV_TIMING_OVERRIDE_CVT"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_CVT_RB: return "NV_TIMING_OVERRIDE_CVT_RB"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_GTF: return "NV_TIMING_OVERRIDE_GTF"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_EIA861: return "NV_TIMING_OVERRIDE_EIA861"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_ANALOG_TV: return "NV_TIMING_OVERRIDE_ANALOG_TV"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_CUST: return "NV_TIMING_OVERRIDE_CUST"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_NV_PREDEFINED: return "NV_TIMING_OVERRIDE_NV_PREDEFINED"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_NV_ASPR: return "NV_TIMING_OVERRIDE_NV_ASPR"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVERRIDE_SDI: return "NV_TIMING_OVERRIDE_SDI"s;
			case _NV_TIMING_OVERRIDE::NV_TIMING_OVRRIDE_MAX: return "NV_TIMING_OVRRIDE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_SPANNING_ORIENTATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DISPLAYCONFIG_SPANNING_ORIENTATION*>(pEnum);
			switch (e) {
			case _NV_DISPLAYCONFIG_SPANNING_ORIENTATION::NV_DISPLAYCONFIG_SPAN_NONE: return "NV_DISPLAYCONFIG_SPAN_NONE"s;
			case _NV_DISPLAYCONFIG_SPANNING_ORIENTATION::NV_DISPLAYCONFIG_SPAN_HORIZONTAL: return "NV_DISPLAYCONFIG_SPAN_HORIZONTAL"s;
			case _NV_DISPLAYCONFIG_SPANNING_ORIENTATION::NV_DISPLAYCONFIG_SPAN_VERTICAL: return "NV_DISPLAYCONFIG_SPAN_VERTICAL"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DISPLAYCONFIG_FLAGS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DISPLAYCONFIG_FLAGS*>(pEnum);
			switch (e) {
			case _NV_DISPLAYCONFIG_FLAGS::NV_DISPLAYCONFIG_VALIDATE_ONLY: return "NV_DISPLAYCONFIG_VALIDATE_ONLY"s;
			case _NV_DISPLAYCONFIG_FLAGS::NV_DISPLAYCONFIG_SAVE_TO_PERSISTENCE: return "NV_DISPLAYCONFIG_SAVE_TO_PERSISTENCE"s;
			case _NV_DISPLAYCONFIG_FLAGS::NV_DISPLAYCONFIG_DRIVER_RELOAD_ALLOWED: return "NV_DISPLAYCONFIG_DRIVER_RELOAD_ALLOWED"s;
			case _NV_DISPLAYCONFIG_FLAGS::NV_DISPLAYCONFIG_FORCE_MODE_ENUMERATION: return "NV_DISPLAYCONFIG_FORCE_MODE_ENUMERATION"s;
			case _NV_DISPLAYCONFIG_FLAGS::NV_FORCE_COMMIT_VIDPN: return "NV_FORCE_COMMIT_VIDPN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID*>(pEnum);
			switch (e) {
			case _NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID::NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE: return "NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE"s;
			case _NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID::NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_UNDEFINED: return "NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_UNDEFINED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_PUBLIC_CLOCK_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_PUBLIC_CLOCK_ID*>(pEnum);
			switch (e) {
			case _NV_GPU_PUBLIC_CLOCK_ID::NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS: return "NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS"s;
			case _NV_GPU_PUBLIC_CLOCK_ID::NVAPI_GPU_PUBLIC_CLOCK_MEMORY: return "NVAPI_GPU_PUBLIC_CLOCK_MEMORY"s;
			case _NV_GPU_PUBLIC_CLOCK_ID::NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR: return "NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR"s;
			case _NV_GPU_PUBLIC_CLOCK_ID::NVAPI_GPU_PUBLIC_CLOCK_VIDEO: return "NVAPI_GPU_PUBLIC_CLOCK_VIDEO"s;
			case _NV_GPU_PUBLIC_CLOCK_ID::NVAPI_GPU_PUBLIC_CLOCK_UNDEFINED: return "NVAPI_GPU_PUBLIC_CLOCK_UNDEFINED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_PERF_PSTATE_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_PERF_PSTATE_ID*>(pEnum);
			switch (e) {
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P0: return "NVAPI_GPU_PERF_PSTATE_P0"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P1: return "NVAPI_GPU_PERF_PSTATE_P1"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P2: return "NVAPI_GPU_PERF_PSTATE_P2"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P3: return "NVAPI_GPU_PERF_PSTATE_P3"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P4: return "NVAPI_GPU_PERF_PSTATE_P4"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P5: return "NVAPI_GPU_PERF_PSTATE_P5"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P6: return "NVAPI_GPU_PERF_PSTATE_P6"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P7: return "NVAPI_GPU_PERF_PSTATE_P7"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P8: return "NVAPI_GPU_PERF_PSTATE_P8"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P9: return "NVAPI_GPU_PERF_PSTATE_P9"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P10: return "NVAPI_GPU_PERF_PSTATE_P10"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P11: return "NVAPI_GPU_PERF_PSTATE_P11"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P12: return "NVAPI_GPU_PERF_PSTATE_P12"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P13: return "NVAPI_GPU_PERF_PSTATE_P13"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P14: return "NVAPI_GPU_PERF_PSTATE_P14"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_P15: return "NVAPI_GPU_PERF_PSTATE_P15"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_UNDEFINED: return "NVAPI_GPU_PERF_PSTATE_UNDEFINED"s;
			case _NV_GPU_PERF_PSTATE_ID::NVAPI_GPU_PERF_PSTATE_ALL: return "NVAPI_GPU_PERF_PSTATE_ALL"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID*>(pEnum);
			switch (e) {
			case NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID::NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_SINGLE: return "NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_SINGLE"s;
			case NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID::NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_RANGE: return "NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_RANGE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_MONITOR_CONN_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_MONITOR_CONN_TYPE*>(pEnum);
			switch (e) {
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_UNINITIALIZED: return "NV_MONITOR_CONN_TYPE_UNINITIALIZED"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_VGA: return "NV_MONITOR_CONN_TYPE_VGA"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_COMPONENT: return "NV_MONITOR_CONN_TYPE_COMPONENT"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_SVIDEO: return "NV_MONITOR_CONN_TYPE_SVIDEO"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_HDMI: return "NV_MONITOR_CONN_TYPE_HDMI"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_DVI: return "NV_MONITOR_CONN_TYPE_DVI"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_LVDS: return "NV_MONITOR_CONN_TYPE_LVDS"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_DP: return "NV_MONITOR_CONN_TYPE_DP"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_COMPOSITE: return "NV_MONITOR_CONN_TYPE_COMPOSITE"s;
			case NV_MONITOR_CONN_TYPE::NV_MONITOR_CONN_TYPE_UNKNOWN: return "NV_MONITOR_CONN_TYPE_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_SYSTEM_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_SYSTEM_TYPE*>(pEnum);
			switch (e) {
			case NV_SYSTEM_TYPE::NV_SYSTEM_TYPE_UNKNOWN: return "NV_SYSTEM_TYPE_UNKNOWN"s;
			case NV_SYSTEM_TYPE::NV_SYSTEM_TYPE_LAPTOP: return "NV_SYSTEM_TYPE_LAPTOP"s;
			case NV_SYSTEM_TYPE::NV_SYSTEM_TYPE_DESKTOP: return "NV_SYSTEM_TYPE_DESKTOP"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_OUTPUT_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_OUTPUT_TYPE*>(pEnum);
			switch (e) {
			case _NV_GPU_OUTPUT_TYPE::NVAPI_GPU_OUTPUT_UNKNOWN: return "NVAPI_GPU_OUTPUT_UNKNOWN"s;
			case _NV_GPU_OUTPUT_TYPE::NVAPI_GPU_OUTPUT_CRT: return "NVAPI_GPU_OUTPUT_CRT"s;
			case _NV_GPU_OUTPUT_TYPE::NVAPI_GPU_OUTPUT_DFP: return "NVAPI_GPU_OUTPUT_DFP"s;
			case _NV_GPU_OUTPUT_TYPE::NVAPI_GPU_OUTPUT_TV: return "NVAPI_GPU_OUTPUT_TV"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_TYPE*>(pEnum);
			switch (e) {
			case _NV_GPU_TYPE::NV_SYSTEM_TYPE_GPU_UNKNOWN: return "NV_SYSTEM_TYPE_GPU_UNKNOWN"s;
			case _NV_GPU_TYPE::NV_SYSTEM_TYPE_IGPU: return "NV_SYSTEM_TYPE_IGPU"s;
			case _NV_GPU_TYPE::NV_SYSTEM_TYPE_DGPU: return "NV_SYSTEM_TYPE_DGPU"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_BUS_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_BUS_TYPE*>(pEnum);
			switch (e) {
			case _NV_GPU_BUS_TYPE::NVAPI_GPU_BUS_TYPE_UNDEFINED: return "NVAPI_GPU_BUS_TYPE_UNDEFINED"s;
			case _NV_GPU_BUS_TYPE::NVAPI_GPU_BUS_TYPE_PCI: return "NVAPI_GPU_BUS_TYPE_PCI"s;
			case _NV_GPU_BUS_TYPE::NVAPI_GPU_BUS_TYPE_AGP: return "NVAPI_GPU_BUS_TYPE_AGP"s;
			case _NV_GPU_BUS_TYPE::NVAPI_GPU_BUS_TYPE_PCI_EXPRESS: return "NVAPI_GPU_BUS_TYPE_PCI_EXPRESS"s;
			case _NV_GPU_BUS_TYPE::NVAPI_GPU_BUS_TYPE_FPCI: return "NVAPI_GPU_BUS_TYPE_FPCI"s;
			case _NV_GPU_BUS_TYPE::NVAPI_GPU_BUS_TYPE_AXI: return "NVAPI_GPU_BUS_TYPE_AXI"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_ARCHITECTURE_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_ARCHITECTURE_ID*>(pEnum);
			switch (e) {
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_T2X: return "NV_GPU_ARCHITECTURE_T2X"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_T3X: return "NV_GPU_ARCHITECTURE_T3X"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_T4X: return "NV_GPU_ARCHITECTURE_T4X"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_NV40: return "NV_GPU_ARCHITECTURE_NV40"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_NV50: return "NV_GPU_ARCHITECTURE_NV50"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_G78: return "NV_GPU_ARCHITECTURE_G78"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_G80: return "NV_GPU_ARCHITECTURE_G80"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_G90: return "NV_GPU_ARCHITECTURE_G90"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GT200: return "NV_GPU_ARCHITECTURE_GT200"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GF100: return "NV_GPU_ARCHITECTURE_GF100"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GF110: return "NV_GPU_ARCHITECTURE_GF110"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GK100: return "NV_GPU_ARCHITECTURE_GK100"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GK110: return "NV_GPU_ARCHITECTURE_GK110"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GK200: return "NV_GPU_ARCHITECTURE_GK200"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GM000: return "NV_GPU_ARCHITECTURE_GM000"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GM200: return "NV_GPU_ARCHITECTURE_GM200"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GP100: return "NV_GPU_ARCHITECTURE_GP100"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GV100: return "NV_GPU_ARCHITECTURE_GV100"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GV110: return "NV_GPU_ARCHITECTURE_GV110"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_TU100: return "NV_GPU_ARCHITECTURE_TU100"s;
			case _NV_GPU_ARCHITECTURE_ID::NV_GPU_ARCHITECTURE_GA100: return "NV_GPU_ARCHITECTURE_GA100"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_ARCH_IMPLEMENTATION_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_ARCH_IMPLEMENTATION_ID*>(pEnum);
			switch (e) {
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_T20: return "NV_GPU_ARCH_IMPLEMENTATION_T20"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_T35: return "NV_GPU_ARCH_IMPLEMENTATION_T35"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV41: return "NV_GPU_ARCH_IMPLEMENTATION_NV41"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV42: return "NV_GPU_ARCH_IMPLEMENTATION_NV42"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV43: return "NV_GPU_ARCH_IMPLEMENTATION_NV43"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV44: return "NV_GPU_ARCH_IMPLEMENTATION_NV44"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV44A: return "NV_GPU_ARCH_IMPLEMENTATION_NV44A"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV46: return "NV_GPU_ARCH_IMPLEMENTATION_NV46"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV47: return "NV_GPU_ARCH_IMPLEMENTATION_NV47"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV49: return "NV_GPU_ARCH_IMPLEMENTATION_NV49"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV4B: return "NV_GPU_ARCH_IMPLEMENTATION_NV4B"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV4C: return "NV_GPU_ARCH_IMPLEMENTATION_NV4C"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_NV4E: return "NV_GPU_ARCH_IMPLEMENTATION_NV4E"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_G98: return "NV_GPU_ARCH_IMPLEMENTATION_G98"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_GT21A: return "NV_GPU_ARCH_IMPLEMENTATION_GT21A"s;
			case _NV_GPU_ARCH_IMPLEMENTATION_ID::NV_GPU_ARCH_IMPLEMENTATION_MCP89: return "NV_GPU_ARCH_IMPLEMENTATION_MCP89"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_CHIP_REVISION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_CHIP_REVISION*>(pEnum);
			switch (e) {
			case _NV_GPU_CHIP_REVISION::NV_GPU_CHIP_REV_EMULATION_QT: return "NV_GPU_CHIP_REV_EMULATION_QT"s;
			case _NV_GPU_CHIP_REVISION::NV_GPU_CHIP_REV_EMULATION_FPGA: return "NV_GPU_CHIP_REV_EMULATION_FPGA"s;
			case _NV_GPU_CHIP_REVISION::NV_GPU_CHIP_REV_A01: return "NV_GPU_CHIP_REV_A01"s;
			case _NV_GPU_CHIP_REVISION::NV_GPU_CHIP_REV_A02: return "NV_GPU_CHIP_REV_A02"s;
			case _NV_GPU_CHIP_REVISION::NV_GPU_CHIP_REV_A03: return "NV_GPU_CHIP_REV_A03"s;
			case _NV_GPU_CHIP_REVISION::NV_GPU_CHIP_REV_UNKNOWN: return "NV_GPU_CHIP_REV_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_I2C_SPEED)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_I2C_SPEED*>(pEnum);
			switch (e) {
			case NV_I2C_SPEED::NVAPI_I2C_SPEED_DEFAULT: return "NVAPI_I2C_SPEED_DEFAULT"s;
			case NV_I2C_SPEED::NVAPI_I2C_SPEED_3KHZ: return "NVAPI_I2C_SPEED_3KHZ"s;
			case NV_I2C_SPEED::NVAPI_I2C_SPEED_10KHZ: return "NVAPI_I2C_SPEED_10KHZ"s;
			case NV_I2C_SPEED::NVAPI_I2C_SPEED_33KHZ: return "NVAPI_I2C_SPEED_33KHZ"s;
			case NV_I2C_SPEED::NVAPI_I2C_SPEED_100KHZ: return "NVAPI_I2C_SPEED_100KHZ"s;
			case NV_I2C_SPEED::NVAPI_I2C_SPEED_200KHZ: return "NVAPI_I2C_SPEED_200KHZ"s;
			case NV_I2C_SPEED::NVAPI_I2C_SPEED_400KHZ: return "NVAPI_I2C_SPEED_400KHZ"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NVAPI_GPU_WORKSTATION_FEATURE_MASK)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NVAPI_GPU_WORKSTATION_FEATURE_MASK*>(pEnum);
			switch (e) {
			case NVAPI_GPU_WORKSTATION_FEATURE_MASK::NVAPI_GPU_WORKSTATION_FEATURE_MASK_SWAPGROUP: return "NVAPI_GPU_WORKSTATION_FEATURE_MASK_SWAPGROUP"s;
			case NVAPI_GPU_WORKSTATION_FEATURE_MASK::NVAPI_GPU_WORKSTATION_FEATURE_MASK_STEREO: return "NVAPI_GPU_WORKSTATION_FEATURE_MASK_STEREO"s;
			case NVAPI_GPU_WORKSTATION_FEATURE_MASK::NVAPI_GPU_WORKSTATION_FEATURE_MASK_WARPING: return "NVAPI_GPU_WORKSTATION_FEATURE_MASK_WARPING"s;
			case NVAPI_GPU_WORKSTATION_FEATURE_MASK::NVAPI_GPU_WORKSTATION_FEATURE_MASK_PIXINTENSITY: return "NVAPI_GPU_WORKSTATION_FEATURE_MASK_PIXINTENSITY"s;
			case NVAPI_GPU_WORKSTATION_FEATURE_MASK::NVAPI_GPU_WORKSTATION_FEATURE_MASK_GRAYSCALE: return "NVAPI_GPU_WORKSTATION_FEATURE_MASK_GRAYSCALE"s;
			case NVAPI_GPU_WORKSTATION_FEATURE_MASK::NVAPI_GPU_WORKSTATION_FEATURE_MASK_BPC10: return "NVAPI_GPU_WORKSTATION_FEATURE_MASK_BPC10"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_HDCP_FUSE_STATE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_HDCP_FUSE_STATE*>(pEnum);
			switch (e) {
			case _NV_GPU_HDCP_FUSE_STATE::NV_GPU_HDCP_FUSE_STATE_UNKNOWN: return "NV_GPU_HDCP_FUSE_STATE_UNKNOWN"s;
			case _NV_GPU_HDCP_FUSE_STATE::NV_GPU_HDCP_FUSE_STATE_DISABLED: return "NV_GPU_HDCP_FUSE_STATE_DISABLED"s;
			case _NV_GPU_HDCP_FUSE_STATE::NV_GPU_HDCP_FUSE_STATE_ENABLED: return "NV_GPU_HDCP_FUSE_STATE_ENABLED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_HDCP_KEY_SOURCE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_HDCP_KEY_SOURCE*>(pEnum);
			switch (e) {
			case _NV_GPU_HDCP_KEY_SOURCE::NV_GPU_HDCP_KEY_SOURCE_UNKNOWN: return "NV_GPU_HDCP_KEY_SOURCE_UNKNOWN"s;
			case _NV_GPU_HDCP_KEY_SOURCE::NV_GPU_HDCP_KEY_SOURCE_NONE: return "NV_GPU_HDCP_KEY_SOURCE_NONE"s;
			case _NV_GPU_HDCP_KEY_SOURCE::NV_GPU_HDCP_KEY_SOURCE_CRYPTO_ROM: return "NV_GPU_HDCP_KEY_SOURCE_CRYPTO_ROM"s;
			case _NV_GPU_HDCP_KEY_SOURCE::NV_GPU_HDCP_KEY_SOURCE_SBIOS: return "NV_GPU_HDCP_KEY_SOURCE_SBIOS"s;
			case _NV_GPU_HDCP_KEY_SOURCE::NV_GPU_HDCP_KEY_SOURCE_I2C_ROM: return "NV_GPU_HDCP_KEY_SOURCE_I2C_ROM"s;
			case _NV_GPU_HDCP_KEY_SOURCE::NV_GPU_HDCP_KEY_SOURCE_FUSES: return "NV_GPU_HDCP_KEY_SOURCE_FUSES"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_HDCP_KEY_SOURCE_STATE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_HDCP_KEY_SOURCE_STATE*>(pEnum);
			switch (e) {
			case _NV_GPU_HDCP_KEY_SOURCE_STATE::NV_GPU_HDCP_KEY_SOURCE_STATE_UNKNOWN: return "NV_GPU_HDCP_KEY_SOURCE_STATE_UNKNOWN"s;
			case _NV_GPU_HDCP_KEY_SOURCE_STATE::NV_GPU_HDCP_KEY_SOURCE_STATE_ABSENT: return "NV_GPU_HDCP_KEY_SOURCE_STATE_ABSENT"s;
			case _NV_GPU_HDCP_KEY_SOURCE_STATE::NV_GPU_HDCP_KEY_SOURCE_STATE_PRESENT: return "NV_GPU_HDCP_KEY_SOURCE_STATE_PRESENT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_ECC_CONFIGURATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_ECC_CONFIGURATION*>(pEnum);
			switch (e) {
			case _NV_ECC_CONFIGURATION::NV_ECC_CONFIGURATION_NOT_SUPPORTED: return "NV_ECC_CONFIGURATION_NOT_SUPPORTED"s;
			case _NV_ECC_CONFIGURATION::NV_ECC_CONFIGURATION_DEFERRED: return "NV_ECC_CONFIGURATION_DEFERRED"s;
			case _NV_ECC_CONFIGURATION::NV_ECC_CONFIGURATION_IMMEDIATE: return "NV_ECC_CONFIGURATION_IMMEDIATE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_QSYNC_EVENT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_QSYNC_EVENT*>(pEnum);
			switch (e) {
			case NV_QSYNC_EVENT::NV_QSYNC_EVENT_NONE: return "NV_QSYNC_EVENT_NONE"s;
			case NV_QSYNC_EVENT::NV_QSYNC_EVENT_SYNC_LOSS: return "NV_QSYNC_EVENT_SYNC_LOSS"s;
			case NV_QSYNC_EVENT::NV_QSYNC_EVENT_SYNC_GAIN: return "NV_QSYNC_EVENT_SYNC_GAIN"s;
			case NV_QSYNC_EVENT::NV_QSYNC_EVENT_HOUSESYNC_GAIN: return "NV_QSYNC_EVENT_HOUSESYNC_GAIN"s;
			case NV_QSYNC_EVENT::NV_QSYNC_EVENT_HOUSESYNC_LOSS: return "NV_QSYNC_EVENT_HOUSESYNC_LOSS"s;
			case NV_QSYNC_EVENT::NV_QSYNC_EVENT_RJ45_GAIN: return "NV_QSYNC_EVENT_RJ45_GAIN"s;
			case NV_QSYNC_EVENT::NV_QSYNC_EVENT_RJ45_LOSS: return "NV_QSYNC_EVENT_RJ45_LOSS"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_EVENT_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_EVENT_TYPE*>(pEnum);
			switch (e) {
			case NV_EVENT_TYPE::NV_EVENT_TYPE_NONE: return "NV_EVENT_TYPE_NONE"s;
			case NV_EVENT_TYPE::NV_EVENT_TYPE_QSYNC: return "NV_EVENT_TYPE_QSYNC"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_WORKSTATION_FEATURE_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_WORKSTATION_FEATURE_TYPE*>(pEnum);
			switch (e) {
			case _NV_GPU_WORKSTATION_FEATURE_TYPE::NV_GPU_WORKSTATION_FEATURE_TYPE_NVIDIA_RTX_VR_READY: return "NV_GPU_WORKSTATION_FEATURE_TYPE_NVIDIA_RTX_VR_READY"s;
			case _NV_GPU_WORKSTATION_FEATURE_TYPE::NV_GPU_WORKSTATION_FEATURE_TYPE_PROVIZ: return "NV_GPU_WORKSTATION_FEATURE_TYPE_PROVIZ"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_WARPING_VERTICE_FORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_WARPING_VERTICE_FORMAT*>(pEnum);
			switch (e) {
			case NV_GPU_WARPING_VERTICE_FORMAT::NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLESTRIP_XYUVRQ: return "NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLESTRIP_XYUVRQ"s;
			case NV_GPU_WARPING_VERTICE_FORMAT::NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLES_XYUVRQ: return "NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLES_XYUVRQ"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_SCANOUT_COMPOSITION_PARAMETER)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_SCANOUT_COMPOSITION_PARAMETER*>(pEnum);
			switch (e) {
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_WARPING_RESAMPLING_METHOD: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_WARPING_RESAMPLING_METHOD"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE*>(pEnum);
			switch (e) {
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_SET_TO_DEFAULT: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_SET_TO_DEFAULT"s;
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BILINEAR: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BILINEAR"s;
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_TRIANGULAR: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_TRIANGULAR"s;
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_BELL_SHAPED: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_BELL_SHAPED"s;
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_BSPLINE: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_BSPLINE"s;
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_ADAPTIVE_TRIANGULAR: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_ADAPTIVE_TRIANGULAR"s;
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_ADAPTIVE_BELL_SHAPED: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_ADAPTIVE_BELL_SHAPED"s;
			case NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE::NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_ADAPTIVE_BSPLINE: return "NV_GPU_SCANOUT_COMPOSITION_PARAMETER_VALUE_WARPING_RESAMPLING_METHOD_BICUBIC_ADAPTIVE_BSPLINE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_VIRTUALIZATION_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_VIRTUALIZATION_MODE*>(pEnum);
			switch (e) {
			case _NV_VIRTUALIZATION_MODE::NV_VIRTUALIZATION_MODE_NONE: return "NV_VIRTUALIZATION_MODE_NONE"s;
			case _NV_VIRTUALIZATION_MODE::NV_VIRTUALIZATION_MODE_NMOS: return "NV_VIRTUALIZATION_MODE_NMOS"s;
			case _NV_VIRTUALIZATION_MODE::NV_VIRTUALIZATION_MODE_VGX: return "NV_VIRTUALIZATION_MODE_VGX"s;
			case _NV_VIRTUALIZATION_MODE::NV_VIRTUALIZATION_MODE_HOST_VGPU: return "NV_VIRTUALIZATION_MODE_HOST_VGPU"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_LICENSE_FEATURE_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_LICENSE_FEATURE_TYPE*>(pEnum);
			switch (e) {
			case _NV_LICENSE_FEATURE_TYPE::NV_LICENSE_FEATURE_UNKNOWN: return "NV_LICENSE_FEATURE_UNKNOWN"s;
			case _NV_LICENSE_FEATURE_TYPE::NV_LICENSE_FEATURE_VGPU: return "NV_LICENSE_FEATURE_VGPU"s;
			case _NV_LICENSE_FEATURE_TYPE::NV_LICENSE_FEATURE_NVIDIA_RTX: return "NV_LICENSE_FEATURE_NVIDIA_RTX"s;
			case _NV_LICENSE_FEATURE_TYPE::NV_LICENSE_FEATURE_GAMING: return "NV_LICENSE_FEATURE_GAMING"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GPU_PERF_DECREASE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GPU_PERF_DECREASE*>(pEnum);
			switch (e) {
			case _NVAPI_GPU_PERF_DECREASE::NV_GPU_PERF_DECREASE_NONE: return "NV_GPU_PERF_DECREASE_NONE"s;
			case _NVAPI_GPU_PERF_DECREASE::NV_GPU_PERF_DECREASE_REASON_THERMAL_PROTECTION: return "NV_GPU_PERF_DECREASE_REASON_THERMAL_PROTECTION"s;
			case _NVAPI_GPU_PERF_DECREASE::NV_GPU_PERF_DECREASE_REASON_POWER_CONTROL: return "NV_GPU_PERF_DECREASE_REASON_POWER_CONTROL"s;
			case _NVAPI_GPU_PERF_DECREASE::NV_GPU_PERF_DECREASE_REASON_AC_BATT: return "NV_GPU_PERF_DECREASE_REASON_AC_BATT"s;
			case _NVAPI_GPU_PERF_DECREASE::NV_GPU_PERF_DECREASE_REASON_API_TRIGGERED: return "NV_GPU_PERF_DECREASE_REASON_API_TRIGGERED"s;
			case _NVAPI_GPU_PERF_DECREASE::NV_GPU_PERF_DECREASE_REASON_INSUFFICIENT_POWER: return "NV_GPU_PERF_DECREASE_REASON_INSUFFICIENT_POWER"s;
			case _NVAPI_GPU_PERF_DECREASE::NV_GPU_PERF_DECREASE_REASON_UNKNOWN: return "NV_GPU_PERF_DECREASE_REASON_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_THERMAL_TARGET)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_THERMAL_TARGET*>(pEnum);
			switch (e) {
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_NONE: return "NVAPI_THERMAL_TARGET_NONE"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_GPU: return "NVAPI_THERMAL_TARGET_GPU"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_MEMORY: return "NVAPI_THERMAL_TARGET_MEMORY"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_POWER_SUPPLY: return "NVAPI_THERMAL_TARGET_POWER_SUPPLY"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_BOARD: return "NVAPI_THERMAL_TARGET_BOARD"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_VCD_BOARD: return "NVAPI_THERMAL_TARGET_VCD_BOARD"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_VCD_INLET: return "NVAPI_THERMAL_TARGET_VCD_INLET"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_VCD_OUTLET: return "NVAPI_THERMAL_TARGET_VCD_OUTLET"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_ALL: return "NVAPI_THERMAL_TARGET_ALL"s;
			case NV_THERMAL_TARGET::NVAPI_THERMAL_TARGET_UNKNOWN: return "NVAPI_THERMAL_TARGET_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_THERMAL_CONTROLLER)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_THERMAL_CONTROLLER*>(pEnum);
			switch (e) {
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_NONE: return "NVAPI_THERMAL_CONTROLLER_NONE"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL: return "NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_ADM1032: return "NVAPI_THERMAL_CONTROLLER_ADM1032"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_MAX6649: return "NVAPI_THERMAL_CONTROLLER_MAX6649"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_MAX1617: return "NVAPI_THERMAL_CONTROLLER_MAX1617"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_LM99: return "NVAPI_THERMAL_CONTROLLER_LM99"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_LM89: return "NVAPI_THERMAL_CONTROLLER_LM89"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_LM64: return "NVAPI_THERMAL_CONTROLLER_LM64"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_ADT7473: return "NVAPI_THERMAL_CONTROLLER_ADT7473"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_SBMAX6649: return "NVAPI_THERMAL_CONTROLLER_SBMAX6649"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_VBIOSEVT: return "NVAPI_THERMAL_CONTROLLER_VBIOSEVT"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_OS: return "NVAPI_THERMAL_CONTROLLER_OS"s;
			case NV_THERMAL_CONTROLLER::NVAPI_THERMAL_CONTROLLER_UNKNOWN: return "NVAPI_THERMAL_CONTROLLER_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE*>(pEnum);
			switch (e) {
			case NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE::NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ: return "NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ"s;
			case NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE::NV_GPU_CLOCK_FREQUENCIES_BASE_CLOCK: return "NV_GPU_CLOCK_FREQUENCIES_BASE_CLOCK"s;
			case NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE::NV_GPU_CLOCK_FREQUENCIES_BOOST_CLOCK: return "NV_GPU_CLOCK_FREQUENCIES_BOOST_CLOCK"s;
			case NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE::NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM: return "NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_ILLUMINATION_ATTRIB)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_ILLUMINATION_ATTRIB*>(pEnum);
			switch (e) {
			case _NV_GPU_ILLUMINATION_ATTRIB::NV_GPU_IA_LOGO_BRIGHTNESS: return "NV_GPU_IA_LOGO_BRIGHTNESS"s;
			case _NV_GPU_ILLUMINATION_ATTRIB::NV_GPU_IA_SLI_BRIGHTNESS: return "NV_GPU_IA_SLI_BRIGHTNESS"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_CTRL_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_CLIENT_ILLUM_CTRL_MODE*>(pEnum);
			switch (e) {
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE::NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL_RGB: return "NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL_RGB"s;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE::NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_RGB: return "NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_RGB"s;
			case NV_GPU_CLIENT_ILLUM_CTRL_MODE::NV_GPU_CLIENT_ILLUM_CTRL_MODE_INVALID: return "NV_GPU_CLIENT_ILLUM_CTRL_MODE_INVALID"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_ZONE_LOCATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_CLIENT_ILLUM_ZONE_LOCATION*>(pEnum);
			switch (e) {
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION::NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0: return "NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION::NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0: return "NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION::NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0: return "NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION::NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0: return "NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION::NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID: return "NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_DEVICE_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_CLIENT_ILLUM_DEVICE_TYPE*>(pEnum);
			switch (e) {
			case NV_GPU_CLIENT_ILLUM_DEVICE_TYPE::NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_INVALID: return "NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_INVALID"s;
			case NV_GPU_CLIENT_ILLUM_DEVICE_TYPE::NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_MCUV10: return "NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_MCUV10"s;
			case NV_GPU_CLIENT_ILLUM_DEVICE_TYPE::NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_GPIO_PWM_RGBW_V10: return "NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_GPIO_PWM_RGBW_V10"s;
			case NV_GPU_CLIENT_ILLUM_DEVICE_TYPE::NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_GPIO_PWM_SINGLE_COLOR_V10: return "NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_GPIO_PWM_SINGLE_COLOR_V10"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_ZONE_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_CLIENT_ILLUM_ZONE_TYPE*>(pEnum);
			switch (e) {
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE::NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID: return "NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE::NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB: return "NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE::NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED: return "NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE::NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW: return "NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW"s;
			case NV_GPU_CLIENT_ILLUM_ZONE_TYPE::NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR: return "NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE*>(pEnum);
			switch (e) {
			case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE::NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_HALF_HALT: return "NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_HALF_HALT"s;
			case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE::NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_HALT: return "NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_HALT"s;
			case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE::NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_REPEAT: return "NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_REPEAT"s;
			case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE::NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_INVALID: return "NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_INVALID"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_CMD)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_CMD*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_GET_DEFAULT: return "NV_INFOFRAME_CMD_GET_DEFAULT"s;
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_RESET: return "NV_INFOFRAME_CMD_RESET"s;
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_GET: return "NV_INFOFRAME_CMD_GET"s;
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_SET: return "NV_INFOFRAME_CMD_SET"s;
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_GET_OVERRIDE: return "NV_INFOFRAME_CMD_GET_OVERRIDE"s;
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_SET_OVERRIDE: return "NV_INFOFRAME_CMD_SET_OVERRIDE"s;
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_GET_PROPERTY: return "NV_INFOFRAME_CMD_GET_PROPERTY"s;
			case NV_INFOFRAME_CMD::NV_INFOFRAME_CMD_SET_PROPERTY: return "NV_INFOFRAME_CMD_SET_PROPERTY"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_PROPERTY_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_PROPERTY_MODE*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_PROPERTY_MODE::NV_INFOFRAME_PROPERTY_MODE_AUTO: return "NV_INFOFRAME_PROPERTY_MODE_AUTO"s;
			case NV_INFOFRAME_PROPERTY_MODE::NV_INFOFRAME_PROPERTY_MODE_ENABLE: return "NV_INFOFRAME_PROPERTY_MODE_ENABLE"s;
			case NV_INFOFRAME_PROPERTY_MODE::NV_INFOFRAME_PROPERTY_MODE_DISABLE: return "NV_INFOFRAME_PROPERTY_MODE_DISABLE"s;
			case NV_INFOFRAME_PROPERTY_MODE::NV_INFOFRAME_PROPERTY_MODE_ALLOW_OVERRIDE: return "NV_INFOFRAME_PROPERTY_MODE_ALLOW_OVERRIDE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_PROPERTY_BLACKLIST)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_PROPERTY_BLACKLIST*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_PROPERTY_BLACKLIST::NV_INFOFRAME_PROPERTY_BLACKLIST_FALSE: return "NV_INFOFRAME_PROPERTY_BLACKLIST_FALSE"s;
			case NV_INFOFRAME_PROPERTY_BLACKLIST::NV_INFOFRAME_PROPERTY_BLACKLIST_TRUE: return "NV_INFOFRAME_PROPERTY_BLACKLIST_TRUE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO::NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_NODATA: return "NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_NODATA"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO::NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_OVERSCAN: return "NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_OVERSCAN"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO::NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_UNDERSCAN: return "NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_UNDERSCAN"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO::NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_FUTURE: return "NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_FUTURE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO::NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA::NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_NOT_PRESENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_NOT_PRESENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA::NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_VERTICAL_PRESENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_VERTICAL_PRESENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA::NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_HORIZONTAL_PRESENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_HORIZONTAL_PRESENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA::NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_BOTH_PRESENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_BOTH_PRESENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA::NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_ACTIVEFORMATINFO)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_ACTIVEFORMATINFO*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_ACTIVEFORMATINFO::NV_INFOFRAME_FIELD_VALUE_AVI_AFI_ABSENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_AFI_ABSENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ACTIVEFORMATINFO::NV_INFOFRAME_FIELD_VALUE_AVI_AFI_PRESENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_AFI_PRESENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ACTIVEFORMATINFO::NV_INFOFRAME_FIELD_VALUE_AVI_AFI_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_AFI_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT::NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_RGB: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_RGB"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT::NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_YCbCr422: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_YCbCr422"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT::NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_YCbCr444: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_YCbCr444"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT::NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_FUTURE: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_FUTURE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT::NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_F17)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_F17*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_F17::NV_INFOFRAME_FIELD_VALUE_AVI_F17_FALSE: return "NV_INFOFRAME_FIELD_VALUE_AVI_F17_FALSE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_F17::NV_INFOFRAME_FIELD_VALUE_AVI_F17_TRUE: return "NV_INFOFRAME_FIELD_VALUE_AVI_F17_TRUE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_F17::NV_INFOFRAME_FIELD_VALUE_AVI_F17_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_F17_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_NO_AFD: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_NO_AFD"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE01: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE01"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE02: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE02"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE03: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE03"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_LETTERBOX_GT16x9: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_LETTERBOX_GT16x9"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE05: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE05"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE06: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE06"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE07: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE07"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_EQUAL_CODEDFRAME: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_EQUAL_CODEDFRAME"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_4x3: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_4x3"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_16x9: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_16x9"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_14x9: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_14x9"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE12: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE12"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_4x3_ON_14x9: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_4x3_ON_14x9"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_16x9_ON_14x9: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_16x9_ON_14x9"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_16x9_ON_4x3: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_16x9_ON_4x3"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_NO_DATA: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_NO_DATA"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_4x3: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_4x3"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_16x9: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_16x9"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_FUTURE: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_FUTURE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME::NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_NO_DATA: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_NO_DATA"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_SMPTE_170M: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_SMPTE_170M"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_ITUR_BT709: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_ITUR_BT709"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_USE_EXTENDED_COLORIMETRY: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_USE_EXTENDED_COLORIMETRY"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING::NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_NO_DATA: return "NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_NO_DATA"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING::NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_HORIZONTAL: return "NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_HORIZONTAL"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING::NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_VERTICAL: return "NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_VERTICAL"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING::NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_BOTH: return "NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_BOTH"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING::NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_DEFAULT: return "NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_DEFAULT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_LIMITED_RANGE: return "NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_LIMITED_RANGE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_FULL_RANGE: return "NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_FULL_RANGE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_RESERVED: return "NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_RESERVED"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_XVYCC601: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_XVYCC601"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_XVYCC709: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_XVYCC709"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_SYCC601: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_SYCC601"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_ADOBEYCC601: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_ADOBEYCC601"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_ADOBERGB: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_ADOBERGB"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED05: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED05"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED06: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED06"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED07: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED07"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY::NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_ITC)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_ITC*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_ITC::NV_INFOFRAME_FIELD_VALUE_AVI_ITC_VIDEO_CONTENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_ITC_VIDEO_CONTENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ITC::NV_INFOFRAME_FIELD_VALUE_AVI_ITC_ITCONTENT: return "NV_INFOFRAME_FIELD_VALUE_AVI_ITC_ITCONTENT"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_ITC::NV_INFOFRAME_FIELD_VALUE_AVI_ITC_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_ITC_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_NONE: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_NONE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X02: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X02"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X03: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X03"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X04: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X04"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X05: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X05"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X06: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X06"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X07: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X07"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X08: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X08"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X09: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X09"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X10: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X10"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED10: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED10"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED11: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED11"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED12: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED12"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED13: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED13"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED14: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED14"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED15: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED15"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION::NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE::NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_GRAPHICS: return "NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_GRAPHICS"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE::NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_PHOTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_PHOTO"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE::NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_CINEMA: return "NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_CINEMA"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE::NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_GAME: return "NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_GAME"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE::NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_LIMITED_RANGE: return "NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_LIMITED_RANGE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_FULL_RANGE: return "NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_FULL_RANGE"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_RESERVED02: return "NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_RESERVED02"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_RESERVED03: return "NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_RESERVED03"s;
			case NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION::NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_IN_HEADER: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_IN_HEADER"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_2: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_2"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_3: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_3"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_4: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_4"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_5: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_5"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_6: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_6"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_7: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_7"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_8: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_8"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_IN_HEADER: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_IN_HEADER"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_PCM: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_PCM"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AC3: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AC3"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MPEG1: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MPEG1"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MP3: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MP3"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MPEG2: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MPEG2"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AACLC: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AACLC"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DTS: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DTS"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_ATRAC: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_ATRAC"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DSD: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DSD"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_EAC3: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_EAC3"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DTSHD: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DTSHD"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MLP: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MLP"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DST: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DST"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_WMAPRO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_WMAPRO"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_USE_CODING_EXTENSION_TYPE: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_USE_CODING_EXTENSION_TYPE"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_IN_HEADER: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_IN_HEADER"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_16BITS: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_16BITS"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_20BITS: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_20BITS"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_24BITS: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_24BITS"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_IN_HEADER: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_IN_HEADER"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_32000HZ: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_32000HZ"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_44100HZ: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_44100HZ"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_48000HZ: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_48000HZ"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_88200KHZ: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_88200KHZ"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_96000KHZ: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_96000KHZ"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_176400KHZ: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_176400KHZ"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_192000KHZ: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_192000KHZ"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY::NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_USE_CODING_TYPE: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_USE_CODING_TYPE"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_HEAAC: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_HEAAC"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_HEAACV2: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_HEAACV2"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_MPEGSURROUND: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_MPEGSURROUND"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE04: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE04"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE05: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE05"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE06: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE06"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE07: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE07"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE08: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE08"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE09: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE09"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE10: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE10"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE11: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE11"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE12: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE12"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE13: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE13"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE14: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE14"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE15: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE15"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE16: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE16"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE17: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE17"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE18: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE18"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE19: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE19"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE20: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE20"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE21: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE21"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE22: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE22"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE23: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE23"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE24: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE24"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE25: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE25"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE26: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE26"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE27: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE27"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE28: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE28"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE29: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE29"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE30: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE30"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE31: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE31"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE::NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_FCH_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_FCH_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_FCH_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_FCH_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_X_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_X_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_X_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_X_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_X_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_X_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_X_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_X_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_RC_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_RC_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_RC_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_RC_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FCH_RC_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FCH_RC_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FCH_RC_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FCH_RC_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_FCH_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_FCH_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_FCH_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_FCH_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_FC_X_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_FC_X_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_FC_LFE_FR_FL: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_FC_LFE_FR_FL"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION::NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL::NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_NO_DATA: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_NO_DATA"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL::NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_0DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_0DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL::NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_PLUS10DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_PLUS10DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL::NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_RESERVED03: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_RESERVED03"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL::NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_0DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_0DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_1DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_1DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_2DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_2DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_3DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_3DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_4DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_4DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_5DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_5DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_6DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_6DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_7DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_7DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_8DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_8DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_9DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_9DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_10DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_10DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_11DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_11DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_12DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_12DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_13DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_13DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_14DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_14DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_15DB: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_15DB"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES::NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX*>(pEnum);
			switch (e) {
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX::NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_PERMITTED: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_PERMITTED"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX::NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_PROHIBITED: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_PROHIBITED"s;
			case NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX::NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_AUTO: return "NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_COLOR_CMD)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_COLOR_CMD*>(pEnum);
			switch (e) {
			case NV_COLOR_CMD::NV_COLOR_CMD_GET: return "NV_COLOR_CMD_GET"s;
			case NV_COLOR_CMD::NV_COLOR_CMD_SET: return "NV_COLOR_CMD_SET"s;
			case NV_COLOR_CMD::NV_COLOR_CMD_IS_SUPPORTED_COLOR: return "NV_COLOR_CMD_IS_SUPPORTED_COLOR"s;
			case NV_COLOR_CMD::NV_COLOR_CMD_GET_DEFAULT: return "NV_COLOR_CMD_GET_DEFAULT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_COLOR_FORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_COLOR_FORMAT*>(pEnum);
			switch (e) {
			case NV_COLOR_FORMAT::NV_COLOR_FORMAT_RGB: return "NV_COLOR_FORMAT_RGB"s;
			case NV_COLOR_FORMAT::NV_COLOR_FORMAT_YUV422: return "NV_COLOR_FORMAT_YUV422"s;
			case NV_COLOR_FORMAT::NV_COLOR_FORMAT_YUV444: return "NV_COLOR_FORMAT_YUV444"s;
			case NV_COLOR_FORMAT::NV_COLOR_FORMAT_YUV420: return "NV_COLOR_FORMAT_YUV420"s;
			case NV_COLOR_FORMAT::NV_COLOR_FORMAT_DEFAULT: return "NV_COLOR_FORMAT_DEFAULT"s;
			case NV_COLOR_FORMAT::NV_COLOR_FORMAT_AUTO: return "NV_COLOR_FORMAT_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_COLOR_COLORIMETRY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_COLOR_COLORIMETRY*>(pEnum);
			switch (e) {
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_RGB: return "NV_COLOR_COLORIMETRY_RGB"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_YCC601: return "NV_COLOR_COLORIMETRY_YCC601"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_YCC709: return "NV_COLOR_COLORIMETRY_YCC709"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_XVYCC601: return "NV_COLOR_COLORIMETRY_XVYCC601"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_XVYCC709: return "NV_COLOR_COLORIMETRY_XVYCC709"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_SYCC601: return "NV_COLOR_COLORIMETRY_SYCC601"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_ADOBEYCC601: return "NV_COLOR_COLORIMETRY_ADOBEYCC601"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_ADOBERGB: return "NV_COLOR_COLORIMETRY_ADOBERGB"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_BT2020RGB: return "NV_COLOR_COLORIMETRY_BT2020RGB"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_BT2020YCC: return "NV_COLOR_COLORIMETRY_BT2020YCC"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_BT2020cYCC: return "NV_COLOR_COLORIMETRY_BT2020cYCC"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_DEFAULT: return "NV_COLOR_COLORIMETRY_DEFAULT"s;
			case NV_COLOR_COLORIMETRY::NV_COLOR_COLORIMETRY_AUTO: return "NV_COLOR_COLORIMETRY_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DYNAMIC_RANGE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DYNAMIC_RANGE*>(pEnum);
			switch (e) {
			case _NV_DYNAMIC_RANGE::NV_DYNAMIC_RANGE_VESA: return "NV_DYNAMIC_RANGE_VESA"s;
			case _NV_DYNAMIC_RANGE::NV_DYNAMIC_RANGE_CEA: return "NV_DYNAMIC_RANGE_CEA"s;
			case _NV_DYNAMIC_RANGE::NV_DYNAMIC_RANGE_AUTO: return "NV_DYNAMIC_RANGE_AUTO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_BPC)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_BPC*>(pEnum);
			switch (e) {
			case _NV_BPC::NV_BPC_DEFAULT: return "NV_BPC_DEFAULT"s;
			case _NV_BPC::NV_BPC_6: return "NV_BPC_6"s;
			case _NV_BPC::NV_BPC_8: return "NV_BPC_8"s;
			case _NV_BPC::NV_BPC_10: return "NV_BPC_10"s;
			case _NV_BPC::NV_BPC_12: return "NV_BPC_12"s;
			case _NV_BPC::NV_BPC_16: return "NV_BPC_16"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_COLOR_SELECTION_POLICY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_COLOR_SELECTION_POLICY*>(pEnum);
			switch (e) {
			case _NV_COLOR_SELECTION_POLICY::NV_COLOR_SELECTION_POLICY_USER: return "NV_COLOR_SELECTION_POLICY_USER"s;
			case _NV_COLOR_SELECTION_POLICY::NV_COLOR_SELECTION_POLICY_BEST_QUALITY: return "NV_COLOR_SELECTION_POLICY_BEST_QUALITY"s;
			case _NV_COLOR_SELECTION_POLICY::NV_COLOR_SELECTION_POLICY_UNKNOWN: return "NV_COLOR_SELECTION_POLICY_UNKNOWN"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_DESKTOP_COLOR_DEPTH)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_DESKTOP_COLOR_DEPTH*>(pEnum);
			switch (e) {
			case _NV_DESKTOP_COLOR_DEPTH::NV_DESKTOP_COLOR_DEPTH_DEFAULT: return "NV_DESKTOP_COLOR_DEPTH_DEFAULT"s;
			case _NV_DESKTOP_COLOR_DEPTH::NV_DESKTOP_COLOR_DEPTH_8BPC: return "NV_DESKTOP_COLOR_DEPTH_8BPC"s;
			case _NV_DESKTOP_COLOR_DEPTH::NV_DESKTOP_COLOR_DEPTH_10BPC: return "NV_DESKTOP_COLOR_DEPTH_10BPC"s;
			case _NV_DESKTOP_COLOR_DEPTH::NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT: return "NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT"s;
			case _NV_DESKTOP_COLOR_DEPTH::NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT_WCG: return "NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT_WCG"s;
			case _NV_DESKTOP_COLOR_DEPTH::NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT_HDR: return "NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT_HDR"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_STATIC_METADATA_DESCRIPTOR_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_STATIC_METADATA_DESCRIPTOR_ID*>(pEnum);
			switch (e) {
			case NV_STATIC_METADATA_DESCRIPTOR_ID::NV_STATIC_METADATA_TYPE_1: return "NV_STATIC_METADATA_TYPE_1"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_HDR_CMD)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_HDR_CMD*>(pEnum);
			switch (e) {
			case NV_HDR_CMD::NV_HDR_CMD_GET: return "NV_HDR_CMD_GET"s;
			case NV_HDR_CMD::NV_HDR_CMD_SET: return "NV_HDR_CMD_SET"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_HDR_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_HDR_MODE*>(pEnum);
			switch (e) {
			case NV_HDR_MODE::NV_HDR_MODE_OFF: return "NV_HDR_MODE_OFF"s;
			case NV_HDR_MODE::NV_HDR_MODE_UHDA: return "NV_HDR_MODE_UHDA"s;
			case NV_HDR_MODE::NV_HDR_MODE_UHDA_PASSTHROUGH: return "NV_HDR_MODE_UHDA_PASSTHROUGH"s;
			case NV_HDR_MODE::NV_HDR_MODE_DOLBY_VISION: return "NV_HDR_MODE_DOLBY_VISION"s;
			case NV_HDR_MODE::NV_HDR_MODE_EDR: return "NV_HDR_MODE_EDR"s;
			case NV_HDR_MODE::NV_HDR_MODE_SDR: return "NV_HDR_MODE_SDR"s;
			case NV_HDR_MODE::NV_HDR_MODE_UHDA_NB: return "NV_HDR_MODE_UHDA_NB"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_MONITOR_CAPS_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_MONITOR_CAPS_TYPE*>(pEnum);
			switch (e) {
			case NV_MONITOR_CAPS_TYPE::NV_MONITOR_CAPS_TYPE_HDMI_VSDB: return "NV_MONITOR_CAPS_TYPE_HDMI_VSDB"s;
			case NV_MONITOR_CAPS_TYPE::NV_MONITOR_CAPS_TYPE_HDMI_VCDB: return "NV_MONITOR_CAPS_TYPE_HDMI_VCDB"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_MOSAIC_TOPO_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_MOSAIC_TOPO_TYPE*>(pEnum);
			switch (e) {
			case NV_MOSAIC_TOPO_TYPE::NV_MOSAIC_TOPO_TYPE_ALL: return "NV_MOSAIC_TOPO_TYPE_ALL"s;
			case NV_MOSAIC_TOPO_TYPE::NV_MOSAIC_TOPO_TYPE_BASIC: return "NV_MOSAIC_TOPO_TYPE_BASIC"s;
			case NV_MOSAIC_TOPO_TYPE::NV_MOSAIC_TOPO_TYPE_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_TYPE_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO_TYPE::NV_MOSAIC_TOPO_TYPE_SCALED_CLONE: return "NV_MOSAIC_TOPO_TYPE_SCALED_CLONE"s;
			case NV_MOSAIC_TOPO_TYPE::NV_MOSAIC_TOPO_TYPE_PASSIVE_STEREO_SCALED_CLONE: return "NV_MOSAIC_TOPO_TYPE_PASSIVE_STEREO_SCALED_CLONE"s;
			case NV_MOSAIC_TOPO_TYPE::NV_MOSAIC_TOPO_TYPE_MAX: return "NV_MOSAIC_TOPO_TYPE_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_MOSAIC_TOPO)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_MOSAIC_TOPO*>(pEnum);
			switch (e) {
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_NONE: return "NV_MOSAIC_TOPO_NONE"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_BEGIN_BASIC: return "NV_MOSAIC_TOPO_BEGIN_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_2x1_BASIC: return "NV_MOSAIC_TOPO_2x1_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_1x3_BASIC: return "NV_MOSAIC_TOPO_1x3_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_3x1_BASIC: return "NV_MOSAIC_TOPO_3x1_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_1x4_BASIC: return "NV_MOSAIC_TOPO_1x4_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_4x1_BASIC: return "NV_MOSAIC_TOPO_4x1_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_2x2_BASIC: return "NV_MOSAIC_TOPO_2x2_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_2x3_BASIC: return "NV_MOSAIC_TOPO_2x3_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_2x4_BASIC: return "NV_MOSAIC_TOPO_2x4_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_3x2_BASIC: return "NV_MOSAIC_TOPO_3x2_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_4x2_BASIC: return "NV_MOSAIC_TOPO_4x2_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_1x5_BASIC: return "NV_MOSAIC_TOPO_1x5_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_1x6_BASIC: return "NV_MOSAIC_TOPO_1x6_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_7x1_BASIC: return "NV_MOSAIC_TOPO_7x1_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_END_BASIC: return "NV_MOSAIC_TOPO_END_BASIC"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_BEGIN_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_BEGIN_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_2x1_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_2x1_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_1x3_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_1x3_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_3x1_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_3x1_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_1x4_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_1x4_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_4x1_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_4x1_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_2x2_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_2x2_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_END_PASSIVE_STEREO: return "NV_MOSAIC_TOPO_END_PASSIVE_STEREO"s;
			case NV_MOSAIC_TOPO::NV_MOSAIC_TOPO_MAX: return "NV_MOSAIC_TOPO_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_PIXEL_SHIFT_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_PIXEL_SHIFT_TYPE*>(pEnum);
			switch (e) {
			case _NV_PIXEL_SHIFT_TYPE::NV_PIXEL_SHIFT_TYPE_NO_PIXEL_SHIFT: return "NV_PIXEL_SHIFT_TYPE_NO_PIXEL_SHIFT"s;
			case _NV_PIXEL_SHIFT_TYPE::NV_PIXEL_SHIFT_TYPE_2x2_TOP_LEFT_PIXELS: return "NV_PIXEL_SHIFT_TYPE_2x2_TOP_LEFT_PIXELS"s;
			case _NV_PIXEL_SHIFT_TYPE::NV_PIXEL_SHIFT_TYPE_2x2_BOTTOM_RIGHT_PIXELS: return "NV_PIXEL_SHIFT_TYPE_2x2_BOTTOM_RIGHT_PIXELS"s;
			case _NV_PIXEL_SHIFT_TYPE::NV_PIXEL_SHIFT_TYPE_2x2_TOP_RIGHT_PIXELS: return "NV_PIXEL_SHIFT_TYPE_2x2_TOP_RIGHT_PIXELS"s;
			case _NV_PIXEL_SHIFT_TYPE::NV_PIXEL_SHIFT_TYPE_2x2_BOTTOM_LEFT_PIXELS: return "NV_PIXEL_SHIFT_TYPE_2x2_BOTTOM_LEFT_PIXELS"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR*>(pEnum);
			switch (e) {
			case _NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR::NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_NONE: return "NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_NONE"s;
			case _NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR::NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_PRIMARY: return "NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_PRIMARY"s;
			case _NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR::NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_SECONDARY: return "NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_SECONDARY"s;
			case _NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR::NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_TERTIARY: return "NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_TERTIARY"s;
			case _NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR::NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_QUARTERNARY: return "NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_QUARTERNARY"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GSYNC_DISPLAY_SYNC_STATE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GSYNC_DISPLAY_SYNC_STATE*>(pEnum);
			switch (e) {
			case _NVAPI_GSYNC_DISPLAY_SYNC_STATE::NVAPI_GSYNC_DISPLAY_SYNC_STATE_UNSYNCED: return "NVAPI_GSYNC_DISPLAY_SYNC_STATE_UNSYNCED"s;
			case _NVAPI_GSYNC_DISPLAY_SYNC_STATE::NVAPI_GSYNC_DISPLAY_SYNC_STATE_SLAVE: return "NVAPI_GSYNC_DISPLAY_SYNC_STATE_SLAVE"s;
			case _NVAPI_GSYNC_DISPLAY_SYNC_STATE::NVAPI_GSYNC_DISPLAY_SYNC_STATE_MASTER: return "NVAPI_GSYNC_DISPLAY_SYNC_STATE_MASTER"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GSYNC_POLARITY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GSYNC_POLARITY*>(pEnum);
			switch (e) {
			case _NVAPI_GSYNC_POLARITY::NVAPI_GSYNC_POLARITY_RISING_EDGE: return "NVAPI_GSYNC_POLARITY_RISING_EDGE"s;
			case _NVAPI_GSYNC_POLARITY::NVAPI_GSYNC_POLARITY_FALLING_EDGE: return "NVAPI_GSYNC_POLARITY_FALLING_EDGE"s;
			case _NVAPI_GSYNC_POLARITY::NVAPI_GSYNC_POLARITY_BOTH_EDGES: return "NVAPI_GSYNC_POLARITY_BOTH_EDGES"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GSYNC_VIDEO_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GSYNC_VIDEO_MODE*>(pEnum);
			switch (e) {
			case _NVAPI_GSYNC_VIDEO_MODE::NVAPI_GSYNC_VIDEO_MODE_NONE: return "NVAPI_GSYNC_VIDEO_MODE_NONE"s;
			case _NVAPI_GSYNC_VIDEO_MODE::NVAPI_GSYNC_VIDEO_MODE_TTL: return "NVAPI_GSYNC_VIDEO_MODE_TTL"s;
			case _NVAPI_GSYNC_VIDEO_MODE::NVAPI_GSYNC_VIDEO_MODE_NTSCPALSECAM: return "NVAPI_GSYNC_VIDEO_MODE_NTSCPALSECAM"s;
			case _NVAPI_GSYNC_VIDEO_MODE::NVAPI_GSYNC_VIDEO_MODE_HDTV: return "NVAPI_GSYNC_VIDEO_MODE_HDTV"s;
			case _NVAPI_GSYNC_VIDEO_MODE::NVAPI_GSYNC_VIDEO_MODE_COMPOSITE: return "NVAPI_GSYNC_VIDEO_MODE_COMPOSITE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GSYNC_SYNC_SOURCE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GSYNC_SYNC_SOURCE*>(pEnum);
			switch (e) {
			case _NVAPI_GSYNC_SYNC_SOURCE::NVAPI_GSYNC_SYNC_SOURCE_VSYNC: return "NVAPI_GSYNC_SYNC_SOURCE_VSYNC"s;
			case _NVAPI_GSYNC_SYNC_SOURCE::NVAPI_GSYNC_SYNC_SOURCE_HOUSESYNC: return "NVAPI_GSYNC_SYNC_SOURCE_HOUSESYNC"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GSYNC_DELAY_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GSYNC_DELAY_TYPE*>(pEnum);
			switch (e) {
			case _NVAPI_GSYNC_DELAY_TYPE::NVAPI_GSYNC_DELAY_TYPE_UNKNOWN: return "NVAPI_GSYNC_DELAY_TYPE_UNKNOWN"s;
			case _NVAPI_GSYNC_DELAY_TYPE::NVAPI_GSYNC_DELAY_TYPE_SYNC_SKEW: return "NVAPI_GSYNC_DELAY_TYPE_SYNC_SKEW"s;
			case _NVAPI_GSYNC_DELAY_TYPE::NVAPI_GSYNC_DELAY_TYPE_STARTUP: return "NVAPI_GSYNC_DELAY_TYPE_STARTUP"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_GSYNC_RJ45_IO)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_GSYNC_RJ45_IO*>(pEnum);
			switch (e) {
			case _NVAPI_GSYNC_RJ45_IO::NVAPI_GSYNC_RJ45_OUTPUT: return "NVAPI_GSYNC_RJ45_OUTPUT"s;
			case _NVAPI_GSYNC_RJ45_IO::NVAPI_GSYNC_RJ45_INPUT: return "NVAPI_GSYNC_RJ45_INPUT"s;
			case _NVAPI_GSYNC_RJ45_IO::NVAPI_GSYNC_RJ45_UNUSED: return "NVAPI_GSYNC_RJ45_UNUSED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_PRESENT_BARRIER_SYNC_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_PRESENT_BARRIER_SYNC_MODE*>(pEnum);
			switch (e) {
			case _NV_PRESENT_BARRIER_SYNC_MODE::PRESENT_BARRIER_NOT_JOINED: return "PRESENT_BARRIER_NOT_JOINED"s;
			case _NV_PRESENT_BARRIER_SYNC_MODE::PRESENT_BARRIER_SYNC_CLIENT: return "PRESENT_BARRIER_SYNC_CLIENT"s;
			case _NV_PRESENT_BARRIER_SYNC_MODE::PRESENT_BARRIER_SYNC_SYSTEM: return "PRESENT_BARRIER_SYNC_SYSTEM"s;
			case _NV_PRESENT_BARRIER_SYNC_MODE::PRESENT_BARRIER_SYNC_CLUSTER: return "PRESENT_BARRIER_SYNC_CLUSTER"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_RESOLVE_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_RESOLVE_MODE*>(pEnum);
			switch (e) {
			case _NV_RESOLVE_MODE::NV_RESOLVE_MODE_SAMPLE_0: return "NV_RESOLVE_MODE_SAMPLE_0"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_LATENCY_MARKER_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_LATENCY_MARKER_TYPE*>(pEnum);
			switch (e) {
			case NV_LATENCY_MARKER_TYPE::SIMULATION_START: return "SIMULATION_START"s;
			case NV_LATENCY_MARKER_TYPE::SIMULATION_END: return "SIMULATION_END"s;
			case NV_LATENCY_MARKER_TYPE::RENDERSUBMIT_START: return "RENDERSUBMIT_START"s;
			case NV_LATENCY_MARKER_TYPE::RENDERSUBMIT_END: return "RENDERSUBMIT_END"s;
			case NV_LATENCY_MARKER_TYPE::PRESENT_START: return "PRESENT_START"s;
			case NV_LATENCY_MARKER_TYPE::PRESENT_END: return "PRESENT_END"s;
			case NV_LATENCY_MARKER_TYPE::INPUT_SAMPLE: return "INPUT_SAMPLE"s;
			case NV_LATENCY_MARKER_TYPE::TRIGGER_FLASH: return "TRIGGER_FLASH"s;
			case NV_LATENCY_MARKER_TYPE::PC_LATENCY_PING: return "PC_LATENCY_PING"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOOWNERTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOOWNERTYPE*>(pEnum);
			switch (e) {
			case _NVVIOOWNERTYPE::NVVIOOWNERTYPE_NONE: return "NVVIOOWNERTYPE_NONE"s;
			case _NVVIOOWNERTYPE::NVVIOOWNERTYPE_APPLICATION: return "NVVIOOWNERTYPE_APPLICATION"s;
			case _NVVIOOWNERTYPE::NVVIOOWNERTYPE_DESKTOP: return "NVVIOOWNERTYPE_DESKTOP"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOSIGNALFORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOSIGNALFORMAT*>(pEnum);
			switch (e) {
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_NONE: return "NVVIOSIGNALFORMAT_NONE"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_487I_59_94_SMPTE259_NTSC: return "NVVIOSIGNALFORMAT_487I_59_94_SMPTE259_NTSC"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_576I_50_00_SMPTE259_PAL: return "NVVIOSIGNALFORMAT_576I_50_00_SMPTE259_PAL"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1035I_60_00_SMPTE260: return "NVVIOSIGNALFORMAT_1035I_60_00_SMPTE260"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1035I_59_94_SMPTE260: return "NVVIOSIGNALFORMAT_1035I_59_94_SMPTE260"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_50_00_SMPTE295: return "NVVIOSIGNALFORMAT_1080I_50_00_SMPTE295"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_60_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080I_60_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_59_94_SMPTE274: return "NVVIOSIGNALFORMAT_1080I_59_94_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_50_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080I_50_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_30_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080P_30_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_29_97_SMPTE274: return "NVVIOSIGNALFORMAT_1080P_29_97_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_25_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080P_25_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_24_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080P_24_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_23_976_SMPTE274: return "NVVIOSIGNALFORMAT_1080P_23_976_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_60_00_SMPTE296: return "NVVIOSIGNALFORMAT_720P_60_00_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_59_94_SMPTE296: return "NVVIOSIGNALFORMAT_720P_59_94_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_50_00_SMPTE296: return "NVVIOSIGNALFORMAT_720P_50_00_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_48_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080I_48_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_47_96_SMPTE274: return "NVVIOSIGNALFORMAT_1080I_47_96_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_30_00_SMPTE296: return "NVVIOSIGNALFORMAT_720P_30_00_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_29_97_SMPTE296: return "NVVIOSIGNALFORMAT_720P_29_97_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_25_00_SMPTE296: return "NVVIOSIGNALFORMAT_720P_25_00_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_24_00_SMPTE296: return "NVVIOSIGNALFORMAT_720P_24_00_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_720P_23_98_SMPTE296: return "NVVIOSIGNALFORMAT_720P_23_98_SMPTE296"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_30_00_SMPTE372: return "NVVIOSIGNALFORMAT_2048P_30_00_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_29_97_SMPTE372: return "NVVIOSIGNALFORMAT_2048P_29_97_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_60_00_SMPTE372: return "NVVIOSIGNALFORMAT_2048I_60_00_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_59_94_SMPTE372: return "NVVIOSIGNALFORMAT_2048I_59_94_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_25_00_SMPTE372: return "NVVIOSIGNALFORMAT_2048P_25_00_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_50_00_SMPTE372: return "NVVIOSIGNALFORMAT_2048I_50_00_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_24_00_SMPTE372: return "NVVIOSIGNALFORMAT_2048P_24_00_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_23_98_SMPTE372: return "NVVIOSIGNALFORMAT_2048P_23_98_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_48_00_SMPTE372: return "NVVIOSIGNALFORMAT_2048I_48_00_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_47_96_SMPTE372: return "NVVIOSIGNALFORMAT_2048I_47_96_SMPTE372"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080PSF_25_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080PSF_25_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080PSF_29_97_SMPTE274: return "NVVIOSIGNALFORMAT_1080PSF_29_97_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080PSF_30_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080PSF_30_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080PSF_24_00_SMPTE274: return "NVVIOSIGNALFORMAT_1080PSF_24_00_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080PSF_23_98_SMPTE274: return "NVVIOSIGNALFORMAT_1080PSF_23_98_SMPTE274"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_50_00_SMPTE274_3G_LEVEL_A: return "NVVIOSIGNALFORMAT_1080P_50_00_SMPTE274_3G_LEVEL_A"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_59_94_SMPTE274_3G_LEVEL_A: return "NVVIOSIGNALFORMAT_1080P_59_94_SMPTE274_3G_LEVEL_A"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_60_00_SMPTE274_3G_LEVEL_A: return "NVVIOSIGNALFORMAT_1080P_60_00_SMPTE274_3G_LEVEL_A"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_60_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_60_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_60_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080I_60_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_60_00_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048I_60_00_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_50_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_50_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_50_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080I_50_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_50_00_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048I_50_00_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_30_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_30_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_30_00_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048P_30_00_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_25_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_25_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_25_00_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048P_25_00_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_24_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_24_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_24_00_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048P_24_00_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_48_00_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080I_48_00_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_48_00_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048I_48_00_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_59_94_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_59_94_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_59_94_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080I_59_94_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_59_94_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048I_59_94_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_29_97_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_29_97_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_29_97_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048P_29_97_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080P_23_98_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080P_23_98_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048P_23_98_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048P_23_98_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_1080I_47_96_SMPTE274_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_1080I_47_96_SMPTE274_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_2048I_47_96_SMPTE372_3G_LEVEL_B: return "NVVIOSIGNALFORMAT_2048I_47_96_SMPTE372_3G_LEVEL_B"s;
			case _NVVIOSIGNALFORMAT::NVVIOSIGNALFORMAT_END: return "NVVIOSIGNALFORMAT_END"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOVIDEOSTANDARD)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOVIDEOSTANDARD*>(pEnum);
			switch (e) {
			case _NVVIOVIDEOSTANDARD::NVVIOVIDEOSTANDARD_SMPTE259: return "NVVIOVIDEOSTANDARD_SMPTE259"s;
			case _NVVIOVIDEOSTANDARD::NVVIOVIDEOSTANDARD_SMPTE260: return "NVVIOVIDEOSTANDARD_SMPTE260"s;
			case _NVVIOVIDEOSTANDARD::NVVIOVIDEOSTANDARD_SMPTE274: return "NVVIOVIDEOSTANDARD_SMPTE274"s;
			case _NVVIOVIDEOSTANDARD::NVVIOVIDEOSTANDARD_SMPTE295: return "NVVIOVIDEOSTANDARD_SMPTE295"s;
			case _NVVIOVIDEOSTANDARD::NVVIOVIDEOSTANDARD_SMPTE296: return "NVVIOVIDEOSTANDARD_SMPTE296"s;
			case _NVVIOVIDEOSTANDARD::NVVIOVIDEOSTANDARD_SMPTE372: return "NVVIOVIDEOSTANDARD_SMPTE372"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOVIDEOTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOVIDEOTYPE*>(pEnum);
			switch (e) {
			case _NVVIOVIDEOTYPE::NVVIOVIDEOTYPE_SD: return "NVVIOVIDEOTYPE_SD"s;
			case _NVVIOVIDEOTYPE::NVVIOVIDEOTYPE_HD: return "NVVIOVIDEOTYPE_HD"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOINTERLACEMODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOINTERLACEMODE*>(pEnum);
			switch (e) {
			case _NVVIOINTERLACEMODE::NVVIOINTERLACEMODE_PROGRESSIVE: return "NVVIOINTERLACEMODE_PROGRESSIVE"s;
			case _NVVIOINTERLACEMODE::NVVIOINTERLACEMODE_INTERLACE: return "NVVIOINTERLACEMODE_INTERLACE"s;
			case _NVVIOINTERLACEMODE::NVVIOINTERLACEMODE_PSF: return "NVVIOINTERLACEMODE_PSF"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIODATAFORMAT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIODATAFORMAT*>(pEnum);
			switch (e) {
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_UNKNOWN: return "NVVIODATAFORMAT_UNKNOWN"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R8G8B8_TO_YCRCB444: return "NVVIODATAFORMAT_R8G8B8_TO_YCRCB444"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R8G8B8A8_TO_YCRCBA4444: return "NVVIODATAFORMAT_R8G8B8A8_TO_YCRCBA4444"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R8G8B8Z10_TO_YCRCBZ4444: return "NVVIODATAFORMAT_R8G8B8Z10_TO_YCRCBZ4444"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R8G8B8_TO_YCRCB422: return "NVVIODATAFORMAT_R8G8B8_TO_YCRCB422"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R8G8B8A8_TO_YCRCBA4224: return "NVVIODATAFORMAT_R8G8B8A8_TO_YCRCBA4224"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R8G8B8Z10_TO_YCRCBZ4224: return "NVVIODATAFORMAT_R8G8B8Z10_TO_YCRCBZ4224"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X8X8X8_444_PASSTHRU: return "NVVIODATAFORMAT_X8X8X8_444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X8X8X8A8_4444_PASSTHRU: return "NVVIODATAFORMAT_X8X8X8A8_4444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X8X8X8Z10_4444_PASSTHRU: return "NVVIODATAFORMAT_X8X8X8Z10_4444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X10X10X10_444_PASSTHRU: return "NVVIODATAFORMAT_X10X10X10_444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X10X8X8_444_PASSTHRU: return "NVVIODATAFORMAT_X10X8X8_444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X10X8X8A10_4444_PASSTHRU: return "NVVIODATAFORMAT_X10X8X8A10_4444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X10X8X8Z10_4444_PASSTHRU: return "NVVIODATAFORMAT_X10X8X8Z10_4444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_DUAL_R8G8B8_TO_DUAL_YCRCB422: return "NVVIODATAFORMAT_DUAL_R8G8B8_TO_DUAL_YCRCB422"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_DUAL_X8X8X8_TO_DUAL_422_PASSTHRU: return "NVVIODATAFORMAT_DUAL_X8X8X8_TO_DUAL_422_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R10G10B10_TO_YCRCB422: return "NVVIODATAFORMAT_R10G10B10_TO_YCRCB422"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R10G10B10_TO_YCRCB444: return "NVVIODATAFORMAT_R10G10B10_TO_YCRCB444"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X12X12X12_444_PASSTHRU: return "NVVIODATAFORMAT_X12X12X12_444_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_X12X12X12_422_PASSTHRU: return "NVVIODATAFORMAT_X12X12X12_422_PASSTHRU"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_Y10CR10CB10_TO_YCRCB422: return "NVVIODATAFORMAT_Y10CR10CB10_TO_YCRCB422"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_Y8CR8CB8_TO_YCRCB422: return "NVVIODATAFORMAT_Y8CR8CB8_TO_YCRCB422"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_Y10CR8CB8A10_TO_YCRCBA4224: return "NVVIODATAFORMAT_Y10CR8CB8A10_TO_YCRCBA4224"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R10G10B10_TO_RGB444: return "NVVIODATAFORMAT_R10G10B10_TO_RGB444"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R12G12B12_TO_YCRCB444: return "NVVIODATAFORMAT_R12G12B12_TO_YCRCB444"s;
			case _NVVIODATAFORMAT::NVVIODATAFORMAT_R12G12B12_TO_YCRCB422: return "NVVIODATAFORMAT_R12G12B12_TO_YCRCB422"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOOUTPUTAREA)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOOUTPUTAREA*>(pEnum);
			switch (e) {
			case _NVVIOOUTPUTAREA::NVVIOOUTPUTAREA_FULLSIZE: return "NVVIOOUTPUTAREA_FULLSIZE"s;
			case _NVVIOOUTPUTAREA::NVVIOOUTPUTAREA_SAFEACTION: return "NVVIOOUTPUTAREA_SAFEACTION"s;
			case _NVVIOOUTPUTAREA::NVVIOOUTPUTAREA_SAFETITLE: return "NVVIOOUTPUTAREA_SAFETITLE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOSYNCSOURCE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOSYNCSOURCE*>(pEnum);
			switch (e) {
			case _NVVIOSYNCSOURCE::NVVIOSYNCSOURCE_SDISYNC: return "NVVIOSYNCSOURCE_SDISYNC"s;
			case _NVVIOSYNCSOURCE::NVVIOSYNCSOURCE_COMPSYNC: return "NVVIOSYNCSOURCE_COMPSYNC"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOCOMPSYNCTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOCOMPSYNCTYPE*>(pEnum);
			switch (e) {
			case _NVVIOCOMPSYNCTYPE::NVVIOCOMPSYNCTYPE_AUTO: return "NVVIOCOMPSYNCTYPE_AUTO"s;
			case _NVVIOCOMPSYNCTYPE::NVVIOCOMPSYNCTYPE_BILEVEL: return "NVVIOCOMPSYNCTYPE_BILEVEL"s;
			case _NVVIOCOMPSYNCTYPE::NVVIOCOMPSYNCTYPE_TRILEVEL: return "NVVIOCOMPSYNCTYPE_TRILEVEL"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOINPUTOUTPUTSTATUS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOINPUTOUTPUTSTATUS*>(pEnum);
			switch (e) {
			case _NVVIOINPUTOUTPUTSTATUS::NVINPUTOUTPUTSTATUS_OFF: return "NVINPUTOUTPUTSTATUS_OFF"s;
			case _NVVIOINPUTOUTPUTSTATUS::NVINPUTOUTPUTSTATUS_ERROR: return "NVINPUTOUTPUTSTATUS_ERROR"s;
			case _NVVIOINPUTOUTPUTSTATUS::NVINPUTOUTPUTSTATUS_SDI_SD: return "NVINPUTOUTPUTSTATUS_SDI_SD"s;
			case _NVVIOINPUTOUTPUTSTATUS::NVINPUTOUTPUTSTATUS_SDI_HD: return "NVINPUTOUTPUTSTATUS_SDI_HD"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOSYNCSTATUS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOSYNCSTATUS*>(pEnum);
			switch (e) {
			case _NVVIOSYNCSTATUS::NVVIOSYNCSTATUS_OFF: return "NVVIOSYNCSTATUS_OFF"s;
			case _NVVIOSYNCSTATUS::NVVIOSYNCSTATUS_ERROR: return "NVVIOSYNCSTATUS_ERROR"s;
			case _NVVIOSYNCSTATUS::NVVIOSYNCSTATUS_SYNCLOSS: return "NVVIOSYNCSTATUS_SYNCLOSS"s;
			case _NVVIOSYNCSTATUS::NVVIOSYNCSTATUS_COMPOSITE: return "NVVIOSYNCSTATUS_COMPOSITE"s;
			case _NVVIOSYNCSTATUS::NVVIOSYNCSTATUS_SDI_SD: return "NVVIOSYNCSTATUS_SDI_SD"s;
			case _NVVIOSYNCSTATUS::NVVIOSYNCSTATUS_SDI_HD: return "NVVIOSYNCSTATUS_SDI_HD"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOCAPTURESTATUS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOCAPTURESTATUS*>(pEnum);
			switch (e) {
			case _NVVIOCAPTURESTATUS::NVVIOSTATUS_STOPPED: return "NVVIOSTATUS_STOPPED"s;
			case _NVVIOCAPTURESTATUS::NVVIOSTATUS_RUNNING: return "NVVIOSTATUS_RUNNING"s;
			case _NVVIOCAPTURESTATUS::NVVIOSTATUS_ERROR: return "NVVIOSTATUS_ERROR"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOSTATUSTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOSTATUSTYPE*>(pEnum);
			switch (e) {
			case _NVVIOSTATUSTYPE::NVVIOSTATUSTYPE_IN: return "NVVIOSTATUSTYPE_IN"s;
			case _NVVIOSTATUSTYPE::NVVIOSTATUSTYPE_OUT: return "NVVIOSTATUSTYPE_OUT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOCONFIGTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOCONFIGTYPE*>(pEnum);
			switch (e) {
			case _NVVIOCONFIGTYPE::NVVIOCONFIGTYPE_IN: return "NVVIOCONFIGTYPE_IN"s;
			case _NVVIOCONFIGTYPE::NVVIOCONFIGTYPE_OUT: return "NVVIOCONFIGTYPE_OUT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOCOLORSPACE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOCOLORSPACE*>(pEnum);
			switch (e) {
			case _NVVIOCOLORSPACE::NVVIOCOLORSPACE_UNKNOWN: return "NVVIOCOLORSPACE_UNKNOWN"s;
			case _NVVIOCOLORSPACE::NVVIOCOLORSPACE_YCBCR: return "NVVIOCOLORSPACE_YCBCR"s;
			case _NVVIOCOLORSPACE::NVVIOCOLORSPACE_YCBCRA: return "NVVIOCOLORSPACE_YCBCRA"s;
			case _NVVIOCOLORSPACE::NVVIOCOLORSPACE_YCBCRD: return "NVVIOCOLORSPACE_YCBCRD"s;
			case _NVVIOCOLORSPACE::NVVIOCOLORSPACE_GBR: return "NVVIOCOLORSPACE_GBR"s;
			case _NVVIOCOLORSPACE::NVVIOCOLORSPACE_GBRA: return "NVVIOCOLORSPACE_GBRA"s;
			case _NVVIOCOLORSPACE::NVVIOCOLORSPACE_GBRD: return "NVVIOCOLORSPACE_GBRD"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOCOMPONENTSAMPLING)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOCOMPONENTSAMPLING*>(pEnum);
			switch (e) {
			case _NVVIOCOMPONENTSAMPLING::NVVIOCOMPONENTSAMPLING_UNKNOWN: return "NVVIOCOMPONENTSAMPLING_UNKNOWN"s;
			case _NVVIOCOMPONENTSAMPLING::NVVIOCOMPONENTSAMPLING_4444: return "NVVIOCOMPONENTSAMPLING_4444"s;
			case _NVVIOCOMPONENTSAMPLING::NVVIOCOMPONENTSAMPLING_4224: return "NVVIOCOMPONENTSAMPLING_4224"s;
			case _NVVIOCOMPONENTSAMPLING::NVVIOCOMPONENTSAMPLING_444: return "NVVIOCOMPONENTSAMPLING_444"s;
			case _NVVIOCOMPONENTSAMPLING::NVVIOCOMPONENTSAMPLING_422: return "NVVIOCOMPONENTSAMPLING_422"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOBITSPERCOMPONENT)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOBITSPERCOMPONENT*>(pEnum);
			switch (e) {
			case _NVVIOBITSPERCOMPONENT::NVVIOBITSPERCOMPONENT_UNKNOWN: return "NVVIOBITSPERCOMPONENT_UNKNOWN"s;
			case _NVVIOBITSPERCOMPONENT::NVVIOBITSPERCOMPONENT_8: return "NVVIOBITSPERCOMPONENT_8"s;
			case _NVVIOBITSPERCOMPONENT::NVVIOBITSPERCOMPONENT_10: return "NVVIOBITSPERCOMPONENT_10"s;
			case _NVVIOBITSPERCOMPONENT::NVVIOBITSPERCOMPONENT_12: return "NVVIOBITSPERCOMPONENT_12"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOLINKID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOLINKID*>(pEnum);
			switch (e) {
			case _NVVIOLINKID::NVVIOLINKID_UNKNOWN: return "NVVIOLINKID_UNKNOWN"s;
			case _NVVIOLINKID::NVVIOLINKID_A: return "NVVIOLINKID_A"s;
			case _NVVIOLINKID::NVVIOLINKID_B: return "NVVIOLINKID_B"s;
			case _NVVIOLINKID::NVVIOLINKID_C: return "NVVIOLINKID_C"s;
			case _NVVIOLINKID::NVVIOLINKID_D: return "NVVIOLINKID_D"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOANCPARITYCOMPUTATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOANCPARITYCOMPUTATION*>(pEnum);
			switch (e) {
			case _NVVIOANCPARITYCOMPUTATION::NVVIOANCPARITYCOMPUTATION_AUTO: return "NVVIOANCPARITYCOMPUTATION_AUTO"s;
			case _NVVIOANCPARITYCOMPUTATION::NVVIOANCPARITYCOMPUTATION_ON: return "NVVIOANCPARITYCOMPUTATION_ON"s;
			case _NVVIOANCPARITYCOMPUTATION::NVVIOANCPARITYCOMPUTATION_OFF: return "NVVIOANCPARITYCOMPUTATION_OFF"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOPCILINKRATE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOPCILINKRATE*>(pEnum);
			switch (e) {
			case _NVVIOPCILINKRATE::NVVIOPCILINKRATE_UNKNOWN: return "NVVIOPCILINKRATE_UNKNOWN"s;
			case _NVVIOPCILINKRATE::NVVIOPCILINKRATE_GEN1: return "NVVIOPCILINKRATE_GEN1"s;
			case _NVVIOPCILINKRATE::NVVIOPCILINKRATE_GEN2: return "NVVIOPCILINKRATE_GEN2"s;
			case _NVVIOPCILINKRATE::NVVIOPCILINKRATE_GEN3: return "NVVIOPCILINKRATE_GEN3"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVVIOPCILINKWIDTH)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVVIOPCILINKWIDTH*>(pEnum);
			switch (e) {
			case _NVVIOPCILINKWIDTH::NVVIOPCILINKWIDTH_UNKNOWN: return "NVVIOPCILINKWIDTH_UNKNOWN"s;
			case _NVVIOPCILINKWIDTH::NVVIOPCILINKWIDTH_x1: return "NVVIOPCILINKWIDTH_x1"s;
			case _NVVIOPCILINKWIDTH::NVVIOPCILINKWIDTH_x2: return "NVVIOPCILINKWIDTH_x2"s;
			case _NVVIOPCILINKWIDTH::NVVIOPCILINKWIDTH_x4: return "NVVIOPCILINKWIDTH_x4"s;
			case _NVVIOPCILINKWIDTH::NVVIOPCILINKWIDTH_x8: return "NVVIOPCILINKWIDTH_x8"s;
			case _NVVIOPCILINKWIDTH::NVVIOPCILINKWIDTH_x16: return "NVVIOPCILINKWIDTH_x16"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_StereoRegistryProfileType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_StereoRegistryProfileType*>(pEnum);
			switch (e) {
			case _NV_StereoRegistryProfileType::NVAPI_STEREO_DEFAULT_REGISTRY_PROFILE: return "NVAPI_STEREO_DEFAULT_REGISTRY_PROFILE"s;
			case _NV_StereoRegistryProfileType::NVAPI_STEREO_DX9_REGISTRY_PROFILE: return "NVAPI_STEREO_DX9_REGISTRY_PROFILE"s;
			case _NV_StereoRegistryProfileType::NVAPI_STEREO_DX10_REGISTRY_PROFILE: return "NVAPI_STEREO_DX10_REGISTRY_PROFILE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_StereoRegistryID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_StereoRegistryID*>(pEnum);
			switch (e) {
			case _NV_StereoRegistryID::NVAPI_CONVERGENCE_ID: return "NVAPI_CONVERGENCE_ID"s;
			case _NV_StereoRegistryID::NVAPI_FRUSTUM_ADJUST_MODE_ID: return "NVAPI_FRUSTUM_ADJUST_MODE_ID"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_FrustumAdjustMode)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_FrustumAdjustMode*>(pEnum);
			switch (e) {
			case _NV_FrustumAdjustMode::NVAPI_NO_FRUSTUM_ADJUST: return "NVAPI_NO_FRUSTUM_ADJUST"s;
			case _NV_FrustumAdjustMode::NVAPI_FRUSTUM_STRETCH: return "NVAPI_FRUSTUM_STRETCH"s;
			case _NV_FrustumAdjustMode::NVAPI_FRUSTUM_CLEAR_EDGES: return "NVAPI_FRUSTUM_CLEAR_EDGES"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_STEREO_INIT_ACTIVATION_FLAGS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_STEREO_INIT_ACTIVATION_FLAGS*>(pEnum);
			switch (e) {
			case _NVAPI_STEREO_INIT_ACTIVATION_FLAGS::NVAPI_STEREO_INIT_ACTIVATION_IMMEDIATE: return "NVAPI_STEREO_INIT_ACTIVATION_IMMEDIATE"s;
			case _NVAPI_STEREO_INIT_ACTIVATION_FLAGS::NVAPI_STEREO_INIT_ACTIVATION_DELAYED: return "NVAPI_STEREO_INIT_ACTIVATION_DELAYED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_StereoActiveEye)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_StereoActiveEye*>(pEnum);
			switch (e) {
			case _NV_StereoActiveEye::NVAPI_STEREO_EYE_RIGHT: return "NVAPI_STEREO_EYE_RIGHT"s;
			case _NV_StereoActiveEye::NVAPI_STEREO_EYE_LEFT: return "NVAPI_STEREO_EYE_LEFT"s;
			case _NV_StereoActiveEye::NVAPI_STEREO_EYE_MONO: return "NVAPI_STEREO_EYE_MONO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_StereoDriverMode)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_StereoDriverMode*>(pEnum);
			switch (e) {
			case _NV_StereoDriverMode::NVAPI_STEREO_DRIVER_MODE_AUTOMATIC: return "NVAPI_STEREO_DRIVER_MODE_AUTOMATIC"s;
			case _NV_StereoDriverMode::NVAPI_STEREO_DRIVER_MODE_DIRECT: return "NVAPI_STEREO_DRIVER_MODE_DIRECT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVAPI_STEREO_SURFACECREATEMODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVAPI_STEREO_SURFACECREATEMODE*>(pEnum);
			switch (e) {
			case _NVAPI_STEREO_SURFACECREATEMODE::NVAPI_STEREO_SURFACECREATEMODE_AUTO: return "NVAPI_STEREO_SURFACECREATEMODE_AUTO"s;
			case _NVAPI_STEREO_SURFACECREATEMODE::NVAPI_STEREO_SURFACECREATEMODE_FORCESTEREO: return "NVAPI_STEREO_SURFACECREATEMODE_FORCESTEREO"s;
			case _NVAPI_STEREO_SURFACECREATEMODE::NVAPI_STEREO_SURFACECREATEMODE_FORCEMONO: return "NVAPI_STEREO_SURFACECREATEMODE_FORCEMONO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_StereoSwapChainMode)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_StereoSwapChainMode*>(pEnum);
			switch (e) {
			case _NV_StereoSwapChainMode::NVAPI_STEREO_SWAPCHAIN_DEFAULT: return "NVAPI_STEREO_SWAPCHAIN_DEFAULT"s;
			case _NV_StereoSwapChainMode::NVAPI_STEREO_SWAPCHAIN_STEREO: return "NVAPI_STEREO_SWAPCHAIN_STEREO"s;
			case _NV_StereoSwapChainMode::NVAPI_STEREO_SWAPCHAIN_MONO: return "NVAPI_STEREO_SWAPCHAIN_MONO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVDRS_SETTING_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVDRS_SETTING_TYPE*>(pEnum);
			switch (e) {
			case _NVDRS_SETTING_TYPE::NVDRS_DWORD_TYPE: return "NVDRS_DWORD_TYPE"s;
			case _NVDRS_SETTING_TYPE::NVDRS_BINARY_TYPE: return "NVDRS_BINARY_TYPE"s;
			case _NVDRS_SETTING_TYPE::NVDRS_STRING_TYPE: return "NVDRS_STRING_TYPE"s;
			case _NVDRS_SETTING_TYPE::NVDRS_WSTRING_TYPE: return "NVDRS_WSTRING_TYPE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NVDRS_SETTING_LOCATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NVDRS_SETTING_LOCATION*>(pEnum);
			switch (e) {
			case _NVDRS_SETTING_LOCATION::NVDRS_CURRENT_PROFILE_LOCATION: return "NVDRS_CURRENT_PROFILE_LOCATION"s;
			case _NVDRS_SETTING_LOCATION::NVDRS_GLOBAL_PROFILE_LOCATION: return "NVDRS_GLOBAL_PROFILE_LOCATION"s;
			case _NVDRS_SETTING_LOCATION::NVDRS_BASE_PROFILE_LOCATION: return "NVDRS_BASE_PROFILE_LOCATION"s;
			case _NVDRS_SETTING_LOCATION::NVDRS_DEFAULT_PROFILE_LOCATION: return "NVDRS_DEFAULT_PROFILE_LOCATION"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(NV_CHIPSET_INFO_FLAGS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const NV_CHIPSET_INFO_FLAGS*>(pEnum);
			switch (e) {
			case NV_CHIPSET_INFO_FLAGS::NV_CHIPSET_INFO_HYBRID: return "NV_CHIPSET_INFO_HYBRID"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_NV_GPU_CLIENT_UTIL_DOMAIN_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _NV_GPU_CLIENT_UTIL_DOMAIN_ID*>(pEnum);
			switch (e) {
			case _NV_GPU_CLIENT_UTIL_DOMAIN_ID::NV_GPU_CLIENT_UTIL_DOMAIN_GRAPHICS: return "NV_GPU_CLIENT_UTIL_DOMAIN_GRAPHICS"s;
			case _NV_GPU_CLIENT_UTIL_DOMAIN_ID::NV_GPU_CLIENT_UTIL_DOMAIN_FRAME_BUFFER: return "NV_GPU_CLIENT_UTIL_DOMAIN_FRAME_BUFFER"s;
			case _NV_GPU_CLIENT_UTIL_DOMAIN_ID::NV_GPU_CLIENT_UTIL_DOMAIN_VIDEO: return "NV_GPU_CLIENT_UTIL_DOMAIN_VIDEO"s;
			case _NV_GPU_CLIENT_UTIL_DOMAIN_ID::NV_GPU_CLIENT_UTIL_DOMAIN_RSVD: return "NV_GPU_CLIENT_UTIL_DOMAIN_RSVD"s;
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
		dumpers[typeid(nvmlClockType_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const nvmlClockType_t*>(pEnum);
			switch (e) {
			case nvmlClockType_t::NVML_CLOCK_GRAPHICS: return "NVML_CLOCK_GRAPHICS"s;
			case nvmlClockType_t::NVML_CLOCK_SM: return "NVML_CLOCK_SM"s;
			case nvmlClockType_t::NVML_CLOCK_MEM: return "NVML_CLOCK_MEM"s;
			case nvmlClockType_t::NVML_CLOCK_VIDEO: return "NVML_CLOCK_VIDEO"s;
			case nvmlClockType_t::NVML_CLOCK_COUNT: return "NVML_CLOCK_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(nvmlClockId_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const nvmlClockId_t*>(pEnum);
			switch (e) {
			case nvmlClockId_t::NVML_CLOCK_ID_CURRENT: return "NVML_CLOCK_ID_CURRENT"s;
			case nvmlClockId_t::NVML_CLOCK_ID_APP_CLOCK_TARGET: return "NVML_CLOCK_ID_APP_CLOCK_TARGET"s;
			case nvmlClockId_t::NVML_CLOCK_ID_APP_CLOCK_DEFAULT: return "NVML_CLOCK_ID_APP_CLOCK_DEFAULT"s;
			case nvmlClockId_t::NVML_CLOCK_ID_CUSTOMER_BOOST_MAX: return "NVML_CLOCK_ID_CUSTOMER_BOOST_MAX"s;
			case nvmlClockId_t::NVML_CLOCK_ID_COUNT: return "NVML_CLOCK_ID_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(nvmlTemperatureSensors_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const nvmlTemperatureSensors_t*>(pEnum);
			switch (e) {
			case nvmlTemperatureSensors_t::NVML_TEMPERATURE_GPU: return "NVML_TEMPERATURE_GPU"s;
			case nvmlTemperatureSensors_t::NVML_TEMPERATURE_COUNT: return "NVML_TEMPERATURE_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(nvmlSamplingType_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const nvmlSamplingType_t*>(pEnum);
			switch (e) {
			case nvmlSamplingType_t::NVML_TOTAL_POWER_SAMPLES: return "NVML_TOTAL_POWER_SAMPLES"s;
			case nvmlSamplingType_t::NVML_GPU_UTILIZATION_SAMPLES: return "NVML_GPU_UTILIZATION_SAMPLES"s;
			case nvmlSamplingType_t::NVML_MEMORY_UTILIZATION_SAMPLES: return "NVML_MEMORY_UTILIZATION_SAMPLES"s;
			case nvmlSamplingType_t::NVML_ENC_UTILIZATION_SAMPLES: return "NVML_ENC_UTILIZATION_SAMPLES"s;
			case nvmlSamplingType_t::NVML_DEC_UTILIZATION_SAMPLES: return "NVML_DEC_UTILIZATION_SAMPLES"s;
			case nvmlSamplingType_t::NVML_PROCESSOR_CLK_SAMPLES: return "NVML_PROCESSOR_CLK_SAMPLES"s;
			case nvmlSamplingType_t::NVML_MEMORY_CLK_SAMPLES: return "NVML_MEMORY_CLK_SAMPLES"s;
			case nvmlSamplingType_t::NVML_SAMPLINGTYPE_COUNT: return "NVML_SAMPLINGTYPE_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(nvmlValueType_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const nvmlValueType_t*>(pEnum);
			switch (e) {
			case nvmlValueType_t::NVML_VALUE_TYPE_DOUBLE: return "NVML_VALUE_TYPE_DOUBLE"s;
			case nvmlValueType_t::NVML_VALUE_TYPE_UNSIGNED_INT: return "NVML_VALUE_TYPE_UNSIGNED_INT"s;
			case nvmlValueType_t::NVML_VALUE_TYPE_UNSIGNED_LONG: return "NVML_VALUE_TYPE_UNSIGNED_LONG"s;
			case nvmlValueType_t::NVML_VALUE_TYPE_UNSIGNED_LONG_LONG: return "NVML_VALUE_TYPE_UNSIGNED_LONG_LONG"s;
			case nvmlValueType_t::NVML_VALUE_TYPE_SIGNED_LONG_LONG: return "NVML_VALUE_TYPE_SIGNED_LONG_LONG"s;
			case nvmlValueType_t::NVML_VALUE_TYPE_COUNT: return "NVML_VALUE_TYPE_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(nvmlPerfPolicyType_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const nvmlPerfPolicyType_t*>(pEnum);
			switch (e) {
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_POWER: return "NVML_PERF_POLICY_POWER"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_THERMAL: return "NVML_PERF_POLICY_THERMAL"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_SYNC_BOOST: return "NVML_PERF_POLICY_SYNC_BOOST"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_BOARD_LIMIT: return "NVML_PERF_POLICY_BOARD_LIMIT"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_LOW_UTILIZATION: return "NVML_PERF_POLICY_LOW_UTILIZATION"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_RELIABILITY: return "NVML_PERF_POLICY_RELIABILITY"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_TOTAL_APP_CLOCKS: return "NVML_PERF_POLICY_TOTAL_APP_CLOCKS"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_TOTAL_BASE_CLOCKS: return "NVML_PERF_POLICY_TOTAL_BASE_CLOCKS"s;
			case nvmlPerfPolicyType_t::NVML_PERF_POLICY_COUNT: return "NVML_PERF_POLICY_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLThreadingModel)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLThreadingModel*>(pEnum);
			switch (e) {
			case ADLThreadingModel::ADL_THREADING_UNLOCKED: return "ADL_THREADING_UNLOCKED"s;
			case ADLThreadingModel::ADL_THREADING_LOCKED: return "ADL_THREADING_LOCKED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLPurposeCode)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLPurposeCode*>(pEnum);
			switch (e) {
			case ADLPurposeCode::ADL_PURPOSECODE_NORMAL: return "ADL_PURPOSECODE_NORMAL"s;
			case ADLPurposeCode::ADL_PURPOSECODE_HIDE_MODE_SWITCH: return "ADL_PURPOSECODE_HIDE_MODE_SWITCH"s;
			case ADLPurposeCode::ADL_PURPOSECODE_MODE_SWITCH: return "ADL_PURPOSECODE_MODE_SWITCH"s;
			case ADLPurposeCode::ADL_PURPOSECODE_ATTATCH_DEVICE: return "ADL_PURPOSECODE_ATTATCH_DEVICE"s;
			case ADLPurposeCode::ADL_PURPOSECODE_DETACH_DEVICE: return "ADL_PURPOSECODE_DETACH_DEVICE"s;
			case ADLPurposeCode::ADL_PURPOSECODE_SETPRIMARY_DEVICE: return "ADL_PURPOSECODE_SETPRIMARY_DEVICE"s;
			case ADLPurposeCode::ADL_PURPOSECODE_GDI_ROTATION: return "ADL_PURPOSECODE_GDI_ROTATION"s;
			case ADLPurposeCode::ADL_PURPOSECODE_ATI_ROTATION: return "ADL_PURPOSECODE_ATI_ROTATION"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLAngle)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLAngle*>(pEnum);
			switch (e) {
			case ADLAngle::ADL_ANGLE_LANDSCAPE: return "ADL_ANGLE_LANDSCAPE"s;
			case ADLAngle::ADL_ANGLE_ROTATERIGHT: return "ADL_ANGLE_ROTATERIGHT"s;
			case ADLAngle::ADL_ANGLE_ROTATE180: return "ADL_ANGLE_ROTATE180"s;
			case ADLAngle::ADL_ANGLE_ROTATELEFT: return "ADL_ANGLE_ROTATELEFT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLOrientationDataType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLOrientationDataType*>(pEnum);
			switch (e) {
			case ADLOrientationDataType::ADL_ORIENTATIONTYPE_OSDATATYPE: return "ADL_ORIENTATIONTYPE_OSDATATYPE"s;
			case ADLOrientationDataType::ADL_ORIENTATIONTYPE_NONOSDATATYPE: return "ADL_ORIENTATIONTYPE_NONOSDATATYPE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLPanningMode)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLPanningMode*>(pEnum);
			switch (e) {
			case ADLPanningMode::ADL_PANNINGMODE_NO_PANNING: return "ADL_PANNINGMODE_NO_PANNING"s;
			case ADLPanningMode::ADL_PANNINGMODE_AT_LEAST_ONE_NO_PANNING: return "ADL_PANNINGMODE_AT_LEAST_ONE_NO_PANNING"s;
			case ADLPanningMode::ADL_PANNINGMODE_ALLOW_PANNING: return "ADL_PANNINGMODE_ALLOW_PANNING"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLLARGEDESKTOPTYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLLARGEDESKTOPTYPE*>(pEnum);
			switch (e) {
			case ADLLARGEDESKTOPTYPE::ADL_LARGEDESKTOPTYPE_NORMALDESKTOP: return "ADL_LARGEDESKTOPTYPE_NORMALDESKTOP"s;
			case ADLLARGEDESKTOPTYPE::ADL_LARGEDESKTOPTYPE_PSEUDOLARGEDESKTOP: return "ADL_LARGEDESKTOPTYPE_PSEUDOLARGEDESKTOP"s;
			case ADLLARGEDESKTOPTYPE::ADL_LARGEDESKTOPTYPE_VERYLARGEDESKTOP: return "ADL_LARGEDESKTOPTYPE_VERYLARGEDESKTOP"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLPlatForm)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLPlatForm*>(pEnum);
			switch (e) {
			case ADLPlatForm::GRAPHICS_PLATFORM_DESKTOP: return "GRAPHICS_PLATFORM_DESKTOP"s;
			case ADLPlatForm::GRAPHICS_PLATFORM_MOBILE: return "GRAPHICS_PLATFORM_MOBILE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLGraphicCoreGeneration)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLGraphicCoreGeneration*>(pEnum);
			switch (e) {
			case ADLGraphicCoreGeneration::ADL_GRAPHIC_CORE_GENERATION_UNDEFINED: return "ADL_GRAPHIC_CORE_GENERATION_UNDEFINED"s;
			case ADLGraphicCoreGeneration::ADL_GRAPHIC_CORE_GENERATION_PRE_GCN: return "ADL_GRAPHIC_CORE_GENERATION_PRE_GCN"s;
			case ADLGraphicCoreGeneration::ADL_GRAPHIC_CORE_GENERATION_GCN: return "ADL_GRAPHIC_CORE_GENERATION_GCN"s;
			case ADLGraphicCoreGeneration::ADL_GRAPHIC_CORE_GENERATION_RDNA: return "ADL_GRAPHIC_CORE_GENERATION_RDNA"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(SLS_ImageCropType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const SLS_ImageCropType*>(pEnum);
			switch (e) {
			case SLS_ImageCropType::Fit: return "Fit"s;
			case SLS_ImageCropType::Fill: return "Fill"s;
			case SLS_ImageCropType::Expand: return "Expand"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(DceSettingsType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const DceSettingsType*>(pEnum);
			switch (e) {
			case DceSettingsType::DceSetting_HdmiLq: return "DceSetting_HdmiLq"s;
			case DceSettingsType::DceSetting_DpSettings: return "DceSetting_DpSettings"s;
			case DceSettingsType::DceSetting_Protection: return "DceSetting_Protection"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(DpLinkRate)] = [](const void* pEnum) {
			const auto& e = *static_cast<const DpLinkRate*>(pEnum);
			switch (e) {
			case DpLinkRate::DPLinkRate_Unknown: return "DPLinkRate_Unknown"s;
			case DpLinkRate::DPLinkRate_RBR: return "DPLinkRate_RBR"s;
			case DpLinkRate::DPLinkRate_2_16Gbps: return "DPLinkRate_2_16Gbps"s;
			case DpLinkRate::DPLinkRate_2_43Gbps: return "DPLinkRate_2_43Gbps"s;
			case DpLinkRate::DPLinkRate_HBR: return "DPLinkRate_HBR"s;
			case DpLinkRate::DPLinkRate_4_32Gbps: return "DPLinkRate_4_32Gbps"s;
			case DpLinkRate::DPLinkRate_HBR2: return "DPLinkRate_HBR2"s;
			case DpLinkRate::DPLinkRate_HBR3: return "DPLinkRate_HBR3"s;
			case DpLinkRate::DPLinkRate_UHBR10: return "DPLinkRate_UHBR10"s;
			case DpLinkRate::DPLinkRate_UHBR13D5: return "DPLinkRate_UHBR13D5"s;
			case DpLinkRate::DPLinkRate_UHBR20: return "DPLinkRate_UHBR20"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLPXScheme)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLPXScheme*>(pEnum);
			switch (e) {
			case ADLPXScheme::ADL_PX_SCHEME_INVALID: return "ADL_PX_SCHEME_INVALID"s;
			case ADLPXScheme::ADL_PX_SCHEME_FIXED: return "ADL_PX_SCHEME_FIXED"s;
			case ADLPXScheme::ADL_PX_SCHEME_DYNAMIC: return "ADL_PX_SCHEME_DYNAMIC"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(PXScheme)] = [](const void* pEnum) {
			const auto& e = *static_cast<const PXScheme*>(pEnum);
			switch (e) {
			case PXScheme::PX_SCHEME_INVALID: return "PX_SCHEME_INVALID"s;
			case PXScheme::PX_SCHEME_FIXED: return "PX_SCHEME_FIXED"s;
			case PXScheme::PX_SCHEME_DYNAMIC: return "PX_SCHEME_DYNAMIC"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ApplicationListType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ApplicationListType*>(pEnum);
			switch (e) {
			case ApplicationListType::ADL_PX40_MRU: return "ADL_PX40_MRU"s;
			case ApplicationListType::ADL_PX40_MISSED: return "ADL_PX40_MISSED"s;
			case ApplicationListType::ADL_PX40_DISCRETE: return "ADL_PX40_DISCRETE"s;
			case ApplicationListType::ADL_PX40_INTEGRATED: return "ADL_PX40_INTEGRATED"s;
			case ApplicationListType::ADL_MMD_PROFILED: return "ADL_MMD_PROFILED"s;
			case ApplicationListType::ADL_PX40_TOTAL: return "ADL_PX40_TOTAL"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLProfilePropertyType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLProfilePropertyType*>(pEnum);
			switch (e) {
			case ADLProfilePropertyType::ADL_PROFILEPROPERTY_TYPE_BINARY: return "ADL_PROFILEPROPERTY_TYPE_BINARY"s;
			case ADLProfilePropertyType::ADL_PROFILEPROPERTY_TYPE_BOOLEAN: return "ADL_PROFILEPROPERTY_TYPE_BOOLEAN"s;
			case ADLProfilePropertyType::ADL_PROFILEPROPERTY_TYPE_DWORD: return "ADL_PROFILEPROPERTY_TYPE_DWORD"s;
			case ADLProfilePropertyType::ADL_PROFILEPROPERTY_TYPE_QWORD: return "ADL_PROFILEPROPERTY_TYPE_QWORD"s;
			case ADLProfilePropertyType::ADL_PROFILEPROPERTY_TYPE_ENUMERATED: return "ADL_PROFILEPROPERTY_TYPE_ENUMERATED"s;
			case ADLProfilePropertyType::ADL_PROFILEPROPERTY_TYPE_STRING: return "ADL_PROFILEPROPERTY_TYPE_STRING"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_VIRTUALDISPLAY_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_VIRTUALDISPLAY_TYPE*>(pEnum);
			switch (e) {
			case ADL_VIRTUALDISPLAY_TYPE::ADL_VIRTUALDISPLAY_NONE: return "ADL_VIRTUALDISPLAY_NONE"s;
			case ADL_VIRTUALDISPLAY_TYPE::ADL_VIRTUALDISPLAY_XINPUT: return "ADL_VIRTUALDISPLAY_XINPUT"s;
			case ADL_VIRTUALDISPLAY_TYPE::ADL_VIRTUALDISPLAY_REMOTEPLAY: return "ADL_VIRTUALDISPLAY_REMOTEPLAY"s;
			case ADL_VIRTUALDISPLAY_TYPE::ADL_VIRTUALDISPLAY_GENERIC: return "ADL_VIRTUALDISPLAY_GENERIC"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLMultiChannelSplitStateFlag)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLMultiChannelSplitStateFlag*>(pEnum);
			switch (e) {
			case ADLMultiChannelSplitStateFlag::ADLMultiChannelSplit_Unitialized: return "ADLMultiChannelSplit_Unitialized"s;
			case ADLMultiChannelSplitStateFlag::ADLMultiChannelSplit_Disabled: return "ADLMultiChannelSplit_Disabled"s;
			case ADLMultiChannelSplitStateFlag::ADLMultiChannelSplit_Enabled: return "ADLMultiChannelSplit_Enabled"s;
			case ADLMultiChannelSplitStateFlag::ADLMultiChannelSplit_SaveProfile: return "ADLMultiChannelSplit_SaveProfile"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLSampleRate)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLSampleRate*>(pEnum);
			switch (e) {
			case ADLSampleRate::ADLSampleRate_32KHz: return "ADLSampleRate_32KHz"s;
			case ADLSampleRate::ADLSampleRate_44P1KHz: return "ADLSampleRate_44P1KHz"s;
			case ADLSampleRate::ADLSampleRate_48KHz: return "ADLSampleRate_48KHz"s;
			case ADLSampleRate::ADLSampleRate_88P2KHz: return "ADLSampleRate_88P2KHz"s;
			case ADLSampleRate::ADLSampleRate_96KHz: return "ADLSampleRate_96KHz"s;
			case ADLSampleRate::ADLSampleRate_176P4KHz: return "ADLSampleRate_176P4KHz"s;
			case ADLSampleRate::ADLSampleRate_192KHz: return "ADLSampleRate_192KHz"s;
			case ADLSampleRate::ADLSampleRate_384KHz: return "ADLSampleRate_384KHz"s;
			case ADLSampleRate::ADLSampleRate_768KHz: return "ADLSampleRate_768KHz"s;
			case ADLSampleRate::ADLSampleRate_Undefined: return "ADLSampleRate_Undefined"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLODNControlType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLODNControlType*>(pEnum);
			switch (e) {
			case ADLODNControlType::ODNControlType_Current: return "ODNControlType_Current"s;
			case ADLODNControlType::ODNControlType_Default: return "ODNControlType_Default"s;
			case ADLODNControlType::ODNControlType_Auto: return "ODNControlType_Auto"s;
			case ADLODNControlType::ODNControlType_Manual: return "ODNControlType_Manual"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLODNDPMMaskType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLODNDPMMaskType*>(pEnum);
			switch (e) {
			case ADLODNDPMMaskType::ADL_ODN_DPM_CLOCK: return "ADL_ODN_DPM_CLOCK"s;
			case ADLODNDPMMaskType::ADL_ODN_DPM_VDDC: return "ADL_ODN_DPM_VDDC"s;
			case ADLODNDPMMaskType::ADL_ODN_DPM_MASK: return "ADL_ODN_DPM_MASK"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLODNFeatureControl)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLODNFeatureControl*>(pEnum);
			switch (e) {
			case ADLODNFeatureControl::ADL_ODN_SCLK_DPM: return "ADL_ODN_SCLK_DPM"s;
			case ADLODNFeatureControl::ADL_ODN_MCLK_DPM: return "ADL_ODN_MCLK_DPM"s;
			case ADLODNFeatureControl::ADL_ODN_SCLK_VDD: return "ADL_ODN_SCLK_VDD"s;
			case ADLODNFeatureControl::ADL_ODN_MCLK_VDD: return "ADL_ODN_MCLK_VDD"s;
			case ADLODNFeatureControl::ADL_ODN_FAN_SPEED_MIN: return "ADL_ODN_FAN_SPEED_MIN"s;
			case ADLODNFeatureControl::ADL_ODN_FAN_SPEED_TARGET: return "ADL_ODN_FAN_SPEED_TARGET"s;
			case ADLODNFeatureControl::ADL_ODN_ACOUSTIC_LIMIT_SCLK: return "ADL_ODN_ACOUSTIC_LIMIT_SCLK"s;
			case ADLODNFeatureControl::ADL_ODN_TEMPERATURE_FAN_MAX: return "ADL_ODN_TEMPERATURE_FAN_MAX"s;
			case ADLODNFeatureControl::ADL_ODN_TEMPERATURE_SYSTEM: return "ADL_ODN_TEMPERATURE_SYSTEM"s;
			case ADLODNFeatureControl::ADL_ODN_POWER_LIMIT: return "ADL_ODN_POWER_LIMIT"s;
			case ADLODNFeatureControl::ADL_ODN_SCLK_AUTO_LIMIT: return "ADL_ODN_SCLK_AUTO_LIMIT"s;
			case ADLODNFeatureControl::ADL_ODN_MCLK_AUTO_LIMIT: return "ADL_ODN_MCLK_AUTO_LIMIT"s;
			case ADLODNFeatureControl::ADL_ODN_SCLK_DPM_MASK_ENABLE: return "ADL_ODN_SCLK_DPM_MASK_ENABLE"s;
			case ADLODNFeatureControl::ADL_ODN_MCLK_DPM_MASK_ENABLE: return "ADL_ODN_MCLK_DPM_MASK_ENABLE"s;
			case ADLODNFeatureControl::ADL_ODN_MCLK_UNDERCLOCK_ENABLE: return "ADL_ODN_MCLK_UNDERCLOCK_ENABLE"s;
			case ADLODNFeatureControl::ADL_ODN_SCLK_DPM_THROTTLE_NOTIFY: return "ADL_ODN_SCLK_DPM_THROTTLE_NOTIFY"s;
			case ADLODNFeatureControl::ADL_ODN_POWER_UTILIZATION: return "ADL_ODN_POWER_UTILIZATION"s;
			case ADLODNFeatureControl::ADL_ODN_PERF_TUNING_SLIDER: return "ADL_ODN_PERF_TUNING_SLIDER"s;
			case ADLODNFeatureControl::ADL_ODN_REMOVE_WATTMAN_PAGE: return "ADL_ODN_REMOVE_WATTMAN_PAGE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLODNExtFeatureControl)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLODNExtFeatureControl*>(pEnum);
			switch (e) {
			case ADLODNExtFeatureControl::ADL_ODN_EXT_FEATURE_MEMORY_TIMING_TUNE: return "ADL_ODN_EXT_FEATURE_MEMORY_TIMING_TUNE"s;
			case ADLODNExtFeatureControl::ADL_ODN_EXT_FEATURE_FAN_ZERO_RPM_CONTROL: return "ADL_ODN_EXT_FEATURE_FAN_ZERO_RPM_CONTROL"s;
			case ADLODNExtFeatureControl::ADL_ODN_EXT_FEATURE_AUTO_UV_ENGINE: return "ADL_ODN_EXT_FEATURE_AUTO_UV_ENGINE"s;
			case ADLODNExtFeatureControl::ADL_ODN_EXT_FEATURE_AUTO_OC_ENGINE: return "ADL_ODN_EXT_FEATURE_AUTO_OC_ENGINE"s;
			case ADLODNExtFeatureControl::ADL_ODN_EXT_FEATURE_AUTO_OC_MEMORY: return "ADL_ODN_EXT_FEATURE_AUTO_OC_MEMORY"s;
			case ADLODNExtFeatureControl::ADL_ODN_EXT_FEATURE_FAN_CURVE: return "ADL_ODN_EXT_FEATURE_FAN_CURVE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLODNExtSettingId)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLODNExtSettingId*>(pEnum);
			switch (e) {
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_AC_TIMING: return "ADL_ODN_PARAMETER_AC_TIMING"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_ZERO_RPM_CONTROL: return "ADL_ODN_PARAMETER_FAN_ZERO_RPM_CONTROL"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_AUTO_UV_ENGINE: return "ADL_ODN_PARAMETER_AUTO_UV_ENGINE"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_AUTO_OC_ENGINE: return "ADL_ODN_PARAMETER_AUTO_OC_ENGINE"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_AUTO_OC_MEMORY: return "ADL_ODN_PARAMETER_AUTO_OC_MEMORY"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_1: return "ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_1"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_SPEED_1: return "ADL_ODN_PARAMETER_FAN_CURVE_SPEED_1"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_2: return "ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_2"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_SPEED_2: return "ADL_ODN_PARAMETER_FAN_CURVE_SPEED_2"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_3: return "ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_3"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_SPEED_3: return "ADL_ODN_PARAMETER_FAN_CURVE_SPEED_3"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_4: return "ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_4"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_SPEED_4: return "ADL_ODN_PARAMETER_FAN_CURVE_SPEED_4"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_5: return "ADL_ODN_PARAMETER_FAN_CURVE_TEMPERATURE_5"s;
			case ADLODNExtSettingId::ADL_ODN_PARAMETER_FAN_CURVE_SPEED_5: return "ADL_ODN_PARAMETER_FAN_CURVE_SPEED_5"s;
			case ADLODNExtSettingId::ADL_ODN_POWERGAUGE: return "ADL_ODN_POWERGAUGE"s;
			case ADLODNExtSettingId::ODN_COUNT: return "ODN_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLOD8FeatureControl)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLOD8FeatureControl*>(pEnum);
			switch (e) {
			case ADLOD8FeatureControl::ADL_OD8_GFXCLK_LIMITS: return "ADL_OD8_GFXCLK_LIMITS"s;
			case ADLOD8FeatureControl::ADL_OD8_GFXCLK_CURVE: return "ADL_OD8_GFXCLK_CURVE"s;
			case ADLOD8FeatureControl::ADL_OD8_UCLK_MAX: return "ADL_OD8_UCLK_MAX"s;
			case ADLOD8FeatureControl::ADL_OD8_POWER_LIMIT: return "ADL_OD8_POWER_LIMIT"s;
			case ADLOD8FeatureControl::ADL_OD8_ACOUSTIC_LIMIT_SCLK: return "ADL_OD8_ACOUSTIC_LIMIT_SCLK"s;
			case ADLOD8FeatureControl::ADL_OD8_FAN_SPEED_MIN: return "ADL_OD8_FAN_SPEED_MIN"s;
			case ADLOD8FeatureControl::ADL_OD8_TEMPERATURE_FAN: return "ADL_OD8_TEMPERATURE_FAN"s;
			case ADLOD8FeatureControl::ADL_OD8_TEMPERATURE_SYSTEM: return "ADL_OD8_TEMPERATURE_SYSTEM"s;
			case ADLOD8FeatureControl::ADL_OD8_MEMORY_TIMING_TUNE: return "ADL_OD8_MEMORY_TIMING_TUNE"s;
			case ADLOD8FeatureControl::ADL_OD8_FAN_ZERO_RPM_CONTROL: return "ADL_OD8_FAN_ZERO_RPM_CONTROL"s;
			case ADLOD8FeatureControl::ADL_OD8_AUTO_UV_ENGINE: return "ADL_OD8_AUTO_UV_ENGINE"s;
			case ADLOD8FeatureControl::ADL_OD8_AUTO_OC_ENGINE: return "ADL_OD8_AUTO_OC_ENGINE"s;
			case ADLOD8FeatureControl::ADL_OD8_AUTO_OC_MEMORY: return "ADL_OD8_AUTO_OC_MEMORY"s;
			case ADLOD8FeatureControl::ADL_OD8_FAN_CURVE: return "ADL_OD8_FAN_CURVE"s;
			case ADLOD8FeatureControl::ADL_OD8_WS_AUTO_FAN_ACOUSTIC_LIMIT: return "ADL_OD8_WS_AUTO_FAN_ACOUSTIC_LIMIT"s;
			case ADLOD8FeatureControl::ADL_OD8_GFXCLK_QUADRATIC_CURVE: return "ADL_OD8_GFXCLK_QUADRATIC_CURVE"s;
			case ADLOD8FeatureControl::ADL_OD8_OPTIMIZED_GPU_POWER_MODE: return "ADL_OD8_OPTIMIZED_GPU_POWER_MODE"s;
			case ADLOD8FeatureControl::ADL_OD8_ODVOLTAGE_LIMIT: return "ADL_OD8_ODVOLTAGE_LIMIT"s;
			case ADLOD8FeatureControl::ADL_OD8_POWER_GAUGE: return "ADL_OD8_POWER_GAUGE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLOD8SettingId)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLOD8SettingId*>(pEnum);
			switch (e) {
			case ADLOD8SettingId::OD8_GFXCLK_FMAX: return "OD8_GFXCLK_FMAX"s;
			case ADLOD8SettingId::OD8_GFXCLK_FMIN: return "OD8_GFXCLK_FMIN"s;
			case ADLOD8SettingId::OD8_GFXCLK_FREQ1: return "OD8_GFXCLK_FREQ1"s;
			case ADLOD8SettingId::OD8_GFXCLK_VOLTAGE1: return "OD8_GFXCLK_VOLTAGE1"s;
			case ADLOD8SettingId::OD8_GFXCLK_FREQ2: return "OD8_GFXCLK_FREQ2"s;
			case ADLOD8SettingId::OD8_GFXCLK_VOLTAGE2: return "OD8_GFXCLK_VOLTAGE2"s;
			case ADLOD8SettingId::OD8_GFXCLK_FREQ3: return "OD8_GFXCLK_FREQ3"s;
			case ADLOD8SettingId::OD8_GFXCLK_VOLTAGE3: return "OD8_GFXCLK_VOLTAGE3"s;
			case ADLOD8SettingId::OD8_UCLK_FMAX: return "OD8_UCLK_FMAX"s;
			case ADLOD8SettingId::OD8_POWER_PERCENTAGE: return "OD8_POWER_PERCENTAGE"s;
			case ADLOD8SettingId::OD8_FAN_MIN_SPEED: return "OD8_FAN_MIN_SPEED"s;
			case ADLOD8SettingId::OD8_FAN_ACOUSTIC_LIMIT: return "OD8_FAN_ACOUSTIC_LIMIT"s;
			case ADLOD8SettingId::OD8_FAN_TARGET_TEMP: return "OD8_FAN_TARGET_TEMP"s;
			case ADLOD8SettingId::OD8_OPERATING_TEMP_MAX: return "OD8_OPERATING_TEMP_MAX"s;
			case ADLOD8SettingId::OD8_AC_TIMING: return "OD8_AC_TIMING"s;
			case ADLOD8SettingId::OD8_FAN_ZERORPM_CONTROL: return "OD8_FAN_ZERORPM_CONTROL"s;
			case ADLOD8SettingId::OD8_AUTO_UV_ENGINE_CONTROL: return "OD8_AUTO_UV_ENGINE_CONTROL"s;
			case ADLOD8SettingId::OD8_AUTO_OC_ENGINE_CONTROL: return "OD8_AUTO_OC_ENGINE_CONTROL"s;
			case ADLOD8SettingId::OD8_AUTO_OC_MEMORY_CONTROL: return "OD8_AUTO_OC_MEMORY_CONTROL"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_TEMPERATURE_1: return "OD8_FAN_CURVE_TEMPERATURE_1"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_SPEED_1: return "OD8_FAN_CURVE_SPEED_1"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_TEMPERATURE_2: return "OD8_FAN_CURVE_TEMPERATURE_2"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_SPEED_2: return "OD8_FAN_CURVE_SPEED_2"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_TEMPERATURE_3: return "OD8_FAN_CURVE_TEMPERATURE_3"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_SPEED_3: return "OD8_FAN_CURVE_SPEED_3"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_TEMPERATURE_4: return "OD8_FAN_CURVE_TEMPERATURE_4"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_SPEED_4: return "OD8_FAN_CURVE_SPEED_4"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_TEMPERATURE_5: return "OD8_FAN_CURVE_TEMPERATURE_5"s;
			case ADLOD8SettingId::OD8_FAN_CURVE_SPEED_5: return "OD8_FAN_CURVE_SPEED_5"s;
			case ADLOD8SettingId::OD8_WS_FAN_AUTO_FAN_ACOUSTIC_LIMIT: return "OD8_WS_FAN_AUTO_FAN_ACOUSTIC_LIMIT"s;
			case ADLOD8SettingId::RESERVED_1: return "RESERVED_1"s;
			case ADLOD8SettingId::RESERVED_2: return "RESERVED_2"s;
			case ADLOD8SettingId::RESERVED_3: return "RESERVED_3"s;
			case ADLOD8SettingId::RESERVED_4: return "RESERVED_4"s;
			case ADLOD8SettingId::OD8_UCLK_FMIN: return "OD8_UCLK_FMIN"s;
			case ADLOD8SettingId::OD8_FAN_ZERO_RPM_STOP_TEMPERATURE: return "OD8_FAN_ZERO_RPM_STOP_TEMPERATURE"s;
			case ADLOD8SettingId::OD8_OPTIMZED_POWER_MODE: return "OD8_OPTIMZED_POWER_MODE"s;
			case ADLOD8SettingId::OD8_OD_VOLTAGE: return "OD8_OD_VOLTAGE"s;
			case ADLOD8SettingId::OD8_POWER_GAUGE: return "OD8_POWER_GAUGE"s;
			case ADLOD8SettingId::OD8_COUNT: return "OD8_COUNT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLSensorType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLSensorType*>(pEnum);
			switch (e) {
			case ADLSensorType::SENSOR_MAXTYPES: return "SENSOR_MAXTYPES"s;
			case ADLSensorType::PMLOG_CLK_GFXCLK: return "PMLOG_CLK_GFXCLK"s;
			case ADLSensorType::PMLOG_CLK_MEMCLK: return "PMLOG_CLK_MEMCLK"s;
			case ADLSensorType::PMLOG_CLK_SOCCLK: return "PMLOG_CLK_SOCCLK"s;
			case ADLSensorType::PMLOG_CLK_UVDCLK1: return "PMLOG_CLK_UVDCLK1"s;
			case ADLSensorType::PMLOG_CLK_UVDCLK2: return "PMLOG_CLK_UVDCLK2"s;
			case ADLSensorType::PMLOG_CLK_VCECLK: return "PMLOG_CLK_VCECLK"s;
			case ADLSensorType::PMLOG_CLK_VCNCLK: return "PMLOG_CLK_VCNCLK"s;
			case ADLSensorType::PMLOG_TEMPERATURE_EDGE: return "PMLOG_TEMPERATURE_EDGE"s;
			case ADLSensorType::PMLOG_TEMPERATURE_MEM: return "PMLOG_TEMPERATURE_MEM"s;
			case ADLSensorType::PMLOG_TEMPERATURE_VRVDDC: return "PMLOG_TEMPERATURE_VRVDDC"s;
			case ADLSensorType::PMLOG_TEMPERATURE_VRMVDD: return "PMLOG_TEMPERATURE_VRMVDD"s;
			case ADLSensorType::PMLOG_TEMPERATURE_LIQUID: return "PMLOG_TEMPERATURE_LIQUID"s;
			case ADLSensorType::PMLOG_TEMPERATURE_PLX: return "PMLOG_TEMPERATURE_PLX"s;
			case ADLSensorType::PMLOG_FAN_RPM: return "PMLOG_FAN_RPM"s;
			case ADLSensorType::PMLOG_FAN_PERCENTAGE: return "PMLOG_FAN_PERCENTAGE"s;
			case ADLSensorType::PMLOG_SOC_VOLTAGE: return "PMLOG_SOC_VOLTAGE"s;
			case ADLSensorType::PMLOG_SOC_POWER: return "PMLOG_SOC_POWER"s;
			case ADLSensorType::PMLOG_SOC_CURRENT: return "PMLOG_SOC_CURRENT"s;
			case ADLSensorType::PMLOG_INFO_ACTIVITY_GFX: return "PMLOG_INFO_ACTIVITY_GFX"s;
			case ADLSensorType::PMLOG_INFO_ACTIVITY_MEM: return "PMLOG_INFO_ACTIVITY_MEM"s;
			case ADLSensorType::PMLOG_GFX_VOLTAGE: return "PMLOG_GFX_VOLTAGE"s;
			case ADLSensorType::PMLOG_MEM_VOLTAGE: return "PMLOG_MEM_VOLTAGE"s;
			case ADLSensorType::PMLOG_ASIC_POWER: return "PMLOG_ASIC_POWER"s;
			case ADLSensorType::PMLOG_TEMPERATURE_VRSOC: return "PMLOG_TEMPERATURE_VRSOC"s;
			case ADLSensorType::PMLOG_TEMPERATURE_VRMVDD0: return "PMLOG_TEMPERATURE_VRMVDD0"s;
			case ADLSensorType::PMLOG_TEMPERATURE_VRMVDD1: return "PMLOG_TEMPERATURE_VRMVDD1"s;
			case ADLSensorType::PMLOG_TEMPERATURE_HOTSPOT: return "PMLOG_TEMPERATURE_HOTSPOT"s;
			case ADLSensorType::PMLOG_TEMPERATURE_GFX: return "PMLOG_TEMPERATURE_GFX"s;
			case ADLSensorType::PMLOG_TEMPERATURE_SOC: return "PMLOG_TEMPERATURE_SOC"s;
			case ADLSensorType::PMLOG_GFX_POWER: return "PMLOG_GFX_POWER"s;
			case ADLSensorType::PMLOG_GFX_CURRENT: return "PMLOG_GFX_CURRENT"s;
			case ADLSensorType::PMLOG_TEMPERATURE_CPU: return "PMLOG_TEMPERATURE_CPU"s;
			case ADLSensorType::PMLOG_CPU_POWER: return "PMLOG_CPU_POWER"s;
			case ADLSensorType::PMLOG_CLK_CPUCLK: return "PMLOG_CLK_CPUCLK"s;
			case ADLSensorType::PMLOG_THROTTLER_STATUS: return "PMLOG_THROTTLER_STATUS"s;
			case ADLSensorType::PMLOG_CLK_VCN1CLK1: return "PMLOG_CLK_VCN1CLK1"s;
			case ADLSensorType::PMLOG_CLK_VCN1CLK2: return "PMLOG_CLK_VCN1CLK2"s;
			case ADLSensorType::PMLOG_SMART_POWERSHIFT_CPU: return "PMLOG_SMART_POWERSHIFT_CPU"s;
			case ADLSensorType::PMLOG_SMART_POWERSHIFT_DGPU: return "PMLOG_SMART_POWERSHIFT_DGPU"s;
			case ADLSensorType::PMLOG_BUS_SPEED: return "PMLOG_BUS_SPEED"s;
			case ADLSensorType::PMLOG_BUS_LANES: return "PMLOG_BUS_LANES"s;
			case ADLSensorType::PMLOG_TEMPERATURE_LIQUID0: return "PMLOG_TEMPERATURE_LIQUID0"s;
			case ADLSensorType::PMLOG_TEMPERATURE_LIQUID1: return "PMLOG_TEMPERATURE_LIQUID1"s;
			case ADLSensorType::PMLOG_CLK_FCLK: return "PMLOG_CLK_FCLK"s;
			case ADLSensorType::PMLOG_THROTTLER_STATUS_CPU: return "PMLOG_THROTTLER_STATUS_CPU"s;
			case ADLSensorType::PMLOG_MAX_SENSORS_REAL: return "PMLOG_MAX_SENSORS_REAL"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_THROTTLE_NOTIFICATION)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_THROTTLE_NOTIFICATION*>(pEnum);
			switch (e) {
			case ADL_THROTTLE_NOTIFICATION::ADL_PMLOG_THROTTLE_POWER: return "ADL_PMLOG_THROTTLE_POWER"s;
			case ADL_THROTTLE_NOTIFICATION::ADL_PMLOG_THROTTLE_THERMAL: return "ADL_PMLOG_THROTTLE_THERMAL"s;
			case ADL_THROTTLE_NOTIFICATION::ADL_PMLOG_THROTTLE_CURRENT: return "ADL_PMLOG_THROTTLE_CURRENT"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_PMLOG_SENSORS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_PMLOG_SENSORS*>(pEnum);
			switch (e) {
			case ADL_PMLOG_SENSORS::ADL_SENSOR_MAXTYPES: return "ADL_SENSOR_MAXTYPES"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_GFXCLK: return "ADL_PMLOG_CLK_GFXCLK"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_MEMCLK: return "ADL_PMLOG_CLK_MEMCLK"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_SOCCLK: return "ADL_PMLOG_CLK_SOCCLK"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_UVDCLK1: return "ADL_PMLOG_CLK_UVDCLK1"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_UVDCLK2: return "ADL_PMLOG_CLK_UVDCLK2"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_VCECLK: return "ADL_PMLOG_CLK_VCECLK"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_VCNCLK: return "ADL_PMLOG_CLK_VCNCLK"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_EDGE: return "ADL_PMLOG_TEMPERATURE_EDGE"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_MEM: return "ADL_PMLOG_TEMPERATURE_MEM"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_VRVDDC: return "ADL_PMLOG_TEMPERATURE_VRVDDC"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_VRMVDD: return "ADL_PMLOG_TEMPERATURE_VRMVDD"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_LIQUID: return "ADL_PMLOG_TEMPERATURE_LIQUID"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_PLX: return "ADL_PMLOG_TEMPERATURE_PLX"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_FAN_RPM: return "ADL_PMLOG_FAN_RPM"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_FAN_PERCENTAGE: return "ADL_PMLOG_FAN_PERCENTAGE"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_SOC_VOLTAGE: return "ADL_PMLOG_SOC_VOLTAGE"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_SOC_POWER: return "ADL_PMLOG_SOC_POWER"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_SOC_CURRENT: return "ADL_PMLOG_SOC_CURRENT"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_INFO_ACTIVITY_GFX: return "ADL_PMLOG_INFO_ACTIVITY_GFX"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_INFO_ACTIVITY_MEM: return "ADL_PMLOG_INFO_ACTIVITY_MEM"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_GFX_VOLTAGE: return "ADL_PMLOG_GFX_VOLTAGE"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_MEM_VOLTAGE: return "ADL_PMLOG_MEM_VOLTAGE"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_ASIC_POWER: return "ADL_PMLOG_ASIC_POWER"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_VRSOC: return "ADL_PMLOG_TEMPERATURE_VRSOC"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_VRMVDD0: return "ADL_PMLOG_TEMPERATURE_VRMVDD0"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_VRMVDD1: return "ADL_PMLOG_TEMPERATURE_VRMVDD1"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_HOTSPOT: return "ADL_PMLOG_TEMPERATURE_HOTSPOT"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_GFX: return "ADL_PMLOG_TEMPERATURE_GFX"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_SOC: return "ADL_PMLOG_TEMPERATURE_SOC"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_GFX_POWER: return "ADL_PMLOG_GFX_POWER"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_GFX_CURRENT: return "ADL_PMLOG_GFX_CURRENT"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_CPU: return "ADL_PMLOG_TEMPERATURE_CPU"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CPU_POWER: return "ADL_PMLOG_CPU_POWER"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_CPUCLK: return "ADL_PMLOG_CLK_CPUCLK"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_THROTTLER_STATUS: return "ADL_PMLOG_THROTTLER_STATUS"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_VCN1CLK1: return "ADL_PMLOG_CLK_VCN1CLK1"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_VCN1CLK2: return "ADL_PMLOG_CLK_VCN1CLK2"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_SMART_POWERSHIFT_CPU: return "ADL_PMLOG_SMART_POWERSHIFT_CPU"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_SMART_POWERSHIFT_DGPU: return "ADL_PMLOG_SMART_POWERSHIFT_DGPU"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_BUS_SPEED: return "ADL_PMLOG_BUS_SPEED"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_BUS_LANES: return "ADL_PMLOG_BUS_LANES"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_LIQUID0: return "ADL_PMLOG_TEMPERATURE_LIQUID0"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_TEMPERATURE_LIQUID1: return "ADL_PMLOG_TEMPERATURE_LIQUID1"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_CLK_FCLK: return "ADL_PMLOG_CLK_FCLK"s;
			case ADL_PMLOG_SENSORS::ADL_PMLOG_THROTTLER_STATUS_CPU: return "ADL_PMLOG_THROTTLER_STATUS_CPU"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_RAS_ERROR_INJECTION_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_RAS_ERROR_INJECTION_MODE*>(pEnum);
			switch (e) {
			case ADL_RAS_ERROR_INJECTION_MODE::ADL_RAS_ERROR_INJECTION_MODE_SINGLE: return "ADL_RAS_ERROR_INJECTION_MODE_SINGLE"s;
			case ADL_RAS_ERROR_INJECTION_MODE::ADL_RAS_ERROR_INJECTION_MODE_MULTIPLE: return "ADL_RAS_ERROR_INJECTION_MODE_MULTIPLE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_RAS_BLOCK_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_RAS_BLOCK_ID*>(pEnum);
			switch (e) {
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_UMC: return "ADL_RAS_BLOCK_ID_UMC"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_SDMA: return "ADL_RAS_BLOCK_ID_SDMA"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_GFX_HUB: return "ADL_RAS_BLOCK_ID_GFX_HUB"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_MMHUB: return "ADL_RAS_BLOCK_ID_MMHUB"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_ATHUB: return "ADL_RAS_BLOCK_ID_ATHUB"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_PCIE_BIF: return "ADL_RAS_BLOCK_ID_PCIE_BIF"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_HDP: return "ADL_RAS_BLOCK_ID_HDP"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_XGMI_WAFL: return "ADL_RAS_BLOCK_ID_XGMI_WAFL"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_DF: return "ADL_RAS_BLOCK_ID_DF"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_SMN: return "ADL_RAS_BLOCK_ID_SMN"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_SEM: return "ADL_RAS_BLOCK_ID_SEM"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_MP0: return "ADL_RAS_BLOCK_ID_MP0"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_MP1: return "ADL_RAS_BLOCK_ID_MP1"s;
			case ADL_RAS_BLOCK_ID::ADL_RAS_BLOCK_ID_FUSE: return "ADL_RAS_BLOCK_ID_FUSE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_MEM_SUB_BLOCK_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_MEM_SUB_BLOCK_ID*>(pEnum);
			switch (e) {
			case ADL_MEM_SUB_BLOCK_ID::ADL_RAS__UMC_HBM: return "ADL_RAS__UMC_HBM"s;
			case ADL_MEM_SUB_BLOCK_ID::ADL_RAS__UMC_SRAM: return "ADL_RAS__UMC_SRAM"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(_ADL_RAS_ERROR_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ADL_RAS_ERROR_TYPE*>(pEnum);
			switch (e) {
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__NONE: return "ADL_RAS_ERROR__NONE"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY: return "ADL_RAS_ERROR__PARITY"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__SINGLE_CORRECTABLE: return "ADL_RAS_ERROR__SINGLE_CORRECTABLE"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE: return "ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__MULTI_UNCORRECTABLE: return "ADL_RAS_ERROR__MULTI_UNCORRECTABLE"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY_MULTI_UNCORRECTABLE: return "ADL_RAS_ERROR__PARITY_MULTI_UNCORRECTABLE"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE: return "ADL_RAS_ERROR__SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE: return "ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__POISON: return "ADL_RAS_ERROR__POISON"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY_POISON: return "ADL_RAS_ERROR__PARITY_POISON"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__SINGLE_CORRECTABLE_POISON: return "ADL_RAS_ERROR__SINGLE_CORRECTABLE_POISON"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE_POISON: return "ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE_POISON"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__MULTI_UNCORRECTABLE_POISON: return "ADL_RAS_ERROR__MULTI_UNCORRECTABLE_POISON"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY_MULTI_UNCORRECTABLE_POISON: return "ADL_RAS_ERROR__PARITY_MULTI_UNCORRECTABLE_POISON"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE_POISON: return "ADL_RAS_ERROR__SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE_POISON"s;
			case _ADL_RAS_ERROR_TYPE::ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE_POISON: return "ADL_RAS_ERROR__PARITY_SINGLE_CORRECTABLE_MULTI_UNCORRECTABLE_POISON"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_RAS_INJECTION_METHOD)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_RAS_INJECTION_METHOD*>(pEnum);
			switch (e) {
			case ADL_RAS_INJECTION_METHOD::ADL_RAS_ERROR__UMC_METH_COHERENT: return "ADL_RAS_ERROR__UMC_METH_COHERENT"s;
			case ADL_RAS_INJECTION_METHOD::ADL_RAS_ERROR__UMC_METH_SINGLE_SHOT: return "ADL_RAS_ERROR__UMC_METH_SINGLE_SHOT"s;
			case ADL_RAS_INJECTION_METHOD::ADL_RAS_ERROR__UMC_METH_PERSISTENT: return "ADL_RAS_ERROR__UMC_METH_PERSISTENT"s;
			case ADL_RAS_INJECTION_METHOD::ADL_RAS_ERROR__UMC_METH_PERSISTENT_DISABLE: return "ADL_RAS_ERROR__UMC_METH_PERSISTENT_DISABLE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_DRIVER_EVENT_TYPE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_DRIVER_EVENT_TYPE*>(pEnum);
			switch (e) {
			case ADL_DRIVER_EVENT_TYPE::ADL_EVENT_ID_AUTO_FEATURE_COMPLETED: return "ADL_EVENT_ID_AUTO_FEATURE_COMPLETED"s;
			case ADL_DRIVER_EVENT_TYPE::ADL_EVENT_ID_FEATURE_AVAILABILITY: return "ADL_EVENT_ID_FEATURE_AVAILABILITY"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_UIFEATURES_GROUP)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_UIFEATURES_GROUP*>(pEnum);
			switch (e) {
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_DVR: return "ADL_UIFEATURES_GROUP_DVR"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_TURBOSYNC: return "ADL_UIFEATURES_GROUP_TURBOSYNC"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_FRAMEMETRICSMONITOR: return "ADL_UIFEATURES_GROUP_FRAMEMETRICSMONITOR"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_FRTC: return "ADL_UIFEATURES_GROUP_FRTC"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_XVISION: return "ADL_UIFEATURES_GROUP_XVISION"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_BLOCKCHAIN: return "ADL_UIFEATURES_GROUP_BLOCKCHAIN"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_GAMEINTELLIGENCE: return "ADL_UIFEATURES_GROUP_GAMEINTELLIGENCE"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_CHILL: return "ADL_UIFEATURES_GROUP_CHILL"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_DELAG: return "ADL_UIFEATURES_GROUP_DELAG"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_BOOST: return "ADL_UIFEATURES_GROUP_BOOST"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_USU: return "ADL_UIFEATURES_GROUP_USU"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_XGMI: return "ADL_UIFEATURES_GROUP_XGMI"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_PROVSR: return "ADL_UIFEATURES_GROUP_PROVSR"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_SMA: return "ADL_UIFEATURES_GROUP_SMA"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_CAMERA: return "ADL_UIFEATURES_GROUP_CAMERA"s;
			case ADL_UIFEATURES_GROUP::ADL_UIFEATURES_GROUP_FRTCPRO: return "ADL_UIFEATURES_GROUP_FRTCPRO"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_RADEON_USB_LED_BAR_CONTROLS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_RADEON_USB_LED_BAR_CONTROLS*>(pEnum);
			switch (e) {
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_OFF: return "RadeonLEDBarControl_OFF"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_Static: return "RadeonLEDBarControl_Static"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_Rainbow: return "RadeonLEDBarControl_Rainbow"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_Swirl: return "RadeonLEDBarControl_Swirl"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_Chase: return "RadeonLEDBarControl_Chase"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_Bounce: return "RadeonLEDBarControl_Bounce"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_MorseCode: return "RadeonLEDBarControl_MorseCode"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_ColorCycle: return "RadeonLEDBarControl_ColorCycle"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_Breathing: return "RadeonLEDBarControl_Breathing"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_CustomPattern: return "RadeonLEDBarControl_CustomPattern"s;
			case ADL_RADEON_USB_LED_BAR_CONTROLS::RadeonLEDBarControl_MAX: return "RadeonLEDBarControl_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_RADEON_USB_LED_CONTROL_CONFIGS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_RADEON_USB_LED_CONTROL_CONFIGS*>(pEnum);
			switch (e) {
			case ADL_RADEON_USB_LED_CONTROL_CONFIGS::RadeonLEDPattern_Speed: return "RadeonLEDPattern_Speed"s;
			case ADL_RADEON_USB_LED_CONTROL_CONFIGS::RadeonLEDPattern_Brightness: return "RadeonLEDPattern_Brightness"s;
			case ADL_RADEON_USB_LED_CONTROL_CONFIGS::RadeonLEDPattern_Direction: return "RadeonLEDPattern_Direction"s;
			case ADL_RADEON_USB_LED_CONTROL_CONFIGS::RadeonLEDPattern_Color: return "RadeonLEDPattern_Color"s;
			case ADL_RADEON_USB_LED_CONTROL_CONFIGS::RadeonLEDPattern_MAX: return "RadeonLEDPattern_MAX"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_USER_SETTINGS)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_USER_SETTINGS*>(pEnum);
			switch (e) {
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_ENHANCEDSYNC: return "ADL_USER_SETTINGS_ENHANCEDSYNC"s;
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_CHILL_PROFILE: return "ADL_USER_SETTINGS_CHILL_PROFILE"s;
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_DELAG_PROFILE: return "ADL_USER_SETTINGS_DELAG_PROFILE"s;
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_BOOST_PROFILE: return "ADL_USER_SETTINGS_BOOST_PROFILE"s;
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_USU_PROFILE: return "ADL_USER_SETTINGS_USU_PROFILE"s;
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_CVDC_PROFILE: return "ADL_USER_SETTINGS_CVDC_PROFILE"s;
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_SCE_PROFILE: return "ADL_USER_SETTINGS_SCE_PROFILE"s;
			case ADL_USER_SETTINGS::ADL_USER_SETTINGS_PROVSR: return "ADL_USER_SETTINGS_PROVSR"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLvRamVendors)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLvRamVendors*>(pEnum);
			switch (e) {
			case ADLvRamVendors::ADLvRamVendor_Unsupported: return "ADLvRamVendor_Unsupported"s;
			case ADLvRamVendors::ADLvRamVendor_SAMSUNG: return "ADLvRamVendor_SAMSUNG"s;
			case ADLvRamVendors::ADLvRamVendor_INFINEON: return "ADLvRamVendor_INFINEON"s;
			case ADLvRamVendors::ADLvRamVendor_ELPIDA: return "ADLvRamVendor_ELPIDA"s;
			case ADLvRamVendors::ADLvRamVendor_ETRON: return "ADLvRamVendor_ETRON"s;
			case ADLvRamVendors::ADLvRamVendor_NANYA: return "ADLvRamVendor_NANYA"s;
			case ADLvRamVendors::ADLvRamVendor_HYNIX: return "ADLvRamVendor_HYNIX"s;
			case ADLvRamVendors::ADLvRamVendor_MOSEL: return "ADLvRamVendor_MOSEL"s;
			case ADLvRamVendors::ADLvRamVendor_WINBOND: return "ADLvRamVendor_WINBOND"s;
			case ADLvRamVendors::ADLvRamVendor_ESMT: return "ADLvRamVendor_ESMT"s;
			case ADLvRamVendors::ADLvRamVendor_MICRON: return "ADLvRamVendor_MICRON"s;
			case ADLvRamVendors::ADLvRamVendor_Undefined: return "ADLvRamVendor_Undefined"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLPxType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLPxType*>(pEnum);
			switch (e) {
			case ADLPxType::ADL_PX_NONE: return "ADL_PX_NONE"s;
			case ADLPxType::ADL_SWITCHABLE_AMDAMD: return "ADL_SWITCHABLE_AMDAMD"s;
			case ADLPxType::ADL_HG_AMDAMD: return "ADL_HG_AMDAMD"s;
			case ADLPxType::ADL_SWITCHABLE_AMDOTHER: return "ADL_SWITCHABLE_AMDOTHER"s;
			case ADLPxType::ADL_HG_AMDOTHER: return "ADL_HG_AMDOTHER"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLAppProcessState)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLAppProcessState*>(pEnum);
			switch (e) {
			case ADLAppProcessState::APP_PROC_INVALID: return "APP_PROC_INVALID"s;
			case ADLAppProcessState::APP_PROC_PREMPTION: return "APP_PROC_PREMPTION"s;
			case ADLAppProcessState::APP_PROC_CREATION: return "APP_PROC_CREATION"s;
			case ADLAppProcessState::APP_PROC_READ: return "APP_PROC_READ"s;
			case ADLAppProcessState::APP_PROC_WAIT: return "APP_PROC_WAIT"s;
			case ADLAppProcessState::APP_PROC_RUNNING: return "APP_PROC_RUNNING"s;
			case ADLAppProcessState::APP_PROC_TERMINATE: return "APP_PROC_TERMINATE"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLAppInterceptionListType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLAppInterceptionListType*>(pEnum);
			switch (e) {
			case ADLAppInterceptionListType::ADL_INVALID_FORMAT: return "ADL_INVALID_FORMAT"s;
			case ADLAppInterceptionListType::ADL_IMAGEFILEFORMAT: return "ADL_IMAGEFILEFORMAT"s;
			case ADLAppInterceptionListType::ADL_ENVVAR: return "ADL_ENVVAR"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_AP_DATABASE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_AP_DATABASE*>(pEnum);
			switch (e) {
			case ADL_AP_DATABASE::ADL_AP_DATABASE__SYSTEM: return "ADL_AP_DATABASE__SYSTEM"s;
			case ADL_AP_DATABASE::ADL_AP_DATABASE__USER: return "ADL_AP_DATABASE__USER"s;
			case ADL_AP_DATABASE::ADL_AP_DATABASE__OEM: return "ADL_AP_DATABASE__OEM"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_ERROR_RECORD_SEVERITY)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_ERROR_RECORD_SEVERITY*>(pEnum);
			switch (e) {
			case ADL_ERROR_RECORD_SEVERITY::ADL_GLOBALLY_UNCORRECTED: return "ADL_GLOBALLY_UNCORRECTED"s;
			case ADL_ERROR_RECORD_SEVERITY::ADL_LOCALLY_UNCORRECTED: return "ADL_LOCALLY_UNCORRECTED"s;
			case ADL_ERROR_RECORD_SEVERITY::ADL_DEFFERRED: return "ADL_DEFFERRED"s;
			case ADL_ERROR_RECORD_SEVERITY::ADL_CORRECTED: return "ADL_CORRECTED"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_EDC_BLOCK_ID)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_EDC_BLOCK_ID*>(pEnum);
			switch (e) {
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_SQCIS: return "ADL_EDC_BLOCK_ID_SQCIS"s;
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_SQCDS: return "ADL_EDC_BLOCK_ID_SQCDS"s;
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_SGPR: return "ADL_EDC_BLOCK_ID_SGPR"s;
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_VGPR: return "ADL_EDC_BLOCK_ID_VGPR"s;
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_LDS: return "ADL_EDC_BLOCK_ID_LDS"s;
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_GDS: return "ADL_EDC_BLOCK_ID_GDS"s;
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_TCL1: return "ADL_EDC_BLOCK_ID_TCL1"s;
			case ADL_EDC_BLOCK_ID::ADL_EDC_BLOCK_ID_TCL2: return "ADL_EDC_BLOCK_ID_TCL2"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADL_ERROR_INJECTION_MODE)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADL_ERROR_INJECTION_MODE*>(pEnum);
			switch (e) {
			case ADL_ERROR_INJECTION_MODE::ADL_ERROR_INJECTION_MODE_SINGLE: return "ADL_ERROR_INJECTION_MODE_SINGLE"s;
			case ADL_ERROR_INJECTION_MODE::ADL_ERROR_INJECTION_MODE_MULTIPLE: return "ADL_ERROR_INJECTION_MODE_MULTIPLE"s;
			case ADL_ERROR_INJECTION_MODE::ADL_ERROR_INJECTION_MODE_ADDRESS: return "ADL_ERROR_INJECTION_MODE_ADDRESS"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLODNCurrentPowerType)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLODNCurrentPowerType*>(pEnum);
			switch (e) {
			case ADLODNCurrentPowerType::ODN_GPU_TOTAL_POWER: return "ODN_GPU_TOTAL_POWER"s;
			case ADLODNCurrentPowerType::ODN_GPU_PPT_POWER: return "ODN_GPU_PPT_POWER"s;
			case ADLODNCurrentPowerType::ODN_GPU_SOCKET_POWER: return "ODN_GPU_SOCKET_POWER"s;
			case ADLODNCurrentPowerType::ODN_GPU_CHIP_POWER: return "ODN_GPU_CHIP_POWER"s;
			default: return "{ unknown }"s;
			}
		};
		dumpers[typeid(ADLPreFlipPostProcessingLUTAlgorithm)] = [](const void* pEnum) {
			const auto& e = *static_cast<const ADLPreFlipPostProcessingLUTAlgorithm*>(pEnum);
			switch (e) {
			case ADLPreFlipPostProcessingLUTAlgorithm::ADLPreFlipPostProcessingLUTAlgorithm_Default: return "ADLPreFlipPostProcessingLUTAlgorithm_Default"s;
			case ADLPreFlipPostProcessingLUTAlgorithm::ADLPreFlipPostProcessingLUTAlgorithm_Full: return "ADLPreFlipPostProcessingLUTAlgorithm_Full"s;
			case ADLPreFlipPostProcessingLUTAlgorithm::ADLPreFlipPostProcessingLUTAlgorithm_Approximation: return "ADLPreFlipPostProcessingLUTAlgorithm_Approximation"s;
			default: return "{ unknown }"s;
			}
		};
	}
}
