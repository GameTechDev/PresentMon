// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

enum class nvmlReturn_t {
	NVML_SUCCESS = 0,
	NVML_ERROR_UNINITIALIZED = 1,
	NVML_ERROR_INVALID_ARGUMENT = 2,
	NVML_ERROR_NOT_SUPPORTED = 3,
	NVML_ERROR_NO_PERMISSION = 4,
	NVML_ERROR_ALREADY_INITIALIZED = 5,
	NVML_ERROR_NOT_FOUND = 6,
	NVML_ERROR_INSUFFICIENT_SIZE = 7,
	NVML_ERROR_INSUFFICIENT_POWER = 8,
	NVML_ERROR_DRIVER_NOT_LOADED = 9,
	NVML_ERROR_TIMEOUT = 10,
	NVML_ERROR_IRQ_ISSUE = 11,
	NVML_ERROR_LIBRARY_NOT_FOUND = 12,
	NVML_ERROR_FUNCTION_NOT_FOUND = 13,
	NVML_ERROR_CORRUPTED_INFOROM = 14,
	NVML_ERROR_GPU_IS_LOST = 15,
	NVML_ERROR_RESET_REQUIRED = 16,
	NVML_ERROR_OPERATING_SYSTEM = 17,
	NVML_ERROR_LIB_RM_VERSION_MISMATCH = 18,
	NVML_ERROR_IN_USE = 19,
	NVML_ERROR_MEMORY = 20,
	NVML_ERROR_NO_DATA = 21,
	NVML_ERROR_VGPU_ECC_NOT_SUPPORTED = 22,
	NVML_ERROR_INSUFFICIENT_RESOURCES = 23,
	NVML_ERROR_FREQ_NOT_SUPPORTED = 24,
	NVML_ERROR_UNKNOWN = 999,
};

enum class nvmlClockType_t {
	NVML_CLOCK_GRAPHICS = 0,
	NVML_CLOCK_SM = 1,
	NVML_CLOCK_MEM = 2,
	NVML_CLOCK_VIDEO = 3,
	NVML_CLOCK_COUNT
};

enum class nvmlClockId_t {
	NVML_CLOCK_ID_CURRENT = 0,
	NVML_CLOCK_ID_APP_CLOCK_TARGET = 1,
	NVML_CLOCK_ID_APP_CLOCK_DEFAULT = 2,
	NVML_CLOCK_ID_CUSTOMER_BOOST_MAX = 3,
	NVML_CLOCK_ID_COUNT
};

enum class nvmlTemperatureSensors_t {
	NVML_TEMPERATURE_GPU = 0,
	NVML_TEMPERATURE_COUNT,
};

enum class nvmlSamplingType_t {
	NVML_TOTAL_POWER_SAMPLES = 0,
	NVML_GPU_UTILIZATION_SAMPLES = 1,
	NVML_MEMORY_UTILIZATION_SAMPLES = 2,
	NVML_ENC_UTILIZATION_SAMPLES = 3,
	NVML_DEC_UTILIZATION_SAMPLES = 4,
	NVML_PROCESSOR_CLK_SAMPLES = 5,
	NVML_MEMORY_CLK_SAMPLES = 6,
	NVML_SAMPLINGTYPE_COUNT,
};

enum class nvmlValueType_t {
	NVML_VALUE_TYPE_DOUBLE = 0,
	NVML_VALUE_TYPE_UNSIGNED_INT = 1,
	NVML_VALUE_TYPE_UNSIGNED_LONG = 2,
	NVML_VALUE_TYPE_UNSIGNED_LONG_LONG = 3,
	NVML_VALUE_TYPE_SIGNED_LONG_LONG = 4,
	NVML_VALUE_TYPE_COUNT
};

enum class nvmlPerfPolicyType_t {
	NVML_PERF_POLICY_POWER = 0,
	NVML_PERF_POLICY_THERMAL = 1,
	NVML_PERF_POLICY_SYNC_BOOST = 2,
	NVML_PERF_POLICY_BOARD_LIMIT = 3,
	NVML_PERF_POLICY_LOW_UTILIZATION = 4,
	NVML_PERF_POLICY_RELIABILITY = 5,
	NVML_PERF_POLICY_TOTAL_APP_CLOCKS = 10,
	NVML_PERF_POLICY_TOTAL_BASE_CLOCKS = 11,
	NVML_PERF_POLICY_COUNT
};

union nvmlValue_t {
	double dVal;
	signed long long sllVal;
	unsigned int uiVal;
	unsigned long ulVal;
	unsigned long long ullVal;
};

// Opaque nvml device handle
typedef struct _nvmlDevice_t* nvmlDevice_t;

#define NVML_DEVICE_NAME_BUFFER_SIZE 64
#define NVML_DEVICE_PCI_BUS_ID_BUFFER_SIZE 32
#define NVML_DEVICE_PCI_BUS_ID_BUFFER_V2_SIZE 16

// nvml pci info struct
struct nvmlPciInfo_t {
	unsigned int bus;
	char busId[NVML_DEVICE_PCI_BUS_ID_BUFFER_SIZE];
	char busIdLegacy[NVML_DEVICE_PCI_BUS_ID_BUFFER_V2_SIZE];
	unsigned int device;
	unsigned int domain;
	unsigned int pciDeviceId;
	unsigned int pciSubSystemId;
};

struct nvmlMemory_t {
	unsigned long long free;
	unsigned long long total;
	unsigned long long used;
};

struct nvmlUtilization_t {
	unsigned int gpu;
	unsigned int mem;
};

struct nvmlSample_t {
	nvmlValue_t sampleValue;
	unsigned long long timeStamp;
};

struct nvmlViolationTime_t {
	unsigned long long referenceTime;
	unsigned long long violationTime;
};
