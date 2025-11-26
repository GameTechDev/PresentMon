#pragma once
#include <optional>
#include <string>
#include <memory>
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include "DataStores.h"
#include "MetricCapabilities.h"

namespace pmon::ipc
{
	namespace intro
	{
		struct IntrospectionRoot;
	}

	class ServiceComms
	{
	public:
		virtual ~ServiceComms() = default;
		virtual intro::IntrospectionRoot& GetIntrospectionRoot() = 0;
		virtual void RegisterGpuDevice(PM_DEVICE_VENDOR vendor, std::string deviceName, const MetricCapabilities& caps) = 0;
		virtual void FinalizeGpuDevices() = 0;
		virtual void RegisterCpuDevice(PM_DEVICE_VENDOR vendor, std::string deviceName, const MetricCapabilities& caps) = 0;
		
		// data store access
		virtual FrameDataStore& MakeFrameDataStore(uint32_t pid) = 0;
		virtual void RemoveFrameDataStore(uint32_t pid) = 0;
		virtual FrameDataStore& GetFrameDataStore(uint32_t pid) = 0;
		virtual GpuDataStore& GetGpuDataStore(uint32_t deviceId) = 0;
		virtual SystemDataStore& GetSystemDataStore() = 0;
	};

	class MiddlewareComms
	{
	public:
		virtual ~MiddlewareComms() = default;
		virtual const PM_INTROSPECTION_ROOT* GetIntrospectionRoot(uint32_t timeoutMs = 2000) = 0;

		// data store access
		virtual const FrameDataStore& GetFrameDataStore(uint32_t pid) const = 0;
		virtual const GpuDataStore& GetGpuDataStore(uint32_t deviceId) const = 0;
		virtual const SystemDataStore& GetSystemDataStore() const = 0;

	};

	std::unique_ptr<ServiceComms> MakeServiceComms(std::optional<std::string> sharedMemoryName = {});
	std::unique_ptr<MiddlewareComms> MakeMiddlewareComms(std::optional<std::string> sharedMemoryName = {});
}