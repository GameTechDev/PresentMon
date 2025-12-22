#pragma once
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include "SharedMemoryTypes.h"
#include "MetricCapabilities.h"

// TODO: forward declare the segment manager (or type erase)

namespace pmon::ipc::intro
{
	constexpr uint32_t kSystemDeviceId = 65536;
	void PopulateEnums(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateMetrics(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateUnits(ShmSegmentManager* pSegmentManager, struct IntrospectionRoot& root);
	void PopulateGpuDevice(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root, uint32_t deviceId,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const MetricCapabilities& caps);
	void PopulateCpu(ShmSegmentManager* pSegmentManager, IntrospectionRoot& root,
		PM_DEVICE_VENDOR vendor, const std::string& deviceName, const MetricCapabilities& caps);
}