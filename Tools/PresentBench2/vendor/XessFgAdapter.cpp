// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "XessFgAdapter.h"
#include "../../../IntelPresentMon/CommonUtilities/log/Log.h"
#include <xess_fg/xefg_swapchain.h>
#include <xess_fg/xefg_swapchain_d3d12.h>
#include <xell/xell.h>
#include <xell/xell_d3d12.h>
#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace pb2
{
	namespace
	{
		const char* ToString(xell_result_t result)
		{
			switch (result) {
			case XELL_RESULT_SUCCESS: return "SUCCESS";
			case XELL_RESULT_ERROR_UNSUPPORTED_DEVICE: return "ERROR_UNSUPPORTED_DEVICE";
			case XELL_RESULT_ERROR_UNSUPPORTED_DRIVER: return "ERROR_UNSUPPORTED_DRIVER";
			case XELL_RESULT_ERROR_UNINITIALIZED: return "ERROR_UNINITIALIZED";
			case XELL_RESULT_ERROR_INVALID_ARGUMENT: return "ERROR_INVALID_ARGUMENT";
			case XELL_RESULT_ERROR_DEVICE: return "ERROR_DEVICE";
			case XELL_RESULT_ERROR_NOT_IMPLEMENTED: return "ERROR_NOT_IMPLEMENTED";
			case XELL_RESULT_ERROR_INVALID_CONTEXT: return "ERROR_INVALID_CONTEXT";
			case XELL_RESULT_ERROR_UNSUPPORTED: return "ERROR_UNSUPPORTED";
			case XELL_RESULT_ERROR_UNKNOWN: return "ERROR_UNKNOWN";
			}
			return "UNKNOWN";
		}

		const char* ToString(xefg_swapchain_result_t result)
		{
			switch (result) {
			case XEFG_SWAPCHAIN_RESULT_SUCCESS: return "SUCCESS";
			case XEFG_SWAPCHAIN_RESULT_WARNING_OLD_DRIVER: return "WARNING_OLD_DRIVER";
			case XEFG_SWAPCHAIN_RESULT_WARNING_TOO_FEW_FRAMES: return "WARNING_TOO_FEW_FRAMES";
			case XEFG_SWAPCHAIN_RESULT_WARNING_FRAMES_ID_MISMATCH:
				return "WARNING_FRAMES_ID_MISMATCH";
			case XEFG_SWAPCHAIN_RESULT_WARNING_MISSING_PRESENT_STATUS:
				return "WARNING_MISSING_PRESENT_STATUS";
			case XEFG_SWAPCHAIN_RESULT_WARNING_RESOURCE_SIZES_MISMATCH:
				return "WARNING_RESOURCE_SIZES_MISMATCH";
			case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DEVICE: return "ERROR_UNSUPPORTED_DEVICE";
			case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DRIVER: return "ERROR_UNSUPPORTED_DRIVER";
			case XEFG_SWAPCHAIN_RESULT_ERROR_UNINITIALIZED: return "ERROR_UNINITIALIZED";
			case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_ARGUMENT: return "ERROR_INVALID_ARGUMENT";
			case XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE_OUT_OF_MEMORY:
				return "ERROR_DEVICE_OUT_OF_MEMORY";
			case XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE: return "ERROR_DEVICE";
			case XEFG_SWAPCHAIN_RESULT_ERROR_NOT_IMPLEMENTED: return "ERROR_NOT_IMPLEMENTED";
			case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_CONTEXT: return "ERROR_INVALID_CONTEXT";
			case XEFG_SWAPCHAIN_RESULT_ERROR_OPERATION_IN_PROGRESS:
				return "ERROR_OPERATION_IN_PROGRESS";
			case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED: return "ERROR_UNSUPPORTED";
			case XEFG_SWAPCHAIN_RESULT_ERROR_CANT_LOAD_LIBRARY:
				return "ERROR_CANT_LOAD_LIBRARY";
			case XEFG_SWAPCHAIN_RESULT_ERROR_MISMATCH_INPUT_RESOURCES:
				return "ERROR_MISMATCH_INPUT_RESOURCES";
			case XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_OUTPUT_RESOURCES:
				return "ERROR_INCORRECT_OUTPUT_RESOURCES";
			case XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_INPUT_RESOURCES:
				return "ERROR_INCORRECT_INPUT_RESOURCES";
			case XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_UNSUPPORTED:
				return "ERROR_LATENCY_REDUCTION_UNSUPPORTED";
			case XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_FUNCTION_MISSING:
				return "ERROR_LATENCY_REDUCTION_FUNCTION_MISSING";
			case XEFG_SWAPCHAIN_RESULT_ERROR_HRESULT_FAILURE: return "ERROR_HRESULT_FAILURE";
			case XEFG_SWAPCHAIN_RESULT_ERROR_DXGI_INVALID_CALL: return "ERROR_DXGI_INVALID_CALL";
			case XEFG_SWAPCHAIN_RESULT_ERROR_POINTER_STILL_IN_USE:
				return "ERROR_POINTER_STILL_IN_USE";
			case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_DESCRIPTOR_HEAP:
				return "ERROR_INVALID_DESCRIPTOR_HEAP";
			case XEFG_SWAPCHAIN_RESULT_ERROR_WRONG_CALL_ORDER:
				return "ERROR_WRONG_CALL_ORDER";
			case XEFG_SWAPCHAIN_RESULT_ERROR_UNKNOWN: return "ERROR_UNKNOWN";
			}
			return "UNKNOWN";
		}

		std::string Describe(std::string_view operation, xell_result_t result)
		{
			return std::format("{}: XeLL {} ({})", operation, ToString(result), (int)result);
		}

		std::string Describe(std::string_view operation, xefg_swapchain_result_t result)
		{
			return std::format("{}: XeFG {} ({})", operation, ToString(result), (int)result);
		}

		void LogXefgWarning(std::string_view operation, xefg_swapchain_result_t result)
		{
			const auto message = Describe(operation, result);
			switch (result) {
			case XEFG_SWAPCHAIN_RESULT_WARNING_TOO_FEW_FRAMES:
			case XEFG_SWAPCHAIN_RESULT_WARNING_MISSING_PRESENT_STATUS:
				pmlog_dbg(message);
				break;
			default:
				pmlog_warn(message);
				break;
			}
		}

		void XellLoggingCallback(const char* message, xell_logging_level_t level)
		{
			const std::string text = std::format("XeLL: {}", message ? message : "");
			switch (level) {
			case XELL_LOGGING_LEVEL_DEBUG: pmlog_dbg(text); break;
			case XELL_LOGGING_LEVEL_INFO: pmlog_info(text); break;
			case XELL_LOGGING_LEVEL_WARNING: pmlog_warn(text); break;
			case XELL_LOGGING_LEVEL_ERROR: pmlog_error(text); break;
			}
		}

		void XefgLoggingCallback(const char* message, xefg_swapchain_logging_level_t level,
			void*)
		{
			const std::string text = std::format("XeFG: {}", message ? message : "");
			switch (level) {
			case XEFG_SWAPCHAIN_LOGGING_LEVEL_DEBUG: pmlog_dbg(text); break;
			case XEFG_SWAPCHAIN_LOGGING_LEVEL_INFO: pmlog_info(text); break;
			case XEFG_SWAPCHAIN_LOGGING_LEVEL_WARNING: pmlog_warn(text); break;
			case XEFG_SWAPCHAIN_LOGGING_LEVEL_ERROR: pmlog_error(text); break;
			}
		}

		class XessFgPresentComponent final : public IDx12PresentComponent,
			public IFrameMarkerSink
		{
		public:
			explicit XessFgPresentComponent(uint32_t interpolatedFrames)
				:
				interpolatedFrames_(interpolatedFrames)
			{}

			~XessFgPresentComponent() override
			{
				Shutdown_();
			}

			void CreateSwapChain(const Dx12PresentCreateInfo& info,
				IDXGISwapChain3** swapChain) override
			{
				if (info.swapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_DISCARD &&
					info.swapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL) {
					throw std::runtime_error("XeSS-FG requires a flip-model swap effect");
				}

				CheckXell_(xellD3D12CreateContext(info.device, &xellContext_),
					"xellD3D12CreateContext");
				CheckXell_(xellSetLoggingCallback(xellContext_, XELL_LOGGING_LEVEL_INFO,
					XellLoggingCallback), "xellSetLoggingCallback");
				CheckXefg_(xefgSwapChainD3D12CreateContext(info.device, &xefgContext_),
					"xefgSwapChainD3D12CreateContext");
				CheckXefg_(xefgSwapChainSetLoggingCallback(xefgContext_,
					XEFG_SWAPCHAIN_LOGGING_LEVEL_INFO, XefgLoggingCallback, this),
					"xefgSwapChainSetLoggingCallback");
				CheckXefg_(xefgSwapChainSetLatencyReduction(xefgContext_, xellContext_),
					"xefgSwapChainSetLatencyReduction");

				xefg_swapchain_d3d12_init_params_t params{};
				params.initFlags = XEFG_SWAPCHAIN_INIT_FLAG_USE_NDC_VELOCITY;
				params.maxInterpolatedFrames = interpolatedFrames_;
				params.uiMode = XEFG_SWAPCHAIN_UI_MODE_NONE;
				const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fsDesc =
					info.exclusiveFullscreen ? &info.fullscreenDesc : nullptr;
				CheckXefg_(xefgSwapChainD3D12InitFromSwapChainDesc(xefgContext_, info.hwnd,
					&info.swapChainDesc, fsDesc, info.queue, info.factory, &params),
					"xefgSwapChainD3D12InitFromSwapChainDesc");
				CheckXefg_(xefgSwapChainD3D12GetSwapChainPtr(xefgContext_,
					IID_PPV_ARGS(swapChain)), "xefgSwapChainD3D12GetSwapChainPtr");

				xell_sleep_params_t sleepParams{};
				sleepParams.bLowLatencyMode = 1;
				CheckXell_(xellSetSleepMode(xellContext_, &sleepParams), "xellSetSleepMode");
				CheckXefg_(xefgSwapChainSetNumInterpolatedFrames(xefgContext_,
					interpolatedFrames_), "xefgSwapChainSetNumInterpolatedFrames");
				CheckXefg_(xefgSwapChainSetEnabled(xefgContext_, 1), "xefgSwapChainSetEnabled");

				pmlog_info(std::format(
					"XeFG initialized with {} interpolated frame(s)", interpolatedFrames_));
			}

			void BeginFrame(uint32_t frameId) override
			{
				CheckXell_(xellSleep(xellContext_, frameId), "xellSleep");
			}

			void Mark(FrameMarker marker, uint32_t frameId) override
			{
				xell_latency_marker_type_t type{};
				switch (marker) {
				case FrameMarker::SimulationStart: type = XELL_SIMULATION_START; break;
				case FrameMarker::SimulationEnd: type = XELL_SIMULATION_END; break;
				case FrameMarker::RenderSubmitStart: type = XELL_RENDERSUBMIT_START; break;
				case FrameMarker::RenderSubmitEnd: type = XELL_RENDERSUBMIT_END; break;
				case FrameMarker::PresentStart: type = XELL_PRESENT_START; break;
				case FrameMarker::PresentEnd: type = XELL_PRESENT_END; break;
				default: return;
				}
				CheckXell_(xellAddMarkerData(xellContext_, frameId, type), "xellAddMarkerData");
			}

			void PreparePresent(const Dx12FrameGenerationInputs& inputs) override
			{
				xefg_swapchain_d3d12_resource_data_t velocity{};
				velocity.type = XEFG_SWAPCHAIN_RES_MOTION_VECTOR;
				velocity.validity = XEFG_SWAPCHAIN_RV_UNTIL_NEXT_PRESENT;
				velocity.resourceSize = { inputs.width, inputs.height };
				velocity.pResource = inputs.motionVectors;
				velocity.incomingState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
				CheckXefg_(xefgSwapChainD3D12TagFrameResource(xefgContext_, nullptr,
					inputs.presentId, &velocity), "xefgSwapChainD3D12TagFrameResource(MV)");

				xefg_swapchain_d3d12_resource_data_t depth{};
				depth.type = XEFG_SWAPCHAIN_RES_DEPTH;
				depth.validity = XEFG_SWAPCHAIN_RV_UNTIL_NEXT_PRESENT;
				depth.resourceSize = { inputs.width, inputs.height };
				depth.pResource = inputs.depth;
				depth.incomingState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
				CheckXefg_(xefgSwapChainD3D12TagFrameResource(xefgContext_, nullptr,
					inputs.presentId, &depth), "xefgSwapChainD3D12TagFrameResource(DEPTH)");

				xefg_swapchain_frame_constant_data_t constants{};
				std::copy_n(inputs.viewMatrix, 16, constants.viewMatrix);
				std::copy_n(inputs.projectionMatrix, 16, constants.projectionMatrix);
				constants.motionVectorScaleX = 1.f;
				constants.motionVectorScaleY = 1.f;
				constants.resetHistory = resetHistory_ ? 1u : 0u;
				constants.frameRenderTime = havePreviousFrame_ ?
					std::max(0.f, (inputs.sceneTimeSec - previousSceneTimeSec_) * 1000.f) : 0.f;
				CheckXefg_(xefgSwapChainTagFrameConstants(xefgContext_, inputs.presentId,
					&constants), "xefgSwapChainTagFrameConstants");
				CheckXefg_(xefgSwapChainSetPresentId(xefgContext_, inputs.presentId),
					"xefgSwapChainSetPresentId");
				pendingSceneTimeSec_ = inputs.sceneTimeSec;
			}

			void CompletePresent(uint32_t) override
			{
				xefg_swapchain_present_status_t sdkStatus{};
				const auto queryResult =
					xefgSwapChainGetLastPresentStatus(xefgContext_, &sdkStatus);
				CheckPresentResult_(queryResult, "xefgSwapChainGetLastPresentStatus",
					lastQueryResult_);
				CheckPresentResult_(sdkStatus.frameGenResult, "XeFG frame generation",
					lastFrameGenerationResult_);

				status_.framesPresented = std::max(1u, sdkStatus.framesPresented);
				status_.frameGenerationEnabled = sdkStatus.isFrameGenEnabled != 0;
				previousSceneTimeSec_ = pendingSceneTimeSec_;
				havePreviousFrame_ = true;
				resetHistory_ = false;
			}

			PresentResult GetLastPresentResult() const override { return status_; }
			IFrameMarkerSink* GetMarkerSink() override { return this; }

		private:
			void Shutdown_()
			{
				if (xefgContext_) {
					const auto result = xefgSwapChainDestroy(xefgContext_);
					if (result != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
						if ((int)result > 0) {
							LogXefgWarning("xefgSwapChainDestroy", result);
						}
						else {
							pmlog_error(Describe("xefgSwapChainDestroy", result));
						}
					}
					xefgContext_ = nullptr;
				}
				if (xellContext_) {
					const auto result = xellDestroyContext(xellContext_);
					if (result != XELL_RESULT_SUCCESS) {
						pmlog_error(Describe("xellDestroyContext", result));
					}
					xellContext_ = nullptr;
				}
			}
			void CheckXell_(xell_result_t result, std::string_view operation)
			{
				if (result == XELL_RESULT_SUCCESS) {
					return;
				}
				const auto error = Describe(operation, result);
				pmlog_error(error);
				throw std::runtime_error(error);
			}

			void CheckXefg_(xefg_swapchain_result_t result, std::string_view operation)
			{
				if (result == XEFG_SWAPCHAIN_RESULT_SUCCESS) {
					return;
				}
				if ((int)result > 0) {
					LogXefgWarning(operation, result);
					return;
				}
				const auto error = Describe(operation, result);
				pmlog_error(error);
				throw std::runtime_error(error);
			}

			void CheckPresentResult_(xefg_swapchain_result_t result,
				std::string_view operation, std::optional<xefg_swapchain_result_t>& previous)
			{
				if (previous && *previous == result) {
					return;
				}
				previous = result;
				if (result == XEFG_SWAPCHAIN_RESULT_SUCCESS) {
					return;
				}
				CheckXefg_(result, operation);
			}

			const uint32_t interpolatedFrames_;
			xefg_swapchain_handle_t xefgContext_ = nullptr;
			xell_context_handle_t xellContext_ = nullptr;
			PresentResult status_{};
			bool resetHistory_ = true;
			bool havePreviousFrame_ = false;
			float previousSceneTimeSec_ = 0.f;
			float pendingSceneTimeSec_ = 0.f;
			std::optional<xefg_swapchain_result_t> lastQueryResult_;
			std::optional<xefg_swapchain_result_t> lastFrameGenerationResult_;
		};
	}

	std::unique_ptr<IDx12PresentComponent> CreateXessFgPresentComponent(
		uint32_t interpolatedFrames)
	{
		return std::make_unique<XessFgPresentComponent>(interpolatedFrames);
	}
}
