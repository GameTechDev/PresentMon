// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/str/String.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include <vincentlaucsb-csv-parser/csv.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;
using namespace std::literals;
using namespace pmon;

namespace PacedFrame
{
	class TestFixture : public CommonTestFixture
	{
	public:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-paced-frame-test-ctrl)",
				.shmNamePrefix = "pm_paced_frame_test_intro",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "PacedFramePlayback",
			};
			return args;
		}
	};

	enum class ColumnIndex : size_t
	{
		Application = 0,
		ProcessID = 1,
		SwapChainAddress = 2,
		PresentRuntime = 3,
		SyncInterval = 4,
		PresentFlags = 5,
		AllowsTearing = 6,
		PresentMode = 7,
		FrameType = 8,
		CPUStartTime = 9,
		MsBetweenSimulationStart = 10,
		MsBetweenPresents = 11,
		MsBetweenDisplayChange = 12,
		MsInPresentAPI = 13,
		MsRenderPresentLatency = 14,
		MsUntilDisplayed = 15,
		MsPCLatency = 16,
		MsBetweenAppStart = 17,
		MsCPUBusy = 18,
		MsCPUWait = 19,
		MsGPULatency = 20,
		MsGPUTime = 21,
		MsGPUBusy = 22,
		MsGPUWait = 23,
		MsVideoBusy = 24,
		MsAnimationError = 25,
		AnimationTime = 26,
		MsFlipDelay = 27,
		MsAllInputToPhotonLatency = 28,
		MsClickToPhotonLatency = 29,
		MsInstrumentedLatency = 30,
	};

	const std::array<const char*, 31> kFrameCsvHeader{
		"Application",
		"ProcessID",
		"SwapChainAddress",
		"PresentRuntime",
		"SyncInterval",
		"PresentFlags",
		"AllowsTearing",
		"PresentMode",
		"FrameType",
		"CPUStartTime",
		"MsBetweenSimulationStart",
		"MsBetweenPresents",
		"MsBetweenDisplayChange",
		"MsInPresentAPI",
		"MsRenderPresentLatency",
		"MsUntilDisplayed",
		"MsPCLatency",
		"MsBetweenAppStart",
		"MsCPUBusy",
		"MsCPUWait",
		"MsGPULatency",
		"MsGPUTime",
		"MsGPUBusy",
		"MsGPUWait",
		"MsVideoBusy",
		"MsAnimationError",
		"AnimationTime",
		"MsFlipDelay",
		"MsAllInputToPhotonLatency",
		"MsClickToPhotonLatency",
		"MsInstrumentedLatency",
	};

	struct FrameCsvRow
	{
		std::string application;
		uint32_t processId = 0;
		uint64_t swapChainAddress = 0;
		std::string presentRuntime;
		int32_t syncInterval = 0;
		uint32_t presentFlags = 0;
		uint32_t allowsTearing = 0;
		std::string presentMode;
		std::string frameType;
		std::optional<double> cpuStartTime;
		std::optional<double> msBetweenSimulationStart;
		double msBetweenPresents = 0.0;
		std::optional<double> msBetweenDisplayChange;
		double msInPresentApi = 0.0;
		double msRenderPresentLatency = 0.0;
		std::optional<double> msUntilDisplayed;
		std::optional<double> msPcLatency;
		double msBetweenAppStart = 0.0;
		double msCpuBusy = 0.0;
		double msCpuWait = 0.0;
		double msGpuLatency = 0.0;
		double msGpuTime = 0.0;
		double msGpuBusy = 0.0;
		double msGpuWait = 0.0;
		double msVideoBusy = 0.0;
		std::optional<double> msAnimationError;
		std::optional<double> animationTime;
		std::optional<double> msFlipDelay;
		std::optional<double> msAllInputToPhotonLatency;
		std::optional<double> msClickToPhotonLatency;
		std::optional<double> msInstrumentedLatency;
	};

	std::wstring MakeFailMessage(size_t row, const char* column, const std::string& expected,
		const std::string& actual)
	{
		return pmon::util::str::ToWide(std::format(
			"Row {} column {} expected [{}] got [{}]", row, column, expected, actual));
	}

	std::wstring MakeFailMessage(size_t row, const char* column)
	{
		return pmon::util::str::ToWide(std::format("Row {} column {} mismatch", row, column));
	}

	void StripUtf8Bom(std::string& value)
	{
		const char bom[] = { char(0xEF), char(0xBB), char(0xBF), 0 };
		if (value.rfind(bom, 0) == 0) {
			value.erase(0, 3);
		}
	}

	bool IsMissingToken(const std::string& value)
	{
		return value == "NA" || value == "NaN" || value == "nan";
	}

	uint64_t ParseUint64(const std::string& value, size_t row, const char* column)
	{
		try {
			return std::stoull(value, nullptr, 0);
		}
		catch (...) {
			Assert::Fail(MakeFailMessage(row, column).c_str());
		}
		return 0;
	}

	uint32_t ParseUint32(const std::string& value, size_t row, const char* column)
	{
		try {
			return static_cast<uint32_t>(std::stoul(value, nullptr, 0));
		}
		catch (...) {
			Assert::Fail(MakeFailMessage(row, column).c_str());
		}
		return 0;
	}

	int32_t ParseInt32(const std::string& value, size_t row, const char* column)
	{
		try {
			return std::stoi(value, nullptr, 0);
		}
		catch (...) {
			Assert::Fail(MakeFailMessage(row, column).c_str());
		}
		return 0;
	}

	double ParseDouble(const std::string& value, size_t row, const char* column)
	{
		try {
			return std::stod(value);
		}
		catch (...) {
			Assert::Fail(MakeFailMessage(row, column).c_str());
		}
		return 0.0;
	}

	std::optional<double> ParseOptionalDouble(const std::string& value, size_t row, const char* column)
	{
		if (IsMissingToken(value)) {
			return std::nullopt;
		}
		return ParseDouble(value, row, column);
	}

	void ValidateHeader(const std::vector<std::string>& header)
	{
		Assert::IsTrue(header.size() == kFrameCsvHeader.size(), L"Unexpected header column count");
		for (size_t i = 0; i < kFrameCsvHeader.size(); ++i) {
			Assert::IsTrue(header[i] == kFrameCsvHeader[i],
				MakeFailMessage(0, "Header", kFrameCsvHeader[i], header[i]).c_str());
		}
	}

	FrameCsvRow ParseFrameRow(const std::vector<std::string>& row, size_t rowIndex)
	{
		FrameCsvRow parsed;
		parsed.application = row[static_cast<size_t>(ColumnIndex::Application)];
		parsed.processId = ParseUint32(row[static_cast<size_t>(ColumnIndex::ProcessID)], rowIndex, "ProcessID");
		parsed.swapChainAddress = ParseUint64(row[static_cast<size_t>(ColumnIndex::SwapChainAddress)], rowIndex, "SwapChainAddress");
		parsed.presentRuntime = row[static_cast<size_t>(ColumnIndex::PresentRuntime)];
		parsed.syncInterval = ParseInt32(row[static_cast<size_t>(ColumnIndex::SyncInterval)], rowIndex, "SyncInterval");
		parsed.presentFlags = ParseUint32(row[static_cast<size_t>(ColumnIndex::PresentFlags)], rowIndex, "PresentFlags");
		parsed.allowsTearing = ParseUint32(row[static_cast<size_t>(ColumnIndex::AllowsTearing)], rowIndex, "AllowsTearing");
		parsed.presentMode = row[static_cast<size_t>(ColumnIndex::PresentMode)];
		parsed.frameType = row[static_cast<size_t>(ColumnIndex::FrameType)];
		parsed.cpuStartTime = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::CPUStartTime)], rowIndex, "CPUStartTime");
		parsed.msBetweenSimulationStart = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsBetweenSimulationStart)], rowIndex, "MsBetweenSimulationStart");
		parsed.msBetweenPresents = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsBetweenPresents)], rowIndex, "MsBetweenPresents");
		parsed.msBetweenDisplayChange = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsBetweenDisplayChange)], rowIndex, "MsBetweenDisplayChange");
		parsed.msInPresentApi = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsInPresentAPI)], rowIndex, "MsInPresentAPI");
		parsed.msRenderPresentLatency = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsRenderPresentLatency)], rowIndex, "MsRenderPresentLatency");
		parsed.msUntilDisplayed = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsUntilDisplayed)], rowIndex, "MsUntilDisplayed");
		parsed.msPcLatency = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsPCLatency)], rowIndex, "MsPCLatency");
		parsed.msBetweenAppStart = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsBetweenAppStart)], rowIndex, "MsBetweenAppStart");
		parsed.msCpuBusy = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsCPUBusy)], rowIndex, "MsCPUBusy");
		parsed.msCpuWait = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsCPUWait)], rowIndex, "MsCPUWait");
		parsed.msGpuLatency = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsGPULatency)], rowIndex, "MsGPULatency");
		parsed.msGpuTime = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsGPUTime)], rowIndex, "MsGPUTime");
		parsed.msGpuBusy = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsGPUBusy)], rowIndex, "MsGPUBusy");
		parsed.msGpuWait = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsGPUWait)], rowIndex, "MsGPUWait");
		parsed.msVideoBusy = ParseDouble(
			row[static_cast<size_t>(ColumnIndex::MsVideoBusy)], rowIndex, "MsVideoBusy");
		parsed.msAnimationError = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsAnimationError)], rowIndex, "MsAnimationError");
		parsed.animationTime = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::AnimationTime)], rowIndex, "AnimationTime");
		parsed.msFlipDelay = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsFlipDelay)], rowIndex, "MsFlipDelay");
		parsed.msAllInputToPhotonLatency = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsAllInputToPhotonLatency)], rowIndex, "MsAllInputToPhotonLatency");
		parsed.msClickToPhotonLatency = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsClickToPhotonLatency)], rowIndex, "MsClickToPhotonLatency");
		parsed.msInstrumentedLatency = ParseOptionalDouble(
			row[static_cast<size_t>(ColumnIndex::MsInstrumentedLatency)], rowIndex, "MsInstrumentedLatency");
		return parsed;
	}

	std::vector<FrameCsvRow> LoadCsvRows(const std::string& path, uint32_t targetPid)
	{
		csv::CSVReader reader{ path };
		auto header = reader.get_col_names();
		if (!header.empty()) {
			StripUtf8Bom(header.front());
		}
		ValidateHeader(header);

		std::vector<FrameCsvRow> rows;
		std::vector<std::string> values;
		size_t rowIndex = 0;
		for (auto& row : reader) {
			values.clear();
			values.reserve(row.size());
			for (auto& field : row) {
				values.push_back(field.get<std::string>());
			}
			if (values.size() < kFrameCsvHeader.size()) {
				++rowIndex;
				continue;
			}
			auto rowPid = ParseUint32(values[static_cast<size_t>(ColumnIndex::ProcessID)],
				rowIndex, "ProcessID");
			if (rowPid != targetPid) {
				++rowIndex;
				continue;
			}
			auto parsed = ParseFrameRow(values, rowIndex);
			rows.push_back(std::move(parsed));
			++rowIndex;
		}
		return rows;
	}

	std::optional<std::string> FindProcessNameInCsv(const std::string& path, uint32_t targetPid)
	{
		if (!fs::exists(path)) {
			return std::nullopt;
		}
		csv::CSVReader reader{ path };
		auto header = reader.get_col_names();
		if (header.empty()) {
			return std::nullopt;
		}
		StripUtf8Bom(header.front());

		size_t pidIndex = SIZE_MAX;
		size_t appIndex = SIZE_MAX;
		for (size_t i = 0; i < header.size(); ++i) {
			if (header[i] == "ProcessID") {
				pidIndex = i;
			}
			else if (header[i] == "Application") {
				appIndex = i;
			}
		}
		if (pidIndex == SIZE_MAX || appIndex == SIZE_MAX) {
			return std::nullopt;
		}

		for (auto& row : reader) {
			if (row.size() <= pidIndex || row.size() <= appIndex) {
				continue;
			}
			auto pidValue = row[pidIndex].get<std::string>();
			if (ParseUint32(pidValue, 0, "ProcessID") == targetPid) {
				return row[appIndex].get<std::string>();
			}
		}
		return std::nullopt;
	}

	void CompareOptionalDouble(const std::optional<double>& expected, const std::optional<double>& actual,
		size_t rowIndex, const char* column)
	{
		if (expected.has_value() != actual.has_value()) {
			Assert::Fail(MakeFailMessage(rowIndex, column).c_str());
		}
		if (expected && actual && *expected != *actual) {
			Assert::Fail(MakeFailMessage(rowIndex, column).c_str());
		}
	}

	void CompareRows(const FrameCsvRow& expected, const FrameCsvRow& actual, size_t rowIndex)
	{
		if (expected.application != actual.application) {
			Assert::Fail(MakeFailMessage(rowIndex, "Application", expected.application, actual.application).c_str());
		}
		if (expected.processId != actual.processId) {
			Assert::Fail(MakeFailMessage(rowIndex, "ProcessID").c_str());
		}
		if (expected.swapChainAddress != actual.swapChainAddress) {
			Assert::Fail(MakeFailMessage(rowIndex, "SwapChainAddress").c_str());
		}
		if (expected.presentRuntime != actual.presentRuntime) {
			Assert::Fail(MakeFailMessage(rowIndex, "PresentRuntime",
				expected.presentRuntime, actual.presentRuntime).c_str());
		}
		if (expected.syncInterval != actual.syncInterval) {
			Assert::Fail(MakeFailMessage(rowIndex, "SyncInterval").c_str());
		}
		if (expected.presentFlags != actual.presentFlags) {
			Assert::Fail(MakeFailMessage(rowIndex, "PresentFlags").c_str());
		}
		if (expected.allowsTearing != actual.allowsTearing) {
			Assert::Fail(MakeFailMessage(rowIndex, "AllowsTearing").c_str());
		}
		if (expected.presentMode != actual.presentMode) {
			Assert::Fail(MakeFailMessage(rowIndex, "PresentMode",
				expected.presentMode, actual.presentMode).c_str());
		}
		if (expected.frameType != actual.frameType) {
			Assert::Fail(MakeFailMessage(rowIndex, "FrameType",
				expected.frameType, actual.frameType).c_str());
		}
		CompareOptionalDouble(expected.cpuStartTime, actual.cpuStartTime,
			rowIndex, "CPUStartTime");
		CompareOptionalDouble(expected.msBetweenSimulationStart, actual.msBetweenSimulationStart,
			rowIndex, "MsBetweenSimulationStart");
		if (expected.msBetweenPresents != actual.msBetweenPresents) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsBetweenPresents").c_str());
		}
		CompareOptionalDouble(expected.msBetweenDisplayChange, actual.msBetweenDisplayChange,
			rowIndex, "MsBetweenDisplayChange");
		if (expected.msInPresentApi != actual.msInPresentApi) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsInPresentAPI").c_str());
		}
		if (expected.msRenderPresentLatency != actual.msRenderPresentLatency) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsRenderPresentLatency").c_str());
		}
		CompareOptionalDouble(expected.msUntilDisplayed, actual.msUntilDisplayed,
			rowIndex, "MsUntilDisplayed");
		CompareOptionalDouble(expected.msPcLatency, actual.msPcLatency,
			rowIndex, "MsPCLatency");
		if (expected.msBetweenAppStart != actual.msBetweenAppStart) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsBetweenAppStart").c_str());
		}
		if (expected.msCpuBusy != actual.msCpuBusy) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsCPUBusy").c_str());
		}
		if (expected.msCpuWait != actual.msCpuWait) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsCPUWait").c_str());
		}
		if (expected.msGpuLatency != actual.msGpuLatency) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsGPULatency").c_str());
		}
		if (expected.msGpuTime != actual.msGpuTime) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsGPUTime").c_str());
		}
		if (expected.msGpuBusy != actual.msGpuBusy) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsGPUBusy").c_str());
		}
		if (expected.msGpuWait != actual.msGpuWait) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsGPUWait").c_str());
		}
		if (expected.msVideoBusy != actual.msVideoBusy) {
			Assert::Fail(MakeFailMessage(rowIndex, "MsVideoBusy").c_str());
		}
		CompareOptionalDouble(expected.msAnimationError, actual.msAnimationError,
			rowIndex, "MsAnimationError");
		CompareOptionalDouble(expected.animationTime, actual.animationTime,
			rowIndex, "AnimationTime");
		CompareOptionalDouble(expected.msFlipDelay, actual.msFlipDelay,
			rowIndex, "MsFlipDelay");
		CompareOptionalDouble(expected.msAllInputToPhotonLatency, actual.msAllInputToPhotonLatency,
			rowIndex, "MsAllInputToPhotonLatency");
		CompareOptionalDouble(expected.msClickToPhotonLatency, actual.msClickToPhotonLatency,
			rowIndex, "MsClickToPhotonLatency");
		CompareOptionalDouble(expected.msInstrumentedLatency, actual.msInstrumentedLatency,
			rowIndex, "MsInstrumentedLatency");
	}

	void CompareCsvFiles(const std::string& goldPath, const std::string& runPath, uint32_t targetPid)
	{
		auto goldRows = LoadCsvRows(goldPath, targetPid);
		auto runRows = LoadCsvRows(runPath, targetPid);

		Assert::IsTrue(!goldRows.empty(), L"No gold rows found for target pid");
		Assert::IsTrue(goldRows.size() == runRows.size(), L"Row count mismatch");
		for (size_t i = 0; i < goldRows.size(); ++i) {
			CompareRows(goldRows[i], runRows[i], i);
		}
	}

	void ExecutePacedFrameTest(const std::string& testName, uint32_t targetPid,
		size_t frameLimit, TestFixture& fixture)
	{
		const auto goldCsvPath = std::format(R"(..\..\Tests\AuxData\Data\{}_gold.csv)", testName);
		const auto outCsvPath = std::format("{}\\{}.csv", outFolder_, testName);
		Logger::WriteMessage(std::format("Frame test output csv: {}\n",
			fs::absolute(outCsvPath).string()).c_str());
		Logger::WriteMessage(std::format("Frame test gold csv search path: {}\n",
			fs::absolute(goldCsvPath).string()).c_str());

		std::optional<std::string> processName;
		if (fs::exists(goldCsvPath)) {
			processName = FindProcessNameInCsv(goldCsvPath, targetPid);
		}

		std::vector<std::string> args{
			"--process-id"s, std::to_string(targetPid),
			"--output-path"s, outCsvPath,
			"--frame-limit"s, std::to_string(frameLimit),
		};
		if (processName) {
			args.push_back("--process-name"s);
			args.push_back(*processName);
		}

		fixture.LaunchClient(args);

		Assert::IsTrue(fs::exists(outCsvPath), L"Output CSV not created");
		Assert::IsTrue(fs::exists(goldCsvPath), L"Gold CSV missing");
		CompareCsvFiles(goldCsvPath, outCsvPath, targetPid);
	}

