// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#include "FrameMetricCapabilitiesCsv.h"
#include <CommonUtilities/Exception.h>
#include "../../../Interprocess/source/SystemDeviceId.h"
#include <fstream>
#include <format>
#include <ranges>
#include <vector>

namespace p2c::pmon
{
	namespace rn = std::ranges;
	namespace vi = rn::views;

	namespace
	{
		struct CapabilitiesRow_
		{
			uint32_t deviceId = 0;
			std::string deviceName;
			std::string vendorName;
			PM_DEVICE_TYPE rowDeviceType = PM_DEVICE_TYPE_INDEPENDENT;
			uint32_t queryDeviceId = 0;
		};

		bool MetricSupportsFrameQuery_(PM_METRIC_TYPE type)
		{
			return type != PM_METRIC_TYPE_DYNAMIC;
		}

		// Infer from per-device introspection rows until PM_INTROSPECTION_METRIC exposes PM_DEVICE_TYPE
		// (or we call GetMetricRegisteredDeviceType from metric list metadata).
		PM_DEVICE_TYPE InferMetricDeviceType_(const pmapi::intro::MetricView& metric)
		{
			for (auto info : metric.GetDeviceMetricInfo()) {
				if (info.GetDevice().GetType() == PM_DEVICE_TYPE_SYSTEM) {
					return PM_DEVICE_TYPE_SYSTEM;
				}
			}
			for (auto info : metric.GetDeviceMetricInfo()) {
				if (info.GetDevice().GetType() == PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
					return PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
				}
			}
			return PM_DEVICE_TYPE_INDEPENDENT;
		}

		std::string AvailabilitySymbol_(const pmapi::intro::Root& introRoot, PM_METRIC_AVAILABILITY availability)
		{
			std::string symbol = introRoot.FindEnumKey(PM_ENUM_METRIC_AVAILABILITY, (int)availability).GetSymbol();
			constexpr std::string_view prefix = "PM_METRIC_AVAILABILITY_";
			if (symbol.starts_with(prefix)) {
				symbol.erase(0, prefix.size());
			}
			return symbol;
		}

		void TryPopulateDeviceIdentity_(
			const pmapi::intro::Root& introRoot,
			uint32_t deviceId,
			std::string& deviceName,
			std::string& vendorName)
		{
			try {
				const auto device = introRoot.FindDevice(deviceId);
				deviceName = device.GetName();
				vendorName = device.IntrospectVendor().GetName();
			}
			catch (...) {
			}
		}

		std::string FormatCsvField_(std::string_view value)
		{
			const bool needsQuotes = value.find_first_of(",\"\r\n") != std::string_view::npos;
			if (!needsQuotes) {
				return std::string{ value };
			}
			std::string escaped;
			escaped.reserve(value.size() + 2);
			escaped.push_back('"');
			for (const char ch : value) {
				if (ch == '"') {
					escaped.append("\"\"");
				}
				else {
					escaped.push_back(ch);
				}
			}
			escaped.push_back('"');
			return escaped;
		}

		void WriteCsvField_(std::ostream& out, std::string_view value, bool& firstField)
		{
			if (!firstField) {
				out << ',';
			}
			firstField = false;
			out << FormatCsvField_(value);
		}

