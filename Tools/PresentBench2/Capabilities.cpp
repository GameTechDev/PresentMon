// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "Capabilities.h"
#include "backends/DxgiCommon.h"
#include <ControlLib/igcl/igcl_api.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <format>
#include <nlohmann/json.hpp>
#include <wrl/client.h>
#include <vector>

#if defined(PMON_PRESENTBENCH2_HAS_XESS_FG)
#include <xell/xell_d3d12.h>
#include <xess_fg/xefg_swapchain_d3d12.h>
#endif

namespace pb2
{
	namespace
	{
		using PFN_ctlInit = ctl_result_t(CTL_APICALL*)(ctl_init_args_t*, ctl_api_handle_t*);
		using PFN_ctlClose = ctl_result_t(CTL_APICALL*)(ctl_api_handle_t);
		using PFN_ctlEnumerateDevices = ctl_result_t(CTL_APICALL*)(
			ctl_api_handle_t, uint32_t*, ctl_device_adapter_handle_t*);
		using PFN_ctlGetDeviceProperties = ctl_result_t(CTL_APICALL*)(
			ctl_device_adapter_handle_t, ctl_device_adapter_properties_t*);
		using PFN_ctlGetSupported3DCapabilities = ctl_result_t(CTL_APICALL*)(
			ctl_device_adapter_handle_t, ctl_3d_feature_caps_t*);

		bool FeatureSupported(
			const ctl_3d_feature_details_t* details,
			uint32_t count,
			ctl_3d_feature_t feature)
		{
			for (uint32_t i = 0; i < count; ++i) {
				if (details[i].FeatureType == feature) {
					return true;
				}
			}
			return false;
		}

		void ProbeIgclMachine(XessFgCapability& out, LUID selectedLuid)
		{
			HMODULE lib = LoadLibraryExW(
				L"ControlLib.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			if (!lib) {
				out.igclReason = "ControlLib.dll not found (Intel Graphics Control Library)";
				return;
			}

			const auto pfnInit = (PFN_ctlInit)GetProcAddress(lib, "ctlInit");
			const auto pfnClose = (PFN_ctlClose)GetProcAddress(lib, "ctlClose");
			const auto pfnEnum = (PFN_ctlEnumerateDevices)GetProcAddress(lib, "ctlEnumerateDevices");
			const auto pfnProps = (PFN_ctlGetDeviceProperties)GetProcAddress(
				lib, "ctlGetDeviceProperties");
			const auto pfnCaps = (PFN_ctlGetSupported3DCapabilities)GetProcAddress(
				lib, "ctlGetSupported3DCapabilities");
			if (!pfnInit || !pfnClose || !pfnEnum || !pfnProps || !pfnCaps) {
				out.igclReason = "ControlLib.dll missing required IGCL exports";
				FreeLibrary(lib);
				return;
			}

			ctl_init_args_t initArgs{};
			initArgs.Size = sizeof(initArgs);
			initArgs.AppVersion = CTL_MAKE_VERSION(CTL_IMPL_MAJOR_VERSION, CTL_IMPL_MINOR_VERSION);
			initArgs.flags = CTL_INIT_FLAG_USE_LEVEL_ZERO;
			ctl_api_handle_t api = nullptr;
			const ctl_result_t initResult = pfnInit(&initArgs, &api);
			if (initResult != CTL_RESULT_SUCCESS || !api) {
				out.igclReason = "ctlInit failed (IGCL unavailable or version mismatch)";
				FreeLibrary(lib);
				return;
			}

			uint32_t count = 0;
			ctl_result_t enumResult = pfnEnum(api, &count, nullptr);
			if (enumResult != CTL_RESULT_SUCCESS || count == 0) {
				out.igclReason = count == 0 ? "No IGCL adapters enumerated" :
					"ctlEnumerateDevices(count) failed";
				pfnClose(api);
				FreeLibrary(lib);
				return;
			}

			std::vector<ctl_device_adapter_handle_t> handles((size_t)count);
			enumResult = pfnEnum(api, &count, handles.data());
			if (enumResult != CTL_RESULT_SUCCESS) {
				out.igclReason = "ctlEnumerateDevices(list) failed";
				pfnClose(api);
				FreeLibrary(lib);
				return;
			}

			bool anyFg = false;
			for (uint32_t i = 0; i < count; ++i) {
				XessFgAdapterCapability adapter{};
				LUID luid{};
				ctl_device_adapter_properties_t props{};
				props.Size = sizeof(props);
				props.pDeviceID = &luid;
				props.device_id_size = sizeof(luid);
				if (pfnProps(handles[i], &props) != CTL_RESULT_SUCCESS) {
					continue;
				}
				adapter.name = props.name;
				adapter.graphics = (props.device_type == CTL_DEVICE_TYPE_GRAPHICS);
				adapter.selected = luid.LowPart == selectedLuid.LowPart &&
					luid.HighPart == selectedLuid.HighPart;
				adapter.integrated =
					(props.graphics_adapter_properties & CTL_ADAPTER_PROPERTIES_FLAG_INTEGRATED) != 0;
				adapter.pciVendorId = props.pci_vendor_id;
				adapter.pciDeviceId = props.pci_device_id;
				if (!adapter.graphics) {
					out.adapters.push_back(std::move(adapter));
					continue;
				}

				ctl_3d_feature_caps_t caps{};
				caps.Size = sizeof(caps);
				ctl_result_t capsResult = pfnCaps(handles[i], &caps);
				if (capsResult != CTL_RESULT_SUCCESS || caps.NumSupportedFeatures == 0) {
					out.adapters.push_back(std::move(adapter));
					continue;
				}

				std::vector<ctl_3d_feature_details_t> details((size_t)caps.NumSupportedFeatures);
				caps.pFeatureDetails = details.data();
				capsResult = pfnCaps(handles[i], &caps);
				if (capsResult == CTL_RESULT_SUCCESS) {
					adapter.igclFrameGeneration = FeatureSupported(
						details.data(), caps.NumSupportedFeatures, CTL_3D_FEATURE_FRAME_GENERATION);
					adapter.igclFrameGenerationControl = FeatureSupported(
						details.data(),
						caps.NumSupportedFeatures,
						CTL_3D_FEATURE_FRAME_GENERATION_CONTROL);
					if (adapter.selected && adapter.igclFrameGeneration) {
						anyFg = true;
					}
				}
				out.adapters.push_back(std::move(adapter));
			}

			if (anyFg) {
				out.igclReason.clear();
			}
			else if (out.igclReason.empty()) {
				out.igclReason =
					"The selected adapter did not report IGCL frame-generation support";
			}

			pfnClose(api);
			FreeLibrary(lib);
		}

#if defined(PMON_PRESENTBENCH2_HAS_XESS_FG)
		void ProbeXessFgDevice(XessFgCapability& out, IDXGIAdapter1* adapter)
		{
			Microsoft::WRL::ComPtr<ID3D12Device> device;
			const HRESULT deviceResult = D3D12CreateDevice(
				adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
			if (FAILED(deviceResult)) {
				out.feature.reason = std::format(
					"D3D12CreateDevice failed while probing XeSS-FG: 0x{:08X}",
					(unsigned)deviceResult);
				return;
			}

			xefg_swapchain_handle_t xefgContext = nullptr;
			const xefg_swapchain_result_t xefgResult =
				xefgSwapChainD3D12CreateContext(device.Get(), &xefgContext);
			if (xefgResult != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
				out.feature.reason = std::format(
					"xefgSwapChainD3D12CreateContext reported unsupported: {}",
					(int)xefgResult);
				return;
			}

			xell_context_handle_t xellContext = nullptr;
			const xell_result_t xellResult = xellD3D12CreateContext(device.Get(), &xellContext);
			if (xellResult != XELL_RESULT_SUCCESS) {
				xefgSwapChainDestroy(xefgContext);
				out.feature.reason = std::format(
					"xellD3D12CreateContext reported unsupported: {}",
					(int)xellResult);
				return;
			}

			xellDestroyContext(xellContext);
			xefgSwapChainDestroy(xefgContext);
			out.feature.runtime = true;
			out.feature.reason.clear();
		}
#endif
	}

