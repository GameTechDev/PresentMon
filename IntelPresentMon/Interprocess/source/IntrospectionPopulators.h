#pragma once
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include "SharedMemoryTypes.h"
#include "../../ControlLib/PresentMonPowerTelemetry.h"
#include "../../ControlLib/CpuTelemetryInfo.h"

// TODO: forward declare the segment manager (or type erase)

namespace pmon::ipc::intro
{
	void PopulateEnums(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateMetrics(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateUnits(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateGpuDevice(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root, uint32_t deviceId,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const GpuTelemetryBitset& gpuCaps, uint64_t adapterId);
	void PopulateCpu(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const CpuTelemetryBitset& cpuCaps);
}