		std::vector<CapabilitiesRow_> BuildCapabilityRows_(const pmapi::intro::Root& introRoot,
			const std::string& systemCpuName)
		{
			std::vector<CapabilitiesRow_> rows;

			{
				CapabilitiesRow_ row{
					.deviceId = ::pmon::ipc::kUniversalDeviceId,
					.deviceName = "Universal",
					.rowDeviceType = PM_DEVICE_TYPE_INDEPENDENT,
					.queryDeviceId = ::pmon::ipc::kUniversalDeviceId,
				};
				TryPopulateDeviceIdentity_(introRoot, row.deviceId, row.deviceName, row.vendorName);
				rows.push_back(std::move(row));
			}

			for (auto device : introRoot.GetDevices()) {
				if (device.GetType() != PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
					continue;
				}
				rows.push_back(CapabilitiesRow_{
					.deviceId = device.GetId(),
					.deviceName = device.GetName(),
					.vendorName = device.IntrospectVendor().GetName(),
					.rowDeviceType = PM_DEVICE_TYPE_GRAPHICS_ADAPTER,
					.queryDeviceId = device.GetId(),
				});
			}

			{
				CapabilitiesRow_ row{
					.deviceId = ::pmon::ipc::kSystemDeviceId,
					.deviceName = "System",
					.rowDeviceType = PM_DEVICE_TYPE_SYSTEM,
					.queryDeviceId = ::pmon::ipc::kSystemDeviceId,
				};
				TryPopulateDeviceIdentity_(introRoot, row.deviceId, row.deviceName, row.vendorName);
				if (!systemCpuName.empty()) {
					row.deviceName = std::format("System ({})", systemCpuName);
				}
				rows.push_back(std::move(row));
			}
			return rows;
		}

		struct FrameMetricColumn_
		{
			PM_METRIC id = PM_METRIC_COUNT_;
			std::string symbol;
			PM_DEVICE_TYPE deviceType = PM_DEVICE_TYPE_INDEPENDENT;
		};

		std::vector<FrameMetricColumn_> CollectFrameMetricColumns_(const pmapi::intro::Root& introRoot)
		{
			std::vector<FrameMetricColumn_> columns;
			for (auto metric : introRoot.GetMetrics()) {
				const auto id = metric.GetId();
				if (id == PM_METRIC_COUNT_) {
					continue;
				}
				if (!MetricSupportsFrameQuery_(metric.GetType())) {
					continue;
				}
				columns.push_back(FrameMetricColumn_{
					.id = id,
					.symbol = metric.Introspect().GetSymbol(),
					.deviceType = InferMetricDeviceType_(metric),
				});
			}
			return columns;
		}

		std::string CellValueForMetric_(
			const pmapi::intro::Root& introRoot,
			const pmapi::intro::MetricView& metric,
			PM_DEVICE_TYPE metricDeviceType,
			const CapabilitiesRow_& row)
		{
			if (metricDeviceType != row.rowDeviceType) {
				return "NA";
			}
			for (auto info : metric.GetDeviceMetricInfo()) {
				if (info.GetDevice().GetId() == row.queryDeviceId) {
					return AvailabilitySymbol_(introRoot, info.GetAvailability());
				}
			}
			return AvailabilitySymbol_(introRoot, PM_METRIC_AVAILABILITY_UNAVAILABLE);
		}
	}

	void WriteFrameMetricCapabilitiesCsv(const pmapi::intro::Root& introRoot, const std::filesystem::path& outputPath,
		const std::string& systemCpuName)
	{
		const auto rows = BuildCapabilityRows_(introRoot, systemCpuName);
		const auto columns = CollectFrameMetricColumns_(introRoot);

		std::ofstream out{ outputPath, std::ios::trunc };
		if (!out) {
			throw ::pmon::util::Except<::pmon::util::Exception>(
				std::format("Failed to open output file for writing"));
		}

		bool firstField = true;
		WriteCsvField_(out, "DeviceId", firstField);
		WriteCsvField_(out, "DeviceName", firstField);
		WriteCsvField_(out, "Vendor", firstField);
		for (const auto& column : columns) {
			WriteCsvField_(out, column.symbol, firstField);
		}
		out << '\n';

		for (const auto& row : rows) {
			firstField = true;
			WriteCsvField_(out, std::to_string(row.deviceId), firstField);
			WriteCsvField_(out, row.deviceName, firstField);
			WriteCsvField_(out, row.vendorName, firstField);
			for (const auto& column : columns) {
				const auto& metric = introRoot.FindMetric(column.id);
				const auto cell = CellValueForMetric_(introRoot, metric, column.deviceType, row);
				WriteCsvField_(out, cell, firstField);
			}
			out << '\n';
		}
	}
}