	CapabilityReport ProbeCapabilities(uint32_t adapterIndex)
	{
		CapabilityReport report;
#if defined(PMON_PRESENTBENCH2_HAS_XESS_FG)
		report.xessFg.feature.compiled = true;
#else
		report.xessFg.feature.compiled = false;
		report.xessFg.feature.reason =
			"XeSS-FG SDK not fetched (configure with -DPMON_PRESENTBENCH2_FETCH_XESS_FG=ON)";
#endif
		Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
			report.xessFg.igclReason = "CreateDXGIFactory1 failed";
		}
		else {
			const auto selected = ResolveAdapter(adapterIndex, factory.Get());
			ProbeIgclMachine(report.xessFg, selected.luid);
#if defined(PMON_PRESENTBENCH2_HAS_XESS_FG)
			ProbeXessFgDevice(report.xessFg, selected.adapter.Get());
#endif
		}
		if (report.xessFg.feature.compiled && !report.xessFg.feature.runtime &&
			report.xessFg.feature.reason.empty()) {
			report.xessFg.feature.reason = "XeSS-FG support was not detected on the D3D12 device";
		}
		return report;
	}

	std::string CapabilitiesToJson(const CapabilityReport& report)
	{
		nlohmann::json adapters = nlohmann::json::array();
		for (const auto& adapter : report.xessFg.adapters) {
			adapters.push_back({
				{ "name", adapter.name },
				{ "graphics", adapter.graphics },
				{ "integrated", adapter.integrated },
				{ "selected", adapter.selected },
				{ "igclFrameGeneration", adapter.igclFrameGeneration },
				{ "igclFrameGenerationControl", adapter.igclFrameGenerationControl },
				{ "pciVendorId", adapter.pciVendorId },
				{ "pciDeviceId", adapter.pciDeviceId },
			});
		}

		nlohmann::json j{
			{ "xessFg", {
				{ "compiled", report.xessFg.feature.compiled },
				{ "runtime", report.xessFg.feature.runtime },
				{ "reason", report.xessFg.feature.reason },
				{ "igclReason", report.xessFg.igclReason },
				{ "adapters", adapters },
			}},
		};
		return j.dump();
	}
}
