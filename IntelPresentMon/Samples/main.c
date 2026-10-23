// Copyright (C) 2017-2026 Intel Corporation
// SPDX-License-Identifier: MIT
//
// Minimal PresentMon SDK sample: poll displayed FPS and GPU utilization once
// per second for a target process. Requires the PresentMon service and
// PresentMonAPI2Loader (link PresentMonAPI2Loader.lib, deploy the DLL).
// Exits when the target process terminates (or on Ctrl+C).
//
// Usage: fps_gpu_sample <processId>

#include "PresentMonAPI.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// Picks the first graphics adapter. Callers should confirm needed metrics are
// available on that device.
// Registering an unavailable device metric fails with PM_STATUS_QUERY_MALFORMED.
static int FindFirstGpuDeviceId(PM_SESSION_HANDLE session, uint32_t* outDeviceId)
{
	const PM_INTROSPECTION_ROOT* root = NULL;
	PM_STATUS status = pmGetIntrospectionRoot(session, &root);
	if (status != PM_STATUS_SUCCESS || root == NULL || root->pDevices == NULL) {
		return 0;
	}

	int found = 0;
	for (size_t i = 0; i < root->pDevices->size; ++i) {
		const PM_INTROSPECTION_DEVICE* device =
			(const PM_INTROSPECTION_DEVICE*)root->pDevices->pData[i];
		if (device != NULL && device->type == PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
			*outDeviceId = device->id;
			found = 1;
			break;
		}
	}

	pmFreeIntrospectionRoot(root);
	return found;
}

