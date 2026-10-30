// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../../../IntelPresentMon/CommonUtilities/win/WinAPI.h"
#include "../Backend.h"
#include "../CubeScene.h"
#include "../CubeShadersSpirv.h"
#include "DxgiCommon.h"
#include "../../../IntelPresentMon/CommonUtilities/log/HrLogger.h"
#include "../../../IntelPresentMon/CommonUtilities/log/Log.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <dxgi1_6.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace pb2
{
	namespace
	{
		void VkCheck(VkResult result, const char* what)
		{
			if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				throw std::runtime_error(std::format("{} failed: {}", what, (int)result));
			}
		}

		void ConfigureLocalValidationLayerPath()
		{
			std::vector<wchar_t> modulePathBuffer(32768);
			const DWORD modulePathLength = GetModuleFileNameW(nullptr,
				modulePathBuffer.data(), (DWORD)modulePathBuffer.size());
			if (modulePathLength == 0 ||
				modulePathLength >= (DWORD)modulePathBuffer.size()) {
				pmlog_warn("Unable to resolve the PresentBench2 executable path for "
					"local Vulkan validation layers");
				return;
			}

			const std::filesystem::path modulePath{
				std::wstring(modulePathBuffer.data(), modulePathLength)
			};
			const auto layerPath = modulePath.parent_path() / L"vulkan-layers";
			if (!std::filesystem::exists(
				layerPath / L"VkLayer_khronos_validation.json") ||
				!std::filesystem::exists(
					layerPath / L"VkLayer_khronos_validation.dll")) {
				return;
			}

			std::wstring combinedPath = layerPath.native();
			const DWORD existingSize = GetEnvironmentVariableW(
				L"VK_ADD_LAYER_PATH", nullptr, 0);
			if (existingSize > 0) {
				std::wstring existingPath(existingSize, L'\0');
				const DWORD existingLength = GetEnvironmentVariableW(
					L"VK_ADD_LAYER_PATH", existingPath.data(), existingSize);
				if (existingLength > 0 && existingLength < existingSize) {
					existingPath.resize(existingLength);
					combinedPath += L";";
					combinedPath += existingPath;
				}
			}
			if (!SetEnvironmentVariableW(L"VK_ADD_LAYER_PATH", combinedPath.c_str())) {
				throw std::runtime_error(std::format(
					"SetEnvironmentVariableW(VK_ADD_LAYER_PATH) failed: {}",
					GetLastError()));
			}
			pmlog_info(std::format(
				"Using build-local Vulkan validation layer from {}",
				layerPath.string()));
		}

		VkPresentModeKHR ToVkPresentMode(VkPresentMode mode)
		{
			switch (mode) {
			case VkPresentMode::ImmediateKhr: return VK_PRESENT_MODE_IMMEDIATE_KHR;
			case VkPresentMode::MailboxKhr: return VK_PRESENT_MODE_MAILBOX_KHR;
			case VkPresentMode::FifoKhr: return VK_PRESENT_MODE_FIFO_KHR;
			case VkPresentMode::FifoRelaxedKhr: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
			case VkPresentMode::Count: break;
			}
			return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}

		class VulkanBackend final : public IRenderBackend
		{
		public:
			VulkanBackend(HWND hwnd, const BenchConfig& config, uint32_t width, uint32_t height)
				:
				hwnd_(hwnd),
				config_(config),
				width_(width),
				height_(height)
			{
				try {
				hinstance_ = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
				ComPtr<IDXGIFactory1> factory;
				pmlog_hr << CreateDXGIFactory1(IID_PPV_ARGS(&factory));
				adapterSelection_ = ResolveAdapter(config.adapter, factory.Get());

				VkApplicationInfo appInfo{};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = "PresentBench2";
				appInfo.apiVersion = VK_API_VERSION_1_1;
				const char* extensions[] = {
					VK_KHR_SURFACE_EXTENSION_NAME,
					VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
				};
				VkInstanceCreateInfo ici{};
				ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				ici.pApplicationInfo = &appInfo;
				ici.enabledExtensionCount = 2;
				ici.ppEnabledExtensionNames = extensions;
				const char* validationLayer = "VK_LAYER_KHRONOS_validation";
				if (config.enableDebugLayer) {
					ConfigureLocalValidationLayerPath();
					ici.enabledLayerCount = 1;
					ici.ppEnabledLayerNames = &validationLayer;
				}
				VkCheck(vkCreateInstance(&ici, nullptr, &instance_), "vkCreateInstance");

				VkWin32SurfaceCreateInfoKHR sci{};
				sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
				sci.hinstance = hinstance_;
				sci.hwnd = hwnd_;
				VkCheck(vkCreateWin32SurfaceKHR(instance_, &sci, nullptr, &surface_),
					"vkCreateWin32SurfaceKHR");

				SelectPhysicalDevice_();
				CreateDevice_();
				CreateSceneResources_();
				CreateFrameSlots_();
				CreateSwapchain_();
				}
				catch (...) {
					Cleanup_();
					throw;
				}
			}

			~VulkanBackend() override
			{
				Cleanup_();
			}

		private:
			void Cleanup_()
			{
				if (device_) {
					const auto result = vkDeviceWaitIdle(device_);
					if (result != VK_SUCCESS) {
						pmlog_error(std::format(
							"vkDeviceWaitIdle during Vulkan cleanup failed: {}",
							(int)result));
					}
				}
				DestroySwapchainDependent_();
				DestroyFrameSlots_();
				DestroySceneResources_();
				if (cmdPool_) {
					vkDestroyCommandPool(device_, cmdPool_, nullptr);
					cmdPool_ = VK_NULL_HANDLE;
				}
				if (device_) {
					vkDestroyDevice(device_, nullptr);
				}
				if (surface_) {
					vkDestroySurfaceKHR(instance_, surface_, nullptr);
				}
				if (instance_) {
					vkDestroyInstance(instance_, nullptr);
					instance_ = VK_NULL_HANDLE;
				}
			}

		public:
			void WaitForFrameSlot() override
			{
				auto& slot = slots_[slotIndex_];
				VkCheck(vkWaitForFences(device_, 1, &slot.inFlightFence, VK_TRUE, UINT64_MAX),
					"vkWaitForFences");
				VkCheck(vkResetFences(device_, 1, &slot.inFlightFence), "vkResetFences");
			}

			void Render(const FrameInfo& info) override
			{
				auto& slot = slots_[slotIndex_];
				uint32_t imageIndex = 0;
				const VkResult acq = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
					slot.imageAvailable, VK_NULL_HANDLE, &imageIndex);
				if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
					throw std::runtime_error("vkAcquireNextImageKHR: swapchain out of date");
				}
				VkCheck(acq, "vkAcquireNextImageKHR");
				pendingImageIndex_ = imageIndex;

				VkCheck(vkResetCommandBuffer(slot.cmd, 0), "vkResetCommandBuffer");
				VkCommandBufferBeginInfo bi{};
				bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				VkCheck(vkBeginCommandBuffer(slot.cmd, &bi), "vkBeginCommandBuffer");
				UpdateUniforms_(info, slot);

				VkClearValue clears[2]{};
				clears[0].color.float32[0] = kClearR;
				clears[0].color.float32[1] = kClearG;
				clears[0].color.float32[2] = kClearB;
				clears[0].color.float32[3] = 1.f;
				clears[1].depthStencil.depth = 1.f;
				for (uint32_t i = 0; i < info.clears; ++i) {
					VkRenderPassBeginInfo rpbi{};
					rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					rpbi.renderPass = renderPass_;
					rpbi.framebuffer = framebuffers_[
						slotIndex_ * images_.size() + imageIndex];
					rpbi.renderArea.extent = extent_;
					rpbi.clearValueCount = 2;
					rpbi.pClearValues = clears;
					vkCmdBeginRenderPass(slot.cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
					if (i + 1 == info.clears) {
						vkCmdBindPipeline(slot.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
						VkViewport vp{};
						vp.width = (float)extent_.width;
						vp.height = (float)extent_.height;
						vp.maxDepth = 1.f;
						vkCmdSetViewport(slot.cmd, 0, 1, &vp);
						VkRect2D scissor{};
						scissor.extent = extent_;
						vkCmdSetScissor(slot.cmd, 0, 1, &scissor);
						vkCmdBindDescriptorSets(slot.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipelineLayout_, 0, 1, &slot.descriptorSet, 0, nullptr);
						VkDeviceSize offset = 0;
						vkCmdBindVertexBuffers(slot.cmd, 0, 1, &vertexBuffer_, &offset);
						vkCmdBindIndexBuffer(slot.cmd, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
						vkCmdDrawIndexed(slot.cmd, kCubeIndexCount, 1, 0, 0, 0);
					}
					vkCmdEndRenderPass(slot.cmd);
				}
				VkCheck(vkEndCommandBuffer(slot.cmd), "vkEndCommandBuffer");
				VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				VkSubmitInfo si{};
				si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				si.waitSemaphoreCount = 1;
				si.pWaitSemaphores = &slot.imageAvailable;
				si.pWaitDstStageMask = &waitStage;
				si.commandBufferCount = 1;
				si.pCommandBuffers = &slot.cmd;
				si.signalSemaphoreCount = 1;
				si.pSignalSemaphores = &renderFinished_[pendingImageIndex_];
				VkCheck(vkQueueSubmit(queue_, 1, &si, slot.inFlightFence), "vkQueueSubmit");
			}

			PresentResult Present(const FrameInfo&) override
			{
				VkPresentInfoKHR pi{};
				pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				pi.waitSemaphoreCount = 1;
				pi.pWaitSemaphores = &renderFinished_[pendingImageIndex_];
				pi.swapchainCount = 1;
				pi.pSwapchains = &swapchain_;
				pi.pImageIndices = &pendingImageIndex_;
				const VkResult pr = vkQueuePresentKHR(queue_, &pi);
				if (pr == VK_ERROR_OUT_OF_DATE_KHR) {
					throw std::runtime_error("vkQueuePresentKHR: swapchain out of date");
				}
				VkCheck(pr, "vkQueuePresentKHR");
				slotIndex_ = (slotIndex_ + 1) % slots_.size();
				return {};
			}

		private:
			struct FrameSlot
			{
				VkCommandBuffer cmd = VK_NULL_HANDLE;
				VkSemaphore imageAvailable = VK_NULL_HANDLE;
				VkFence inFlightFence = VK_NULL_HANDLE;
				VkBuffer uniformBuffer = VK_NULL_HANDLE;
				VkDeviceMemory uniformMemory = VK_NULL_HANDLE;
				VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
				VkImage depthImage = VK_NULL_HANDLE;
				VkDeviceMemory depthMemory = VK_NULL_HANDLE;
				VkImageView depthView = VK_NULL_HANDLE;
			};

			void SelectPhysicalDevice_()
			{
				uint32_t physCount = 0;
				VkCheck(vkEnumeratePhysicalDevices(instance_, &physCount, nullptr),
					"vkEnumeratePhysicalDevices(count)");
				if (physCount == 0) {
					throw std::runtime_error("No Vulkan physical devices");
				}
				std::vector<VkPhysicalDevice> phys(physCount);
				VkCheck(vkEnumeratePhysicalDevices(instance_, &physCount, phys.data()),
					"vkEnumeratePhysicalDevices(devices)");

				for (auto device : phys) {
					VkPhysicalDeviceIDProperties idProps{};
					idProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
					VkPhysicalDeviceProperties2 props2{};
					props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
					props2.pNext = &idProps;
					vkGetPhysicalDeviceProperties2(device, &props2);
					LUID luid{};
					std::memcpy(&luid, idProps.deviceLUID, sizeof(luid));
					if (idProps.deviceLUIDValid &&
						luid.LowPart == adapterSelection_.luid.LowPart &&
						luid.HighPart == adapterSelection_.luid.HighPart) {
						physical_ = device;
						pmlog_info(std::format("Using Vulkan device matching DXGI adapter {}: {}",
							adapterSelection_.index,
							props2.properties.deviceName));
						break;
					}
				}
				if (!physical_) {
					throw std::runtime_error(
						"No Vulkan physical device matches the selected DXGI adapter LUID");
				}

				uint32_t queueCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(physical_, &queueCount, nullptr);
				std::vector<VkQueueFamilyProperties> queues(queueCount);
				vkGetPhysicalDeviceQueueFamilyProperties(physical_, &queueCount, queues.data());
				graphicsFamily_ = UINT32_MAX;
				for (uint32_t i = 0; i < queueCount; ++i) {
					VkBool32 presentSupport = VK_FALSE;
					VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(
						physical_, i, surface_, &presentSupport),
						"vkGetPhysicalDeviceSurfaceSupportKHR");
					if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
						graphicsFamily_ = i;
						break;
					}
				}
				if (graphicsFamily_ == UINT32_MAX) {
					throw std::runtime_error("No graphics+present queue family");
				}
			}

			void CreateDevice_()
			{
				float priority = 1.f;
				VkDeviceQueueCreateInfo qci{};
				qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				qci.queueFamilyIndex = graphicsFamily_;
				qci.queueCount = 1;
				qci.pQueuePriorities = &priority;
				const char* deviceExt[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
				VkDeviceCreateInfo dci{};
				dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				dci.queueCreateInfoCount = 1;
				dci.pQueueCreateInfos = &qci;
				dci.enabledExtensionCount = 1;
				dci.ppEnabledExtensionNames = deviceExt;
				VkCheck(vkCreateDevice(physical_, &dci, nullptr, &device_), "vkCreateDevice");
				vkGetDeviceQueue(device_, graphicsFamily_, 0, &queue_);

				VkCommandPoolCreateInfo pci{};
				pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				pci.queueFamilyIndex = graphicsFamily_;
				pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				VkCheck(vkCreateCommandPool(device_, &pci, nullptr, &cmdPool_),
					"vkCreateCommandPool");
			}

			uint32_t FindMemoryType_(uint32_t typeBits, VkMemoryPropertyFlags props) const
			{
				VkPhysicalDeviceMemoryProperties memProps{};
				vkGetPhysicalDeviceMemoryProperties(physical_, &memProps);
				for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
					if ((typeBits & (1u << i)) &&
						(memProps.memoryTypes[i].propertyFlags & props) == props) {
						return i;
					}
				}
				throw std::runtime_error("FindMemoryType failed");
			}

			void CreateBuffer_(VkDeviceSize size, VkBufferUsageFlags usage,
				VkMemoryPropertyFlags memProps, VkBuffer& buffer, VkDeviceMemory& memory)
			{
				VkBufferCreateInfo bi{};
				bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bi.size = size;
				bi.usage = usage;
				bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				VkCheck(vkCreateBuffer(device_, &bi, nullptr, &buffer), "vkCreateBuffer");
				VkMemoryRequirements req{};
				vkGetBufferMemoryRequirements(device_, buffer, &req);
				VkMemoryAllocateInfo ai{};
				ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				ai.allocationSize = req.size;
				ai.memoryTypeIndex = FindMemoryType_(req.memoryTypeBits, memProps);
				VkCheck(vkAllocateMemory(device_, &ai, nullptr, &memory), "vkAllocateMemory");
				VkCheck(vkBindBufferMemory(device_, buffer, memory, 0),
					"vkBindBufferMemory");
			}

			void CreateSwapchain_()
			{
				VkSurfaceCapabilitiesKHR caps{};
				VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_, surface_, &caps),
					"vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
				uint32_t formatCount = 0;
				VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
					physical_, surface_, &formatCount, nullptr),
					"vkGetPhysicalDeviceSurfaceFormatsKHR(count)");
				if (formatCount == 0) {
					throw std::runtime_error("Vulkan surface reported no formats");
				}
				std::vector<VkSurfaceFormatKHR> formats(formatCount);
				VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
					physical_, surface_, &formatCount, formats.data()),
					"vkGetPhysicalDeviceSurfaceFormatsKHR(formats)");
				VkSurfaceFormatKHR format = formats[0];
				for (const auto& f : formats) {
					if (f.format == VK_FORMAT_B8G8R8A8_UNORM) {
						format = f;
						break;
					}
				}
				swapFormat_ = format.format;
				depthFormat_ = VK_FORMAT_D32_SFLOAT;

				uint32_t modeCount = 0;
				VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
					physical_, surface_, &modeCount, nullptr),
					"vkGetPhysicalDeviceSurfacePresentModesKHR(count)");
				std::vector<VkPresentModeKHR> modes(modeCount);
				VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
					physical_, surface_, &modeCount, modes.data()),
					"vkGetPhysicalDeviceSurfacePresentModesKHR(modes)");
				const VkPresentModeKHR desired = ToVkPresentMode(config_.swap.vkMode);
				if (std::find(modes.begin(), modes.end(), desired) == modes.end()) {
					throw std::runtime_error("Requested Vulkan present mode is not supported");
				}

				extent_ = { width_, height_ };
				if (caps.currentExtent.width != UINT32_MAX) {
					extent_ = caps.currentExtent;
				}
				else {
					extent_.width = (std::max)(caps.minImageExtent.width,
						(std::min)(caps.maxImageExtent.width, extent_.width));
					extent_.height = (std::max)(caps.minImageExtent.height,
						(std::min)(caps.maxImageExtent.height, extent_.height));
				}

				VkSwapchainCreateInfoKHR ci{};
				ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				ci.surface = surface_;
				ci.minImageCount = (std::max)(config_.swap.bufferCount, caps.minImageCount);
				if (caps.maxImageCount > 0 && ci.minImageCount > caps.maxImageCount) {
					ci.minImageCount = caps.maxImageCount;
				}
				ci.imageFormat = format.format;
				ci.imageColorSpace = format.colorSpace;
				ci.imageExtent = extent_;
				ci.imageArrayLayers = 1;
				ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				ci.preTransform = caps.currentTransform;
				ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
				ci.presentMode = desired;
				ci.clipped = VK_TRUE;
				VkCheck(vkCreateSwapchainKHR(device_, &ci, nullptr, &swapchain_),
					"vkCreateSwapchainKHR");
				uint32_t imageCount = 0;
				VkCheck(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr),
					"vkGetSwapchainImagesKHR(count)");
				if (imageCount == 0) {
					throw std::runtime_error("Vulkan swapchain returned no images");
				}
				images_.resize(imageCount);
				VkCheck(vkGetSwapchainImagesKHR(
					device_, swapchain_, &imageCount, images_.data()),
					"vkGetSwapchainImagesKHR(images)");
				CreateSwapchainDependent_();
			}

			void CreateSwapchainDependent_()
			{
				CreateRenderPass_();
				for (auto& slot : slots_) {
					CreateDepthImage_(slot);
				}
				imageViews_.resize(images_.size());
				framebuffers_.resize(images_.size() * slots_.size());
				renderFinished_.resize(images_.size());
				for (size_t i = 0; i < images_.size(); ++i) {
					VkImageViewCreateInfo vci{};
					vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					vci.image = images_[i];
					vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
					vci.format = swapFormat_;
					vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					vci.subresourceRange.levelCount = 1;
					vci.subresourceRange.layerCount = 1;
					VkCheck(vkCreateImageView(device_, &vci, nullptr, &imageViews_[i]),
						"vkCreateImageView");
					for (size_t slotIndex = 0; slotIndex < slots_.size(); ++slotIndex) {
						VkImageView attachments[] = {
							imageViews_[i], slots_[slotIndex].depthView
						};
						VkFramebufferCreateInfo fbci{};
						fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
						fbci.renderPass = renderPass_;
						fbci.attachmentCount = 2;
						fbci.pAttachments = attachments;
						fbci.width = extent_.width;
						fbci.height = extent_.height;
						fbci.layers = 1;
						VkCheck(vkCreateFramebuffer(device_, &fbci, nullptr,
							&framebuffers_[slotIndex * images_.size() + i]),
							"vkCreateFramebuffer");
					}
					VkSemaphoreCreateInfo sei{};
					sei.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
					VkCheck(vkCreateSemaphore(device_, &sei, nullptr, &renderFinished_[i]),
						"vkCreateSemaphore");
				}
				CreatePipeline_();
			}

			void CreateRenderPass_()
			{
				VkAttachmentDescription color{};
				color.format = swapFormat_;
				color.samples = VK_SAMPLE_COUNT_1_BIT;
				color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				VkAttachmentDescription depth{};
				depth.format = depthFormat_;
				depth.samples = VK_SAMPLE_COUNT_1_BIT;
				depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

				VkAttachmentReference colorRef{};
				colorRef.attachment = 0;
				colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				VkAttachmentReference depthRef{};
				depthRef.attachment = 1;
				depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

				VkSubpassDescription subpass{};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = 1;
				subpass.pColorAttachments = &colorRef;
				subpass.pDepthStencilAttachment = &depthRef;

				VkSubpassDependency dep{};
				dep.srcSubpass = VK_SUBPASS_EXTERNAL;
				dep.dstSubpass = 0;
				dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

				VkAttachmentDescription attachments[] = { color, depth };
				VkRenderPassCreateInfo rpci{};
				rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				rpci.attachmentCount = 2;
				rpci.pAttachments = attachments;
				rpci.subpassCount = 1;
				rpci.pSubpasses = &subpass;
				rpci.dependencyCount = 1;
				rpci.pDependencies = &dep;
				VkCheck(vkCreateRenderPass(device_, &rpci, nullptr, &renderPass_),
					"vkCreateRenderPass");
			}

			void CreateDepthImage_(FrameSlot& slot)
			{
				VkImageCreateInfo ici{};
				ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				ici.imageType = VK_IMAGE_TYPE_2D;
				ici.format = depthFormat_;
				ici.extent = { extent_.width, extent_.height, 1 };
				ici.mipLevels = 1;
				ici.arrayLayers = 1;
				ici.samples = VK_SAMPLE_COUNT_1_BIT;
				ici.tiling = VK_IMAGE_TILING_OPTIMAL;
				ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				VkCheck(vkCreateImage(device_, &ici, nullptr, &slot.depthImage),
					"vkCreateImage(depth)");
				VkMemoryRequirements req{};
				vkGetImageMemoryRequirements(device_, slot.depthImage, &req);
				VkMemoryAllocateInfo ai{};
				ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				ai.allocationSize = req.size;
				ai.memoryTypeIndex = FindMemoryType_(req.memoryTypeBits,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				VkCheck(vkAllocateMemory(device_, &ai, nullptr, &slot.depthMemory),
					"vkAllocateMemory(depth)");
				VkCheck(vkBindImageMemory(device_, slot.depthImage, slot.depthMemory, 0),
					"vkBindImageMemory(depth)");

				VkImageViewCreateInfo vci{};
				vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				vci.image = slot.depthImage;
				vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
				vci.format = depthFormat_;
				vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				vci.subresourceRange.levelCount = 1;
				vci.subresourceRange.layerCount = 1;
				VkCheck(vkCreateImageView(device_, &vci, nullptr, &slot.depthView),
					"vkCreateImageView(depth)");
			}

			void CreatePipeline_()
			{
				VkShaderModuleCreateInfo vci{};
				vci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				vci.codeSize = shaders::kSpirvVertCount * sizeof(uint32_t);
				vci.pCode = shaders::kSpirvVert;
				VkCheck(vkCreateShaderModule(device_, &vci, nullptr, &vertModule_),
					"vkCreateShaderModule(vert)");
				vci.codeSize = shaders::kSpirvFragCount * sizeof(uint32_t);
				vci.pCode = shaders::kSpirvFrag;
				VkCheck(vkCreateShaderModule(device_, &vci, nullptr, &fragModule_),
					"vkCreateShaderModule(frag)");

				VkPipelineShaderStageCreateInfo stages[2]{};
				stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
				stages[0].module = vertModule_;
				stages[0].pName = "main";
				stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				stages[1].module = fragModule_;
				stages[1].pName = "main";

				VkVertexInputBindingDescription binding{};
				binding.binding = 0;
				binding.stride = sizeof(CubeVertex);
				binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				VkVertexInputAttributeDescription attrs[2]{};
				attrs[0].location = 0;
				attrs[0].binding = 0;
				attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
				attrs[0].offset = offsetof(CubeVertex, position);
				attrs[1].location = 1;
				attrs[1].binding = 0;
				attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				attrs[1].offset = offsetof(CubeVertex, normal);

				VkPipelineVertexInputStateCreateInfo vi{};
				vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vi.vertexBindingDescriptionCount = 1;
				vi.pVertexBindingDescriptions = &binding;
				vi.vertexAttributeDescriptionCount = 2;
				vi.pVertexAttributeDescriptions = attrs;

				VkPipelineInputAssemblyStateCreateInfo ia{};
				ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				VkPipelineViewportStateCreateInfo vp{};
				vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				vp.viewportCount = 1;
				vp.scissorCount = 1;
				VkPipelineRasterizationStateCreateInfo rs{};
				rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rs.polygonMode = VK_POLYGON_MODE_FILL;
				rs.cullMode = VK_CULL_MODE_BACK_BIT;
				rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
				rs.lineWidth = 1.f;
				VkPipelineMultisampleStateCreateInfo ms{};
				ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				VkPipelineDepthStencilStateCreateInfo ds{};
				ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				ds.depthTestEnable = VK_TRUE;
				ds.depthWriteEnable = VK_TRUE;
				ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				VkPipelineColorBlendAttachmentState blendAtt{};
				blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
					VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				VkPipelineColorBlendStateCreateInfo blend{};
				blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				blend.attachmentCount = 1;
				blend.pAttachments = &blendAtt;
				VkDynamicState dynStates[] = {
					VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
				};
				VkPipelineDynamicStateCreateInfo dyn{};
				dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dyn.dynamicStateCount = 2;
				dyn.pDynamicStates = dynStates;

				VkGraphicsPipelineCreateInfo gpci{};
				gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				gpci.stageCount = 2;
				gpci.pStages = stages;
				gpci.pVertexInputState = &vi;
				gpci.pInputAssemblyState = &ia;
				gpci.pViewportState = &vp;
				gpci.pRasterizationState = &rs;
				gpci.pMultisampleState = &ms;
				gpci.pDepthStencilState = &ds;
				gpci.pColorBlendState = &blend;
				gpci.pDynamicState = &dyn;
				gpci.layout = pipelineLayout_;
				gpci.renderPass = renderPass_;
				VkCheck(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &gpci, nullptr,
					&pipeline_), "vkCreateGraphicsPipelines");
				vkDestroyShaderModule(device_, fragModule_, nullptr);
				fragModule_ = VK_NULL_HANDLE;
				vkDestroyShaderModule(device_, vertModule_, nullptr);
				vertModule_ = VK_NULL_HANDLE;
			}

			void CreateSceneResources_()
			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = 0;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				VkDescriptorSetLayoutCreateInfo dslci{};
				dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				dslci.bindingCount = 1;
				dslci.pBindings = &binding;
				VkCheck(vkCreateDescriptorSetLayout(device_, &dslci, nullptr, &setLayout_),
					"vkCreateDescriptorSetLayout");
				VkPipelineLayoutCreateInfo plci{};
				plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				plci.setLayoutCount = 1;
				plci.pSetLayouts = &setLayout_;
				VkCheck(vkCreatePipelineLayout(device_, &plci, nullptr, &pipelineLayout_),
					"vkCreatePipelineLayout");

				const auto& verts = CubeVertices();
				const auto& indices = CubeIndices();
				CreateBuffer_(sizeof(CubeVertex) * verts.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					vertexBuffer_, vertexMemory_);
				CreateBuffer_(sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					indexBuffer_, indexMemory_);
				void* mapped = nullptr;
				VkCheck(vkMapMemory(device_, vertexMemory_, 0,
					sizeof(CubeVertex) * verts.size(), 0, &mapped),
					"vkMapMemory(vertex)");
				std::memcpy(mapped, verts.data(), sizeof(CubeVertex) * verts.size());
				vkUnmapMemory(device_, vertexMemory_);
				VkCheck(vkMapMemory(device_, indexMemory_, 0,
					sizeof(uint16_t) * indices.size(), 0, &mapped),
					"vkMapMemory(index)");
				std::memcpy(mapped, indices.data(), sizeof(uint16_t) * indices.size());
				vkUnmapMemory(device_, indexMemory_);
			}

			void CreateFrameSlots_()
			{
				slots_.resize(config_.framesInFlight);
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolSize.descriptorCount = (uint32_t)slots_.size();
				VkDescriptorPoolCreateInfo dpci{};
				dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				dpci.maxSets = (uint32_t)slots_.size();
				dpci.poolSizeCount = 1;
				dpci.pPoolSizes = &poolSize;
				VkCheck(vkCreateDescriptorPool(device_, &dpci, nullptr, &descriptorPool_),
					"vkCreateDescriptorPool");

				for (auto& slot : slots_) {
					VkCommandBufferAllocateInfo cai{};
					cai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					cai.commandPool = cmdPool_;
					cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					cai.commandBufferCount = 1;
					VkCheck(vkAllocateCommandBuffers(device_, &cai, &slot.cmd),
						"vkAllocateCommandBuffers");
					VkSemaphoreCreateInfo sei{};
					sei.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
					VkCheck(vkCreateSemaphore(device_, &sei, nullptr, &slot.imageAvailable),
						"vkCreateSemaphore");
					VkFenceCreateInfo fi{};
					fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
					fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
					VkCheck(vkCreateFence(device_, &fi, nullptr, &slot.inFlightFence),
						"vkCreateFence");

					CreateBuffer_(sizeof(FrameConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						slot.uniformBuffer, slot.uniformMemory);

					VkDescriptorSetAllocateInfo dsai{};
					dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					dsai.descriptorPool = descriptorPool_;
					dsai.descriptorSetCount = 1;
					dsai.pSetLayouts = &setLayout_;
					VkCheck(vkAllocateDescriptorSets(device_, &dsai, &slot.descriptorSet),
						"vkAllocateDescriptorSets");
					VkDescriptorBufferInfo dbi{};
					dbi.buffer = slot.uniformBuffer;
					dbi.range = sizeof(FrameConstants);
					VkWriteDescriptorSet write{};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = slot.descriptorSet;
					write.dstBinding = 0;
					write.descriptorCount = 1;
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write.pBufferInfo = &dbi;
					vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
				}
			}

			void UpdateUniforms_(const FrameInfo& info, FrameSlot& slot)
			{
				const FrameConstants constants = BuildFrameConstants(info.sceneTimeSec,
					extent_.width, extent_.height, true);
				void* mapped = nullptr;
				VkCheck(vkMapMemory(device_, slot.uniformMemory, 0,
					sizeof(constants), 0, &mapped), "vkMapMemory(uniform)");
				std::memcpy(mapped, &constants, sizeof(constants));
				vkUnmapMemory(device_, slot.uniformMemory);
			}

			void DestroyFrameSlots_()
			{
				for (auto& slot : slots_) {
					if (slot.imageAvailable) {
						vkDestroySemaphore(device_, slot.imageAvailable, nullptr);
					}
					if (slot.inFlightFence) {
						vkDestroyFence(device_, slot.inFlightFence, nullptr);
					}
					if (slot.uniformBuffer) {
						vkDestroyBuffer(device_, slot.uniformBuffer, nullptr);
					}
					if (slot.uniformMemory) {
						vkFreeMemory(device_, slot.uniformMemory, nullptr);
					}
				}
				slots_.clear();
				if (descriptorPool_) {
					vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
					descriptorPool_ = VK_NULL_HANDLE;
				}
			}

			void DestroySceneResources_()
			{
				if (pipelineLayout_) {
					vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
					pipelineLayout_ = VK_NULL_HANDLE;
				}
				if (setLayout_) {
					vkDestroyDescriptorSetLayout(device_, setLayout_, nullptr);
					setLayout_ = VK_NULL_HANDLE;
				}
				if (indexBuffer_) {
					vkDestroyBuffer(device_, indexBuffer_, nullptr);
					indexBuffer_ = VK_NULL_HANDLE;
				}
				if (indexMemory_) {
					vkFreeMemory(device_, indexMemory_, nullptr);
					indexMemory_ = VK_NULL_HANDLE;
				}
				if (vertexBuffer_) {
					vkDestroyBuffer(device_, vertexBuffer_, nullptr);
					vertexBuffer_ = VK_NULL_HANDLE;
				}
				if (vertexMemory_) {
					vkFreeMemory(device_, vertexMemory_, nullptr);
					vertexMemory_ = VK_NULL_HANDLE;
				}
				if (fragModule_) {
					vkDestroyShaderModule(device_, fragModule_, nullptr);
					fragModule_ = VK_NULL_HANDLE;
				}
				if (vertModule_) {
					vkDestroyShaderModule(device_, vertModule_, nullptr);
					vertModule_ = VK_NULL_HANDLE;
				}
			}

			void DestroySwapchainDependent_()
			{
				if (pipeline_) {
					vkDestroyPipeline(device_, pipeline_, nullptr);
					pipeline_ = VK_NULL_HANDLE;
				}
				for (auto sem : renderFinished_) {
					if (sem) vkDestroySemaphore(device_, sem, nullptr);
				}
				renderFinished_.clear();
				for (auto fb : framebuffers_) {
					if (fb) vkDestroyFramebuffer(device_, fb, nullptr);
				}
				framebuffers_.clear();
				for (auto view : imageViews_) {
					if (view) vkDestroyImageView(device_, view, nullptr);
				}
				imageViews_.clear();
				for (auto& slot : slots_) {
					if (slot.depthView) {
						vkDestroyImageView(device_, slot.depthView, nullptr);
						slot.depthView = VK_NULL_HANDLE;
					}
					if (slot.depthImage) {
						vkDestroyImage(device_, slot.depthImage, nullptr);
						slot.depthImage = VK_NULL_HANDLE;
					}
					if (slot.depthMemory) {
						vkFreeMemory(device_, slot.depthMemory, nullptr);
						slot.depthMemory = VK_NULL_HANDLE;
					}
				}
				if (renderPass_) {
					vkDestroyRenderPass(device_, renderPass_, nullptr);
					renderPass_ = VK_NULL_HANDLE;
				}
				if (swapchain_) {
					vkDestroySwapchainKHR(device_, swapchain_, nullptr);
					swapchain_ = VK_NULL_HANDLE;
				}
				images_.clear();
			}

			HWND hwnd_ = nullptr;
			HINSTANCE hinstance_ = nullptr;
			BenchConfig config_{};
			uint32_t width_ = 0;
			uint32_t height_ = 0;
			AdapterSelection adapterSelection_{};
			VkInstance instance_ = VK_NULL_HANDLE;
			VkSurfaceKHR surface_ = VK_NULL_HANDLE;
			VkPhysicalDevice physical_ = VK_NULL_HANDLE;
			VkDevice device_ = VK_NULL_HANDLE;
			VkQueue queue_ = VK_NULL_HANDLE;
			uint32_t graphicsFamily_ = 0;
			VkCommandPool cmdPool_ = VK_NULL_HANDLE;
			VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
			VkFormat swapFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
			VkFormat depthFormat_ = VK_FORMAT_D32_SFLOAT;
			VkExtent2D extent_{};
			std::vector<VkImage> images_;
			std::vector<VkImageView> imageViews_;
			std::vector<VkFramebuffer> framebuffers_;
			std::vector<VkSemaphore> renderFinished_;
			VkRenderPass renderPass_ = VK_NULL_HANDLE;
			VkShaderModule vertModule_ = VK_NULL_HANDLE;
			VkShaderModule fragModule_ = VK_NULL_HANDLE;
			VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
			VkPipeline pipeline_ = VK_NULL_HANDLE;
			VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
			VkDeviceMemory vertexMemory_ = VK_NULL_HANDLE;
			VkBuffer indexBuffer_ = VK_NULL_HANDLE;
			VkDeviceMemory indexMemory_ = VK_NULL_HANDLE;
			VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
			std::vector<FrameSlot> slots_;
			size_t slotIndex_ = 0;
			uint32_t pendingImageIndex_ = 0;
		};
	}

	std::unique_ptr<IRenderBackend> CreateVulkanBackend(HWND hwnd, const BenchConfig& config,
		uint32_t width, uint32_t height)
	{
		return std::make_unique<VulkanBackend>(hwnd, config, width, height);
	}
}
