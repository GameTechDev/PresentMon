#pragma once
#include <optional>
#include <string>
#include <memory>
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include "DataStores.h"
#include "OwnedDataSegment.h"
#include "MetricCapabilities.h"
#include "ShmNamer.h"

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
		virtual void RegisterGpuDevice(uint32_t deviceId, PM_DEVICE_VENDOR vendor, std::string deviceName, const MetricCapabilities& caps) = 0;
		virtual void FinalizeGpuDevices() = 0;
		virtual void RegisterCpuDevice(PM_DEVICE_VENDOR vendor, std::string deviceName, const MetricCapabilities& caps) = 0;
		virtual const ShmNamer& GetNamer() const = 0;
		
		// data store access
		virtual std::shared_ptr<OwnedDataSegment<FrameDataStore>> CreateOrGetFrameDataSegment(uint32_t pid, bool backpressured) = 0;
		virtual std::shared_ptr<OwnedDataSegment<FrameDataStore>> GetFrameDataSegment(uint32_t pid) = 0;
		virtual std::vector<uint32_t> GetFramePids() const = 0;
		virtual GpuDataStore& GetGpuDataStore(uint32_t deviceId) = 0;
		virtual SystemDataStore& GetSystemDataStore() = 0;
	};

	class MiddlewareComms
	{
	public:
		virtual ~MiddlewareComms() = default;
		virtual const PM_INTROSPECTION_ROOT* GetIntrospectionRoot(uint32_t timeoutMs = 2000) = 0;

		// data store access
		// not const because of the backpressure case
		// TODO: consider more separation of backpressure and broadcast cases
		virtual const FrameDataStore& GetFrameDataStore(uint32_t pid) const = 0;
		virtual const GpuDataStore& GetGpuDataStore(uint32_t deviceId) const = 0;
		virtual const SystemDataStore& GetSystemDataStore() const = 0;
		virtual void OpenFrameDataStore(uint32_t pid) = 0;
		virtual void CloseFrameDataStore(uint32_t pid) = 0;
	};

	std::unique_ptr<ServiceComms> MakeServiceComms(std::string prefix,
		size_t frameRingSamples,
		size_t telemetryRingSamples);
	std::unique_ptr<MiddlewareComms> MakeMiddlewareComms(std::string prefix, std::string salt);
}