static int MetricAvailableOnDevice(
	PM_SESSION_HANDLE session, PM_METRIC metricId, uint32_t deviceId)
{
	const PM_INTROSPECTION_ROOT* root = NULL;
	if (pmGetIntrospectionRoot(session, &root) != PM_STATUS_SUCCESS ||
		root == NULL || root->pMetrics == NULL) {
		return 0;
	}

	int available = 0;
	for (size_t m = 0; m < root->pMetrics->size && !available; ++m) {
		const PM_INTROSPECTION_METRIC* metric =
			(const PM_INTROSPECTION_METRIC*)root->pMetrics->pData[m];
		if (metric == NULL || metric->id != metricId ||
			metric->pDeviceMetricInfo == NULL) {
			continue;
		}
		for (size_t d = 0; d < metric->pDeviceMetricInfo->size; ++d) {
			const PM_INTROSPECTION_DEVICE_METRIC_INFO* info =
				(const PM_INTROSPECTION_DEVICE_METRIC_INFO*)metric->pDeviceMetricInfo->pData[d];
			if (info != NULL && info->deviceId == deviceId &&
				info->availability == PM_METRIC_AVAILABILITY_AVAILABLE &&
				info->arraySize > 0) {
				available = 1;
				break;
			}
		}
	}

	pmFreeIntrospectionRoot(root);
	return available;
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <processId>\n", argv[0]);
		return 1;
	}

	char* end = NULL;
	unsigned long pidLong = strtoul(argv[1], &end, 10);
	if (end == argv[1] || *end != '\0' || pidLong == 0ul || pidLong > 0xfffffffful) {
		fprintf(stderr, "Invalid process id: %s\n", argv[1]);
		return 1;
	}
	const uint32_t processId = (uint32_t)pidLong;

	// Resources released at cleanup (must be declared before any goto).
	HANDLE hProcess = NULL;
	PM_SESSION_HANDLE session = NULL;
	PM_DYNAMIC_QUERY_HANDLE query = NULL;
	uint8_t* blob = NULL;
	int exitCode = 1;

	// Use the OS handle to check if the process is still running.
	hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
	if (hProcess == NULL) {
		fprintf(stderr, "OpenProcess failed for PID %u.\n", processId);
		goto cleanup;
	}

	PM_STATUS status = pmOpenSession(&session);
	if (status != PM_STATUS_SUCCESS) {
		fprintf(stderr, "pmOpenSession failed: %d\n", (int)status);
		goto cleanup;
	}

	uint32_t gpuDeviceId = 0;
	if (!FindFirstGpuDeviceId(session, &gpuDeviceId)) {
		fprintf(stderr, "No graphics adapter found via introspection.\n");
		goto cleanup;
	}
	if (!MetricAvailableOnDevice(
		session, PM_METRIC_GPU_UTILIZATION, gpuDeviceId)) {
		fprintf(stderr,
			"GPU utilization is not available on graphics adapter id %u.\n",
			gpuDeviceId);
		goto cleanup;
	}

	status = pmSetTelemetryPollingPeriod(session, 0, 1000);
	if (status != PM_STATUS_SUCCESS) {
		fprintf(stderr, "pmSetTelemetryPollingPeriod failed: %d\n", (int)status);
		goto cleanup;
	}

	status = pmStartTrackingProcess(session, processId);
	if (status != PM_STATUS_SUCCESS) {
		fprintf(stderr, "pmStartTrackingProcess failed: %d\n", (int)status);
		goto cleanup;
	}

	PM_QUERY_ELEMENT elements[2];
	memset(elements, 0, sizeof(elements));
	// Frame metrics do not need a device id (deviceId = 0).
	elements[0].metric = PM_METRIC_DISPLAYED_FPS;
	elements[0].stat = PM_STAT_AVG;
	elements[0].deviceId = 0;
	elements[0].arrayIndex = 0;
	// GPU telemetry need a device id.
	elements[1].metric = PM_METRIC_GPU_UTILIZATION;
	elements[1].stat = PM_STAT_AVG;
	elements[1].deviceId = gpuDeviceId;
	elements[1].arrayIndex = 0;

	// Stats are averaged over [now - offset - window, now - offset], not the
	// last wall-clock second. The offset leaves room for late ETW/telemetry so
	// the newest part of the window is not empty.
	const double windowSizeMs = 1000.0;
	const double metricOffsetMs = 1064.0;
	status = pmRegisterDynamicQuery(
		session, &query, elements, 2, windowSizeMs, metricOffsetMs);
	if (status != PM_STATUS_SUCCESS) {
		fprintf(stderr, "pmRegisterDynamicQuery failed: %d\n", (int)status);
		goto cleanup;
	}

	// Register fills dataOffset/dataSize. Size the blob to cover the last element.
	const size_t blobSize =
		(size_t)(elements[1].dataOffset + elements[1].dataSize);
	blob = (uint8_t*)malloc(blobSize);
	if (blob == NULL) {
		fprintf(stderr, "Failed to allocate query blob.\n");
		goto cleanup;
	}

	printf("Tracking PID %u (GPU device id %u). Ctrl+C to stop.\n",
		processId, gpuDeviceId);

	for (;;) {
		DWORD processExitCode = 0;
		if (!GetExitCodeProcess(hProcess, &processExitCode) ||
			processExitCode != STILL_ACTIVE) {
			printf("Process %u exited.\n", processId);
			status = PM_STATUS_SUCCESS;
			break;
		}

		uint32_t numSwapChains = 1;
		status = pmPollDynamicQuery(query, processId, blob, &numSwapChains);
		if (status != PM_STATUS_SUCCESS) {
			fprintf(stderr, "pmPollDynamicQuery failed: %d\n", (int)status);
			break;
		}

		double fps = 0.0;
		double gpuUtil = 0.0;
		if (numSwapChains > 0) {
			memcpy(&fps, blob + elements[0].dataOffset, sizeof(fps));
			memcpy(&gpuUtil, blob + elements[1].dataOffset, sizeof(gpuUtil));
		}

		printf("Displayed FPS: %.2f  GPU Utilization: %.2f%%\n", fps, gpuUtil);
		fflush(stdout);
		Sleep(1000);
	}

	exitCode = (status == PM_STATUS_SUCCESS) ? 0 : 1;

cleanup:
	free(blob);
	pmFreeDynamicQuery(query);
	pmStopTrackingProcess(session, processId);
	pmCloseSession(session);
	if (hProcess) {
		CloseHandle(hProcess);
	}
	return exitCode;
}
