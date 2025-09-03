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
#include <IntelPresentMon/ControlLib/igcl_api.h>
#include <IntelPresentMon/ControlLib/ctlpvttemp_api.h>

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
				<< " .Data4 = " << DumpArray_<uint8_t, 8, true>(s.Data4)
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
				<< " .Reserved = " << DumpArray_<uint32_t, 4, true>(s.Reserved)
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
				<< " .Reserved = " << DumpArray_<uint64_t, 4, true>(s.Reserved)
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
				<< " .Reserved = " << DumpArray_<int64_t, 4, true>(s.Reserved)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .Data = " << DumpArray_<uint8_t, 128, true>(s.Data)
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
				<< " .Data = " << DumpArray_<uint8_t, 128, true>(s.Data)
				<< " .ReservedFields = " << DumpArray_<uint32_t, 4, true>(s.ReservedFields)
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
				<< " .Data = " << DumpArray_<uint8_t, 132, true>(s.Data)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 4, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 4, true>(s.ReservedFields)
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
				<< " .PreOffsets = " << DumpArray_<double, 3, true>(s.PreOffsets)
				<< " .PostOffsets = " << DumpArray_<double, 3, true>(s.PostOffsets)
				<< " .Matrix = " << DumpArray_<double[3], 3, false>(s.Matrix)
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
		dumpers[typeid(_ctl_mux_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_mux_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_mux_properties_t {"
				<< " .Size = " << s.Size
				<< " .Version = " << (int)s.Version
				<< " .MuxId = " << (int)s.MuxId
				<< " .Count = " << s.Count
				<< " .phDisplayOutputs = " << (s.phDisplayOutputs ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(s.phDisplayOutputs)) : "null"s)
				<< " .IndexOfDisplayOutputOwningMux = " << (int)s.IndexOfDisplayOutputOwningMux
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 4, true>(s.ReservedFields)
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
				<< " .VblankTS = " << DumpArray_<uint64_t, 16, true>(s.VblankTS)
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
				<< " .Reserved = " << DumpArray_<uint64_t, 4, true>(s.Reserved)
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
				<< " .SupportedWireFormat = " << DumpArray_<ctl_wire_format_t, 4, false>(s.SupportedWireFormat)
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
				<< " .Reserved = " << DumpArray_<uint32_t, 25, true>(s.Reserved)
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
				<< " .table = " << DumpArray_<ctl_fan_temp_speed_t, 32, false>(s.table)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 15, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .ReservedFields = " << DumpArray_<uint32_t, 16, true>(s.ReservedFields)
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
				<< " .psu = " << DumpArray_<ctl_psu_info_t, 5, false>(s.psu)
				<< " .fanSpeed = " << DumpArray_<ctl_oc_telemetry_item_t, 5, false>(s.fanSpeed)
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
		dumpers[typeid(_ctl_voltage_frequency_point_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_voltage_frequency_point_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_voltage_frequency_point_t {"
				<< " .Voltage = " << s.Voltage
				<< " .Frequency = " << s.Frequency
				<< " }";
			return oss.str();
		};
		dumpers[typeid(_ctl_oc_properties2_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_oc_properties2_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_oc_properties2_t {"
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
		dumpers[typeid(_ctl_power_telemetry2_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const _ctl_power_telemetry2_t*>(pStruct);
			std::ostringstream oss;
			oss << std::boolalpha << "struct _ctl_power_telemetry2_t {"
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
				<< " .psu = " << DumpArray_<ctl_psu_info_t, 5, false>(s.psu)
				<< " .fanSpeed = " << DumpArray_<ctl_oc_telemetry_item_t, 5, false>(s.fanSpeed)
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

		// enums
		dumpers[typeid(_ctl_init_flag_t)] = [](const void* pEnum) {
			const auto& e = *static_cast<const _ctl_init_flag_t*>(pEnum);
			switch (e) {
			case _ctl_init_flag_t::CTL_INIT_FLAG_USE_LEVEL_ZERO: return "CTL_INIT_FLAG_USE_LEVEL_ZERO"s;
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
	}
}
