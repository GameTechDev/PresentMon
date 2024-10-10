#pragma once
#include <optional>
#include <string>
#include <memory>
#include "../../ControlLib/PresentMonPowerTelemetry.h"
#include "../../ControlLib/CpuTelemetryInfo.h"
#include "../../PresentMonAPI2/PresentMonAPI.h"

struct PM_INTROSPECTION_ROOT;

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
		virtual void RegisterGpuDevice(PM_DEVICE_VENDOR vendor, std::string deviceName, const GpuTelemetryBitset& gpuCaps) = 0;
		virtual void FinalizeGpuDevices() = 0;
		virtual void RegisterCpuDevice(PM_DEVICE_VENDOR vendor, std::string deviceName, const CpuTelemetryBitset& cpuCaps) = 0;
	};

	class MiddlewareComms
	{
	public:
		virtual ~MiddlewareComms() = default;
		virtual const PM_INTROSPECTION_ROOT* GetIntrospectionRoot(uint32_t timeoutMs = 2000) = 0;
	};

	std::unique_ptr<ServiceComms> MakeServiceComms(std::optional<std::string> sharedMemoryName = {});
	std::unique_ptr<MiddlewareComms> MakeMiddlewareComms(std::optional<std::string> sharedMemoryName = {});
}