#define TEST_NAME F00HeaWin2080
#define ETL_NAME P00HeaWin2080
	TEST_CLASS(TEST_NAME)
	{
		TestFixture fixture_;
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			Logger::WriteMessage(std::format("Frame test etl path: {}\n",
				fs::absolute(std::format(R"(..\..\Tests\AuxData\Data\{}.etl)", STRINGIFY(ETL_NAME))).string()).c_str());
			fixture_.Setup({
				"--etl-test-file"s, std::format(R"(..\..\Tests\AuxData\Data\{}.etl)", STRINGIFY(ETL_NAME)),
				"--pace-playback"s,
				});
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		TEST_METHOD(PollFrame)
		{
			const uint32_t targetPid = 12820;
			const size_t frameLimit = 1903;
			ExecutePacedFrameTest(STRINGIFY(TEST_NAME), targetPid, frameLimit, fixture_);
		}
	};
#undef TEST_NAME
#undef ETL_NAME

#define TEST_NAME F01TimeSpyDemoFS2080
#define ETL_NAME P01TimeSpyDemoFS2080
	TEST_CLASS(TEST_NAME)
	{
		TestFixture fixture_;
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			Logger::WriteMessage(std::format("Frame test etl path: {}\n",
				fs::absolute(std::format(R"(..\..\Tests\AuxData\Data\{}.etl)", STRINGIFY(ETL_NAME))).string()).c_str());
			fixture_.Setup({
				"--etl-test-file"s, std::format(R"(..\..\Tests\AuxData\Data\{}.etl)", STRINGIFY(ETL_NAME)),
				"--pace-playback"s,
				});
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		TEST_METHOD(PollFrame)
		{
			const uint32_t targetPid = 19736;
			const size_t frameLimit = 0;
			ExecutePacedFrameTest(STRINGIFY(TEST_NAME), targetPid, frameLimit, fixture_);
		}
	};
#undef TEST_NAME
#undef ETL_NAME
}
